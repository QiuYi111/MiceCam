#include "PreflightValidator.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/statvfs.h>
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>
#include <thread>

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

int PreflightValidator::compute_min_gop(uint64_t i_frame_latency_ns,
                                         uint64_t p_frame_latency_ns,
                                         double fps) {
    if (fps <= 0.0) return -1;
    double frame_interval_ns = 1e9 / fps;
    double p_ns = static_cast<double>(p_frame_latency_ns);
    if (p_ns >= frame_interval_ns) return -1;
    double divisor = frame_interval_ns - p_ns;
    if (divisor <= 0.0) return -1;
    return static_cast<int>(std::ceil(static_cast<double>(i_frame_latency_ns) / divisor));
}

std::map<std::string, domain::CalibrationResult> PreflightValidator::run_phase1_calibration(
    const std::vector<domain::StreamConfig>& configs,
    ICalibrationClient* client) {

    std::map<std::string, domain::CalibrationResult> results;

    for (const auto& config : configs) {
        std::string stream_id = config.device_id + ":" + std::to_string(config.stream_index);

        auto cal = client->calibrate(config.device_id, config.stream_index,
                                     config.width, config.height,
                                     static_cast<double>(config.framerate));
        cal.stream_id = stream_id;

        if (!cal.success) {
            int half_w = config.width / 2;
            int half_h = config.height / 2;
            auto retry = client->calibrate(config.device_id, config.stream_index,
                                           half_w, half_h,
                                           static_cast<double>(config.framerate));
            retry.stream_id = stream_id;
            retry.degraded_resolution = true;
            retry.actual_width = half_w;
            retry.actual_height = half_h;

            if (!retry.success) {
                retry.warnings.push_back("Calibration failed even at reduced resolution");
                results[stream_id] = retry;
                continue;
            }
            retry.warnings.push_back("Degraded to " + std::to_string(half_w) +
                                     "x" + std::to_string(half_h));
            cal = retry;
        }

        int min_gop = compute_min_gop(cal.i_frame_latency_ns, cal.p_frame_latency_ns,
                                       static_cast<double>(config.framerate));

        if (min_gop < 0) {
            cal.success = false;
            cal.warnings.push_back("Encoder cannot sustain " + std::to_string(config.framerate) +
                                   " fps: P-frame latency exceeds frame interval");
        }

        cal.min_gop = min_gop;
        results[stream_id] = cal;
    }

    return results;
}

StressTestResult PreflightValidator::run_phase2_stress_test(
    const std::vector<domain::StreamConfig>& configs,
    IStreamTestController* controller,
    int duration_ms) {

    StressTestResult result;
    result.passed = true;

    std::vector<std::string> stream_ids;
    for (const auto& config : configs) {
        std::string stream_id = config.device_id + ":" + std::to_string(config.stream_index);
        if (!controller->openStream(config)) {
            result.passed = false;
            result.warnings.push_back("Failed to open stream: " + stream_id);
            continue;
        }
        stream_ids.push_back(stream_id);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));

    for (const auto& sid : stream_ids) {
        uint64_t drops = controller->getDropCount(sid);
        result.drop_counts[sid] = drops;
        if (drops > 0) {
            result.passed = false;
            std::ostringstream ss;
            ss << "Stream " << sid << " dropped " << drops << " frames during stress test";
            result.warnings.push_back(ss.str());
        }
    }

    for (const auto& sid : stream_ids) {
        controller->closeStream(sid);
    }

    return result;
}

PreflightResult PreflightValidator::validate(
    const std::vector<domain::StreamConfig>& configs,
    const std::string& output_dir,
    int estimated_duration_s,
    ICalibrationClient* calibration_client,
    IStreamTestController* stream_controller,
    int stress_test_duration_ms) {

    PreflightResult result = validate(configs, output_dir, estimated_duration_s);
    if (!result.passed) return result;

    if (calibration_client) {
        auto cal_results = run_phase1_calibration(configs, calibration_client);
        result.calibration_results = cal_results;

        for (const auto& [sid, cal] : cal_results) {
            if (!cal.success) {
                result.passed = false;
                result.message = "Phase 1 calibration failed for stream: " + sid;
                return result;
            }
        }
    }

    if (stream_controller) {
        auto stress_result = run_phase2_stress_test(configs, stream_controller,
                                                     stress_test_duration_ms);
        for (const auto& w : stress_result.warnings) {
            result.warnings.push_back(w);
        }
    }

    return result;
}

} // namespace micecam::pipeline
