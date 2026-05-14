#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include "api/micecam/ICameraBackend.h"
#include "domain/CameraStream.h"
#include "domain/DeviceInfo.h"
#include "domain/StreamConfig.h"

namespace micecam::infrastructure {

class CameraManager {
public:
    void register_backend(std::unique_ptr<api::ICameraBackend> backend);
    std::vector<domain::DeviceInfo> discover_all();
    std::unique_ptr<domain::CameraStream> open_stream(const domain::StreamConfig& config);

private:
    api::ICameraBackend* find_backend_for_device(const std::string& device_id);

    std::mutex mutex_;
    std::vector<std::unique_ptr<api::ICameraBackend>> backends_;
};

} // namespace micecam::infrastructure
