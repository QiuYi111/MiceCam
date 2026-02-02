// USB Camera Test - 实机相机测试
// 用于验证 Windows 上的 USB Camera 功能

#include "micecam/camera/usb_camera_backend.h"
#include "micecam/pipeline/ingestion_pipeline.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>

using namespace micecam;
using namespace std::chrono_literals;

#include <filesystem>

struct TestConfig {
    std::string name;
    int width;
    int height;
    double fps;
};

void run_benchmark(int camera_id, const TestConfig& test) {
    std::cout << "\n>>> BENCHMARK: " << test.name << " (" << test.width << "x" << test.height << " @ " << test.fps << " fps)\n";

    // Ensure output directory exists
    std::filesystem::create_directories("test_output");

    auto camera = std::make_unique<USBCameraBackend>();
    CameraConfig config;
    config.width = test.width;
    config.height = test.height;
    config.fps = test.fps;
    config.device_id = camera_id;

    if (!camera->initialize(config)) {
        std::cerr << "  [FAIL] Failed to initialize camera\n";
        return;
    }

    SessionConfig session;
    session.session_name = "bench_" + test.name;
    session.output_dir = "test_output";
    session.enable_checksums = false;  // 禁用校验和以提高性能
    session.ring_buffer_size = 1200;   // 20 秒缓冲 @ 60fps
    session.camera_backend_name = camera->get_backend_name();
    session.width = config.width;
    session.height = config.height;
    session.fps = config.fps;

    IngestionPipeline pipeline(std::move(camera), session);

    std::cout << "  Starting 10-minute stress test..." << std::endl;
    if (!pipeline.start()) {
        std::cerr << "  [FAIL] Failed to start pipeline\n";
        return;
    }

    // Capture for 600 seconds (10 minutes)
    const int total_seconds = 600;
    for (int i = 1; i <= total_seconds; ++i) {
        std::this_thread::sleep_for(1s);
        
        // Periodic status update every 10 seconds
        if (i % 10 == 0) {
            double current_drop_rate = pipeline.get_drop_rate() * 100.0;
            uint64_t current_captured = pipeline.get_frames_captured();
            
            // Check disk space
            auto space = std::filesystem::space("test_output");
            double free_gb = space.available / (1024.0 * 1024.0 * 1024.0);
            
            std::cout << "  [" << i << "s / 600s] Captured: " << current_captured 
                      << ", Drops: " << pipeline.get_frames_dropped() 
                      << " (" << std::fixed << std::setprecision(2) << current_drop_rate << "%), "
                      << "Disk Free: " << free_gb << " GB" << std::endl;

            if (free_gb < 5.0) {
                std::cerr << "\n  [CRITICAL] Disk space low (< 5GB). Stopping test." << std::endl;
                break;
            }
        }
    }
    std::cout << "\n  Stopping pipeline..." << std::endl;
    pipeline.stop();

    double drop_rate = pipeline.get_drop_rate() * 100.0;
    uint64_t captured = pipeline.get_frames_captured();
    double actual_fps = captured / static_cast<double>(total_seconds);
    // Estimated data volume
    double total_gb = (captured * (config.width * config.height * 3.0)) / (1024.0 * 1024.0 * 1024.0);

    std::cout << "  [RESULT] " << captured << " frames captured, " 
              << pipeline.get_frames_dropped() << " dropped (" << drop_rate << "%)\n";
    std::cout << "  [RESULT] Total data: " << std::fixed << std::setprecision(2) << total_gb << " GB\n";
    std::cout << "  [RESULT] Actual throughput: " << actual_fps << " fps\n";
    
    // Delete the large bin file to save space for next tests
    std::string bin_file = "test_output/bench_" + test.name + ".bin";
    if (std::filesystem::exists(bin_file)) {
        std::cout << "  [INFO] Deleting benchmark file " << bin_file << " to save space.\n";
        std::filesystem::remove(bin_file);
    }
    
    if (drop_rate < 0.1) {
        std::cout << "  [STATUS] PASS ✅\n";
    } else {
        std::cout << "  [STATUS] DROPS DETECTED ⚠️\n";
    }
}

int main() {
    std::cout << "=== MiceCam Multi-Parameter Benchmark ===\n";

    auto cameras = USBCameraBackend::enumerate_cameras(5);
    if (cameras.empty()) {
        std::cerr << "No cameras found!\n";
        return 1;
    }
    int camera_id = cameras[0];

    std::vector<TestConfig> tests = {
        {"2K_30fps", 2560, 1440, 30.0},
        {"1080p_60fps", 1920, 1080, 60.0},
        {"720p_120fps", 1280, 720, 120.0}
    };

    for (const auto& test : tests) {
        run_benchmark(camera_id, test);
    }

    std::cout << "\n✅ Benchmark suite completed.\n";
    return 0;
}
