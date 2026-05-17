#pragma once

#include <string>
#include <vector>

#include "domain/PluginDescriptor.h"

namespace micecam::domain {

enum class PluginDiagnosticsState { OK, MISSING, DISABLED, ERROR };

struct PluginSource {
    std::string source_id;
    std::string source_name;
    PluginSourceType source_type = PluginSourceType::BUNDLED;
    std::string plugin_path;
    std::string plugin_version;
    uint32_t plugin_api_version = 0;
    bool enabled = true;
    PluginDiagnosticsState diagnostics_state = PluginDiagnosticsState::OK;
    std::vector<std::string> device_ids;
};

} // namespace micecam::domain
