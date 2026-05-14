#pragma once

#include <string>

namespace micecam::domain {

struct PluginDescriptor {
    std::string name;
    std::string version;
    std::string type;
    bool enabled = true;
};

} // namespace micecam::domain
