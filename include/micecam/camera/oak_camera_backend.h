#pragma once

#include "micecam/camera/camera_backend.h"
#include <atomic>
#include <mutex>
#include <memory>

namespace micecam {

class OAKCameraBackend : public ICameraBackend {
public:
    OAKCameraBackend();
    ~OAKCameraBackend() override;

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
        return "OAKCameraBackend";
    }

private:
    CameraConfig config_;
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> frame_count_{0};
    
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace micecam
