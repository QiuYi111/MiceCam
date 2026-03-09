# MiceCam SDK API Reference

## Overview
The MiceCam SDK allows you to integrate high-speed camera acquisition into your C++ applications.

### Include Paths
*   `#include <micecam/pipeline/ingestion_pipeline.h>`
*   `#include <micecam/camera/camera_config.h>`

### Libraries
Link against:
*   `micecam_core.lib`
*   `micecam_camera.lib`

---

## Core Classes

### 1. `micecam::IngestionPipeline`
The main controller for the data acquisition process.

**Constructor:**
```cpp
IngestionPipeline(std::unique_ptr<ICameraBackend> camera, const SessionConfig& config);
```

**Methods:**
*   `bool start()`: Starts the camera and disk writer threads.
*   `void stop()`: Stops capture and finalizes the session file.
*   `void join()`: Blocks until the pipeline is stopped.
*   `double get_drop_rate()`: Returns current frame drop rate (0.0 - 1.0).

---

### 2. `micecam::SessionConfig`
Configuration for the recording session.

```cpp
struct SessionConfig {
    std::string output_dir;         // Directory to save .bin files
    std::string session_name;       // Filename prefix
    size_t ring_buffer_size = 100;  // Number of frames to buffer
    bool enable_checksums = true;   // Enable CRC32 integrity check
    
    // Metadata Header Info
    std::string camera_backend_name;
    int width = 0;
    int height = 0;
    double fps = 0.0;
};
```

---

### 3. `micecam::CameraConfig`
Configuration for camera initialization.

```cpp
struct CameraConfig {
    int width = 0;      // Requested width
    int height = 0;     // Requested height
    double fps = 0.0;   // Requested FPS
    int device_id = 0;  // Device index (0, 1, ...)
};
```

---

## Example Usage

```cpp
#include <micecam/pipeline/ingestion_pipeline.h>
#include <micecam/camera/usb_camera_backend.h>

int main() {
    // 1. Create Backend
    auto camera = std::make_unique<micecam::USBCameraBackend>(0);
    micecam::CameraConfig cam_config{.width=1920, .height=1080, .fps=30.0};
    
    if (!camera->initialize(cam_config)) return -1;

    // 2. Configure Session
    micecam::SessionConfig session_config;
    session_config.output_dir = "C:/Data";
    session_config.session_name = "recording_01";
    
    // 3. Run Pipeline
    micecam::IngestionPipeline pipeline(std::move(camera), session_config);
    pipeline.start();
    
    // ... wait or handle user input ...
    
    pipeline.stop();
    return 0;
}
```
