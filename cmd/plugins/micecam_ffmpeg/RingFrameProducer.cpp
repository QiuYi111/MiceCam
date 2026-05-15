#include "RingFrameProducer.h"

#include <cerrno>
#include <cstring>
#include <chrono>
#include <thread>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <spdlog/spdlog.h>

namespace micecam::plugin {

RingFrameProducer::~RingFrameProducer() {
    release();
}

bool RingFrameProducer::create(const std::string& ring_id, uint32_t slot_count, uint32_t slot_size) {
    ring_id_ = ring_id;
    slot_count_ = slot_count;
    slot_size_ = slot_size;

    shm_name_ = "/micecam_ring_" + ring_id;
    shm_size_ = kHeaderSize + static_cast<uint64_t>(slot_count) * slot_size;

    shm_fd_ = shm_open(shm_name_.c_str(), O_CREAT | O_RDWR | O_EXCL, 0600);
    if (shm_fd_ < 0) {
        // If already exists, retry with O_EXCL removed
        shm_fd_ = shm_open(shm_name_.c_str(), O_RDWR, 0600);
        if (shm_fd_ < 0) {
            spdlog::error("shm_open failed for {}: {}", shm_name_, strerror(errno));
            return false;
        }
    }

    if (ftruncate(shm_fd_, static_cast<off_t>(shm_size_)) != 0) {
        spdlog::error("ftruncate failed for {}: {}", shm_name_, strerror(errno));
        ::close(shm_fd_);
        shm_fd_ = -1;
        shm_unlink(shm_name_.c_str());
        return false;
    }

    mapped_mem_ = mmap(nullptr, shm_size_, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0);
    if (mapped_mem_ == MAP_FAILED) {
        spdlog::error("mmap failed for {}: {}", shm_name_, strerror(errno));
        ::close(shm_fd_);
        shm_fd_ = -1;
        shm_unlink(shm_name_.c_str());
        mapped_mem_ = nullptr;
        return false;
    }

    // Initialize header
    header_ = static_cast<RingHeader*>(mapped_mem_);
    header_->producer_seq.store(0, std::memory_order_release);
    header_->consumer_seq.store(0, std::memory_order_release);
    header_->slot_count = slot_count;
    header_->slot_size = slot_size;

    // Initialize slots to zero
    uint8_t* slots = static_cast<uint8_t*>(mapped_mem_) + kHeaderSize;
    std::memset(slots, 0, static_cast<size_t>(slot_count) * slot_size);

    spdlog::info("RingFrameProducer created: {} ({} slots x {} bytes, total {})",
                 shm_name_, slot_count, slot_size, shm_size_);
    return true;
}

bool RingFrameProducer::writeFrame(const FrameData& frame, int timeout_ms) {
    if (!mapped_mem_) return false;

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);

    while (true) {
        uint64_t producer = header_->producer_seq.load(std::memory_order_acquire);
        uint64_t consumer = header_->consumer_seq.load(std::memory_order_acquire);

        // Check if ring is full (producer is slot_count ahead of consumer)
        if (producer - consumer >= slot_count_) {
            if (std::chrono::steady_clock::now() >= deadline) {
                spdlog::warn("RingFrameProducer: ring full, backpressure timeout");
                return false;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            continue;
        }

        uint32_t slot_index = static_cast<uint32_t>(producer % slot_count_);
        uint8_t* slot = static_cast<uint8_t*>(mapped_mem_) + kHeaderSize
                        + static_cast<uint64_t>(slot_index) * slot_size_;

        writePayloadHeader(slot, frame, producer);

        // Copy payload after header
        uint32_t payload_max = slot_size_ - static_cast<uint32_t>(domain::PayloadHeader::header_size());
        uint32_t copy_size = (frame.size < payload_max) ? frame.size : payload_max;
        if (frame.data && copy_size > 0) {
            std::memcpy(slot + domain::PayloadHeader::header_size(), frame.data, copy_size);
        }

        header_->producer_seq.store(producer + 1, std::memory_order_release);
        return true;
    }
}

void RingFrameProducer::writePayloadHeader(uint8_t* slot, const FrameData& frame, uint64_t seq) {
    domain::PayloadHeader ph;
    ph.kind = frame.kind;
    ph.pts_ns = frame.pts_ns;
    ph.sequence = seq;
    ph.keyframe = frame.keyframe;
    ph.size = frame.size;
    ph.width = frame.width;
    ph.height = frame.height;

    // Simple checksum: XOR of first 32 bytes of payload
    uint32_t checksum = 0;
    if (frame.data && frame.size > 0) {
        const uint32_t* data32 = reinterpret_cast<const uint32_t*>(frame.data);
        uint32_t words = std::min(frame.size / 4, 64u);
        for (uint32_t i = 0; i < words; i++) {
            checksum ^= data32[i];
        }
    }
    ph.checksum = checksum;

    // Write header into slot using memcpy to handle alignment
    std::memcpy(slot, &ph.kind, 4);           // kind (4 bytes for enum)
    std::memcpy(slot + 8, &ph.pts_ns, 8);     // pts_ns
    std::memcpy(slot + 16, &ph.sequence, 8);  // sequence
    std::memcpy(slot + 24, &ph.keyframe, 1);  // keyframe
    std::memcpy(slot + 28, &ph.size, 4);      // size
    std::memcpy(slot + 32, &ph.width, 4);     // width
    std::memcpy(slot + 36, &ph.height, 4);    // height
    std::memcpy(slot + 40, &ph.checksum, 4);  // checksum
}

domain::StreamRingDescriptor RingFrameProducer::descriptor(const std::string& stream_id) const {
    domain::StreamRingDescriptor desc;
    desc.ring_id = ring_id_;
    desc.stream_id = stream_id;
    desc.slot_count = slot_count_;
    desc.slot_size = slot_size_;
    desc.platform_handle_type = "posix_shm";
    desc.platform_handle_value = 0; // shm_name is the reference, not fd
    desc.ownership = domain::RingOwnership::PLUGIN_OWNS;
    desc.policy = domain::RingPolicy::NO_DROP;
    desc.producer_sequence_offset = 0;
    desc.consumer_sequence_offset = 8;
    desc.payload_offset = kHeaderSize;
    return desc;
}

void RingFrameProducer::release() {
    if (mapped_mem_) {
        munmap(mapped_mem_, shm_size_);
        mapped_mem_ = nullptr;
        header_ = nullptr;
    }
    if (shm_fd_ >= 0) {
        ::close(shm_fd_);
        shm_fd_ = -1;
    }
    if (!shm_name_.empty()) {
        shm_unlink(shm_name_.c_str());
        shm_name_.clear();
    }
    slot_count_ = 0;
    slot_size_ = 0;
    ring_id_.clear();
}

uint64_t RingFrameProducer::producerSeq() const {
    if (!header_) return 0;
    return header_->producer_seq.load(std::memory_order_acquire);
}

} // namespace micecam::plugin
