#include "micecam/camera/oak_camera_backend.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "OAK Camera Backend Test\n";
    std::cout << "======================\n";

    micecam::OAKCameraBackend camera;
    micecam::CameraConfig config{
        .width = 640,
        .height = 480,
        .fps = 30.0,
        .device_id = 0
    };

    std::cout << "Initializing OAK camera...\n";
    if (!camera.initialize(config)) {
        std::cerr << "Failed to initialize OAK camera backend. Check if device is connected.\n";
        return 1;
    }

    std::cout << "Starting capture for 5 seconds...\n";
    if (!camera.start()) {
        std::cerr << "Failed to start capture.\n";
        return 1;
    }

    auto start_time = std::chrono::steady_clock::now();
    uint64_t last_frame_count = 0;

    while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(5)) {
        auto frame = camera.get_frame();
        if (frame) {
            uint64_t current_count = camera.get_frame_count();
            if (current_count % 30 == 0 && current_count != last_frame_count) {
                std::cout << "Captured " << current_count << " frames...\n";
                last_frame_count = current_count;
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    std::cout << "Stopping capture...\n";
    camera.stop();

    std::cout << "\nTest complete.\n";
    std::cout << "Total frames captured: " << camera.get_frame_count() << "\n";

    if (camera.get_frame_count() > 0) {
        std::cout << "SUCCESS: Frames successfully acquired from OAK camera.\n";
        return 0;
    } else {
        std::cout << "FAILURE: No frames were captured.\n";
        return 1;
    }
}
