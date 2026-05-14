#pragma once

#include <chrono>
#include <mutex>

#include "FrameTimestamp.h"

namespace micecam::domain {

class TimestampEngine {
public:
    void capture_wall_anchor();
    uint64_t to_session_offset(std::chrono::steady_clock::time_point frame_time);
    FrameTimestamp with_hardware_pts(uint64_t hw_pts);

private:
    std::mutex mutex_;
    std::chrono::system_clock::time_point wall_anchor_;
    std::chrono::steady_clock::time_point session_start_;
};

} // namespace micecam::domain
