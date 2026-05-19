#include "PlatformUtil.h"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

namespace micecam::plugin {

std::string PlatformUtil::currentOsName() {
#if defined(__APPLE__)
    return "darwin";
#elif defined(_WIN32)
    return "win32";
#elif defined(__linux__)
    return "linux";
#else
    return "unknown";
#endif
}

std::string PlatformUtil::currentArch() {
#if defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "arm64";
#else
    return "unknown";
#endif
}

} // namespace micecam::plugin
