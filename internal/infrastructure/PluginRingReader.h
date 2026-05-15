#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "domain/StreamRingDescriptor.h"

namespace micecam::infrastructure {

struct RingHeader {
    std::atomic<uint64_t> producer_seq;
    std::atomic<uint64_t> consumer_seq;
    uint32_t slot_count;
    uint32_t slot_size;
    char _pad[40];
};
static_assert(sizeof(RingHeader) == 64, "RingHeader must be 64 bytes");

struct ReadSlotData {
    uint64_t sequence = 0;
    uint64_t timestamp_ns = 0;
    domain::PayloadKind payload_kind = domain::PayloadKind::RAW;
    uint32_t payload_size = 0;
    int32_t width = 0;
    int32_t height = 0;
    bool keyframe = false;
    uint32_t checksum = 0;
    std::vector<uint8_t> data;
};

struct ReaderStats {
    uint64_t total_reads = 0;
    uint64_t total_drops = 0;
    int64_t current_lag = 0;
};

class PluginRingReader {
public:
    PluginRingReader() = default;
    ~PluginRingReader();

    PluginRingReader(const PluginRingReader&) = delete;
    PluginRingReader& operator=(const PluginRingReader&) = delete;

    bool open(const std::string& shm_name);
    bool readNextFrame(ReadSlotData& out, int timeout_ms);
    void close();
    ReaderStats stats() const;
    bool isOpen() const { return mapped_mem_ != nullptr; }

    static constexpr uint64_t kHeaderSize = 64;
    static constexpr uint64_t kPayloadHeaderSize = 44;

private:
    void readPayloadHeader(const uint8_t* slot, ReadSlotData& out);
    static uint32_t computeChecksum(const uint8_t* data, uint32_t size);

    std::string shm_name_;
    int shm_fd_ = -1;
    void* mapped_mem_ = nullptr;
    uint64_t shm_size_ = 0;
    RingHeader* header_ = nullptr;
    uint32_t slot_count_ = 0;
    uint32_t slot_size_ = 0;
    uint64_t next_consumer_seq_ = 0;

    mutable std::mutex stats_mutex_;
    uint64_t total_reads_ = 0;
    uint64_t total_drops_ = 0;
    int64_t current_lag_ = 0;
};

} // namespace micecam::infrastructure
