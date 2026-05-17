#include "infrastructure/HardwareEncoderSelector.h"

namespace micecam::infrastructure {

std::string HardwareEncoderSelector::detect_platform_encoder() {
#ifdef __APPLE__
    return "h264_videotoolbox";
#elif defined(_WIN32)
    return "h264_nvenc";
#elif defined(__linux__)
    return "h264_vaapi";
#else
    return "libx264";
#endif
}

bool HardwareEncoderSelector::is_hardware_encoder(const std::string& name) {
    if (name.empty()) return false;
    if (name == "libx264") return false;
    if (name == "h264") return false;
    if (name.find("h264_") == 0 || name.find("hevc_") == 0) {
        return name != "libx264";
    }
    return name == "h264_videotoolbox"
        || name == "h264_nvenc"
        || name == "h264_qsv"
        || name == "h264_vaapi"
        || name == "h264_amf";
}

std::string HardwareEncoderSelector::get_fallback_encoder() {
    return "libx264";
}

} // namespace micecam::infrastructure
