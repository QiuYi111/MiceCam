#pragma once

#include "micecam/camera/camera_backend.h"
#include <atomic>
#include <mutex>
#include <memory>
#include <string>

// Forward declarations for FFmpeg structures
struct AVFormatContext;
struct AVPacket;
struct AVInputFormat;

namespace micecam {

class FFmpegCameraBackend : public ICameraBackend {
public:
    FFmpegCameraBackend();
    ~FFmpegCameraBackend() override;

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
        return "FFmpegCameraBackend (Direct MJPEG)";
    }

private:
    CameraConfig config_;
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> frame_count_{0};

    // FFmpeg internal state
    AVFormatContext* fmt_ctx_{nullptr};
    AVPacket* pkt_{nullptr};
    int video_stream_index_{-1};

    std::mutex capture_mutex_;

    bool open_device();
    void close_device();
};

} // namespace micecam
