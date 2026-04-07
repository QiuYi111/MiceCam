#include "micecam/camera/oak_camera_backend.h"

#include <iostream>

int main() {
    auto backend = micecam::OAKCameraBackend::create_master();
    micecam::CameraConfig config{
        .width = 1280,
        .height = 800,
        .fps = 30.0,
        .device_id = 0
    };

    std::cout << "oak_backend_probe: initialize begin\n";
    const bool ok = backend->initialize(config);
    std::cout << "oak_backend_probe: initialize result=" << (ok ? "true" : "false") << "\n";
    return ok ? 0 : 1;
}
