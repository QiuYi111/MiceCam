#include "TimestampEngine.h"

namespace micecam::domain {

void TimestampEngine::capture_wall_anchor() {
    std::lock_guard<std::mutex> lock(mutex_);
    wall_anchor_ = std::chrono::system_clock::now();
    session_start_ = std::chrono::steady_clock::now();
}

uint64_t TimestampEngine::to_session_offset(std::chrono::steady_clock::time_point frame_time) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto delta = std::chrono::duration_cast<std::chrono::microseconds>(frame_time - session_start_);
    return static_cast<uint64_t>(delta.count());
}

FrameTimestamp TimestampEngine::with_hardware_pts(uint64_t hw_pts) {
    FrameTimestamp ts;
    ts.has_hardware_pts = true;
    ts.hardware_pts = hw_pts;
    ts.session_offset_us = 0;
    return ts;
}

} // namespace micecam::domain
