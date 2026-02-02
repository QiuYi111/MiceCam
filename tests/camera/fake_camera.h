#pragma once

#include "micecam/camera/camera_backend.h"
#include <atomic>
#include <memory>
#include <vector>

namespace micecam {

// Fake camera for testing - generates synthetic frames
class FakeCamera : public ICameraBackend {
public:
    explicit FakeCamera(size_t frame_size = 640 * 480 * 3);
    ~FakeCamera() override = default;

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
        return "FakeCamera";
    }

    // Test controls
    void set_max_frames(uint64_t max) { max_frames_ = max; }
    void inject_delay_ms(int ms) { delay_ms_ = ms; }

private:
    CameraConfig config_;
    size_t frame_size_;
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> frame_count_{0};
    uint64_t max_frames_ = UINT64_MAX;  // Unlimited by default
    int delay_ms_ = 0;  // For simulating slow cameras
};

}  // namespace micecam
