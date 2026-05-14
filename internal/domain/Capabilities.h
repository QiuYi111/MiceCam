#pragma once

#include <string>
#include <vector>

#include "DeviceInfo.h"

namespace micecam::domain {

struct Capabilities {
    bool supports_hardware_encode = false;
    std::string encoder_name;
    std::string fallback_encoder_name;
    std::vector<StreamInfo> streams;
};

} // namespace micecam::domain
