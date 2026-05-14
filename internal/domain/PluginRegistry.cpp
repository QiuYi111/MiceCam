#include "PluginRegistry.h"

namespace micecam::domain {

void PluginRegistry::register_backend(std::unique_ptr<api::ICameraBackend> backend) {
    backends_.push_back(std::move(backend));
}

void PluginRegistry::register_enumerator(std::unique_ptr<api::IDeviceEnumerator> enumerator) {
    enumerators_.push_back(std::move(enumerator));
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
