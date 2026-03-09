/**
 * @file imgui_preview.cpp
 * @brief ImGui-based preview window for MiceCam
 *
 * This example demonstrates how to create a real-time preview window
 * using the GpuJpegDecoder and ImGui.
 *
 * **Build Requirements:**
 * - GLFW3
 * - ImGui (Dear ImGui)
 * - OpenGL 3.3+
 * - CUDA (optional, for hardware decoding)
 *
 * **Usage:**
 *   imgui_preview.exe --device 0 --width 1920 --height 1080 --fps 30
 */

#include <iostream>
#include <chrono>
#include <thread>

// Note: In a full implementation, these would be real includes
// #include <GLFW/glfw3.h>
// #include <imgui.h>
// #include <imgui_impl_glfw.h>
// #include <imgui_impl_opengl3.h>

#include "infrastructure/ingestion_pipeline.h"
#include "infrastructure/usb_camera_backend.h"
#include "micecam/gpu/gpu_jpeg_decoder.h"

namespace {

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [options]\n"
              << "Options:\n"
              << "  --device <id>     Camera device index (default: 0)\n"
              << "  --width <w>       Frame width (default: 1920)\n"
              << "  --height <h>      Frame height (default: 1080)\n"
              << "  --fps <fps>       Target frame rate (default: 30)\n"
              << "  --no-record       Preview only, do not record to disk\n"
              << "  --help            Show this help message\n";
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    // Parse command line arguments
    int device_id = 0;
    int width = 1920;
    int height = 1080;
    double fps = 30.0;
    bool record = true;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--device" && i + 1 < argc) {
            device_id = std::stoi(argv[++i]);
        } else if (arg == "--width" && i + 1 < argc) {
            width = std::stoi(argv[++i]);
        } else if (arg == "--height" && i + 1 < argc) {
            height = std::stoi(argv[++i]);
        } else if (arg == "--fps" && i + 1 < argc) {
            fps = std::stod(argv[++i]);
        } else if (arg == "--no-record") {
            record = false;
        }
    }

    std::cout << "=== MiceCam ImGui Preview ===\n";
    std::cout << "Device: " << device_id << "\n";
    std::cout << "Resolution: " << width << "x" << height << " @ " << fps << " fps\n";
    std::cout << "Recording: " << (record ? "enabled" : "disabled") << "\n";
    std::cout << "==============================\n\n";

    // NOTE: This is a skeleton implementation.
    // A full implementation would:
    // 1. Initialize GLFW window
    // 2. Create OpenGL context
    // 3. Initialize ImGui
    // 4. Create GpuJpegDecoder
    // 5. Start pipeline with decoder attached
    // 6. Main loop: poll events, render ImGui with decoder texture
    // 7. Cleanup

    std::cout << "[INFO] This is a skeleton example.\n";
    std::cout << "[INFO] Full ImGui integration requires GLFW and ImGui libraries.\n";
    std::cout << "[INFO] See examples/imgui_preview.cpp for implementation details.\n";

    // Demonstrate observer attachment
    try {
        // Create camera backend
        micecam::CameraConfig cam_config;
        cam_config.device_id = device_id;
        cam_config.width = width;
        cam_config.height = height;
        cam_config.fps = fps;

        auto camera = std::make_unique<micecam::USBCameraBackend>();

        std::cout << "[INFO] Camera backend created (not started in skeleton mode)\n";

        // Create GPU decoder
        auto decoder = std::make_shared<micecam::GpuJpegDecoder>(width, height);
        std::cout << "[INFO] GPU decoder created\n";

        // In a full implementation, we would:
        // pipeline.attach_observer(decoder);
        // pipeline.start();
        // ... render loop ...
        // pipeline.stop();

        std::cout << "\n[SUCCESS] Preview skeleton initialized.\n";
        std::cout << "To build a full preview, add GLFW and ImGui dependencies.\n";

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }

    return 0;
}
