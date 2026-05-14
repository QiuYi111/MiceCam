#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json_fwd.hpp>

namespace micecam::domain {

struct StreamStats {
    std::string stream_id;
    uint64_t frames_expected = 0;
    uint64_t frames_actual = 0;
    double drop_rate = 0.0;
    double avg_encode_latency_us = 0.0;
    double max_encode_latency_us = 0.0;
    double min_encode_latency_us = 0.0;
    double avg_frame_interval_us = 0.0;
    double max_frame_interval_deviation_us = 0.0;
    uint64_t bytes_written = 0;
    std::string encoder_used;
    bool encoder_fallback = false;

    nlohmann::json to_json() const;
};

} // namespace micecam::domain
