#include "MockCameraBackend.h"

#include <chrono>

#include "domain/Capabilities.h"

namespace micecam::infrastructure {

MockCameraStream::MockCameraStream(int width, int height, int fps, int drop_every_n)
    : width_(width), height_(height), fps_(fps), drop_every_n_(drop_every_n) {}

bool MockCameraStream::read_frame(std::vector<uint8_t>& out_data, int64_t& out_pts) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!is_open_) return false;

    frame_counter_++;

    if (drop_every_n_ > 0 && (frame_counter_ % drop_every_n_ == 0)) {
        out_pts = frame_counter_ * (1000000LL / fps_);
        return false;
    }

    size_t size = static_cast<size_t>(width_) * height_ * 3;
    out_data.resize(size);
    for (int y = 0; y < height_; y++) {
        for (int x = 0; x < width_; x++) {
            size_t idx = (static_cast<size_t>(y) * width_ + x) * 3;
            uint8_t r = static_cast<uint8_t>((x * 255) / width_);
            uint8_t g = static_cast<uint8_t>((y * 255) / height_);
            uint8_t b = static_cast<uint8_t>((frame_counter_ * 17) % 256);
            out_data[idx] = r;
            out_data[idx + 1] = g;
            out_data[idx + 2] = b;
        }
    }

    out_pts = static_cast<int64_t>(frame_counter_) * (1000000LL / fps_);
    return true;
}

MockCameraBackend::MockCameraBackend() = default;

std::vector<domain::DeviceInfo> MockCameraBackend::enumerate_devices() {
    domain::DeviceInfo info;
    info.id = "mock_cam_0";
    info.name = "Mock Camera 0";
    info.vendor = "MiceCam";
    info.serial = "MOCK-0000";
    info.type = "mock";
    domain::StreamInfo si;
    si.index = 0;
    si.max_width = 4096;
    si.max_height = 2160;
    si.supported_formats = {"rgb24", "yuv420p"};
    si.supported_framerates = {15, 30, 60};
    info.streams.push_back(si);
    return {info};
}

std::unique_ptr<domain::CameraStream> MockCameraBackend::open_stream(const domain::StreamConfig& config) {
    int w = config.width > 0 ? config.width : 640;
    int h = config.height > 0 ? config.height : 480;
    int fps = config.framerate > 0 ? config.framerate : 30;

    auto stream = std::make_unique<MockCameraStream>(w, h, fps, drop_every_n_);

    std::lock_guard<std::mutex> lock(mutex_);
    active_streams_.push_back(stream.get());

    return stream;
}

domain::Capabilities MockCameraBackend::get_capabilities() {
    domain::Capabilities caps;
    caps.supports_hardware_encode = false;
    caps.encoder_name = "libx264";
    domain::StreamInfo si;
    si.index = 0;
    si.max_width = 4096;
    si.max_height = 2160;
    si.supported_formats = {"rgb24", "yuv420p", "nv12"};
    si.supported_framerates = {15, 30, 60, 120};
    caps.streams.push_back(si);
    return caps;
}

void MockCameraBackend::simulate_disconnect(const std::string& /*device_id*/) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto* s : active_streams_) {
        if (s) s->mark_disconnected();
    }
}

} // namespace micecam::infrastructure
