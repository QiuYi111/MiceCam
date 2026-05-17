#pragma once

#include <memory>
#include <string>
#include <vector>

#include "api/micecam/ICameraBackend.h"
#include "api/micecam/IDeviceEnumerator.h"
#include "domain/DeviceInfo.h"
#include "domain/PluginDescriptor.h"
#include "domain/PluginSource.h"

namespace micecam::domain {

class PluginRegistry {
public:
    void register_backend(std::unique_ptr<api::ICameraBackend> backend);
    void register_enumerator(std::unique_ptr<api::IDeviceEnumerator> enumerator);

    // External plugin support
    void register_external(PluginDescriptor descriptor);
    bool has_external() const;
    std::vector<PluginDescriptor*> get_external_plugins();
    std::vector<PluginDescriptor*> get_source_grouped_plugins();
    std::vector<PluginSource> get_sources() const;

    std::vector<DeviceInfo> discover_all();
    api::ICameraBackend* get_backend(const std::string& type);

private:
    std::vector<std::unique_ptr<api::ICameraBackend>> backends_;
    std::vector<std::unique_ptr<api::IDeviceEnumerator>> enumerators_;
    std::vector<PluginDescriptor> external_plugins_;
};

} // namespace micecam::domain
