#pragma once

#include <cstdint>
#include <string>

namespace micecam::domain {

enum class PluginSourceType { BUNDLED, LINKED };

struct PluginDescriptor {
    std::string id;
    std::string name;
    std::string version;
    uint32_t api_version = 0;
    std::string path;
    std::string type;
    PluginSourceType source_type = PluginSourceType::BUNDLED;
    bool enabled = true;
};

} // namespace micecam::domain
