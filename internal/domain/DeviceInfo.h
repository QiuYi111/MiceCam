#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace micecam::domain {

struct ResolutionOption {
    int width = 0;
    int height = 0;
    std::string label;
};

struct StreamInfo {
    int index = 0;
    int max_width = 0;
    int max_height = 0;
    std::string label;
    std::vector<ResolutionOption> resolutions;
    std::vector<std::string> supported_formats;
    std::vector<int> supported_framerates;
    bool available = true;
    std::string unavailable_reason;
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
