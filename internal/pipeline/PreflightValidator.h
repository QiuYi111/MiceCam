#pragma once

#include <cstdint>
#include <string>
#include <vector>

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

private:
    uint64_t available_bytes_;
};

} // namespace micecam::pipeline
