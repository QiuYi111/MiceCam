#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace micecam::domain {

struct CalibrationResult {
    std::string stream_id;
    uint64_t i_frame_latency_ns = 0;
    uint64_t p_frame_latency_ns = 0;
    int min_gop = 0;
    uint64_t recommended_slot_size = 0;
    std::string actual_encoder_name;
    double max_sustainable_fps = 0.0;
    bool success = false;
    bool degraded_resolution = false;
    int actual_width = 0;
    int actual_height = 0;
    std::vector<std::string> warnings;
};

} // namespace micecam::domain
