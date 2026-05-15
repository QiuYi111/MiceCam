#pragma once

#include <memory>
#include <string>
#include <vector>

#include "internal/domain/DeviceInfo.h"
#include "internal/domain/StreamConfig.h"
#include "internal/domain/Capabilities.h"

namespace micecam::domain {
class CameraStream;
} // namespace micecam::domain

namespace micecam::api {

class ICameraBackend {
public:
    virtual ~ICameraBackend() = default;

    virtual std::vector<domain::DeviceInfo> enumerate_devices() = 0;
    virtual std::unique_ptr<domain::CameraStream> open_stream(const domain::StreamConfig& config) = 0;
    virtual domain::Capabilities get_capabilities() = 0;
    virtual domain::Capabilities get_capabilities(const std::string& device_id, int stream_index) {
        (void)device_id;
        (void)stream_index;
        return get_capabilities();
    }
    virtual std::string backend_name() const = 0;
};

} // namespace micecam::api
