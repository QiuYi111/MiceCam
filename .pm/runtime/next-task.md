# Task: Stage 2 — CMake v2 + Domain Model + Plugin Interfaces

## Objective

Establish the project foundation: rewrite CMakeLists.txt for v2, define all domain types, plugin interfaces, and pipeline interfaces. Build must succeed on macOS.

## Context

This is Phase 1 of the MiceCam v2 rewrite. All subsequent phases depend on the domain model and interfaces defined here. Read `specs/001-micecam-v2-rewrite/spec.md` and `specs/001-micecam-v2-rewrite/plan.md` for full design.

## Bounded Scope

### 1. CMakeLists.txt v2 (rewrite root CMakeLists.txt)

Create `CMakeLists.txt.v2` alongside existing `CMakeLists.txt` (do NOT overwrite the v1 file yet). Requirements:
- C++20 standard (`set(CMAKE_CXX_STANDARD 20)`)
- vcpkg toolchain support (existing `vcpkg.json` preserved, add missing deps if needed)
- FFmpeg finder: `find_package(FFmpeg REQUIRED COMPONENTS avcodec avformat avdevice avutil swscale)` — use FindFFmpeg.cmake or PkgConfig
- Qt6 finder: `find_package(Qt6 REQUIRED COMPONENTS Core Qml Quick QuickControls2)`
- spdlog: header-only, find via vcpkg or FetchContent
- nlohmann_json: header-only, find via vcpkg or FetchContent
- depthai-core: optional, `find_package(depthai QUIET)`, define `WITH_DEPTHAI` if found
- GTest: `find_package(GTest REQUIRED)`, enable testing `enable_testing()`
- Build the `micecam` target: links all libs, executable output `cmd/micecam/micecam`
- Preserve existing `BUILD_SPIKE` conditional from spike phase

### 2. Domain Types (`internal/domain/`)

Create header-only domain types (all new files, do NOT modify v1 files):

**`DeviceInfo.h`:**
```cpp
namespace micecam::domain {
struct DeviceInfo {
    std::string id;           // unique identifier (serial or bus-location)
    std::string name;         // human-readable name
    std::string vendor;
    std::string serial;
    std::string type;         // "OAK", "UVC", etc.
    std::vector<StreamInfo> streams;
};
struct StreamInfo {
    int index;
    int max_width;
    int max_height;
    std::vector<std::string> supported_formats; // "MJPEG", "UYVY422", "H264"
    std::vector<int> supported_framerates;     // 15, 30, 60
};
}
```

**`StreamConfig.h`:**
```cpp
struct StreamConfig {
    std::string device_id;
    int stream_index;
    int width;
    int height;
    int framerate;
    std::string pixel_format;
};
```

**`FrameTimestamp.h`:**
```cpp
struct FrameTimestamp {
    uint64_t session_offset_us;   // steady_clock offset from session start
    uint64_t hardware_pts;        // raw hardware PTS if available, 0 otherwise
    bool has_hardware_pts;
};
```

**`SessionMetadata.h`:**
```cpp
struct SessionMetadata {
    std::string session_id;
    uint64_t wall_clock_anchor_ns;  // system_clock::now() at session start
    std::vector<StreamConfig> stream_configs;
    std::string encoder_name;
    int bitrate_kbps;
    int keyframe_interval;
    std::string output_dir;
    uint64_t start_time_ns;
    uint64_t end_time_ns;  // 0 during recording
    nlohmann::json to_json() const;
    static SessionMetadata from_json(const nlohmann::json& j);
};
```

**`StreamStats.h`:**
```cpp
struct StreamStats {
    std::string stream_id;
    uint64_t frames_expected;
    uint64_t frames_actual;
    double drop_rate;                   // (expected - actual) / expected
    double avg_encode_latency_us;
    double max_encode_latency_us;
    double min_encode_latency_us;
    double avg_frame_interval_us;
    double max_frame_interval_deviation_us;
    uint64_t bytes_written;
    std::string encoder_used;
    bool encoder_fallback;
    nlohmann::json to_json() const;
};
```

**`AlertRecord.h`:**
```cpp
enum class AlertSeverity { YELLOW, RED };
enum class AlertType {
    CAMERA_DISCONNECT, CAMERA_RECONNECT, HIGH_DROP_RATE,
    ENCODE_STALL, ENCODER_FALLBACK, DISK_FULL, PIPELINE_STALL
};
struct AlertRecord {
    uint64_t timestamp_ns;
    AlertSeverity severity;
    AlertType type;
    std::string stream_id;
    std::string message;
};
```

**`EncoderConfig.h`:**
```cpp
struct EncoderConfig {
    int bitrate_kbps = 5000;
    int keyframe_interval = 60;
    int crf = 18;               // constant rate factor (for software encoder)
    int max_b_frames = 0;       // 0 for hardware encoders
    bool prefer_hardware = true;
};
```

**`Capabilities.h`:**
```cpp
struct Capabilities {
    bool supports_hardware_encode;
    std::string encoder_name;           // e.g., "h264_videotoolbox"
    std::string fallback_encoder_name;  // e.g., "libx264"
    std::vector<StreamInfo> streams;
};
```

**`PluginDescriptor.h`:**
```cpp
struct PluginDescriptor {
    std::string name;
    std::string version;
    std::string type;  // "OAK", "FFMPEG", etc.
    bool enabled;
};
```

### 3. Plugin Interfaces (`api/micecam/`)

Create new header files with pure virtual interfaces:

**`ICameraBackend.h`:**
```cpp
namespace micecam::api {
class ICameraBackend {
public:
    virtual ~ICameraBackend() = default;
    virtual std::vector<domain::DeviceInfo> enumerate_devices() = 0;
    virtual std::unique_ptr<CameraStream> open_stream(const domain::StreamConfig& config) = 0;
    virtual domain::Capabilities get_capabilities() = 0;
    virtual std::string backend_name() const = 0;
};
}
```

**`IDeviceEnumerator.h`:**
```cpp
namespace micecam::api {
class IDeviceEnumerator {
public:
    virtual ~IDeviceEnumerator() = default;
    virtual std::vector<domain::DeviceInfo> enumerate() = 0;
};
}
```

**`WatchdogObserver.h`:**
```cpp
namespace micecam::api {
class WatchdogObserver {
public:
    virtual ~WatchdogObserver() = default;
    virtual void on_alert(const domain::AlertRecord& alert) = 0;
};
}
```

### 4. Pipeline Interfaces (`internal/pipeline/`)

**`IEncoder.h`:**
```cpp
namespace micecam::pipeline {
class IEncoder {
public:
    virtual ~IEncoder() = default;
    virtual bool initialize(const domain::EncoderConfig& config) = 0;
    virtual std::vector<uint8_t> encode(const uint8_t* data, size_t size, int64_t pts) = 0;
    virtual bool flush(std::vector<uint8_t>& out) = 0;
    virtual std::string encoder_name() const = 0;
};
}
```

**`IStreamWriter.h`:**
```cpp
class IStreamWriter {
public:
    virtual ~IStreamWriter() = default;
    virtual bool open(const std::string& path, int width, int height, int fps) = 0;
    virtual bool write_packet(const uint8_t* data, size_t size, int64_t pts, int64_t dts, bool keyframe) = 0;
    virtual bool close() = 0;
};
```

**`IStatsCollector.h`:**
```cpp
class IStatsCollector {
public:
    virtual ~IStatsCollector() = default;
    virtual void record_frame(uint64_t expected_seq, uint64_t actual_seq, double encode_latency_us, uint64_t frame_interval_us) = 0;
    virtual domain::StreamStats finalize() = 0;
    virtual void add_alert(const domain::AlertRecord& alert) = 0;
};
```

### 5. Utility Classes (`internal/domain/`)

**`TimestampEngine.h` + `.cpp`:**
- `capture_wall_anchor()` → stores `system_clock::now()` as session start
- `to_session_offset(steady_clock::time_point frame_time)` → returns session-relative microseconds
- `with_hardware_pts(uint64_t hw_pts)` → hardware PTS for interval correction
- All methods thread-safe (mutex on anchor)

**`PluginRegistry.h` + `.cpp`:**
- `register_backend(std::unique_ptr<ICameraBackend>)`
- `register_enumerator(std::unique_ptr<IDeviceEnumerator>)`
- `discover_all()` → aggregates `DeviceInfo` from all registered enumerators
- `get_backend(const std::string& type)` → finds by type string

### 6. Skeleton main.cpp (`cmd/micecam/main.cpp`)

Minimal Qt application:
```cpp
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <spdlog/spdlog.h>

int main(int argc, char *argv[]) {
    spdlog::set_level(spdlog::level::info);
    spdlog::info("MiceCam v2 starting...");
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.load(QUrl("qrc:/main.qml"));
    if (engine.rootObjects().isEmpty()) return -1;
    return app.exec();
}
```

### 7. Minimal QML (`cmd/micecam/qml/main.qml`)

```qml
import QtQuick
import QtQuick.Controls
ApplicationWindow {
    visible: true
    width: 1280
    height: 800
    title: "MiceCam v2"
    Rectangle {
        anchors.fill: parent
        color: "#F2F2F7"
        Text {
            anchors.centerIn: parent
            text: "MiceCam v2\nFoundation Ready"
            font.pixelSize: 24
            horizontalAlignment: Text.AlignHCenter
            color: "#1C1C1E"
        }
    }
}
```

## Acceptance Criteria

- [ ] AC-001: `CMakeLists.txt.v2` exists alongside v1 `CMakeLists.txt`
- [ ] AC-002: All domain type headers compile (`DeviceInfo.h`, `StreamConfig.h`, `FrameTimestamp.h`, `SessionMetadata.h`, `StreamStats.h`, `AlertRecord.h`, `EncoderConfig.h`, `Capabilities.h`, `PluginDescriptor.h`)
- [ ] AC-003: All plugin interface headers compile (`ICameraBackend.h`, `IDeviceEnumerator.h`, `WatchdogObserver.h`)
- [ ] AC-004: All pipeline interface headers compile (`IEncoder.h`, `IStreamWriter.h`, `IStatsCollector.h`)
- [ ] AC-005: `TimestampEngine` compiles and links (header+cpp)
- [ ] AC-006: `PluginRegistry` compiles and links (header+cpp)
- [ ] AC-007: `SessionMetadata::to_json()` compiles — depends on nlohmann_json
- [ ] AC-008: `StreamStats::to_json()` compiles
- [ ] AC-009: `cmake -B build -S . && cmake --build build -j` succeeds (using v2 CMake)
- [ ] AC-010: `./build/cmd/micecam/micecam` launches, shows blank Qt window with "MiceCam v2 Foundation Ready"
- [ ] AC-011: No compile warnings (`-Wall -Wextra`)

## Forbidden Scope

- Do NOT overwrite v1 `CMakeLists.txt` — create `CMakeLists.txt.v2`
- Do NOT modify any v1 source files in `internal/`, `api/`, `cmd/micecam_ui/`, `cmd/gui/`
- Do NOT implement camera backends (OAK, FFmpeg) — interfaces only
- Do NOT implement FFmpeg encoding — `IEncoder` is interface-only
- Do NOT implement UI beyond the skeleton main.qml
- Do NOT write tests yet (TDD starts from Phase 2)
- Do NOT modify `.github/workflows/` for CI
- Do NOT delete legacy files (they'll be removed in final cleanup)

## Required Harness Process

Branch+ risk task:
1. `harness-risk` — classify as `branch` (touches multiple modules, defines shared contracts, no infrastructure changes)
2. Write code following domain types from spec
3. Build and run verification
4. Write `.pm/runtime/worker-report.md`
5. One git commit

## Verification Commands

```bash
cd /Volumes/DataHub/Projects/MiceCam
cmake -B build -S .
cmake --build build -j
./build/cmd/micecam/micecam  # should show Qt window (Ctrl+C to exit)
```

## Output

`.pm/runtime/worker-report.md` with build output, file listing, and verification results.
