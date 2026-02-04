#include "micecam/camera/ffmpeg_camera_backend.h"
#include "micecam/pipeline/ingestion_pipeline.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <filesystem>

using namespace std::chrono_literals;

void run_test(const std::string& name, int width, int height, double fps) {
    std::cout << "\n=== Testing Resolution: " << width << "x" << height << " @ " << fps << " FPS ===" << std::endl;

    micecam::SessionConfig session_config;
    session_config.output_dir = "test_output";
    session_config.session_name = "ffmpeg_bench_" + name;
    session_config.ring_buffer_size = 256; 
    session_config.camera_backend_name = "FFmpegCameraBackend (Direct MJPEG)";
    session_config.width = width;
    session_config.height = height;
    session_config.fps = fps;

    std::filesystem::create_directories(session_config.output_dir);

    micecam::CameraConfig cam_config;
    cam_config.width = width;
    cam_config.height = height;
    cam_config.fps = fps;
    cam_config.device_id = 0;

    auto camera = std::make_unique<micecam::FFmpegCameraBackend>();
    if (!camera->initialize(cam_config)) {
        std::cerr << "  [FAIL] Failed to initialize FFmpeg camera" << std::endl;
        return;
    }

    micecam::IngestionPipeline pipeline(std::move(camera), session_config);
    
    std::cout << "  Starting verification..." << std::endl;
    if (!pipeline.start()) {
        std::cerr << "  [FAIL] Failed to start pipeline" << std::endl;
        return;
    }

    const int total_seconds = 10;
    for (int i = 1; i <= total_seconds; ++i) {
        std::this_thread::sleep_for(1s);
        
        if (i % 2 == 0) {
            double current_drop_rate = pipeline.get_drop_rate() * 100.0;
            uint64_t current_captured = pipeline.get_frames_captured();
            std::cout << "  [" << i << "s] Captured: " << current_captured 
                      << " | Drop Rate: " << current_drop_rate << "%" << std::endl;
        }
    }

    pipeline.stop();
    std::cout << "  [PASS] Test completed" << std::endl;
}

int main() {
    std::cout << "MiceCam FFmpeg Backend Stability Test" << std::endl;
    std::cout << "======================================" << std::endl;

    try {
        // Standard test
        run_test("1080p_30", 1920, 1080, 30.0);
        
        // High Speed test
        run_test("720p_60", 1280, 720, 60.0);
        
    } catch (const std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
