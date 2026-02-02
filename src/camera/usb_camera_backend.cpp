#include "micecam/camera/usb_camera_backend.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <stdexcept>

namespace micecam {

// PIMPL implementation detail
class USBCameraBackend::Impl {
public:
    cv::VideoCapture cap;
    int actual_width = 0;
    int actual_height = 0;
    double actual_fps = 0.0;
};

USBCameraBackend::USBCameraBackend() = default;
USBCameraBackend::~USBCameraBackend() = default;

bool USBCameraBackend::initialize(const CameraConfig& config) {
    if (!config.validate()) {
        std::cerr << "Invalid camera configuration\n";
        return false;
    }

    config_ = config;
    impl_ = std::make_unique<Impl>();

    // Try different backends in order of preference for high FPS on Windows
    // MSMF (Media Foundation) typically performs better for high frame rates
    // DSHOW (DirectShow) is more compatible but may have lower FPS
    bool opened = false;
    
#ifdef _WIN32
    // For raw MJPEG capture, DirectShow often performs better than MSMF 
    // which tends to decode to NV12 even when told not to.
    std::cout << "Trying DirectShow backend (preferred for raw)..." << std::endl;
    impl_->cap.open(config.device_id, cv::CAP_DSHOW);
    if (impl_->cap.isOpened()) {
        std::cout << "Using DirectShow backend" << std::endl;
        opened = true;
    } else {
        // Fallback to MSMF
        std::cout << "DirectShow failed, trying MSMF backend..." << std::endl;
        impl_->cap.open(config.device_id, cv::CAP_MSMF);
        if (impl_->cap.isOpened()) {
            std::cout << "Using MSMF backend" << std::endl;
            opened = true;
        }
    }
#else
    impl_->cap.open(config.device_id);
    opened = impl_->cap.isOpened();
#endif

    if (!opened) {
        // Last resort: default backend
        impl_->cap.open(config.device_id);
        if (!impl_->cap.isOpened()) {
            std::cerr << "Failed to open camera device " << config.device_id << "\n";
            return false;
        }
        std::cout << "Using default backend\n";
    }

    // Set MJPEG format first - this typically allows higher frame rates
    // FourCC code for MJPEG
    impl_->cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
    
    // Set resolution BEFORE fps for best results
    impl_->cap.set(cv::CAP_PROP_FRAME_WIDTH, config.width);
    impl_->cap.set(cv::CAP_PROP_FRAME_HEIGHT, config.height);
    
    // Set FPS
    impl_->cap.set(cv::CAP_PROP_FPS, config.fps);
    
    // Disable BGR conversion to get raw MJPEG bytes
    std::cout << "Disabling BGR conversion for raw capture..." << std::endl;
    impl_->cap.set(cv::CAP_PROP_CONVERT_RGB, false);
    
    // Reduce buffer size for lower latency (1 frame buffer)
    impl_->cap.set(cv::CAP_PROP_BUFFERSIZE, 1);
    
    // Disable auto exposure if possible (can cause frame rate drops)
    impl_->cap.set(cv::CAP_PROP_AUTO_EXPOSURE, 0.25);  // 0.25 = manual mode on some cameras
    
    // Read back actual values
    impl_->actual_width = static_cast<int>(impl_->cap.get(cv::CAP_PROP_FRAME_WIDTH));
    impl_->actual_height = static_cast<int>(impl_->cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    impl_->actual_fps = impl_->cap.get(cv::CAP_PROP_FPS);
    bool convert_rgb = impl_->cap.get(cv::CAP_PROP_CONVERT_RGB);
    
    // Get actual FourCC
    int fourcc = static_cast<int>(impl_->cap.get(cv::CAP_PROP_FOURCC));
    char fourcc_str[5] = {
        static_cast<char>(fourcc & 0xFF),
        static_cast<char>((fourcc >> 8) & 0xFF),
        static_cast<char>((fourcc >> 16) & 0xFF),
        static_cast<char>((fourcc >> 24) & 0xFF),
        '\0'
    };

    std::cout << "Camera initialized:\n"
              << "  Requested:   " << config.width << "x" << config.height << " @ " << config.fps << " fps\n"
              << "  Actual:      " << impl_->actual_width << "x" << impl_->actual_height 
              << " @ " << impl_->actual_fps << " fps\n"
              << "  Format:      " << fourcc_str << "\n"
              << "  Convert RGB: " << (convert_rgb ? "Yes" : "No (Raw Mode)") << "\n";

    return true;
}

bool USBCameraBackend::start() {
    if (running_.load()) {
        return false;  // Already started
    }

    if (!impl_ || !impl_->cap.isOpened()) {
        std::cerr << "Cannot start: camera not initialized\n";
        return false;
    }

    running_.store(true);
    frame_count_.store(0);
    std::cout << "Camera capture started\n";
    return true;
}

void USBCameraBackend::stop() {
    running_.store(false);
    std::cout << "Camera capture stopped. Total frames: " << frame_count_.load() << "\n";
}

std::unique_ptr<Frame> USBCameraBackend::get_frame() {
    if (!running_.load()) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(capture_mutex_);

    if (!impl_ || !impl_->cap.isOpened()) {
        return nullptr;
    }

    cv::Mat mat;
    if (!impl_->cap.read(mat)) {
        std::cerr << "Failed to read frame from camera\n";
        return nullptr;
    }

    if (mat.empty()) {
        std::cerr << "Received empty frame\n";
        return nullptr;
    }

    // Ensure data is contiguous
    if (!mat.isContinuous()) {
        mat = mat.clone();
    }

    // Convert to contiguous buffer
    // When CONVERT_RGB is false, the mat often holds raw MJPEG data in a 1D array
    size_t data_size = mat.total() * mat.elemSize();
    auto data = std::make_unique<std::vector<uint8_t>>(
        mat.data, mat.data + data_size);

    auto frame = std::make_unique<Frame>(
        frame_count_.fetch_add(1) + 1,
        std::move(data)
    );

    return frame;
}

// Static function to enumerate available cameras
std::vector<int> USBCameraBackend::enumerate_cameras(int max_devices) {
    std::vector<int> available_ids;
    
#ifdef _WIN32
    // Use MSMF for enumeration on Windows (more reliable than DirectShow)
    int apiPreference = cv::CAP_MSMF;
#else
    int apiPreference = cv::CAP_ANY;
#endif

    for (int i = 0; i < max_devices; ++i) {
        cv::VideoCapture test_cap;
        test_cap.open(i, apiPreference);
        if (test_cap.isOpened()) {
            available_ids.push_back(i);
            test_cap.release();
        }
    }
    
    return available_ids;
}

}  // namespace micecam

