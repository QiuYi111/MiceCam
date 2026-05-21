#pragma once

#include <string>
#include <vector>

namespace micecam::infrastructure {

struct NativeCameraInfo {
    std::string id;
    std::string name;
};

/// Enumerate video cameras using native platform APIs.
/// macOS: AVFoundation AVCaptureDeviceDiscoverySession.
/// Other platforms: returns empty (use FFmpeg avdevice instead).
std::vector<NativeCameraInfo> enumerate_native_cameras();

} // namespace micecam::infrastructure
