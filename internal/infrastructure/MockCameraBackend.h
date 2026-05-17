#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "api/micecam/ICameraBackend.h"
#include "domain/CameraStream.h"

namespace micecam::infrastructure {

class MockCameraStream : public domain::CameraStream {
public:
    MockCameraStream(int width, int height, int fps, int drop_every_n);
    ~MockCameraStream() override = default;

    bool read_frame(std::vector<uint8_t>& out_data, int64_t& out_pts) override;
    int width() const override { return width_; }
    int height() const override { return height_; }
    int fps() const override { return fps_; }
    std::string pixel_format() const override { return "rgb24"; }
    bool is_open() const override { return is_open_.load(); }
    void close() override { is_open_ = false; }
    void mark_disconnected() { is_open_ = false; }

private:
    int width_;
    int height_;
    int fps_;
    int drop_every_n_;
    std::atomic<bool> is_open_{true};
    std::mutex mutex_;
    int64_t frame_counter_ = 0;
};

class MockCameraBackend : public api::ICameraBackend {
public:
    MockCameraBackend();
    ~MockCameraBackend() override = default;

    std::vector<domain::DeviceInfo> enumerate_devices() override;
    std::unique_ptr<domain::CameraStream> open_stream(const domain::StreamConfig& config) override;
    domain::Capabilities get_capabilities() override;
    domain::Capabilities get_capabilities(const std::string& device_id, int stream_index) override;
    std::string backend_name() const override { return "Mock"; }

    void set_drop_every_n(int n) { drop_every_n_ = n; }
    void simulate_disconnect(const std::string& device_id);

private:
    int drop_every_n_ = 0;
    std::mutex mutex_;
    std::vector<MockCameraStream*> active_streams_;
};

} // namespace micecam::infrastructure
