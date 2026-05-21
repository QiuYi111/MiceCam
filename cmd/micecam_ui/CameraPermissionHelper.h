#pragma once

#include <string>
#include <vector>

struct MacCameraDevice {
    std::string id;
    std::string name;
};

/// Request camera access permission on platforms that require it.
bool micecam_request_camera_access();

/// Enumerate cameras using native platform APIs.
/// Returns vector of {id, name} pairs. Empty if no cameras or platform not supported.
std::vector<MacCameraDevice> micecam_enumerate_cameras();
