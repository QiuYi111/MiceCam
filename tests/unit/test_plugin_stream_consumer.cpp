#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "domain/StreamRingDescriptor.h"
#include "infrastructure/PluginStreamConsumer.h"
#include "pipeline/RecordingPipeline.h"

using namespace micecam;
using namespace micecam::infrastructure;
using namespace micecam::pipeline;

namespace {

constexpr uint32_t kSlotCount = 8;
constexpr uint32_t kSlotSize = 65536;
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

void writePayloadHeader(uint8_t* slot, domain::PayloadKind kind,
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
                     domain::PayloadKind kind, uint64_t pts_ns,
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
    return "/micecam_test_consumer_" + std::to_string(getpid()) + "_" + std::to_string(counter++);
}

PluginStreamConfig makeConfig(const std::string& shm_name) {
    PluginStreamConfig cfg;
    cfg.plugin_id = "test_plugin";
    cfg.device_id = "dev0";
    cfg.stream_id = "mock_cam_0_0";
    cfg.shm_name = shm_name;
    cfg.slot_count = kSlotCount;
    cfg.slot_size = kSlotSize;
    return cfg;
}

std::vector<uint8_t> rgb_frame(int width, int height, int seed) {
    std::vector<uint8_t> data(static_cast<size_t>(width) * height * 3);
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<uint8_t>((seed + static_cast<int>(i)) % 256);
    }
    return data;
}

} // anonymous namespace

TEST(PluginStreamConsumerTest, GetPluginSourceInfo) {
    RecordingPipeline pipeline;
    auto config = makeConfig("/nonexistent");
    PluginStreamConsumer consumer(pipeline, config);

    auto info = consumer.getPluginSourceInfo();
    EXPECT_EQ(info.plugin_id, "test_plugin");
    EXPECT_EQ(info.device_id, "dev0");
    EXPECT_EQ(info.transport, "posix_shm");
    EXPECT_EQ(info.ring_slot_count, kSlotCount);
    EXPECT_EQ(info.ring_slot_size, kSlotSize);
}

TEST(PluginStreamConsumerTest, InitialTransportStatsZeroed) {
    RecordingPipeline pipeline;
    auto config = makeConfig("/nonexistent");
    PluginStreamConsumer consumer(pipeline, config);

    auto stats = consumer.getTransportStats();
    EXPECT_EQ(stats.frames_read, 0u);
    EXPECT_EQ(stats.frames_dropped, 0u);
    EXPECT_EQ(stats.backpressure_events, 0u);
    EXPECT_DOUBLE_EQ(stats.avg_consumer_lag, 0.0);
    EXPECT_DOUBLE_EQ(stats.max_consumer_lag, 0.0);
}

TEST(PluginStreamConsumerTest, StartFailsOnInvalidRing) {
    RecordingPipeline pipeline;
    auto config = makeConfig("/nonexistent");
    PluginStreamConsumer consumer(pipeline, config);

    EXPECT_FALSE(consumer.start());
}

TEST(PluginStreamConsumerTest, StopWithoutStartIsHarmless) {
    RecordingPipeline pipeline;
    auto config = makeConfig("/nonexistent");
    PluginStreamConsumer consumer(pipeline, config);

    consumer.stop();
    auto stats = consumer.getTransportStats();
    EXPECT_EQ(stats.frames_read, 0u);
}

TEST(PluginStreamConsumerTest, ConsumesFramesFromRing) {
    std::string name = unique_shm_name();
    TestRing ring;
    ASSERT_TRUE(ring.create(name, kSlotCount, kSlotSize));

    RecordingPipeline pipeline;

    micecam::pipeline::SessionConfig session_cfg;
    session_cfg.session_id = "consumer_test";
    session_cfg.output_dir = "/tmp/micecam_consumer_test";
    session_cfg.encoder.prefer_hardware = false;
    session_cfg.encoder.bitrate_kbps = 1000;
    session_cfg.encoder.keyframe_interval = 15;

    micecam::domain::StreamConfig stream_cfg;
    stream_cfg.device_id = "mock_cam_0";
    stream_cfg.stream_index = 0;
    stream_cfg.width = 160;
    stream_cfg.height = 120;
    stream_cfg.framerate = 30;
    session_cfg.streams.push_back(stream_cfg);

    ASSERT_TRUE(pipeline.start(session_cfg));

    auto config = makeConfig(name);
    PluginStreamConsumer consumer(pipeline, config);
    ASSERT_TRUE(consumer.start());

    constexpr int kFrameCount = 5;
    for (int i = 0; i < kFrameCount; i++) {
        auto frame = rgb_frame(160, 120, i);
        ring.writeFrame(static_cast<uint64_t>(i), frame, domain::PayloadKind::RAW,
                        static_cast<uint64_t>(i) * 33333333, 160, 120, i == 0);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    auto stats = consumer.getTransportStats();
    EXPECT_EQ(stats.frames_read, static_cast<uint64_t>(kFrameCount));
    EXPECT_EQ(stats.frames_dropped, 0u);

    consumer.stop();
    pipeline.stop();
    ring.destroy();

    std::filesystem::remove_all("/tmp/micecam_consumer_test");
}

TEST(PluginStreamConsumerTest, DetectsDroppedFrames) {
    std::string name = unique_shm_name();
    TestRing ring;
    ASSERT_TRUE(ring.create(name, 4, kSlotSize));

    RecordingPipeline pipeline;

    micecam::pipeline::SessionConfig session_cfg;
    session_cfg.session_id = "drop_test";
    session_cfg.output_dir = "/tmp/micecam_consumer_drop_test";
    session_cfg.encoder.prefer_hardware = false;
    session_cfg.encoder.bitrate_kbps = 1000;
    session_cfg.encoder.keyframe_interval = 15;

    micecam::domain::StreamConfig stream_cfg;
    stream_cfg.device_id = "mock_cam_0";
    stream_cfg.stream_index = 0;
    stream_cfg.width = 160;
    stream_cfg.height = 120;
    stream_cfg.framerate = 30;
    session_cfg.streams.push_back(stream_cfg);

    ASSERT_TRUE(pipeline.start(session_cfg));

    auto config = makeConfig(name);
    config.slot_count = 4;
    PluginStreamConsumer consumer(pipeline, config);
    ASSERT_TRUE(consumer.start());

    for (uint64_t i = 0; i < 20; i++) {
        auto frame = rgb_frame(160, 120, static_cast<int>(i));
        ring.writeFrame(i, frame, domain::PayloadKind::RAW,
                        i * 33333333, 160, 120, i == 0);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    auto stats = consumer.getTransportStats();
    EXPECT_GT(stats.frames_read, 0u);
    EXPECT_GT(stats.frames_dropped, 0u);
    EXPECT_GT(stats.backpressure_events, 0u);

    consumer.stop();
    pipeline.stop();
    ring.destroy();

    std::filesystem::remove_all("/tmp/micecam_consumer_drop_test");
}

TEST(PluginStreamConsumerTest, H264FramesPassThrough) {
    std::string name = unique_shm_name();
    TestRing ring;
    ASSERT_TRUE(ring.create(name, kSlotCount, kSlotSize));

    RecordingPipeline pipeline;

    micecam::pipeline::SessionConfig session_cfg;
    session_cfg.session_id = "h264_passthrough";
    session_cfg.output_dir = "/tmp/micecam_consumer_h264";
    session_cfg.encoder.prefer_hardware = false;
    session_cfg.encoder.bitrate_kbps = 1000;
    session_cfg.encoder.keyframe_interval = 15;

    micecam::domain::StreamConfig stream_cfg;
    stream_cfg.device_id = "mock_cam_0";
    stream_cfg.stream_index = 0;
    stream_cfg.width = 160;
    stream_cfg.height = 120;
    stream_cfg.framerate = 30;
    session_cfg.streams.push_back(stream_cfg);

    ASSERT_TRUE(pipeline.start(session_cfg));

    auto config = makeConfig(name);
    PluginStreamConsumer consumer(pipeline, config);
    ASSERT_TRUE(consumer.start());

    std::vector<uint8_t> h264_data{0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xC0, 0x1E};
    for (int i = 0; i < 3; i++) {
        ring.writeFrame(static_cast<uint64_t>(i), h264_data, domain::PayloadKind::H264,
                        static_cast<uint64_t>(i) * 33333333, 160, 120, i == 0);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    auto stats = consumer.getTransportStats();
    EXPECT_EQ(stats.frames_read, 3u);

    consumer.stop();
    pipeline.stop();
    ring.destroy();

    std::filesystem::remove_all("/tmp/micecam_consumer_h264");
}

TEST(PluginStreamConsumerTest, StopJoinsThreadCleanly) {
    std::string name = unique_shm_name();
    TestRing ring;
    ASSERT_TRUE(ring.create(name, kSlotCount, kSlotSize));

    RecordingPipeline pipeline;

    micecam::pipeline::SessionConfig session_cfg;
    session_cfg.session_id = "stop_test";
    session_cfg.output_dir = "/tmp/micecam_consumer_stop";
    session_cfg.encoder.prefer_hardware = false;
    session_cfg.encoder.bitrate_kbps = 1000;
    session_cfg.encoder.keyframe_interval = 15;

    micecam::domain::StreamConfig stream_cfg;
    stream_cfg.device_id = "mock_cam_0";
    stream_cfg.stream_index = 0;
    stream_cfg.width = 160;
    stream_cfg.height = 120;
    stream_cfg.framerate = 30;
    session_cfg.streams.push_back(stream_cfg);

    ASSERT_TRUE(pipeline.start(session_cfg));

    auto config = makeConfig(name);
    PluginStreamConsumer consumer(pipeline, config);
    ASSERT_TRUE(consumer.start());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto start = std::chrono::steady_clock::now();
    consumer.stop();
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 2000);

    pipeline.stop();
    ring.destroy();

    std::filesystem::remove_all("/tmp/micecam_consumer_stop");
}

TEST(PluginStreamConsumerTest, TransportStatsAreSerializable) {
    RecordingPipeline pipeline;
    auto config = makeConfig("/nonexistent");
    PluginStreamConsumer consumer(pipeline, config);

    auto stats = consumer.getTransportStats();
    nlohmann::json j;
    j["frames_read"] = stats.frames_read;
    j["frames_dropped"] = stats.frames_dropped;
    j["backpressure_events"] = stats.backpressure_events;
    j["avg_consumer_lag"] = stats.avg_consumer_lag;
    j["max_consumer_lag"] = stats.max_consumer_lag;

    EXPECT_EQ(j["frames_read"], 0);
    EXPECT_TRUE(j.is_object());

    auto info = consumer.getPluginSourceInfo();
    nlohmann::json info_j;
    info_j["plugin_id"] = info.plugin_id;
    info_j["device_id"] = info.device_id;
    info_j["transport"] = info.transport;
    info_j["ring_slot_count"] = info.ring_slot_count;
    info_j["ring_slot_size"] = info.ring_slot_size;

    EXPECT_EQ(info_j["plugin_id"], "test_plugin");
    EXPECT_EQ(info_j["transport"], "posix_shm");
}

TEST(PluginStreamConsumerTest, H265PayloadKindAccepted) {
    std::string name = unique_shm_name();
    TestRing ring;
    ASSERT_TRUE(ring.create(name, kSlotCount, kSlotSize));

    RecordingPipeline pipeline;

    micecam::pipeline::SessionConfig session_cfg;
    session_cfg.session_id = "h265_test";
    session_cfg.output_dir = "/tmp/micecam_consumer_h265";
    session_cfg.encoder.prefer_hardware = false;
    session_cfg.encoder.bitrate_kbps = 1000;
    session_cfg.encoder.keyframe_interval = 15;

    micecam::domain::StreamConfig stream_cfg;
    stream_cfg.device_id = "mock_cam_0";
    stream_cfg.stream_index = 0;
    stream_cfg.width = 160;
    stream_cfg.height = 120;
    stream_cfg.framerate = 30;
    session_cfg.streams.push_back(stream_cfg);

    ASSERT_TRUE(pipeline.start(session_cfg));

    auto config = makeConfig(name);
    PluginStreamConsumer consumer(pipeline, config);
    ASSERT_TRUE(consumer.start());

    std::vector<uint8_t> h265_data{0x00, 0x00, 0x00, 0x01, 0x40, 0x01, 0x0C};
    ring.writeFrame(0, h265_data, domain::PayloadKind::H265, 33333333, 160, 120, true);

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    auto stats = consumer.getTransportStats();
    EXPECT_EQ(stats.frames_read, 1u);

    consumer.stop();
    pipeline.stop();
    ring.destroy();

    std::filesystem::remove_all("/tmp/micecam_consumer_h265");
}
