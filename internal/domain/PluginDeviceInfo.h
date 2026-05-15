#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace micecam::domain {

struct PluginDeviceInfo {
    std::string device_id;
    std::string display_name;
    std::string plugin_id;
    std::string status;
    bool supports_raw = false;
    bool supports_mjpeg = false;
    bool supports_h264 = false;
    bool supports_h265 = false;
    int32_t max_width = 0;
    int32_t max_height = 0;
    double max_framerate = 0.0;
    std::optional<std::string> exclusive_resource_id;
    bool has_diagnostics = false;
    std::string diagnostics_code;
    std::string diagnostics_message;
};

} // namespace micecam::domain
