#include "CameraPermissionHelper.h"

bool micecam_request_camera_access() {
    return true;
}

std::vector<MacCameraDevice> micecam_enumerate_cameras() {
    return {};
}
