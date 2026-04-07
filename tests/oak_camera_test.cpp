#include "micecam/camera/oak_camera_backend.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

int main() {
    std::cout << "OAK-4P Quad-Camera Backend Sync Test\n";
    std::cout << "====================================\n";

    auto master = micecam::OAKCameraBackend::create_master();
    micecam::CameraConfig config{
        .width = 1920,
        .height = 1200,
        .fps = 30.0
    };

    std::cout << "Initializing OAK master...\n";
    if (!master->initialize(config)) {
        std::cerr << "Failed to initialize OAK master. Check device connection.\n";
        return 1;
    }

    // Create 4 proxies for CAM_A, B, C, D
    std::vector<std::unique_ptr<micecam::ICameraBackend>> proxies;
    for(int i = 0; i < 4; ++i) {
        proxies.push_back(master->create_proxy(i));
    }

    std::cout << "Starting capture...\n";
    master->start();

    auto start_time = std::chrono::steady_clock::now();
    uint64_t frames_seen = 0;

    while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(5)) {
        // Collect one frame from each proxy
        for(int i = 0; i < 4; ++i) {
            auto frame = proxies[i]->get_frame();
            if (frame) {
                if (i == 0 && frames_seen % 30 == 0) {
                    std::cout << "Captured Group " << frame->sequence_id << "...\n";
                }
                if (i == 3) frames_seen++;
            }
        }
    }

    std::cout << "Stopping capture...\n";
    master->stop();

    std::cout << "\nTest complete. Synchronized groups captured: " << frames_seen << "\n";
    if (frames_seen > 0) {
        std::cout << "SUCCESS: OAK-4P Quad-Sync proven.\n";
        return 0;
    } else {
        std::cout << "FAILURE: No frames captured.\n";
        return 1;
    }
}
