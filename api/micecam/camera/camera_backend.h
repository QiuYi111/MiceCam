#pragma once

#include "domain/frame.h"
#include <cstdint>
#include <string>

namespace micecam {

struct CameraConfig {
    int width = 640;
    int height = 480;
    double fps = 30.0;
    int device_id = 0;

    // Validate configuration
    [[nodiscard]] bool validate() const noexcept {
        return width > 0 && height > 0 && fps > 0.0;
    }
};

class ICameraBackend {
public:
    virtual ~ICameraBackend() = default;

    // Initialize camera with configuration
    virtual bool initialize(const CameraConfig& config) = 0;

    // Start capture (non-blocking)
    virtual bool start() = 0;

    // Stop capture
    virtual void stop() = 0;

    // Get next frame (blocking until available or timeout)
    // Returns nullptr if camera is stopped or error occurs
    [[nodiscard]] virtual std::unique_ptr<Frame> get_frame() = 0;

    // Get camera properties
    [[nodiscard]] virtual uint64_t get_frame_count() const = 0;
    [[nodiscard]] virtual bool is_running() const = 0;
    [[nodiscard]] virtual std::string get_backend_name() const = 0;
    [[nodiscard]] virtual PixelFormat get_current_format() const = 0;

    // Capability discovery (optional default implementation)
    [[nodiscard]] virtual std::vector<std::string> get_supported_resolutions() const {
        return {"1920x1080", "1280x720", "640x480"};
    }
    [[nodiscard]] virtual std::vector<int> get_supported_fps() const {
        return {30, 60};
    }
};

}  // namespace micecam
