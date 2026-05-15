#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "api/micecam/ICameraBackend.h"
#include "domain/CameraStream.h"
#include "domain/DeviceInfo.h"
#include "domain/PluginDeviceInfo.h"
#include "domain/PluginSource.h"
#include "domain/StreamConfig.h"

namespace micecam::infrastructure {

class PluginRegistryService;

class CameraManager {
public:
    void register_backend(std::unique_ptr<api::ICameraBackend> backend);
    void set_plugin_registry(PluginRegistryService* registry);

    std::vector<domain::DeviceInfo> discover_all();
    std::vector<domain::PluginSource> get_sources();
    std::vector<domain::PluginDeviceInfo> get_devices_for_source(const std::string& source_id);
    std::unique_ptr<domain::CameraStream> open_stream(const domain::StreamConfig& config);

private:
    api::ICameraBackend* find_backend_for_device(const std::string& device_id);

    std::mutex mutex_;
    std::vector<std::unique_ptr<api::ICameraBackend>> backends_;
    PluginRegistryService* plugin_registry_ = nullptr;
};

} // namespace micecam::infrastructure
