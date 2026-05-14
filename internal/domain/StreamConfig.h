#pragma once

#include <string>

namespace micecam::domain {

struct StreamConfig {
    std::string device_id;
    int stream_index = 0;
    int width = 0;
    int height = 0;
    int framerate = 0;
    std::string pixel_format;
};

} // namespace micecam::domain
