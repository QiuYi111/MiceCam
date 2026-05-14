#pragma once

#include <string>

namespace micecam::infrastructure {

class HardwareEncoderSelector {
public:
    static std::string detect_platform_encoder();
    static bool is_hardware_encoder(const std::string& name);
    static std::string get_fallback_encoder();
};

} // namespace micecam::infrastructure
