#include "micecam/pipeline/ingestion_pipeline.h"
#include "micecam/camera/oak_camera_backend.h"
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    std::cout << "MiceCam OAK-4P Quad-Camera Recorder\n";
    std::cout << "==================================\n\n";

    // 1. Setup Master OAK Backend
    auto master_backend = micecam::OAKCameraBackend::create_master();
    micecam::CameraConfig config{
        .width = 1920,
        .height = 1200,
        .fps = 30.0
    };

    std::cout << "Initializing OAK-4P Hardware (4x IMX296 GS)...\n";
    if (!master_backend->initialize(config)) {
        std::cerr << "CRITICAL: Could not initialize OAK hardware. Check connection.\n";
        return 1;
    }

    // 2. Setup 4 Ingestion Pipelines (One per camera)
    std::string output_base = "captures";
    fs::create_directories(output_base);

    std::vector<std::unique_ptr<micecam::IngestionPipeline>> pipelines;
    for (int i = 0; i < 4; ++i) {
        char cam_id = 'A' + i;
        std::string session_name = "cam_" + std::string(1, cam_id);
        
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

    std::cout << "\nRecording... Press ENTER to stop.\n";
    std::cin.get();

    // 4. Stop and finalize
    std::cout << "Stopping pipelines...\n";
    for (auto& p : pipelines) {
        p->stop();
    }
    master_backend->stop();

    std::cout << "\nRecording complete. Files saved in " << output_base << "\n";
    return 0;
}
