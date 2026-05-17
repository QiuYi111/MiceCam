#pragma once
#include <chrono>
#include <cstdint>
#include <string>

namespace micecam::domain {

enum class StreamLivenessState {
    ACTIVE,
    STALLED,
    FINALIZED
};

struct StreamStallEvent {
    std::string stream_id;
    std::string plugin_id;
    uint64_t stall_duration_ms;
    std::chrono::steady_clock::time_point detected_at;
};

} // namespace micecam::domain
