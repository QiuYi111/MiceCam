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

PreflightResult PreflightValidator::validate_stream_capabilities(
    const domain::StreamConfig& config,
    const domain::Capabilities& caps) const {

    PreflightResult result;
    result.passed = true;

    const domain::StreamInfo* matched_stream = nullptr;
    for (const auto& si : caps.streams) {
        if (si.index == config.stream_index) {
            matched_stream = &si;
            break;
        }
    }

    if (!matched_stream) {
        result.passed = false;
        result.items.push_back({
            .severity = PreflightSeverity::Error,
            .code = "missing_capabilities",
            .stream_id = config.device_id,
        });
        result.message = "Stream index not found in device capabilities";
        return result;
    }

    if (!matched_stream->resolutions.empty()) {
        bool res_ok = false;
        for (const auto& res : matched_stream->resolutions) {
            if (res.width == config.width && res.height == config.height) {
                res_ok = true;
                break;
            }
        }
        if (!res_ok) {
            result.items.push_back({
                .severity = PreflightSeverity::Error,
                .code = "unsupported_resolution",
                .stream_id = config.device_id,
            });
        }
    }

    bool fps_ok = false;
    for (auto fps : matched_stream->supported_framerates) {
        if (fps == config.framerate) {
            fps_ok = true;
            break;
        }
    }
    if (!fps_ok) {
        result.items.push_back({
            .severity = PreflightSeverity::Error,
            .code = "unsupported_framerate",
            .stream_id = config.device_id,
        });
    }

    bool format_ok = false;
    for (const auto& fmt : matched_stream->supported_formats) {
        if (fmt == config.pixel_format) {
            format_ok = true;
            break;
        }
    }
    if (!format_ok) {
        result.items.push_back({
            .severity = PreflightSeverity::Error,
            .code = "unsupported_format",
            .stream_id = config.device_id,
        });
    }

    result.passed = result.items.empty();
    result.message = result.passed ? "Preflight checks passed" : "Preflight checks failed";
    return result;
}

} // namespace micecam::pipeline
