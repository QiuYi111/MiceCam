#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace micecam::domain {

struct StreamInfo {
    int index = 0;
    int max_width = 0;
    int max_height = 0;
    std::vector<std::string> supported_formats;
    std::vector<int> supported_framerates;
};

struct DeviceInfo {
    std::string id;
    std::string name;
    std::string vendor;
    std::string serial;
    std::string type;
    std::vector<StreamInfo> streams;
};

} // namespace micecam::domain
