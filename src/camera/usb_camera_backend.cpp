#include "micecam/camera/usb_camera_backend.h"
#include <opencv2/opencv.hpp>
#include <stdexcept>

namespace micecam {

// PIMPL implementation detail
class USBCameraBackend::Impl {
public:
    cv::VideoCapture cap;
};

USBCameraBackend::~USBCameraBackend() = default;

bool USBCameraBackend::initialize(const CameraConfig& config) {
    if (!config.validate()) {
        return false;
    }

    config_ = config;
    impl_ = std::make_unique<Impl>();

    impl_->cap.open(config.device_id);
    if (!impl_->cap.isOpened()) {
        return false;
    }

    impl_->cap.set(cv::CAP_PROP_FRAME_WIDTH, config.width);
    impl_->cap.set(cv::CAP_PROP_FRAME_HEIGHT, config.height);
    impl_->cap.set(cv::CAP_PROP_FPS, config.fps);

    return true;
}

bool USBCameraBackend::start() {
    if (running_.load()) {
        return false;  // Already started
    }

    if (!impl_ || !impl_->cap.isOpened()) {
        return false;
    }

    running_.store(true);
    frame_count_.store(0);
    return true;
}

void USBCameraBackend::stop() {
    running_.store(false);
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
        return nullptr;
    }

    // Convert to contiguous buffer
    auto data = std::make_unique<std::vector<uint8_t>>(
        mat.data, mat.data + mat.total() * mat.elemSize());

    auto frame = std::make_unique<Frame>(
        frame_count_.fetch_add(1) + 1,
        std::move(data)
    );

    return frame;
}

}  // namespace micecam
