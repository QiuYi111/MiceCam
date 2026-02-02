#include "micecam/pipeline/ingestion_pipeline.h"
#include "micecam/camera/oak_camera_backend.h"
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    // Parse args
    int duration_sec = 0;
    std::string output_base = "captures";
    std::string session_prefix = "cam";

    for(int i=1; i<argc; ++i) {
        std::string arg = argv[i];
        if(arg == "--duration" && i+1 < argc) {
            duration_sec = std::stoi(argv[++i]);
        } else if(arg == "--output_dir" && i+1 < argc) {
            output_base = argv[++i];
        } else if(arg == "--session_name" && i+1 < argc) {
            session_prefix = argv[++i];
        }
    }

    std::cout << "MiceCam OAK-4P Quad-Camera Recorder\n";
    std::cout << "==================================\n";
    std::cout << "Output Dir: " << output_base << "\n";
    std::cout << "Duration: " << (duration_sec > 0 ? std::to_string(duration_sec) + "s" : "Manual (Enter)") << "\n\n";

    // 1. Setup Master OAK Backend
    auto master_backend = micecam::OAKCameraBackend::create_master();
    micecam::CameraConfig config{
        .width = 1280,
        .height = 800,
        .fps = 30.0
    };

    std::cout << "Initializing OAK-4P Hardware (4x OV9782)...\n";
    if (!master_backend->initialize(config)) {
        std::cerr << "CRITICAL: Could not initialize OAK hardware. Check connection.\n";
        return 1;
    }

    // 2. Setup 4 Ingestion Pipelines
    fs::create_directories(output_base);

    std::vector<std::unique_ptr<micecam::IngestionPipeline>> pipelines;
    for (int i = 0; i < 4; ++i) {
        char cam_id = 'A' + i;
        // Construct session name: prefix + "_A"
        std::string session_name = session_prefix + "_" + std::string(1, cam_id);
        
        micecam::SessionConfig session_config{
            .output_dir = output_base,
            .session_name = session_name,
            .enable_checksums = true
        };

        auto proxy = master_backend->create_proxy(i);
        pipelines.push_back(std::make_unique<micecam::IngestionPipeline>(
            std::move(proxy), 
            session_config
        ));
    }

    // 3. Start all pipelines
    std::cout << "Starting 4 synchronized recording pipelines...\n";
    master_backend->start();
    for (auto& p : pipelines) {
        p->start();
    }

    if (duration_sec > 0) {
        std::cout << "\nRecording for " << duration_sec << " seconds...\n";
        auto start = std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now() - start < std::chrono::seconds(duration_sec)) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();
            std::cout << "  Recorded: " << elapsed << "s / " << duration_sec << "s\r" << std::flush;
        }
        std::cout << "\nDuration reached.\n";
    } else {
        std::cout << "\nRecording... Press ENTER to stop.\n";
        std::cin.get();
    }

    // 4. Stop and finalize
    std::cout << "Stopping pipelines...\n";
    for (auto& p : pipelines) {
        p->stop();
    }
    master_backend->stop();

    std::cout << "\nRecording complete. Files saved in " << output_base << "\n";
    return 0;
}
