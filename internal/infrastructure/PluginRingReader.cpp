#include "infrastructure/PluginRingReader.h"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <thread>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <spdlog/spdlog.h>

namespace micecam::infrastructure {

PluginRingReader::~PluginRingReader() {
    close();
}

bool PluginRingReader::open(const std::string& shm_name) {
    shm_name_ = shm_name;

    shm_fd_ = shm_open(shm_name.c_str(), O_RDWR, 0);
    if (shm_fd_ < 0) {
        spdlog::error("PluginRingReader: shm_open failed for {}: {}", shm_name, strerror(errno));
        return false;
    }

    struct stat st;
    if (fstat(shm_fd_, &st) != 0) {
        spdlog::error("PluginRingReader: fstat failed for {}: {}", shm_name, strerror(errno));
        ::close(shm_fd_);
        shm_fd_ = -1;
        return false;
    }
    shm_size_ = static_cast<uint64_t>(st.st_size);

    if (shm_size_ < kHeaderSize) {
        spdlog::error("PluginRingReader: SHM too small ({}) for {}", shm_size_, shm_name);
        ::close(shm_fd_);
        shm_fd_ = -1;
        return false;
    }

    mapped_mem_ = mmap(nullptr, shm_size_, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0);
    if (mapped_mem_ == MAP_FAILED) {
        spdlog::error("PluginRingReader: mmap failed for {}: {}", shm_name, strerror(errno));
        ::close(shm_fd_);
        shm_fd_ = -1;
        mapped_mem_ = nullptr;
        return false;
    }

    header_ = static_cast<RingHeader*>(mapped_mem_);
    slot_count_ = header_->slot_count;
    slot_size_ = header_->slot_size;

    uint64_t expected_size = kHeaderSize + static_cast<uint64_t>(slot_count_) * slot_size_;
    if (shm_size_ < expected_size) {
        spdlog::error("PluginRingReader: SHM size {} < expected {} for {}",
                      shm_size_, expected_size, shm_name);
        munmap(mapped_mem_, shm_size_);
        ::close(shm_fd_);
        mapped_mem_ = nullptr;
        shm_fd_ = -1;
        header_ = nullptr;
        return false;
    }

    next_consumer_seq_ = header_->consumer_seq.load(std::memory_order_acquire);

    spdlog::info("PluginRingReader opened: {} ({} slots x {} bytes, consumer_seq={})",
                 shm_name, slot_count_, slot_size_, next_consumer_seq_);
    return true;
}

void PluginRingReader::readPayloadHeader(const uint8_t* slot, ReadSlotData& out) {
    uint32_t kind_val;
    std::memcpy(&kind_val, slot, 4);
    out.payload_kind = static_cast<domain::PayloadKind>(kind_val);

    std::memcpy(&out.timestamp_ns, slot + 8, 8);
    std::memcpy(&out.sequence, slot + 16, 8);

    uint8_t kf;
    std::memcpy(&kf, slot + 24, 1);
    out.keyframe = (kf != 0);

    std::memcpy(&out.payload_size, slot + 28, 4);
    std::memcpy(&out.width, slot + 32, 4);
    std::memcpy(&out.height, slot + 36, 4);
    std::memcpy(&out.checksum, slot + 40, 4);
}

uint32_t PluginRingReader::computeChecksum(const uint8_t* data, uint32_t size) {
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

bool PluginRingReader::readNextFrame(ReadSlotData& out, int timeout_ms) {
    if (!mapped_mem_) return false;

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);

    while (true) {
        uint64_t producer = header_->producer_seq.load(std::memory_order_acquire);

        if (next_consumer_seq_ >= producer) {
            if (std::chrono::steady_clock::now() >= deadline) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(500));
            continue;
        }

        uint64_t lag = producer - next_consumer_seq_;
        if (lag > slot_count_) {
            uint64_t skip_count = lag - slot_count_;
            next_consumer_seq_ = producer - slot_count_;

            std::lock_guard<std::mutex> lock(stats_mutex_);
            total_drops_ += skip_count;
            spdlog::warn("PluginRingReader: backpressure, skipped {} frames (lag={})", skip_count, lag);
            continue;
        }

        uint32_t slot_index = static_cast<uint32_t>(next_consumer_seq_ % slot_count_);
        uint8_t* slot = static_cast<uint8_t*>(mapped_mem_) + kHeaderSize
                        + static_cast<uint64_t>(slot_index) * slot_size_;

        readPayloadHeader(slot, out);

        uint32_t copy_size = out.payload_size;
        uint32_t payload_max = slot_size_ - static_cast<uint32_t>(kPayloadHeaderSize);
        if (copy_size > payload_max) {
            copy_size = payload_max;
        }

        out.data.resize(copy_size);
        if (copy_size > 0) {
            std::memcpy(out.data.data(), slot + kPayloadHeaderSize, copy_size);
        }

        uint32_t actual_checksum = computeChecksum(out.data.data(), copy_size);

        next_consumer_seq_++;
        header_->consumer_seq.store(next_consumer_seq_, std::memory_order_release);

        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            total_reads_++;
            current_lag_ = static_cast<int64_t>(header_->producer_seq.load(std::memory_order_acquire) - next_consumer_seq_);
        }

        if (actual_checksum != out.checksum) {
            spdlog::warn("PluginRingReader: checksum mismatch on seq {} (expected {}, got {})",
                         out.sequence, out.checksum, actual_checksum);
        }

        return true;
    }
}

void PluginRingReader::close() {
    if (mapped_mem_) {
        munmap(mapped_mem_, shm_size_);
        mapped_mem_ = nullptr;
        header_ = nullptr;
    }
    if (shm_fd_ >= 0) {
        ::close(shm_fd_);
        shm_fd_ = -1;
    }
    shm_name_.clear();
    slot_count_ = 0;
    slot_size_ = 0;
    next_consumer_seq_ = 0;
}

ReaderStats PluginRingReader::stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    ReaderStats s;
    s.total_reads = total_reads_;
    s.total_drops = total_drops_;
    s.current_lag = current_lag_;
    return s;
}

} // namespace micecam::infrastructure
