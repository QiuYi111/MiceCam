#pragma once

#include <memory>
#include <string>
#include <vector>

#include "api/micecam/ICameraBackend.h"
#include "api/micecam/IDeviceEnumerator.h"
#include "domain/DeviceInfo.h"

namespace micecam::domain {

class PluginRegistry {
public:
    void register_backend(std::unique_ptr<api::ICameraBackend> backend);
    void register_enumerator(std::unique_ptr<api::IDeviceEnumerator> enumerator);
    std::vector<DeviceInfo> discover_all();
    api::ICameraBackend* get_backend(const std::string& type);

private:
    std::vector<std::unique_ptr<api::ICameraBackend>> backends_;
    std::vector<std::unique_ptr<api::IDeviceEnumerator>> enumerators_;
};

} // namespace micecam::domain
