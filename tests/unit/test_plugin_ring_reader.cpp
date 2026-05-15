#include <gtest/gtest.h>

#include <algorithm>
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
#include "infrastructure/PluginRingReader.h"

using namespace micecam;
using namespace micecam::infrastructure;

namespace {

constexpr uint32_t kSlotCount = 4;
constexpr uint32_t kSlotSize = 4096;
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

uint32_t computeTestChecksum(const uint8_t* data, uint32_t size) {
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

        uint32_t checksum = computeTestChecksum(payload.data(), copy_size);
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
    return "/micecam_test_ring_" + std::to_string(getpid()) + "_" + std::to_string(counter++);
}

} // anonymous namespace

TEST(PluginRingReaderTest, OpenAndClose) {
    std::string name = unique_shm_name();
    TestRing ring;
    ASSERT_TRUE(ring.create(name, kSlotCount, kSlotSize));

    PluginRingReader reader;
    EXPECT_TRUE(reader.open(name));
    EXPECT_TRUE(reader.isOpen());
    reader.close();
    EXPECT_FALSE(reader.isOpen());

    ring.destroy();
}

TEST(PluginRingReaderTest, OpenNonexistentFails) {
    PluginRingReader reader;
    EXPECT_FALSE(reader.open("/micecam_nonexistent_ring_xyz"));
}

TEST(PluginRingReaderTest, ReadSingleFrame) {
    std::string name = unique_shm_name();
    TestRing ring;
    ASSERT_TRUE(ring.create(name, kSlotCount, kSlotSize));

    std::vector<uint8_t> payload(128, 0xAB);
    ring.writeFrame(0, payload, domain::PayloadKind::RAW, 1000000, 640, 480, false);

    PluginRingReader reader;
    ASSERT_TRUE(reader.open(name));

    ReadSlotData slot;
    EXPECT_TRUE(reader.readNextFrame(slot, 1000));

    EXPECT_EQ(slot.sequence, 0u);
    EXPECT_EQ(slot.timestamp_ns, 1000000u);
    EXPECT_EQ(slot.payload_kind, domain::PayloadKind::RAW);
    EXPECT_EQ(slot.width, 640);
    EXPECT_EQ(slot.height, 480);
    EXPECT_FALSE(slot.keyframe);
    EXPECT_EQ(slot.data.size(), 128u);

    for (auto b : slot.data) {
        EXPECT_EQ(b, 0xAB);
    }

    reader.close();
    ring.destroy();
}

TEST(PluginRingReaderTest, ReadMultipleFramesInOrder) {
    std::string name = unique_shm_name();
    TestRing ring;
    ASSERT_TRUE(ring.create(name, kSlotCount, kSlotSize));

    for (uint64_t i = 0; i < kSlotCount; i++) {
        std::vector<uint8_t> payload(4, static_cast<uint8_t>(i));
        ring.writeFrame(i, payload, domain::PayloadKind::RAW,
                        i * 33333333, 640, 480, i == 0);
    }

    PluginRingReader reader;
    ASSERT_TRUE(reader.open(name));

    for (uint64_t i = 0; i < kSlotCount; i++) {
        ReadSlotData slot;
        ASSERT_TRUE(reader.readNextFrame(slot, 1000));
        EXPECT_EQ(slot.sequence, i);
        EXPECT_EQ(slot.timestamp_ns, i * 33333333);
    }

    auto stats = reader.stats();
    EXPECT_EQ(stats.total_reads, kSlotCount);
    EXPECT_EQ(stats.total_drops, 0u);

    reader.close();
    ring.destroy();
}

TEST(PluginRingReaderTest, ReadH264Passthrough) {
    std::string name = unique_shm_name();
    TestRing ring;
    ASSERT_TRUE(ring.create(name, kSlotCount, kSlotSize));

    std::vector<uint8_t> h264_data{0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xC0};
    ring.writeFrame(0, h264_data, domain::PayloadKind::H264, 5000000, 1920, 1080, true);

    PluginRingReader reader;
    ASSERT_TRUE(reader.open(name));

    ReadSlotData slot;
    EXPECT_TRUE(reader.readNextFrame(slot, 1000));
    EXPECT_EQ(slot.payload_kind, domain::PayloadKind::H264);
    EXPECT_EQ(slot.data.size(), h264_data.size());
    EXPECT_TRUE(slot.keyframe);

    reader.close();
    ring.destroy();
}

TEST(PluginRingReaderTest, ChecksumVerification) {
    std::string name = unique_shm_name();
    TestRing ring;
    ASSERT_TRUE(ring.create(name, kSlotCount, kSlotSize));

    std::vector<uint8_t> payload(256);
    for (size_t i = 0; i < payload.size(); i++) {
        payload[i] = static_cast<uint8_t>(i & 0xFF);
    }
    ring.writeFrame(0, payload, domain::PayloadKind::RAW, 1000000, 640, 480, false);

    PluginRingReader reader;
    ASSERT_TRUE(reader.open(name));

    ReadSlotData slot;
    EXPECT_TRUE(reader.readNextFrame(slot, 1000));
    EXPECT_EQ(slot.data.size(), 256u);
    EXPECT_EQ(slot.checksum, computeTestChecksum(slot.data.data(), static_cast<uint32_t>(slot.data.size())));

    reader.close();
    ring.destroy();
}

TEST(PluginRingReaderTest, BackpressureSkipsSlots) {
    std::string name = unique_shm_name();
    TestRing ring;
    ASSERT_TRUE(ring.create(name, kSlotCount, kSlotSize));

    for (uint64_t i = 0; i < kSlotCount * 3; i++) {
        std::vector<uint8_t> payload(4, static_cast<uint8_t>(i & 0xFF));
        ring.writeFrame(i, payload, domain::PayloadKind::RAW,
                        i * 1000, 320, 240, false);
    }

    PluginRingReader reader;
    ASSERT_TRUE(reader.open(name));

    ReadSlotData slot;
    EXPECT_TRUE(reader.readNextFrame(slot, 1000));

    auto stats = reader.stats();
    EXPECT_EQ(stats.total_reads, 1u);
    EXPECT_GT(stats.total_drops, 0u);

    reader.close();
    ring.destroy();
}

TEST(PluginRingReaderTest, TimeoutWhenNoFrames) {
    std::string name = unique_shm_name();
    TestRing ring;
    ASSERT_TRUE(ring.create(name, kSlotCount, kSlotSize));

    PluginRingReader reader;
    ASSERT_TRUE(reader.open(name));

    ReadSlotData slot;
    auto start = std::chrono::steady_clock::now();
    bool result = reader.readNextFrame(slot, 200);
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_FALSE(result);
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 150);

    reader.close();
    ring.destroy();
}

TEST(PluginRingReaderTest, RoundTripWithProducer) {
    std::string name = unique_shm_name();
    TestRing ring;
    ASSERT_TRUE(ring.create(name, kSlotCount, kSlotSize));

    constexpr int kFrameCount = 3;
    for (int i = 0; i < kFrameCount; i++) {
        std::vector<uint8_t> payload(64, static_cast<uint8_t>(i));
        ring.writeFrame(static_cast<uint64_t>(i), payload, domain::PayloadKind::RAW,
                        static_cast<uint64_t>(i) * 33333333, 640, 480, i == 0);
    }

    PluginRingReader reader;
    ASSERT_TRUE(reader.open(name));

    int read_count = 0;
    for (int i = 0; i < kFrameCount; i++) {
        ReadSlotData slot;
        if (reader.readNextFrame(slot, 1000)) {
            read_count++;
        }
    }

    EXPECT_EQ(read_count, kFrameCount);

    auto stats = reader.stats();
    EXPECT_EQ(stats.total_reads, static_cast<uint64_t>(kFrameCount));

    reader.close();
    ring.destroy();
}

TEST(PluginRingReaderTest, StatsAreAccurate) {
    std::string name = unique_shm_name();
    TestRing ring;
    ASSERT_TRUE(ring.create(name, kSlotCount, kSlotSize));

    std::vector<uint8_t> payload(32, 0x42);
    ring.writeFrame(0, payload, domain::PayloadKind::RAW, 1000, 640, 480, false);
    ring.writeFrame(1, payload, domain::PayloadKind::RAW, 2000, 640, 480, false);

    PluginRingReader reader;
    ASSERT_TRUE(reader.open(name));

    ReadSlotData slot;
    reader.readNextFrame(slot, 1000);
    reader.readNextFrame(slot, 1000);

    auto stats = reader.stats();
    EXPECT_EQ(stats.total_reads, 2u);
    EXPECT_EQ(stats.total_drops, 0u);
    EXPECT_EQ(stats.current_lag, 0);

    reader.close();
    ring.destroy();
}

TEST(PluginRingReaderTest, MultiplePayloadKinds) {
    std::string name = unique_shm_name();
    TestRing ring;
    ASSERT_TRUE(ring.create(name, 8, 4096));

    std::vector<uint8_t> raw_data(100, 0x11);
    std::vector<uint8_t> mjpeg_data{0xFF, 0xD8, 0xFF, 0xE0};
    std::vector<uint8_t> h264_data{0x00, 0x00, 0x00, 0x01, 0x67};
    std::vector<uint8_t> h265_data{0x00, 0x00, 0x00, 0x01, 0x40};

    ring.writeFrame(0, raw_data, domain::PayloadKind::RAW, 1000, 640, 480, false);
    ring.writeFrame(1, mjpeg_data, domain::PayloadKind::MJPEG, 2000, 640, 480, false);
    ring.writeFrame(2, h264_data, domain::PayloadKind::H264, 3000, 1920, 1080, true);
    ring.writeFrame(3, h265_data, domain::PayloadKind::H265, 4000, 1920, 1080, true);

    PluginRingReader reader;
    ASSERT_TRUE(reader.open(name));

    ReadSlotData slot;

    ASSERT_TRUE(reader.readNextFrame(slot, 1000));
    EXPECT_EQ(slot.payload_kind, domain::PayloadKind::RAW);

    ASSERT_TRUE(reader.readNextFrame(slot, 1000));
    EXPECT_EQ(slot.payload_kind, domain::PayloadKind::MJPEG);

    ASSERT_TRUE(reader.readNextFrame(slot, 1000));
    EXPECT_EQ(slot.payload_kind, domain::PayloadKind::H264);

    ASSERT_TRUE(reader.readNextFrame(slot, 1000));
    EXPECT_EQ(slot.payload_kind, domain::PayloadKind::H265);

    reader.close();
    ring.destroy();
}
