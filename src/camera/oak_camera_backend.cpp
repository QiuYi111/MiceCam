#include "micecam/camera/oak_camera_backend.h"
#include <depthai/depthai.hpp>
#include <iostream>
#include <vector>

namespace micecam {

class OAKCameraBackend::Impl {
public:
    std::shared_ptr<dai::Device> device;
    dai::Pipeline pipeline;
    std::shared_ptr<dai::node::Camera> camNode;
    std::shared_ptr<dai::DataOutputQueue> videoQueue;
};

OAKCameraBackend::OAKCameraBackend() = default;
OAKCameraBackend::~OAKCameraBackend() {
    stop();
}

bool OAKCameraBackend::initialize(const CameraConfig& config) {
    if (!config.validate()) {
        std::cerr << "Invalid camera configuration\n";
        return false;
    }

    config_ = config;
    impl_ = std::make_unique<Impl>();

    try {
        // Create pipeline
        impl_->camNode = impl_->pipeline.create<dai::node::Camera>();
        
        // Build camera node with requested parameters
        // Note: For OAK cameras, we often need to choose a supported resolution
        // Here we request an output with the specified size
        impl_->camNode->build();
        
        // Request video output
        impl_->camNode->requestOutput(
            {static_cast<uint32_t>(config.width), static_cast<uint32_t>(config.height)},
            std::nullopt,
            dai::ImgResizeMode::CROP,
            static_cast<float>(config.fps)
        );

        // Connect to device
        impl_->device = std::make_shared<dai::Device>(impl_->pipeline);
        
        // Start pipeline
        impl_->device->startPipeline();

        // Get output queue
        // The output name for Camera node's requested outputs is "video" or "isp" depending on config,
        // but when using requestOutput it creates dynamic outputs.
        // For simplicity in this initial version, let's use the first available output
        auto outputs = impl_->camNode->getOutputs();
        if (outputs.empty()) {
            std::cerr << "OAK Camera: No outputs available\n";
            return false;
        }

        // The dynamic output name is usually the size string like "640x480"
        std::string outputName = std::to_string(config.width) + "x" + std::to_string(config.height);
        impl_->videoQueue = impl_->device->getOutputQueue(outputName, 4, false);

        std::cout << "OAK Camera initialized: " << config.width << "x" << config.height << " @ " << config.fps << " fps\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize OAK camera: " << e.what() << "\n";
        return false;
    }
}

bool OAKCameraBackend::start() {
    if (running_.load()) return false;
    if (!impl_ || !impl_->device) return false;

    running_.store(true);
    frame_count_.store(0);
    std::cout << "OAK Camera capture started\n";
    return true;
}

void OAKCameraBackend::stop() {
    running_.store(false);
    if (impl_ && impl_->device) {
        impl_->device->close();
    }
    std::cout << "OAK Camera capture stopped. Total frames: " << frame_count_.load() << "\n";
}

std::unique_ptr<Frame> OAKCameraBackend::get_frame() {
    if (!running_.load() || !impl_ || !impl_->videoQueue) {
        return nullptr;
    }

    try {
        auto imgFrame = impl_->videoQueue->get<dai::ImgFrame>();
        if (!imgFrame) return nullptr;

        // Convert dai::ImgFrame to micecam::Frame
        // OAK frames are often NV12 or BGR. getCvFrame() handles conversion to BGR efficiently.
        cv::Mat mat = imgFrame->getCvFrame();
        
        if (mat.empty()) return nullptr;
        if (!mat.isContinuous()) mat = mat.clone();

        size_t data_size = mat.total() * mat.elemSize();
        auto data = std::make_unique<std::vector<uint8_t>>(
            mat.data, mat.data + data_size
        );

        return std::make_unique<Frame>(
            frame_count_.fetch_add(1) + 1,
            std::move(data)
        );
    } catch (const std::exception& e) {
        std::cerr << "Error getting frame from OAK camera: " << e.what() << "\n";
        return nullptr;
    }
}

}  // namespace micecam
