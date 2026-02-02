#include "micecam/pipeline/ingestion_pipeline.h"
#include "micecam/camera/ffmpeg_camera_backend.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <filesystem>

using namespace micecam;
using namespace std::chrono_literals;

struct TestConfig {
    std::string name;
    int width;
    int height;
    double fps;
};

void run_benchmark(int camera_id, const TestConfig& test) {
    std::cout << "\n>>> BENCHMARK: " << test.name << " (" << test.width << "x" << test.height << " @ " << test.fps << " fps)" << std::endl;
    std::cout << " [FFMPEG MJPEG]" << std::endl;

    CameraConfig cam_config;
    cam_config.width = test.width;
    cam_config.height = test.height;
    cam_config.fps = test.fps;
    cam_config.device_id = camera_id;

    SessionConfig session_config;
    session_config.output_dir = "test_output";
    session_config.session_name = "ffmpeg_bench_" + test.name;
    session_config.ring_buffer_size = 256; 
    session_config.camera_backend_name = "FFmpegCameraBackend (Direct MJPEG)";
    session_config.width = test.width;
    session_config.height = test.height;
    session_config.fps = test.fps;

    std::filesystem::create_directories(session_config.output_dir);

    auto camera = std::make_unique<FFmpegCameraBackend>();
    if (!camera->initialize(cam_config)) {
        std::cerr << "  [FAIL] Failed to initialize FFmpeg camera" << std::endl;
        return;
    }

    IngestionPipeline pipeline(std::move(camera), session_config);
    
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
            
            auto space = std::filesystem::space("test_output");
            double free_gb = space.available / (1024.0 * 1024.0 * 1024.0);
            
            std::cout << "  [" << i << "s / " << total_seconds << "s] Captured: " << current_captured 
                      << ", Drops: " << pipeline.get_frames_dropped() 
                      << " (" << std::fixed << std::setprecision(2) << current_drop_rate << "%), "
                      << "Disk Free: " << free_gb << " GB" << std::endl;
        }
    }

    std::cout << "Stopping ingestion pipeline..." << std::endl;
    pipeline.stop();
    
    double drop_rate = pipeline.get_drop_rate() * 100.0;
    uint64_t total_frames = pipeline.get_frames_captured();
    uint64_t dropped_frames = pipeline.get_frames_dropped();

    std::cout << "  [RESULT] Total frames: " << total_frames << std::endl;
    std::cout << "  [RESULT] Drop rate: " << std::fixed << std::setprecision(2) << drop_rate << "%" << std::endl;
    
    if (drop_rate == 0.0) {
        std::cout << "  [STATUS] PASS ✅" << std::endl;
    } else {
        std::cout << "  [STATUS] WARNING: " << dropped_frames << " frames dropped" << std::endl;
    }
}

int main() {
    std::cout << "=== FFmpeg-Native Camera Test ===\n";
    
    std::vector<TestConfig> tests = {
        {"4K_debug", 3840, 2160, 30.0},
        {"960p_debug", 1280, 960, 120.0}
    };

    for (const auto& test : tests) {
        run_benchmark(0, test);
    }

    return 0;
}
