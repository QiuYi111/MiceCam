#include "CameraManager.h"

namespace micecam::infrastructure {

void CameraManager::register_backend(std::unique_ptr<api::ICameraBackend> backend) {
    std::lock_guard<std::mutex> lock(mutex_);
    backends_.push_back(std::move(backend));
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
