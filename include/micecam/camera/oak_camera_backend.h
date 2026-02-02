#pragma once

#include "micecam/camera/camera_backend.h"
#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace micecam {

/**
 * @brief Multi-Camera Backend for OAK-4P-New
 * 
 * Supports CAM_A, CAM_B, CAM_C, CAM_D with hardware synchronization and device-side MJPEG encoding.
 */
class OAKCameraBackend : public ICameraBackend, public std::enable_shared_from_this<OAKCameraBackend> {
public:
    OAKCameraBackend();
    ~OAKCameraBackend() override;

    // Standard ICameraBackend (defaults to CAM_A)
    bool initialize(const CameraConfig& config) override;
    bool start() override;
    void stop() override;
    [[nodiscard]] std::unique_ptr<Frame> get_frame() override;
    [[nodiscard]] uint64_t get_frame_count() const override;
    [[nodiscard]] bool is_running() const override;
    [[nodiscard]] std::string get_backend_name() const override { return "OAKCameraBackend"; }

    /**
     * @brief Create a proxy backend for a specific camera socket
     * 
     * Allows multiple IngestionPipelines to share a single physical device.
     */
    static std::shared_ptr<OAKCameraBackend> create_master();
    std::unique_ptr<ICameraBackend> create_proxy(int socket_index);

private:
    friend class VirtualOAKBackend;
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace micecam
