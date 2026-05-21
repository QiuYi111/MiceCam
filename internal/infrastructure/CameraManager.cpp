#include "CameraManager.h"

#include "infrastructure/PluginRegistryService.h"

namespace micecam::infrastructure {

void CameraManager::register_backend(std::unique_ptr<api::ICameraBackend> backend) {
    std::lock_guard<std::mutex> lock(mutex_);
    backends_.push_back(std::move(backend));
}

void CameraManager::set_plugin_registry(PluginRegistryService* registry) {
    plugin_registry_ = registry;
}

std::vector<domain::DeviceInfo> CameraManager::discover_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<domain::DeviceInfo> all;
    for (auto& backend : backends_) {
        auto devices = backend->enumerate_devices();
        all.insert(all.end(), devices.begin(), devices.end());
    }
    return all;
}

std::unique_ptr<domain::CameraStream> CameraManager::open_stream(const domain::StreamConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto* backend = find_backend_for_device(config.device_id);
    if (!backend) return nullptr;
    return backend->open_stream(config);
}

std::vector<domain::PluginSource> CameraManager::get_sources() {
    if (plugin_registry_) {
        return plugin_registry_->getSources();
    }
    return {};
}

std::vector<domain::PluginDeviceInfo> CameraManager::get_devices_for_source(const std::string& source_id) {
    std::vector<domain::PluginDeviceInfo> result;
    // Return devices from registered backends that match this source.
    // For bundled plugins without running plugin processes, the main
    // backend (FFmpegCameraBackend) provides all devices.
    for (auto& backend : backends_) {
        auto devices = backend->enumerate_devices();
        for (const auto& d : devices) {
            domain::PluginDeviceInfo pdi;
            pdi.device_id = d.id;
            pdi.display_name = d.name;
            pdi.plugin_id = source_id;
            pdi.status = "available";
            for (const auto& s : d.streams) {
                pdi.max_width = std::max(pdi.max_width, s.max_width);
                pdi.max_height = std::max(pdi.max_height, s.max_height);
                pdi.max_framerate = std::max(pdi.max_framerate, 0.0);
            }
            result.push_back(pdi);
        }
    }
    return result;
}

api::ICameraBackend* CameraManager::find_backend_for_device(const std::string& device_id) {
    for (auto& backend : backends_) {
        auto devices = backend->enumerate_devices();
        for (auto& d : devices) {
            if (d.id == device_id) {
                return backend.get();
            }
        }
    }
    return nullptr;
}

} // namespace micecam::infrastructure
