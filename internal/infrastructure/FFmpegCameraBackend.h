#pragma once

#include <memory>
#include <string>
#include <vector>

#include "api/micecam/ICameraBackend.h"
#include "domain/CameraStream.h"

namespace micecam::infrastructure {

class FFmpegCameraBackend : public api::ICameraBackend {
public:
    FFmpegCameraBackend();
    ~FFmpegCameraBackend() override = default;

    std::vector<domain::DeviceInfo> enumerate_devices() override;
    std::unique_ptr<domain::CameraStream> open_stream(const domain::StreamConfig& config) override;
    domain::Capabilities get_capabilities() override;
    std::string backend_name() const override { return "FFmpeg"; }
};

} // namespace micecam::infrastructure
