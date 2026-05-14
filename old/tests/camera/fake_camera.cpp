#include "camera/fake_camera.h"
#include <algorithm>
#include <random>
#include <thread>

namespace micecam {

FakeCamera::FakeCamera(size_t frame_size) : frame_size_(frame_size) {}

bool FakeCamera::initialize(const CameraConfig& config) {
    if (!config.validate()) {
        return false;
    }
    config_ = config;
    return true;
}

bool FakeCamera::start() {
    if (running_.load()) {
        return false;
    }
    running_.store(true);
    frame_count_.store(0);
    return true;
}

void FakeCamera::stop() {
    running_.store(false);
}

std::unique_ptr<Frame> FakeCamera::get_frame() {
    if (!running_.load()) {
        return nullptr;
    }

    const uint64_t current = frame_count_.load();
    if (current >= max_frames_) {
        running_.store(false);
        return nullptr;
    }

    // Simulate camera latency if configured
    if (delay_ms_ > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms_));
    }

    // Generate test pattern: gradient + sequence number
    auto data = std::make_unique<std::vector<uint8_t>>(frame_size_);

    // Fill with a test pattern (gradient + sequence info)
    for (size_t i = 0; i < frame_size_; ++i) {
        data->at(i) = static_cast<uint8_t>((i + current) % 256);
    }

    auto frame = std::make_unique<Frame>(
        frame_count_.fetch_add(1) + 1,
        std::move(data)
    );

    return frame;
}

}  // namespace micecam
