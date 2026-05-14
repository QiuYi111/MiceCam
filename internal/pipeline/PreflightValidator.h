#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "domain/Capabilities.h"
#include "domain/StreamConfig.h"

namespace micecam::pipeline {

struct PreflightResult {
    bool passed = false;
    std::string message;
    std::vector<std::string> warnings;
};

class PreflightValidator {
public:
    bool check_disk_space(const std::string& output_dir, uint64_t estimated_bytes);
    bool check_capabilities(const domain::StreamConfig& config, const domain::Capabilities& caps);
    PreflightResult validate(const std::vector<domain::StreamConfig>& configs,
                             const std::string& output_dir,
                             int estimated_duration_s);

private:
    uint64_t available_bytes_;
};

} // namespace micecam::pipeline
