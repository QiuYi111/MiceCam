#include "PluginRegistry.h"

namespace micecam::domain {

void PluginRegistry::register_backend(std::unique_ptr<api::ICameraBackend> backend) {
    backends_.push_back(std::move(backend));
}

void PluginRegistry::register_enumerator(std::unique_ptr<api::IDeviceEnumerator> enumerator) {
    enumerators_.push_back(std::move(enumerator));
}

void PluginRegistry::register_external(PluginDescriptor descriptor) {
    external_plugins_.push_back(std::move(descriptor));
}

bool PluginRegistry::has_external() const {
    return !external_plugins_.empty();
}

std::vector<PluginDescriptor*> PluginRegistry::get_external_plugins() {
    std::vector<PluginDescriptor*> result;
    result.reserve(external_plugins_.size());
    for (auto& p : external_plugins_) {
        result.push_back(&p);
    }
    return result;
}

std::vector<PluginDescriptor*> PluginRegistry::get_source_grouped_plugins() {
    return get_external_plugins();
}

std::vector<DeviceInfo> PluginRegistry::discover_all() {
    std::vector<DeviceInfo> all;
    for (auto& enumerator : enumerators_) {
        auto devices = enumerator->enumerate();
        all.insert(all.end(), devices.begin(), devices.end());
    }
    return all;
}

api::ICameraBackend* PluginRegistry::get_backend(const std::string& type) {
    for (auto& backend : backends_) {
        if (backend->backend_name() == type) {
            return backend.get();
        }
    }
    return nullptr;
}

} // namespace micecam::domain
