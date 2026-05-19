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
    std::lock_guard<std::mutex> lock(mutex_);
    FrameTimestamp ts;
    ts.has_hardware_pts = true;
    ts.hardware_pts = hw_pts;
    ts.session_offset_us = 0;
    return ts;
}

FrameTimestamp TimestampEngine::populate(std::chrono::steady_clock::time_point frame_time) {
    std::lock_guard<std::mutex> lock(mutex_);
    FrameTimestamp ts;

    auto steady_delta = std::chrono::duration_cast<std::chrono::microseconds>(frame_time - session_start_);
    ts.session_offset_us = static_cast<uint64_t>(steady_delta.count());

    auto wall_delta = std::chrono::duration_cast<std::chrono::nanoseconds>(frame_time - session_start_);
    auto wall_ns = std::chrono::time_point_cast<std::chrono::nanoseconds>(wall_anchor_).time_since_epoch().count();
    ts.wall_time_ns = static_cast<uint64_t>(wall_ns) + static_cast<uint64_t>(wall_delta.count());

    return ts;
}

} // namespace micecam::domain
