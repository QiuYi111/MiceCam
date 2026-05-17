#pragma once

#include <cstdint>
#include <string>

namespace micecam::domain {

enum class PayloadKind { RAW = 0, MJPEG = 1, H264 = 2, H265 = 3 };

enum class RingOwnership { HOST_OWNS, PLUGIN_OWNS };

enum class RingPolicy { NO_DROP, LATEST_FRAME };

struct PayloadHeader {
    PayloadKind kind = PayloadKind::RAW;
    uint64_t pts_ns = 0;
    uint64_t sequence = 0;
    bool keyframe = false;
    uint32_t size = 0;
    int32_t width = 0;
    int32_t height = 0;
    uint32_t checksum = 0;

    static constexpr uint64_t header_size() {
        // kind(1) + padding(7) + pts_ns(8) + sequence(8) + keyframe(1) + padding(3) + size(4) + width(4) + height(4) + checksum(4)
        return 44;
    }
};

struct StreamRingDescriptor {
    std::string ring_id;
    std::string stream_id;
    uint32_t slot_count = 0;
    uint32_t slot_size = 0;
    // platform_handle_type: e.g. "posix_shm_fd", "win32_mapping"
    std::string platform_handle_type;
    // platform_handle_value: FD number or handle name
    uintptr_t platform_handle_value = 0;
    RingOwnership ownership = RingOwnership::HOST_OWNS;
    RingPolicy policy = RingPolicy::NO_DROP;
    uint64_t producer_sequence_offset = 0;
    uint64_t consumer_sequence_offset = 0;
    uint64_t payload_offset = 0;

    static constexpr uint64_t header_size() {
        return PayloadHeader::header_size();
    }
};

} // namespace micecam::domain
