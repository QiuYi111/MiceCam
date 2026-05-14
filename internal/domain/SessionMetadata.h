#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "StreamConfig.h"

namespace micecam::domain {

struct SessionMetadata {
    std::string session_id;
    uint64_t wall_clock_anchor_ns = 0;
    std::vector<StreamConfig> stream_configs;
    std::string encoder_name;
    int bitrate_kbps = 0;
    int keyframe_interval = 0;
    std::string output_dir;
    uint64_t start_time_ns = 0;
    uint64_t end_time_ns = 0;

    nlohmann::json to_json() const;
    static SessionMetadata from_json(const nlohmann::json& j);
};

} // namespace micecam::domain
