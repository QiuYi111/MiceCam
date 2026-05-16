#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <nlohmann/json.hpp>

#include "domain/StreamRingDescriptor.h"
#include "infrastructure/PluginStreamConsumer.h"
#include "pipeline/RecordingPipeline.h"

namespace {

constexpr uint32_t kSlotCount = 8;
constexpr uint32_t kSlotSize = 262144;
constexpr uint64_t kHeaderSize = 64;
constexpr uint64_t kPayloadHeaderSize = 44;

struct TestRingHeader {
    std::atomic<uint64_t> producer_seq{0};
    std::atomic<uint64_t> consumer_seq{0};
    uint32_t slot_count = 0;
    uint32_t slot_size = 0;
    char _pad[40];
};
static_assert(sizeof(TestRingHeader) == 64);

void writePayloadHeader(uint8_t* slot, micecam::domain::PayloadKind kind,
                         uint64_t pts_ns, uint64_t sequence, bool keyframe,
                         uint32_t size, int32_t width, int32_t height,
                         uint32_t checksum) {
    uint32_t kind_val = static_cast<uint32_t>(kind);
    std::memcpy(slot, &kind_val, 4);
    std::memcpy(slot + 8, &pts_ns, 8);
    std::memcpy(slot + 16, &sequence, 8);
    uint8_t kf = keyframe ? 1 : 0;
    std::memcpy(slot + 24, &kf, 1);
    std::memcpy(slot + 28, &size, 4);
    std::memcpy(slot + 32, &width, 4);
    std::memcpy(slot + 36, &height, 4);
    std::memcpy(slot + 40, &checksum, 4);
}

uint32_t computeChecksum(const uint8_t* data, uint32_t size) {
    uint32_t checksum = 0;
    if (data && size > 0) {
        const uint32_t* data32 = reinterpret_cast<const uint32_t*>(data);
        uint32_t words = std::min(size / 4u, 64u);
        for (uint32_t i = 0; i < words; i++) {
            checksum ^= data32[i];
        }
    }
    return checksum;
}

class TestRing {
public:
    ~TestRing() { destroy(); }

    bool create(const std::string& name, uint32_t slot_count, uint32_t slot_size) {
        name_ = name;
        slot_count_ = slot_count;
        slot_size_ = slot_size;
        total_size_ = kHeaderSize + static_cast<uint64_t>(slot_count) * slot_size;

        fd_ = shm_open(name.c_str(), O_CREAT | O_RDWR | O_EXCL, 0600);
        if (fd_ < 0) {
            fd_ = shm_open(name.c_str(), O_RDWR, 0600);
            if (fd_ < 0) return false;
        }

        if (ftruncate(fd_, static_cast<off_t>(total_size_)) != 0) {
            ::close(fd_);
            shm_unlink(name.c_str());
            fd_ = -1;
            return false;
        }

        mem_ = mmap(nullptr, total_size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (mem_ == MAP_FAILED) {
            ::close(fd_);
            shm_unlink(name.c_str());
            fd_ = -1;
            mem_ = nullptr;
            return false;
        }

        std::memset(mem_, 0, total_size_);
        header_ = static_cast<TestRingHeader*>(mem_);
        header_->slot_count = slot_count;
        header_->slot_size = slot_size;
        header_->producer_seq.store(0, std::memory_order_release);
        header_->consumer_seq.store(0, std::memory_order_release);
        return true;
    }

    void writeFrame(uint64_t seq, const std::vector<uint8_t>& payload,
                     micecam::domain::PayloadKind kind, uint64_t pts_ns,
                     int32_t width, int32_t height, bool keyframe) {
        uint32_t slot_idx = static_cast<uint32_t>(seq % slot_count_);
        uint8_t* slot = static_cast<uint8_t*>(mem_) + kHeaderSize
                        + static_cast<uint64_t>(slot_idx) * slot_size_;

        uint32_t payload_size = static_cast<uint32_t>(payload.size());
        uint32_t copy_size = std::min(payload_size, slot_size_ - static_cast<uint32_t>(kPayloadHeaderSize));

        uint32_t checksum = computeChecksum(payload.data(), copy_size);
        writePayloadHeader(slot, kind, pts_ns, seq, keyframe,
                           payload_size, width, height, checksum);

        if (copy_size > 0) {
            std::memcpy(slot + kPayloadHeaderSize, payload.data(), copy_size);
        }

        header_->producer_seq.store(seq + 1, std::memory_order_release);
    }

    void destroy() {
        if (mem_) {
            munmap(mem_, total_size_);
            mem_ = nullptr;
            header_ = nullptr;
        }
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
        if (!name_.empty()) {
            shm_unlink(name_.c_str());
            name_.clear();
        }
    }

    std::string name_;
    int fd_ = -1;
    void* mem_ = nullptr;
    uint64_t total_size_ = 0;
    TestRingHeader* header_ = nullptr;
    uint32_t slot_count_ = 0;
    uint32_t slot_size_ = 0;
};

std::string unique_shm_name() {
    static std::atomic<int> counter{0};
    return "/micecam_integ_ring_" + std::to_string(getpid()) + "_" + std::to_string(counter++);
}

std::vector<uint8_t> rgb_frame(int width, int height, int seed) {
    std::vector<uint8_t> data(static_cast<size_t>(width) * height * 3);
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<uint8_t>((seed + static_cast<int>(i)) % 256);
    }
    return data;
}

} // anonymous namespace

TEST(PluginPipelineIntegration, RawPluginFramesToMp4WithMetadata) {
    const std::string root = "/tmp/micecam_plugin_pipeline_integ";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    std::string shm_name = unique_shm_name();
    TestRing ring;
    ASSERT_TRUE(ring.create(shm_name, 16, kSlotSize));

    micecam::pipeline::RecordingPipeline pipeline;

    micecam::pipeline::SessionConfig config;
    config.session_id = "plugin_raw_test";
    config.output_dir = root;
    config.encoder.prefer_hardware = false;
    config.encoder.bitrate_kbps = 1000;
    config.encoder.keyframe_interval = 15;

    micecam::domain::StreamConfig stream_cfg;
    stream_cfg.device_id = "mock_cam_0";
    stream_cfg.stream_index = 0;
    stream_cfg.width = 160;
    stream_cfg.height = 120;
    stream_cfg.framerate = 30;
    config.streams.push_back(stream_cfg);

    ASSERT_TRUE(pipeline.start(config));

    nlohmann::json plugin_source;
    plugin_source["plugin_id"] = "test_ffmpeg_plugin";
    plugin_source["device_id"] = "usb_cam_0";
    plugin_source["transport"] = "posix_shm";
    plugin_source["ring_slot_count"] = 16;
    plugin_source["ring_slot_size"] = kSlotSize;
    pipeline.set_plugin_source(plugin_source);

    micecam::infrastructure::PluginStreamConfig consumer_cfg;
    consumer_cfg.plugin_id = "test_ffmpeg_plugin";
    consumer_cfg.device_id = "usb_cam_0";
    consumer_cfg.stream_id = "mock_cam_0_0";
    consumer_cfg.shm_name = shm_name;
    consumer_cfg.slot_count = 16;
    consumer_cfg.slot_size = kSlotSize;

    micecam::infrastructure::PluginStreamConsumer consumer(pipeline, consumer_cfg);
    ASSERT_TRUE(consumer.start());

    constexpr int kFrameCount = 10;
    for (int i = 0; i < kFrameCount; i++) {
        auto frame = rgb_frame(160, 120, i);
        ring.writeFrame(static_cast<uint64_t>(i), frame,
                        micecam::domain::PayloadKind::RAW,
                        static_cast<uint64_t>(i) * 33333333, 160, 120, i == 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(800));

    auto transport_stats = consumer.getTransportStats();
    EXPECT_EQ(transport_stats.frames_read, static_cast<uint64_t>(kFrameCount));
    EXPECT_EQ(transport_stats.frames_dropped, 0u);

    nlohmann::json transport_json;
    transport_json["frames_read"] = transport_stats.frames_read;
    transport_json["frames_dropped"] = transport_stats.frames_dropped;
    transport_json["backpressure_events"] = transport_stats.backpressure_events;
    transport_json["avg_consumer_lag"] = transport_stats.avg_consumer_lag;
    transport_json["max_consumer_lag"] = transport_stats.max_consumer_lag;
    pipeline.set_stream_transport_stats("mock_cam_0_0", transport_json);

    consumer.stop();
    pipeline.stop();
    const auto [meta, stats] = pipeline.result();

    EXPECT_EQ(meta.plugin_source["plugin_id"], "test_ffmpeg_plugin");
    EXPECT_EQ(meta.plugin_source["device_id"], "usb_cam_0");
    EXPECT_EQ(meta.plugin_source["transport"], "posix_shm");

    ASSERT_EQ(stats.size(), 1u);
    EXPECT_EQ(stats[0].frames_actual, static_cast<uint64_t>(kFrameCount));
    EXPECT_TRUE(stats[0].transport.is_object());
    EXPECT_EQ(stats[0].transport["frames_read"], kFrameCount);
    EXPECT_EQ(stats[0].transport["frames_dropped"], 0);

    std::string meta_path = root + "/plugin_raw_test/_meta.json";
    ASSERT_TRUE(std::filesystem::exists(meta_path));
    {
        std::ifstream in(meta_path);
        nlohmann::json meta_json;
        in >> meta_json;
        EXPECT_TRUE(meta_json.contains("plugin_source"));
        EXPECT_EQ(meta_json["plugin_source"]["plugin_id"], "test_ffmpeg_plugin");
    }

    std::string stats_path = root + "/plugin_raw_test/_stats.json";
    ASSERT_TRUE(std::filesystem::exists(stats_path));
    {
        std::ifstream in(stats_path);
        nlohmann::json stats_json;
        in >> stats_json;
        ASSERT_TRUE(stats_json.is_object());
        ASSERT_TRUE(stats_json.contains("mock_cam_0_0"));
        EXPECT_TRUE(stats_json["mock_cam_0_0"].contains("transport"));
        EXPECT_EQ(stats_json["mock_cam_0_0"]["transport"]["frames_read"], kFrameCount);
    }

    ring.destroy();
    std::filesystem::remove_all(root);
}

TEST(PluginPipelineIntegration, H264PassthroughToMp4WithMetadata) {
    const std::string root = "/tmp/micecam_plugin_h264_integ";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    std::string shm_name = unique_shm_name();
    TestRing ring;
    ASSERT_TRUE(ring.create(shm_name, kSlotCount, kSlotSize));

    micecam::pipeline::RecordingPipeline pipeline;

    micecam::pipeline::SessionConfig config;
    config.session_id = "plugin_h264_test";
    config.output_dir = root;
    config.encoder.prefer_hardware = false;
    config.encoder.bitrate_kbps = 1000;
    config.encoder.keyframe_interval = 15;

    micecam::domain::StreamConfig stream_cfg;
    stream_cfg.device_id = "mock_cam_0";
    stream_cfg.stream_index = 0;
    stream_cfg.width = 160;
    stream_cfg.height = 120;
    stream_cfg.framerate = 30;
    config.streams.push_back(stream_cfg);

    ASSERT_TRUE(pipeline.start(config));

    nlohmann::json plugin_source;
    plugin_source["plugin_id"] = "test_h264_plugin";
    plugin_source["device_id"] = "h264_cam_0";
    plugin_source["transport"] = "posix_shm";
    pipeline.set_plugin_source(plugin_source);

    micecam::infrastructure::PluginStreamConfig consumer_cfg;
    consumer_cfg.plugin_id = "test_h264_plugin";
    consumer_cfg.device_id = "h264_cam_0";
    consumer_cfg.stream_id = "mock_cam_0_0";
    consumer_cfg.shm_name = shm_name;
    consumer_cfg.slot_count = kSlotCount;
    consumer_cfg.slot_size = kSlotSize;

    micecam::infrastructure::PluginStreamConsumer consumer(pipeline, consumer_cfg);
    ASSERT_TRUE(consumer.start());

    std::vector<uint8_t> h264_nal{0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xC0, 0x1E,
                                    0xD9, 0x00, 0xA0, 0x47, 0xFE, 0x88};
    for (int i = 0; i < 5; i++) {
        ring.writeFrame(static_cast<uint64_t>(i), h264_nal,
                        micecam::domain::PayloadKind::H264,
                        static_cast<uint64_t>(i) * 33333333, 160, 120, i == 0);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    auto transport_stats = consumer.getTransportStats();
    EXPECT_EQ(transport_stats.frames_read, 5u);

    nlohmann::json transport_json;
    transport_json["frames_read"] = transport_stats.frames_read;
    transport_json["frames_dropped"] = transport_stats.frames_dropped;
    transport_json["backpressure_events"] = transport_stats.backpressure_events;
    pipeline.set_stream_transport_stats("mock_cam_0_0", transport_json);

    consumer.stop();
    pipeline.stop();
    const auto [meta, stats] = pipeline.result();

    EXPECT_EQ(meta.plugin_source["plugin_id"], "test_h264_plugin");
    ASSERT_EQ(stats.size(), 1u);
    EXPECT_TRUE(stats[0].transport.is_object());
    EXPECT_EQ(stats[0].transport["frames_read"], 5);

    ring.destroy();
    std::filesystem::remove_all(root);
}

TEST(PluginPipelineIntegration, PluginSourceAbsentWhenNotSet) {
    const std::string root = "/tmp/micecam_plugin_noplugin_integ";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    micecam::pipeline::RecordingPipeline pipeline;

    micecam::pipeline::SessionConfig config;
    config.session_id = "no_plugin_test";
    config.output_dir = root;
    config.encoder.prefer_hardware = false;
    config.encoder.bitrate_kbps = 1000;
    config.encoder.keyframe_interval = 15;

    micecam::domain::StreamConfig stream_cfg;
    stream_cfg.device_id = "mock_cam_0";
    stream_cfg.stream_index = 0;
    stream_cfg.width = 160;
    stream_cfg.height = 120;
    stream_cfg.framerate = 30;
    config.streams.push_back(stream_cfg);

    ASSERT_TRUE(pipeline.start(config));

    auto frame = rgb_frame(160, 120, 0);
    micecam::pipeline::FrameData data;
    data.stream_id = "mock_cam_0_0";
    data.data = frame.data();
    data.size = frame.size();
    data.width = 160;
    data.height = 120;
    data.pts = 0;
    data.source_format = "rgb24";
    pipeline.push_frame(data);

    pipeline.stop();
    const auto [meta, stats] = pipeline.result();

    EXPECT_TRUE(meta.plugin_source.is_null());

    std::string meta_path = root + "/no_plugin_test/_meta.json";
    ASSERT_TRUE(std::filesystem::exists(meta_path));
    {
        std::ifstream in(meta_path);
        nlohmann::json meta_json;
        in >> meta_json;
        EXPECT_FALSE(meta_json.contains("plugin_source"));
    }

    ASSERT_EQ(stats.size(), 1u);
    EXPECT_TRUE(stats[0].transport.is_null());

    std::filesystem::remove_all(root);
}
