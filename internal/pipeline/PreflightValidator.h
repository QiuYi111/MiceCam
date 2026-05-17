#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "domain/CalibrationResult.h"
#include "domain/Capabilities.h"
#include "domain/StreamConfig.h"

namespace micecam::pipeline {

enum class PreflightSeverity { Info, Warning, Error };

struct PreflightItem {
    PreflightSeverity severity = PreflightSeverity::Error;
    std::string code;
    std::string title;
    std::string message;
    std::string stream_id;
};

struct PreflightResult {
    bool passed = false;
    std::string message;
    std::vector<std::string> warnings;
    std::vector<PreflightItem> items;
    std::map<std::string, domain::CalibrationResult> calibration_results;
};

class ICalibrationClient {
public:
    virtual ~ICalibrationClient() = default;
    virtual domain::CalibrationResult calibrate(
        const std::string& device_id, int stream_index,
        int width, int height, double fps) = 0;
};

class IStreamTestController {
public:
    virtual ~IStreamTestController() = default;
    virtual bool openStream(const domain::StreamConfig& config) = 0;
    virtual void closeStream(const std::string& stream_id) = 0;
    virtual uint64_t getDropCount(const std::string& stream_id) = 0;
};

struct StressTestResult {
    bool passed = false;
    std::vector<std::string> warnings;
    std::map<std::string, uint64_t> drop_counts;
};

class PreflightValidator {
public:
    bool check_disk_space(const std::string& output_dir, uint64_t estimated_bytes);
    bool check_capabilities(const domain::StreamConfig& config, const domain::Capabilities& caps);
    PreflightResult validate(const std::vector<domain::StreamConfig>& configs,
                             const std::string& output_dir,
                             int estimated_duration_s);
    PreflightResult validate_stream_capabilities(const domain::StreamConfig& config,
                                                 const domain::Capabilities& caps) const;

    PreflightResult validate(const std::vector<domain::StreamConfig>& configs,
                             const std::string& output_dir,
                             int estimated_duration_s,
                             ICalibrationClient* calibration_client,
                             IStreamTestController* stream_controller,
                             int stress_test_duration_ms = 3000);

    std::map<std::string, domain::CalibrationResult> run_phase1_calibration(
        const std::vector<domain::StreamConfig>& configs,
        ICalibrationClient* client);

    StressTestResult run_phase2_stress_test(
        const std::vector<domain::StreamConfig>& configs,
        IStreamTestController* controller,
        int duration_ms = 3000,
        const std::map<std::string, domain::CalibrationResult>* cal_results = nullptr);

    static int compute_min_gop(uint64_t i_frame_latency_ns,
                               uint64_t p_frame_latency_ns,
                               double fps);

private:
    uint64_t available_bytes_;
};

} // namespace micecam::pipeline
