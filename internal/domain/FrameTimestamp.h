#pragma once

#include <cstdint>

namespace micecam::domain {

struct FrameTimestamp {
    uint64_t session_offset_us = 0;
    uint64_t hardware_pts = 0;
    bool has_hardware_pts = false;
};

} // namespace micecam::domain
