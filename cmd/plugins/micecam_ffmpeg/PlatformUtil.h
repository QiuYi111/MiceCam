#pragma once

#include <string>

namespace micecam::plugin {

struct PlatformUtil {
    static std::string currentOsName();
    static std::string currentArch();
};

} // namespace micecam::plugin
