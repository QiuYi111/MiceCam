#include "PreflightValidator.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/statvfs.h>
#endif

#include <algorithm>
#include <sstream>

namespace micecam::pipeline {

bool PreflightValidator::check_disk_space(const std::string& output_dir, uint64_t estimated_bytes) {
#ifdef _WIN32
    ULARGE_INTEGER free_bytes;
    if (!GetDiskFreeSpaceExA(output_dir.c_str(), &free_bytes, nullptr, nullptr)) {
        return false;
    }
    available_bytes_ = free_bytes.QuadPart;
    return available_bytes_ >= estimated_bytes;
#else
    struct statvfs buf;
    if (statvfs(output_dir.c_str(), &buf) != 0) {
        return false;
    }
    uint64_t avail = static_cast<uint64_t>(buf.f_bavail) * buf.f_frsize;
    available_bytes_ = avail;
    return avail >= estimated_bytes;
#endif
}

bool PreflightValidator::check_capabilities(const domain::StreamConfig& config,
                                             const domain::Capabilities& caps) {
    for (const auto& si : caps.streams) {
        if (config.width <= si.max_width && config.height <= si.max_height) {
            bool format_ok = false;
            for (const auto& fmt : si.supported_formats) {
                if (fmt == config.pixel_format || config.pixel_format.empty()) {
                    format_ok = true;
                    break;
                }
            }
            bool fps_ok = false;
            for (auto fps : si.supported_framerates) {
                if (fps == config.framerate || config.framerate == 0) {
                    fps_ok = true;
                    break;
                }
            }
            if (format_ok && fps_ok) return true;
        }
    }
    return false;
}

PreflightResult PreflightValidator::validate(
    const std::vector<domain::StreamConfig>& configs,
    const std::string& output_dir,
    int estimated_duration_s) {

    PreflightResult result;
    result.passed = true;

    uint64_t estimated_bytes = static_cast<uint64_t>(configs.size()) *
                                static_cast<uint64_t>(5000 * 1000 / 8) * estimated_duration_s;

    if (!check_disk_space(output_dir, estimated_bytes)) {
        result.passed = false;
        std::ostringstream ss;
        ss << "Insufficient disk space. Need at least " << (estimated_bytes / (1024 * 1024))
           << " MB, available: " << (available_bytes_ / (1024 * 1024)) << " MB";
        result.message = ss.str();
    } else {
        result.message = "Preflight checks passed";
    }

    return result;
}

} // namespace micecam::pipeline
