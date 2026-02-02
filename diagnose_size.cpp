#include "micecam/camera/usb_camera_backend.h"
#include <iostream>
#include <opencv2/opencv.hpp>

using namespace micecam;

int main() {
    CameraConfig config;
    config.width = 1280;
    config.height = 720;
    config.fps = 60;
    
    USBCameraBackend camera;
    if (!camera.initialize(config)) return 1;
    if (!camera.start()) return 1;
    
    std::cout << "Capturing 10 frames to check size..." << std::endl;
    for (int i = 0; i < 10; ++i) {
        auto frame = camera.get_frame();
        if (frame) {
            std::cout << "Frame " << i << " size: " << frame->size() << " bytes" << std::endl;
            // Expected NV12 for 1280x720: 1280 * 720 * 1.5 = 1,382,400
            // Expected MJPEG for 1280x720: variable, but much smaller (e.g. 50KB - 200KB)
        }
    }
    
    camera.stop();
    return 0;
}
