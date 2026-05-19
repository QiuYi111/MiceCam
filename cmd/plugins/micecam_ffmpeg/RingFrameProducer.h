#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <memory>

#include "domain/StreamRingDescriptor.h"
#include "infrastructure/SharedMemoryBackend.h"

namespace micecam::plugin {

struct RingHeader {
    std::atomic<uint64_t> producer_seq;
    std::atomic<uint64_t> consumer_seq;
    uint32_t slot_count;
    uint32_t slot_size;
    char _pad[40];
};
static_assert(sizeof(RingHeader) == 64, "RingHeader must be 64 bytes");

struct FrameData {
    const uint8_t* data = nullptr;
    uint32_t size = 0;
    int width = 0;
    int height = 0;
    domain::PayloadKind kind = domain::PayloadKind::RAW;
    uint64_t pts_ns = 0;
    bool keyframe = false;
};

class RingFrameProducer {
public:
    RingFrameProducer() = default;
    ~RingFrameProducer();

    RingFrameProducer(const RingFrameProducer&) = delete;
    RingFrameProducer& operator=(const RingFrameProducer&) = delete;
    RingFrameProducer(RingFrameProducer&&) = delete;
    RingFrameProducer& operator=(RingFrameProducer&&) = delete;

    bool create(const std::string& ring_id, uint32_t slot_count, uint32_t slot_size);

    bool writeFrame(const FrameData& frame, int timeout_ms = 100);

    domain::StreamRingDescriptor descriptor(const std::string& stream_id) const;

    void release();

    bool isValid() const { return mapped_mem_ != nullptr; }
    uint64_t producerSeq() const;

    static constexpr uint64_t kHeaderSize = 64;

private:
    std::string shm_name_;
    int shm_fd_ = -1;
    void* mapped_mem_ = nullptr;
    uint64_t shm_size_ = 0;
    RingHeader* header_ = nullptr;
    uint32_t slot_count_ = 0;
    uint32_t slot_size_ = 0;
    std::string ring_id_;

    void writePayloadHeader(uint8_t* slot, const FrameData& frame, uint64_t seq);

    std::unique_ptr<micecam::infrastructure::SharedMemoryBackend> backend_ =
        micecam::infrastructure::create_shared_memory_backend();
};

} // namespace micecam::plugin
