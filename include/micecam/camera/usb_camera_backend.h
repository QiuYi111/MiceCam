#ifdef WITH_USB_CAMERA
#pragma once

#include "micecam/camera/camera_backend.h"
#include <atomic>
#include <mutex>
#include <vector>

namespace micecam {

// USB Webcam backend using OpenCV
class USBCameraBackend : public ICameraBackend {
public:
    USBCameraBackend();
    ~USBCameraBackend() override;

    bool initialize(const CameraConfig& config) override;
    bool start() override;
    void stop() override;

    [[nodiscard]] std::unique_ptr<Frame> get_frame() override;

    [[nodiscard]] uint64_t get_frame_count() const override {
        return frame_count_.load();
    }

    [[nodiscard]] bool is_running() const override {
        return running_.load();
    }

    [[nodiscard]] std::string get_backend_name() const override {
        return "USBCameraBackend";
    }

    // Static utility to enumerate available cameras
    // Returns a vector of device IDs that can be opened
    [[nodiscard]] static std::vector<int> enumerate_cameras(int max_devices = 10);

private:
    CameraConfig config_;
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> frame_count_{0};
    std::mutex capture_mutex_;

    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace micecam
#endif

