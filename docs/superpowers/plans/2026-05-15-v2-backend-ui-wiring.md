# MiceCam v2 Backend/UI Wiring Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the existing polished QML UI consume real MiceCam v2 backend data, configuration, preflight, recording, stats, and alerts without changing the visual design.

**Architecture:** Add a narrow app-facing contract between QML and the existing v2 C++ backend. First make backend data/config/recording outputs trustworthy, then expose them through Qt models and a controller. QML keeps its current layout and styling; it only replaces hardcoded mock values with bound model properties and controller actions.

**Tech Stack:** C++20, CMake, GTest, Qt 6/QML, FFmpeg, nlohmann_json, spdlog.

---

## Current Findings

The UI and backend are not yet wired:

- `cmd/micecam_ui` is the real polished UI target. It currently builds with only `main.cpp` and `MockCameraModel.cpp`.
- `cmd/micecam_ui/main.cpp` registers `MockCameraModel` into QML. It does not create a backend manager, controller, settings service, alert model, or recording service.
- `cmd/micecam_ui/qml/components/CameraGridView.qml` hardcodes five camera cards. It does not consume a model.
- `cmd/micecam_ui/qml/components/AppToolbar.qml` starts in `isRecording: true` and the click handler only opens preflight when not recording.
- `cmd/micecam_ui/qml/components/AppStatusBar.qml`, `PreflightModal.qml`, `NotificationPopup.qml`, and several detail/settings views contain fixed demo values.
- Backend modules exist, but they are not yet UI-ready. The most important backend defect is that `RecordingPipeline::push_frame()` writes raw frame bytes directly to `StreamWriter` instead of encoding through `TranscodeStage::process()`.

## File Structure

### Backend Contract and Pipeline

- Modify `internal/domain/DeviceInfo.h`  
  Add explicit resolution options, backend availability/status, and UI-safe stream labels.

- Modify `internal/domain/Capabilities.h`  
  Represent capability options that the camera detail view can bind to directly.

- Modify `api/micecam/ICameraBackend.h`  
  Add per-device/per-stream capability lookup while keeping backend-level capability for compatibility.

- Modify `internal/infrastructure/MockCameraBackend.{h,cpp}`  
  Return deterministic v2-style mock devices and capability options for UI smoke mode.

- Modify `internal/infrastructure/OAKCameraBackend.cpp` and `FFmpegCameraBackend.cpp`  
  Populate the richer capability/device fields.

- Modify `internal/pipeline/PreflightValidator.{h,cpp}`  
  Return a detailed list of failures/warnings that the preflight modal can render.

- Modify `internal/pipeline/RecordingPipeline.{h,cpp}`  
  Encode frames through `TranscodeStage`, flush encoders on stop, track bytes, encoder name, fallback, and write failures.

- Modify `internal/pipeline/StatsCollector.{h,cpp}` and `internal/domain/StreamStats.h`  
  Support live stats snapshots as well as final stats.

- Modify `internal/infrastructure/AlertManager.{h,cpp}`  
  Preserve a queryable alert history for notification popups.

- Modify `internal/infrastructure/ConfigLoader.{h,cpp}`  
  Add mutable config state and JSON save.

### Qt App Adapter

- Create `cmd/micecam_ui/AppCameraModel.{h,cpp}`  
  Qt list model for QML camera rows and card data.

- Create `cmd/micecam_ui/AppAlertModel.{h,cpp}`  
  Qt list model for alert history and badge count.

- Create `cmd/micecam_ui/AppSettings.{h,cpp}`  
  Qt-facing settings object backed by `ConfigLoader`.

- Create `cmd/micecam_ui/AppController.{h,cpp}`  
  Owns `CameraManager`, `RecordingPipeline`, `PreflightValidator`, models, and high-level app state.

- Modify `cmd/micecam_ui/main.cpp`  
  Register controller/models through QML context properties.

- Modify `cmd/micecam_ui/CMakeLists.txt`  
  Link `micecam_ui` to `micecam_encoding` and compile the adapter files.

### QML Binding Only

Visual layout must remain intact. Only replace data sources and click actions:

- Modify `cmd/micecam_ui/qml/main.qml`
- Modify `cmd/micecam_ui/qml/components/AppToolbar.qml`
- Modify `cmd/micecam_ui/qml/components/AppSidebar.qml`
- Modify `cmd/micecam_ui/qml/components/CameraGridView.qml`
- Modify `cmd/micecam_ui/qml/components/AppStatusBar.qml`
- Modify `cmd/micecam_ui/qml/components/PreflightModal.qml`
- Modify `cmd/micecam_ui/qml/components/NotificationPopup.qml`
- Modify `cmd/micecam_ui/qml/components/CameraDetailView.qml`
- Modify `cmd/micecam_ui/qml/components/EncodingSettings.qml`
- Modify `cmd/micecam_ui/qml/components/AlertsSettings.qml`
- Modify `cmd/micecam_ui/qml/components/LoggingSettings.qml`

### Tests

- Add `tests/unit/test_backend_ui_contract.cpp`
- Add `tests/unit/test_preflight_detail.cpp`
- Add `tests/integration/test_recording_pipeline_outputs.cpp`
- Add `tests/unit/test_app_controller.cpp`
- Add `tests/unit/test_app_models.cpp`

---

## Task 1: Backend UI Contract for Camera Data and Capabilities

**Files:**
- Modify: `internal/domain/DeviceInfo.h`
- Modify: `internal/domain/Capabilities.h`
- Modify: `api/micecam/ICameraBackend.h`
- Modify: `internal/infrastructure/MockCameraBackend.h`
- Modify: `internal/infrastructure/MockCameraBackend.cpp`
- Modify: `internal/infrastructure/OAKCameraBackend.cpp`
- Modify: `internal/infrastructure/FFmpegCameraBackend.cpp`
- Test: `tests/unit/test_backend_ui_contract.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing contract tests**

Create `tests/unit/test_backend_ui_contract.cpp`:

```cpp
#include <gtest/gtest.h>

#include "domain/DeviceInfo.h"
#include "infrastructure/MockCameraBackend.h"

using namespace micecam;

TEST(BackendUiContract, MockDiscoveryProvidesUiReadyCameraRows) {
    infrastructure::MockCameraBackend backend;

    auto devices = backend.enumerate_devices();

    ASSERT_GE(devices.size(), 1u);
    const auto& device = devices.front();
    EXPECT_FALSE(device.id.empty());
    EXPECT_FALSE(device.name.empty());
    EXPECT_FALSE(device.type.empty());
    ASSERT_GE(device.streams.size(), 1u);

    const auto& stream = device.streams.front();
    EXPECT_FALSE(stream.label.empty());
    EXPECT_GE(stream.resolutions.size(), 1u);
    EXPECT_GE(stream.supported_framerates.size(), 1u);
    EXPECT_GE(stream.supported_formats.size(), 1u);
    EXPECT_TRUE(stream.available);
}

TEST(BackendUiContract, MockCapabilitiesArePerStreamAndSelectable) {
    infrastructure::MockCameraBackend backend;

    auto devices = backend.enumerate_devices();
    ASSERT_GE(devices.size(), 1u);
    ASSERT_GE(devices.front().streams.size(), 1u);

    auto caps = backend.get_capabilities(devices.front().id, devices.front().streams.front().index);

    ASSERT_GE(caps.streams.size(), 1u);
    const auto& stream = caps.streams.front();
    EXPECT_GE(stream.resolutions.size(), 2u);
    EXPECT_GE(stream.supported_framerates.size(), 2u);
    EXPECT_GE(stream.supported_formats.size(), 1u);
    EXPECT_FALSE(caps.encoder_name.empty());
}
```

- [ ] **Step 2: Register the test target**

In `CMakeLists.txt`, add:

```cmake
add_micecam_test(test_backend_ui_contract tests/unit/test_backend_ui_contract.cpp)
```

- [ ] **Step 3: Run the test to verify it fails**

Run:

```bash
cmake -B build -S . -DBUILD_UI=ON
cmake --build build --target test_backend_ui_contract -j
```

Expected: compile failure because `StreamInfo::label`, `StreamInfo::resolutions`, `StreamInfo::available`, and `ICameraBackend::get_capabilities(device_id, stream_index)` do not exist.

- [ ] **Step 4: Extend domain capability types**

In `internal/domain/DeviceInfo.h`, replace `StreamInfo` with:

```cpp
struct ResolutionOption {
    int width = 0;
    int height = 0;
    std::string label;
};

struct StreamInfo {
    int index = 0;
    std::string id;
    std::string label;
    int max_width = 0;
    int max_height = 0;
    std::vector<ResolutionOption> resolutions;
    std::vector<std::string> supported_formats;
    std::vector<int> supported_framerates;
    bool available = true;
    std::string unavailable_reason;
};
```

Keep `DeviceInfo` as:

```cpp
struct DeviceInfo {
    std::string id;
    std::string name;
    std::string vendor;
    std::string serial;
    std::string type;
    std::vector<StreamInfo> streams;
    bool available = true;
    std::string unavailable_reason;
};
```

- [ ] **Step 5: Add per-stream capability lookup**

In `api/micecam/ICameraBackend.h`, add this virtual method with a default implementation:

```cpp
virtual domain::Capabilities get_capabilities(const std::string& device_id, int stream_index) {
    (void)device_id;
    (void)stream_index;
    return get_capabilities();
}
```

- [ ] **Step 6: Populate mock backend UI data**

In `internal/infrastructure/MockCameraBackend.cpp`, make `enumerate_devices()` return one mock OAK-like device with five streams for UI smoke mode:

```cpp
std::vector<domain::DeviceInfo> MockCameraBackend::enumerate_devices() {
    domain::DeviceInfo info;
    info.id = "mock_device_0";
    info.name = "Mock Lab Rig";
    info.vendor = "MiceCam";
    info.serial = "MOCK-0000";
    info.type = "mock";

    const std::vector<std::string> labels = {"CAM_A", "CAM_B", "CAM_C", "CAM_D", "USB-1"};
    for (int i = 0; i < static_cast<int>(labels.size()); ++i) {
        domain::StreamInfo si;
        si.index = i;
        si.id = "mock_cam_" + std::to_string(i);
        si.label = labels[i];
        si.max_width = 1920;
        si.max_height = 1080;
        si.resolutions = {
            {1920, 1080, "1920 x 1080"},
            {1280, 720, "1280 x 720"},
            {640, 480, "640 x 480"}
        };
        si.supported_formats = {"rgb24"};
        si.supported_framerates = {15, 30, 60};
        info.streams.push_back(si);
    }
    return {info};
}
```

Also update `open_stream()` so it accepts `mock_cam_0` through `mock_cam_4`:

```cpp
if (config.device_id.rfind("mock_cam_", 0) != 0 && config.device_id != "mock_device_0") {
    return nullptr;
}
```

- [ ] **Step 7: Add backend-specific capability overrides**

In `MockCameraBackend.h`, declare:

```cpp
domain::Capabilities get_capabilities(const std::string& device_id, int stream_index) override;
```

In `MockCameraBackend.cpp`, implement:

```cpp
domain::Capabilities MockCameraBackend::get_capabilities(const std::string& device_id, int stream_index) {
    (void)device_id;
    (void)stream_index;
    return get_capabilities();
}
```

For OAK and FFmpeg backends, ensure every returned `StreamInfo` includes `id`, `label`, `resolutions`, `available`, and format/fps options. Use labels `CAM_A` through `CAM_D` for OAK streams. Use the backend device name for USB labels.

- [ ] **Step 8: Run tests**

Run:

```bash
cmake --build build --target test_backend_ui_contract -j
build/tests/test_backend_ui_contract
```

Expected: all tests pass.

- [ ] **Step 9: Commit**

```bash
git add CMakeLists.txt api/micecam/ICameraBackend.h internal/domain/DeviceInfo.h internal/domain/Capabilities.h internal/infrastructure/MockCameraBackend.h internal/infrastructure/MockCameraBackend.cpp internal/infrastructure/OAKCameraBackend.cpp internal/infrastructure/FFmpegCameraBackend.cpp tests/unit/test_backend_ui_contract.cpp
git commit -m "feat(ui): define backend camera capability contract"
```

---

## Task 2: Detailed Preflight Contract for UI Failures and Warnings

**Files:**
- Modify: `internal/pipeline/PreflightValidator.h`
- Modify: `internal/pipeline/PreflightValidator.cpp`
- Test: `tests/unit/test_preflight_detail.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write failing preflight detail tests**

Create `tests/unit/test_preflight_detail.cpp`:

```cpp
#include <gtest/gtest.h>

#include "domain/Capabilities.h"
#include "domain/StreamConfig.h"
#include "pipeline/PreflightValidator.h"

using namespace micecam;

TEST(PreflightDetail, ReportsUnsupportedResolutionAsFieldFailure) {
    pipeline::PreflightValidator validator;

    domain::StreamConfig config;
    config.device_id = "mock_cam_0";
    config.stream_index = 0;
    config.width = 3840;
    config.height = 2160;
    config.framerate = 30;
    config.pixel_format = "rgb24";

    domain::Capabilities caps;
    domain::StreamInfo stream;
    stream.index = 0;
    stream.resolutions = {{1920, 1080, "1920 x 1080"}};
    stream.supported_framerates = {30};
    stream.supported_formats = {"rgb24"};
    caps.streams.push_back(stream);

    auto result = validator.validate_stream_capabilities(config, caps);

    EXPECT_FALSE(result.passed);
    ASSERT_EQ(result.items.size(), 1u);
    EXPECT_EQ(result.items.front().code, "unsupported_resolution");
    EXPECT_EQ(result.items.front().stream_id, "mock_cam_0");
}

TEST(PreflightDetail, PassingCapabilityProducesNoItems) {
    pipeline::PreflightValidator validator;

    domain::StreamConfig config;
    config.device_id = "mock_cam_0";
    config.stream_index = 0;
    config.width = 1920;
    config.height = 1080;
    config.framerate = 30;
    config.pixel_format = "rgb24";

    domain::Capabilities caps;
    domain::StreamInfo stream;
    stream.index = 0;
    stream.resolutions = {{1920, 1080, "1920 x 1080"}};
    stream.supported_framerates = {15, 30, 60};
    stream.supported_formats = {"rgb24"};
    caps.streams.push_back(stream);

    auto result = validator.validate_stream_capabilities(config, caps);

    EXPECT_TRUE(result.passed);
    EXPECT_TRUE(result.items.empty());
}
```

- [ ] **Step 2: Register the test**

In `CMakeLists.txt`, add:

```cmake
add_micecam_test(test_preflight_detail tests/unit/test_preflight_detail.cpp)
```

- [ ] **Step 3: Run the test to verify it fails**

Run:

```bash
cmake -B build -S . -DBUILD_UI=ON
cmake --build build --target test_preflight_detail -j
```

Expected: compile failure because `PreflightItem` and `validate_stream_capabilities()` do not exist.

- [ ] **Step 4: Add detailed preflight result types**

In `internal/pipeline/PreflightValidator.h`, add:

```cpp
enum class PreflightSeverity {
    Info,
    Warning,
    Error
};

struct PreflightItem {
    PreflightSeverity severity = PreflightSeverity::Error;
    std::string code;
    std::string title;
    std::string message;
    std::string stream_id;
};
```

Change `PreflightResult` to:

```cpp
struct PreflightResult {
    bool passed = false;
    std::string message;
    std::vector<std::string> warnings;
    std::vector<PreflightItem> items;
};
```

Declare:

```cpp
PreflightResult validate_stream_capabilities(const domain::StreamConfig& config,
                                             const domain::Capabilities& caps) const;
```

- [ ] **Step 5: Implement capability detail validation**

In `PreflightValidator.cpp`, implement exact matching against `resolutions`, fps, and format:

```cpp
PreflightResult PreflightValidator::validate_stream_capabilities(
    const domain::StreamConfig& config,
    const domain::Capabilities& caps) const {

    PreflightResult result;
    result.passed = true;

    const domain::StreamInfo* matched_stream = nullptr;
    for (const auto& stream : caps.streams) {
        if (stream.index == config.stream_index) {
            matched_stream = &stream;
            break;
        }
    }
    if (!matched_stream && !caps.streams.empty()) {
        matched_stream = &caps.streams.front();
    }

    if (!matched_stream) {
        result.passed = false;
        result.items.push_back({PreflightSeverity::Error, "missing_capabilities",
                                "Camera capabilities unavailable",
                                "No capability data is available for this stream.",
                                config.device_id});
        result.message = "Preflight checks failed";
        return result;
    }

    bool resolution_ok = matched_stream->resolutions.empty();
    for (const auto& option : matched_stream->resolutions) {
        if (option.width == config.width && option.height == config.height) {
            resolution_ok = true;
            break;
        }
    }
    if (!resolution_ok) {
        result.passed = false;
        result.items.push_back({PreflightSeverity::Error, "unsupported_resolution",
                                "Unsupported resolution",
                                "Selected resolution is not advertised by the camera backend.",
                                config.device_id});
    }

    bool fps_ok = config.framerate == 0;
    for (int fps : matched_stream->supported_framerates) {
        if (fps == config.framerate) {
            fps_ok = true;
            break;
        }
    }
    if (!fps_ok) {
        result.passed = false;
        result.items.push_back({PreflightSeverity::Error, "unsupported_framerate",
                                "Unsupported frame rate",
                                "Selected frame rate is not advertised by the camera backend.",
                                config.device_id});
    }

    bool format_ok = config.pixel_format.empty();
    for (const auto& format : matched_stream->supported_formats) {
        if (format == config.pixel_format) {
            format_ok = true;
            break;
        }
    }
    if (!format_ok) {
        result.passed = false;
        result.items.push_back({PreflightSeverity::Error, "unsupported_format",
                                "Unsupported pixel format",
                                "Selected pixel format is not advertised by the camera backend.",
                                config.device_id});
    }

    result.message = result.passed ? "Preflight checks passed" : "Preflight checks failed";
    return result;
}
```

- [ ] **Step 6: Run tests**

Run:

```bash
cmake --build build --target test_preflight_detail -j
build/tests/test_preflight_detail
build/tests/test_preflight
```

Expected: both test executables pass.

- [ ] **Step 7: Commit**

```bash
git add CMakeLists.txt internal/pipeline/PreflightValidator.h internal/pipeline/PreflightValidator.cpp tests/unit/test_preflight_detail.cpp
git commit -m "feat(ui): expose detailed preflight failures"
```

---

## Task 3: RecordingPipeline Produces Valid H264 MP4 from Raw Frames

**Files:**
- Modify: `internal/pipeline/RecordingPipeline.h`
- Modify: `internal/pipeline/RecordingPipeline.cpp`
- Modify: `internal/pipeline/StatsCollector.h`
- Modify: `internal/pipeline/StatsCollector.cpp`
- Modify: `internal/domain/StreamStats.h`
- Modify: `internal/domain/StreamStats.cpp`
- Test: `tests/integration/test_recording_pipeline_outputs.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write failing integration test for valid output**

Create `tests/integration/test_recording_pipeline_outputs.cpp`:

```cpp
#include <gtest/gtest.h>

#include <filesystem>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "domain/EncoderConfig.h"
#include "domain/StreamConfig.h"
#include "pipeline/RecordingPipeline.h"

namespace {

std::vector<uint8_t> rgb_frame(int width, int height, int seed) {
    std::vector<uint8_t> data(static_cast<size_t>(width) * height * 3);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const size_t idx = (static_cast<size_t>(y) * width + x) * 3;
            data[idx] = static_cast<uint8_t>((x + seed) % 256);
            data[idx + 1] = static_cast<uint8_t>((y + seed) % 256);
            data[idx + 2] = static_cast<uint8_t>((x + y + seed) % 256);
        }
    }
    return data;
}

bool has_h264_video_stream(const std::string& path) {
    AVFormatContext* ctx = nullptr;
    if (avformat_open_input(&ctx, path.c_str(), nullptr, nullptr) < 0) {
        return false;
    }
    bool found = false;
    if (avformat_find_stream_info(ctx, nullptr) >= 0) {
        for (unsigned i = 0; i < ctx->nb_streams; ++i) {
            if (ctx->streams[i]->codecpar->codec_id == AV_CODEC_ID_H264) {
                found = true;
                break;
            }
        }
    }
    avformat_close_input(&ctx);
    return found;
}

} // namespace

TEST(RecordingPipelineOutputs, RawRgbFramesProduceValidH264Mp4) {
    const std::string root = "/tmp/micecam_recording_pipeline_outputs";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    micecam::pipeline::SessionConfig config;
    config.session_id = "valid_h264";
    config.output_dir = root;
    config.encoder.prefer_hardware = false;
    config.encoder.bitrate_kbps = 1000;
    config.encoder.keyframe_interval = 15;

    micecam::domain::StreamConfig stream;
    stream.device_id = "mock_cam_0";
    stream.stream_index = 0;
    stream.width = 160;
    stream.height = 120;
    stream.framerate = 30;
    stream.pixel_format = "rgb24";
    config.streams.push_back(stream);

    micecam::pipeline::RecordingPipeline pipeline;
    ASSERT_TRUE(pipeline.start(config));

    for (int i = 0; i < 30; ++i) {
        const auto frame = rgb_frame(stream.width, stream.height, i);
        micecam::pipeline::FrameData data;
        data.stream_id = "mock_cam_0_0";
        data.data = frame.data();
        data.size = frame.size();
        data.width = stream.width;
        data.height = stream.height;
        data.pts = i;
        data.source_format = "rgb24";
        ASSERT_TRUE(pipeline.push_frame(data));
    }

    pipeline.stop();
    const auto [meta, stats] = pipeline.result();

    const std::string mp4_path = root + "/valid_h264/mock_cam_0_0.mp4";
    EXPECT_TRUE(std::filesystem::exists(mp4_path));
    EXPECT_TRUE(has_h264_video_stream(mp4_path));
    ASSERT_EQ(stats.size(), 1u);
    EXPECT_EQ(stats.front().frames_actual, 30u);
    EXPECT_GT(stats.front().bytes_written, 0u);
    EXPECT_FALSE(stats.front().encoder_used.empty());
}
```

- [ ] **Step 2: Register the test**

In `CMakeLists.txt`, add:

```cmake
add_micecam_test(test_recording_pipeline_outputs tests/integration/test_recording_pipeline_outputs.cpp)
```

- [ ] **Step 3: Run the test to verify it fails**

Run:

```bash
cmake -B build -S . -DBUILD_UI=ON
cmake --build build --target test_recording_pipeline_outputs -j
build/tests/test_recording_pipeline_outputs
```

Expected: test fails because the MP4 does not contain a valid H264 stream or stats bytes/encoder fields are empty.

- [ ] **Step 4: Update stats collector for bytes and encoder**

In `StatsCollector.h`, add:

```cpp
void add_bytes(uint64_t bytes);
void set_encoder(std::string encoder_name, bool fallback);
domain::StreamStats snapshot();
```

In `StatsCollector.cpp`, implement:

```cpp
void StatsCollector::add_bytes(uint64_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    bytes_written_ += bytes;
}

void StatsCollector::set_encoder(std::string encoder_name, bool fallback) {
    std::lock_guard<std::mutex> lock(mutex_);
    encoder_used_ = std::move(encoder_name);
    encoder_fallback_ = fallback;
}

domain::StreamStats StatsCollector::snapshot() {
    std::lock_guard<std::mutex> lock(mutex_);
    domain::StreamStats stats;
    stats.stream_id = stream_id_;
    stats.frames_expected = frames_expected_;
    stats.frames_actual = frames_actual_;
    stats.drop_rate = (frames_expected_ > 0)
        ? static_cast<double>(frames_expected_ - frames_actual_) / frames_expected_
        : 0.0;
    stats.avg_encode_latency_us = (encode_count_ > 0) ? encode_sum_us_ / encode_count_ : 0.0;
    stats.max_encode_latency_us = encode_max_us_;
    stats.min_encode_latency_us = (encode_min_us_ > 1e17) ? 0.0 : encode_min_us_;
    stats.avg_frame_interval_us = (encode_count_ > 0) ? frame_interval_sum_us_ / encode_count_ : 0.0;
    stats.max_frame_interval_deviation_us = frame_interval_max_deviation_us_;
    stats.bytes_written = bytes_written_;
    stats.encoder_used = encoder_used_;
    stats.encoder_fallback = encoder_fallback_;
    return stats;
}
```

Change `finalize()` to return `snapshot()`.

- [ ] **Step 5: Encode through TranscodeStage in RecordingPipeline**

In `RecordingPipeline.cpp`, replace the direct writer call in `push_frame()` with:

```cpp
const auto encode_start = std::chrono::steady_clock::now();
auto packet = sp.transcoder->process(frame.data, frame.size, frame.width, frame.height,
                                     frame.pts, frame.source_format);
const auto encode_end = std::chrono::steady_clock::now();
const auto encode_latency_us =
    std::chrono::duration<double, std::micro>(encode_end - encode_start).count();

if (packet.empty()) {
    return true;
}

sp.stats->set_encoder(sp.transcoder->encoder_name(),
                      sp.transcoder->encoder_name() == "libx264" && config_.encoder.prefer_hardware);
sp.stats->record_frame(frame_seq, frame_seq, encode_latency_us, frame_interval_us);

domain::FrameTimestamp fts;
fts.session_offset_us = static_cast<uint64_t>(frame.pts);
sp.srt->write_entry(frame_seq, fts, false);

const bool keyframe = (frame_seq % static_cast<uint64_t>(std::max(1, config_.encoder.keyframe_interval)) == 0);
if (sp.writer->write_packet(packet.data(), packet.size(), frame.pts, frame.pts, keyframe)) {
    sp.stats->add_bytes(packet.size());
}
```

In `stop()`, before closing each writer, flush:

```cpp
std::vector<uint8_t> flushed;
if (sp->transcoder && sp->transcoder->flush(flushed) && !flushed.empty()) {
    const int64_t pts = static_cast<int64_t>(sp->frame_seq);
    if (sp->writer->write_packet(flushed.data(), flushed.size(), pts, pts, true)) {
        sp->stats->add_bytes(flushed.size());
    }
}
```

- [ ] **Step 6: Run output and existing pipeline tests**

Run:

```bash
cmake --build build --target test_recording_pipeline_outputs test_recording_pipeline test_camera_pipeline_integration -j
build/tests/test_recording_pipeline_outputs
build/tests/test_recording_pipeline
build/tests/test_camera_pipeline_integration
```

Expected: all three pass.

- [ ] **Step 7: Commit**

```bash
git add CMakeLists.txt internal/pipeline/RecordingPipeline.h internal/pipeline/RecordingPipeline.cpp internal/pipeline/StatsCollector.h internal/pipeline/StatsCollector.cpp internal/domain/StreamStats.h internal/domain/StreamStats.cpp tests/integration/test_recording_pipeline_outputs.cpp
git commit -m "fix(pipeline): encode raw frames before writing mp4"
```

---

## Task 4: Mutable Settings Contract for UI

**Files:**
- Modify: `internal/infrastructure/ConfigLoader.h`
- Modify: `internal/infrastructure/ConfigLoader.cpp`
- Test: `tests/unit/test_config_loader.cpp`

- [ ] **Step 1: Add failing settings save test**

Append to `tests/unit/test_config_loader.cpp`:

```cpp
TEST(ConfigLoader, SavePersistsUiEditableSettings) {
    const std::string path = "/tmp/micecam_ui_settings_test.json";
    std::remove(path.c_str());

    micecam::infrastructure::ConfigLoader config;
    config.set_watchdog_timeout_s(7);
    config.set_drop_rate_yellow_pct(0.2);
    config.set_drop_rate_red_pct(1.5);
    config.set_webhook_url("https://example.invalid/hook");
    config.set_default_bitrate_kbps(8000);
    config.set_output_dir("/tmp/micecam-output");
    config.set_log_level("debug");

    ASSERT_TRUE(config.save(path));

    micecam::infrastructure::ConfigLoader loaded;
    ASSERT_TRUE(loaded.load(path));
    EXPECT_EQ(loaded.watchdog_timeout_s(), 7);
    EXPECT_DOUBLE_EQ(loaded.drop_rate_yellow_pct(), 0.2);
    EXPECT_DOUBLE_EQ(loaded.drop_rate_red_pct(), 1.5);
    EXPECT_EQ(loaded.webhook_url(), "https://example.invalid/hook");
    EXPECT_EQ(loaded.default_bitrate_kbps(), 8000);
    EXPECT_EQ(loaded.output_dir(), "/tmp/micecam-output");
    EXPECT_EQ(loaded.log_level(), "debug");
}
```

- [ ] **Step 2: Run the test to verify it fails**

Run:

```bash
cmake --build build --target test_config_loader -j
```

Expected: compile failure because setters and `save()` do not exist.

- [ ] **Step 3: Add setters and save**

In `ConfigLoader.h`, add:

```cpp
void set_watchdog_timeout_s(int value) { watchdog_timeout_s_ = value; }
void set_drop_rate_yellow_pct(double value) { drop_rate_yellow_pct_ = value; }
void set_drop_rate_red_pct(double value) { drop_rate_red_pct_ = value; }
void set_webhook_url(std::string value) { webhook_url_ = std::move(value); }
void set_default_bitrate_kbps(int value) { default_bitrate_kbps_ = value; }
void set_output_dir(std::string value) { output_dir_ = std::move(value); }
void set_log_level(std::string value) { log_level_ = std::move(value); }
bool save(const std::string& config_path) const;
```

In `ConfigLoader.cpp`, implement:

```cpp
bool ConfigLoader::save(const std::string& config_path) const {
    nlohmann::json j;
    j["watchdog_timeout_s"] = watchdog_timeout_s_;
    j["drop_rate_yellow_pct"] = drop_rate_yellow_pct_;
    j["drop_rate_red_pct"] = drop_rate_red_pct_;
    j["webhook_url"] = webhook_url_;
    j["default_bitrate_kbps"] = default_bitrate_kbps_;
    j["output_dir"] = output_dir_;
    j["log_level"] = log_level_;

    std::ofstream file(config_path);
    if (!file.is_open()) {
        return false;
    }
    file << j.dump(2);
    return file.good();
}
```

- [ ] **Step 4: Run tests**

Run:

```bash
cmake --build build --target test_config_loader -j
build/tests/test_config_loader
```

Expected: pass.

- [ ] **Step 5: Commit**

```bash
git add internal/infrastructure/ConfigLoader.h internal/infrastructure/ConfigLoader.cpp tests/unit/test_config_loader.cpp
git commit -m "feat(settings): persist ui editable configuration"
```

---

## Task 5: Alert History Contract for Notification UI

**Files:**
- Modify: `internal/infrastructure/AlertManager.h`
- Modify: `internal/infrastructure/AlertManager.cpp`
- Test: `tests/unit/test_alert_manager.cpp`

- [ ] **Step 1: Add failing alert history test**

Append to `tests/unit/test_alert_manager.cpp`:

```cpp
TEST(AlertManager, StoresHistoryForUiNotificationList) {
    micecam::infrastructure::AlertManager manager;

    micecam::domain::AlertRecord alert;
    alert.type = micecam::domain::AlertType::HIGH_DROP_RATE;
    alert.severity = micecam::domain::AlertSeverity::YELLOW;
    alert.stream_id = "mock_cam_0";
    alert.message = "Drop rate exceeded warning threshold";

    manager.emit(alert);

    auto history = manager.history();
    ASSERT_EQ(history.size(), 1u);
    EXPECT_EQ(history.front().stream_id, "mock_cam_0");
    EXPECT_EQ(history.front().message, "Drop rate exceeded warning threshold");

    manager.clear_history();
    EXPECT_TRUE(manager.history().empty());
}
```

- [ ] **Step 2: Run the test to verify it fails**

Run:

```bash
cmake --build build --target test_alert_manager -j
```

Expected: compile failure because `history()` and `clear_history()` do not exist.

- [ ] **Step 3: Add history APIs**

In `AlertManager.h`, add:

```cpp
std::vector<domain::AlertRecord> history() const;
void clear_history();
```

Add private field:

```cpp
std::vector<domain::AlertRecord> history_;
```

In `AlertManager.cpp`, store non-duplicate alerts:

```cpp
history_.push_back(alert);
```

inside `emit()` after the dedup check and before observer notification.

Implement:

```cpp
std::vector<domain::AlertRecord> AlertManager::history() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return history_;
}

void AlertManager::clear_history() {
    std::lock_guard<std::mutex> lock(mutex_);
    history_.clear();
}
```

Make `mutex_` mutable in the header:

```cpp
mutable std::mutex mutex_;
```

- [ ] **Step 4: Run tests**

Run:

```bash
cmake --build build --target test_alert_manager -j
build/tests/test_alert_manager
```

Expected: pass.

- [ ] **Step 5: Commit**

```bash
git add internal/infrastructure/AlertManager.h internal/infrastructure/AlertManager.cpp tests/unit/test_alert_manager.cpp
git commit -m "feat(alerts): retain alert history for ui"
```

---

## Task 6: Qt App Models and Controller

**Files:**
- Create: `cmd/micecam_ui/AppCameraModel.h`
- Create: `cmd/micecam_ui/AppCameraModel.cpp`
- Create: `cmd/micecam_ui/AppAlertModel.h`
- Create: `cmd/micecam_ui/AppAlertModel.cpp`
- Create: `cmd/micecam_ui/AppSettings.h`
- Create: `cmd/micecam_ui/AppSettings.cpp`
- Create: `cmd/micecam_ui/AppController.h`
- Create: `cmd/micecam_ui/AppController.cpp`
- Modify: `cmd/micecam_ui/CMakeLists.txt`
- Test: `tests/unit/test_app_models.cpp`
- Test: `tests/unit/test_app_controller.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write failing app model tests**

Create `tests/unit/test_app_models.cpp`:

```cpp
#include <gtest/gtest.h>

#include <QCoreApplication>

#include "cmd/micecam_ui/AppAlertModel.h"
#include "cmd/micecam_ui/AppCameraModel.h"

namespace {
int ensure_qt(int argc, char** argv) {
    if (!QCoreApplication::instance()) {
        static QCoreApplication app(argc, argv);
    }
    return 0;
}
} // namespace

TEST(AppCameraModel, LoadsRowsFromBackendSnapshot) {
    micecam::ui::AppCameraModel model;

    micecam::ui::CameraRow row;
    row.cameraId = "mock_cam_0";
    row.name = "CAM_A";
    row.fps = 30.0;
    row.dropCount = 0;
    row.recording = false;
    row.status = 0;
    row.resolutionLabels = {"1920 x 1080", "1280 x 720"};
    row.framerateLabels = {"15 fps", "30 fps"};
    row.formatLabels = {"rgb24"};

    model.replaceRows({row});

    ASSERT_EQ(model.rowCount(), 1);
    const auto index = model.index(0, 0);
    EXPECT_EQ(model.data(index, micecam::ui::AppCameraModel::NameRole).toString(), "CAM_A");
    EXPECT_EQ(model.data(index, micecam::ui::AppCameraModel::IsRecordingRole).toBool(), false);
    EXPECT_EQ(model.data(index, micecam::ui::AppCameraModel::ResolutionOptionsRole).toStringList().size(), 2);
}

TEST(AppAlertModel, LoadsAlertsAndTracksBadgeCount) {
    micecam::ui::AppAlertModel model;

    micecam::ui::AlertRow row;
    row.severity = 1;
    row.title = "High drop rate";
    row.source = "CAM_A";
    row.relativeTime = "now";

    model.replaceRows({row});

    ASSERT_EQ(model.rowCount(), 1);
    EXPECT_EQ(model.badgeCount(), 1);
    model.clear();
    EXPECT_EQ(model.rowCount(), 0);
    EXPECT_EQ(model.badgeCount(), 0);
}

int main(int argc, char** argv) {
    ensure_qt(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

- [ ] **Step 2: Write failing app controller tests**

Create `tests/unit/test_app_controller.cpp`:

```cpp
#include <gtest/gtest.h>

#include <QCoreApplication>

#include "cmd/micecam_ui/AppController.h"

namespace {
int ensure_qt(int argc, char** argv) {
    if (!QCoreApplication::instance()) {
        static QCoreApplication app(argc, argv);
    }
    return 0;
}
} // namespace

TEST(AppController, MockModeDiscoversUiReadyCameras) {
    micecam::ui::AppController controller(micecam::ui::BackendMode::MockOnly);

    controller.refreshCameras();

    ASSERT_NE(controller.cameraModel(), nullptr);
    EXPECT_GE(controller.cameraModel()->rowCount(), 1);
    EXPECT_EQ(controller.cameraCountText(), "5 cameras");
    EXPECT_FALSE(controller.isRecording());
    EXPECT_EQ(controller.recordButtonText(), "Record");
}

TEST(AppController, StartAndStopRecordingUpdatesState) {
    micecam::ui::AppController controller(micecam::ui::BackendMode::MockOnly);
    controller.setOutputDirectory("/tmp/micecam_app_controller");
    controller.refreshCameras();

    ASSERT_TRUE(controller.startRecording());
    EXPECT_TRUE(controller.isRecording());
    EXPECT_EQ(controller.recordButtonText(), "Stop");

    controller.stopRecording();
    EXPECT_FALSE(controller.isRecording());
    EXPECT_EQ(controller.recordButtonText(), "Record");
    EXPECT_FALSE(controller.lastSessionId().isEmpty());
}

int main(int argc, char** argv) {
    ensure_qt(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

- [ ] **Step 3: Register test and UI sources**

In `cmd/micecam_ui/CMakeLists.txt`, replace the executable source list:

```cmake
add_executable(micecam_ui
    main.cpp
    AppCameraModel.cpp
    AppAlertModel.cpp
    AppSettings.cpp
    AppController.cpp
)
```

Link backend:

```cmake
target_link_libraries(micecam_ui PRIVATE
    micecam_encoding
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)
```

In root `CMakeLists.txt`, register tests:

```cmake
add_micecam_test(test_app_models tests/unit/test_app_models.cpp)
add_micecam_test(test_app_controller tests/unit/test_app_controller.cpp)

if(BUILD_UI)
    target_link_libraries(test_app_models PRIVATE Qt6::Core)
    target_link_libraries(test_app_controller PRIVATE Qt6::Core Qt6::Qml)
    target_sources(test_app_models PRIVATE
        cmd/micecam_ui/AppCameraModel.cpp
        cmd/micecam_ui/AppAlertModel.cpp
    )
    target_sources(test_app_controller PRIVATE
        cmd/micecam_ui/AppCameraModel.cpp
        cmd/micecam_ui/AppAlertModel.cpp
        cmd/micecam_ui/AppSettings.cpp
        cmd/micecam_ui/AppController.cpp
    )
endif()
```

- [ ] **Step 4: Run the tests to verify they fail**

Run:

```bash
cmake -B build -S . -DBUILD_UI=ON
cmake --build build --target test_app_models test_app_controller -j
```

Expected: compile failure because app model/controller files do not exist.

- [ ] **Step 5: Implement AppCameraModel**

Create `cmd/micecam_ui/AppCameraModel.h`:

```cpp
#pragma once

#include <QAbstractListModel>
#include <QStringList>

namespace micecam::ui {

struct CameraRow {
    QString cameraId;
    QString name;
    double fps = 0.0;
    int dropCount = 0;
    bool recording = false;
    int status = 0;
    QString alertMessage;
    QStringList resolutionLabels;
    QStringList framerateLabels;
    QStringList formatLabels;
};

class AppCameraModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        CameraIdRole = Qt::UserRole + 1,
        NameRole,
        FpsRole,
        DropCountRole,
        IsRecordingRole,
        StatusRole,
        AlertMessageRole,
        ResolutionOptionsRole,
        FramerateOptionsRole,
        FormatOptionsRole
    };

    explicit AppCameraModel(QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE QVariantMap get(int row) const;
    void replaceRows(std::vector<CameraRow> rows);
    void setRecording(bool recording);

private:
    std::vector<CameraRow> rows_;
};

} // namespace micecam::ui
```

Implement `AppCameraModel.cpp` with direct role mapping from `rows_`.

- [ ] **Step 6: Implement AppAlertModel**

Create `cmd/micecam_ui/AppAlertModel.h` with roles `severity`, `title`, `source`, `relTime`, and a `Q_PROPERTY(int badgeCount READ badgeCount NOTIFY badgeCountChanged)`.

Implement `replaceRows()`, `clear()`, and `badgeCount()` in `AppAlertModel.cpp`.

- [ ] **Step 7: Implement AppSettings**

Create `AppSettings` as a `QObject` with properties:

```cpp
Q_PROPERTY(int watchdogTimeout READ watchdogTimeout WRITE setWatchdogTimeout NOTIFY settingsChanged)
Q_PROPERTY(double yellowDropThreshold READ yellowDropThreshold WRITE setYellowDropThreshold NOTIFY settingsChanged)
Q_PROPERTY(double redDropThreshold READ redDropThreshold WRITE setRedDropThreshold NOTIFY settingsChanged)
Q_PROPERTY(QString webhookUrl READ webhookUrl WRITE setWebhookUrl NOTIFY settingsChanged)
Q_PROPERTY(int defaultBitrateKbps READ defaultBitrateKbps WRITE setDefaultBitrateKbps NOTIFY settingsChanged)
Q_PROPERTY(QString outputDirectory READ outputDirectory WRITE setOutputDirectory NOTIFY settingsChanged)
Q_PROPERTY(QString logLevel READ logLevel WRITE setLogLevel NOTIFY settingsChanged)
```

Back it with `infrastructure::ConfigLoader`.

- [ ] **Step 8: Implement AppController**

`AppController` responsibilities:

- Register backends in constructor.
- Mock mode registers `MockCameraBackend`.
- Production mode registers `OAKCameraBackend` and `FFmpegCameraBackend`.
- `refreshCameras()` flattens devices/streams into `CameraRow`.
- `startRecording()` runs preflight, starts `RecordingPipeline`, marks rows recording.
- `stopRecording()` finalizes pipeline and updates status properties.

Required public surface:

```cpp
enum class BackendMode { Production, MockOnly };
Q_ENUM(BackendMode)

Q_PROPERTY(AppCameraModel* cameraModel READ cameraModel CONSTANT)
Q_PROPERTY(AppAlertModel* alertModel READ alertModel CONSTANT)
Q_PROPERTY(AppSettings* settings READ settings CONSTANT)
Q_PROPERTY(bool isRecording READ isRecording NOTIFY recordingChanged)
Q_PROPERTY(QString recordButtonText READ recordButtonText NOTIFY recordingChanged)
Q_PROPERTY(QString cameraCountText READ cameraCountText NOTIFY camerasChanged)
Q_PROPERTY(QString elapsedText READ elapsedText NOTIFY statusChanged)
Q_PROPERTY(QString totalFramesText READ totalFramesText NOTIFY statusChanged)
Q_PROPERTY(QString averageFpsText READ averageFpsText NOTIFY statusChanged)
Q_PROPERTY(QString bytesWrittenText READ bytesWrittenText NOTIFY statusChanged)
Q_PROPERTY(QString diskRemainingText READ diskRemainingText NOTIFY statusChanged)
Q_PROPERTY(QString preflightMessage READ preflightMessage NOTIFY preflightChanged)
Q_PROPERTY(QString lastSessionId READ lastSessionId NOTIFY statusChanged)

Q_INVOKABLE void refreshCameras();
Q_INVOKABLE bool startRecording();
Q_INVOKABLE void stopRecording();
Q_INVOKABLE QVariantList preflightItems() const;
Q_INVOKABLE QVariantMap cameraAt(int row) const;
```

For this task, it is acceptable for `startRecording()` to start the pipeline and update state without a continuous capture loop. The continuous frame pump is added in Task 7.

- [ ] **Step 9: Run tests**

Run:

```bash
cmake --build build --target test_app_models test_app_controller micecam_ui -j
build/tests/test_app_models
build/tests/test_app_controller
```

Expected: pass.

- [ ] **Step 10: Commit**

```bash
git add CMakeLists.txt cmd/micecam_ui/CMakeLists.txt cmd/micecam_ui/AppCameraModel.h cmd/micecam_ui/AppCameraModel.cpp cmd/micecam_ui/AppAlertModel.h cmd/micecam_ui/AppAlertModel.cpp cmd/micecam_ui/AppSettings.h cmd/micecam_ui/AppSettings.cpp cmd/micecam_ui/AppController.h cmd/micecam_ui/AppController.cpp tests/unit/test_app_models.cpp tests/unit/test_app_controller.cpp
git commit -m "feat(ui): add qt backend adapter models"
```

---

## Task 7: Capture Pump for Mock/Real Streams

**Files:**
- Modify: `cmd/micecam_ui/AppController.h`
- Modify: `cmd/micecam_ui/AppController.cpp`
- Test: `tests/unit/test_app_controller.cpp`

- [ ] **Step 1: Add failing capture pump test**

Append to `tests/unit/test_app_controller.cpp`:

```cpp
TEST(AppController, RecordingPumpUpdatesFrameCounters) {
    micecam::ui::AppController controller(micecam::ui::BackendMode::MockOnly);
    controller.setOutputDirectory("/tmp/micecam_app_controller_pump");
    controller.refreshCameras();

    ASSERT_TRUE(controller.startRecording());
    QThread::msleep(600);
    controller.stopRecording();

    EXPECT_NE(controller.totalFramesText(), "0 frames");
    EXPECT_FALSE(controller.bytesWrittenText().isEmpty());
}
```

Add `#include <QThread>`.

- [ ] **Step 2: Run the test to verify it fails**

Run:

```bash
cmake --build build --target test_app_controller -j
build/tests/test_app_controller --gtest_filter=AppController.RecordingPumpUpdatesFrameCounters
```

Expected: assertion failure because no frames are pumped.

- [ ] **Step 3: Add capture thread ownership**

In `AppController.h`, add:

```cpp
struct ActiveStream {
    domain::StreamConfig config;
    std::unique_ptr<domain::CameraStream> stream;
};

std::atomic<bool> captureRunning_{false};
std::thread captureThread_;
std::vector<ActiveStream> activeStreams_;
void captureLoop();
void stopCaptureLoop();
```

- [ ] **Step 4: Open streams and pump frames**

In `startRecording()`, after `pipeline_.start(config)` succeeds:

```cpp
activeStreams_.clear();
for (const auto& stream_config : config.streams) {
    auto stream = cameraManager_.open_stream(stream_config);
    if (stream) {
        activeStreams_.push_back({stream_config, std::move(stream)});
    }
}
captureRunning_ = true;
captureThread_ = std::thread([this] { captureLoop(); });
```

Implement `captureLoop()`:

```cpp
void AppController::captureLoop() {
    while (captureRunning_) {
        for (auto& active : activeStreams_) {
            if (!active.stream || !active.stream->is_open()) {
                continue;
            }
            std::vector<uint8_t> bytes;
            int64_t pts = 0;
            if (!active.stream->read_frame(bytes, pts)) {
                continue;
            }
            pipeline::FrameData frame;
            frame.stream_id = active.config.device_id + "_" + std::to_string(active.config.stream_index);
            frame.data = bytes.data();
            frame.size = bytes.size();
            frame.width = active.stream->width();
            frame.height = active.stream->height();
            frame.pts = pts;
            frame.source_format = active.stream->pixel_format();
            pipeline_.push_frame(frame);
        }
        QMetaObject::invokeMethod(this, [this] { refreshLiveStatus(); }, Qt::QueuedConnection);
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
}
```

Implement `stopCaptureLoop()` and call it before `pipeline_.stop()`.

- [ ] **Step 5: Update live status from pipeline snapshots**

Add a `RecordingPipeline::snapshot()` method if Task 3 did not already expose one. Use it in `AppController::refreshLiveStatus()` to set total frames, fps, bytes, and camera row fps/drop counts.

- [ ] **Step 6: Run tests**

Run:

```bash
cmake --build build --target test_app_controller -j
build/tests/test_app_controller
```

Expected: pass.

- [ ] **Step 7: Commit**

```bash
git add cmd/micecam_ui/AppController.h cmd/micecam_ui/AppController.cpp tests/unit/test_app_controller.cpp
git commit -m "feat(ui): pump backend frames into recording pipeline"
```

---

## Task 8: Bind Existing QML to AppController Without Visual Polish

**Files:**
- Modify: `cmd/micecam_ui/main.cpp`
- Modify: `cmd/micecam_ui/qml/main.qml`
- Modify: `cmd/micecam_ui/qml/components/AppToolbar.qml`
- Modify: `cmd/micecam_ui/qml/components/AppSidebar.qml`
- Modify: `cmd/micecam_ui/qml/components/CameraGridView.qml`
- Modify: `cmd/micecam_ui/qml/components/AppStatusBar.qml`
- Modify: `cmd/micecam_ui/qml/components/PreflightModal.qml`
- Modify: `cmd/micecam_ui/qml/components/NotificationPopup.qml`

- [ ] **Step 1: Register controller in main.cpp**

Replace the `MockCameraModel` registration in `cmd/micecam_ui/main.cpp` with:

```cpp
#include "AppController.h"

int main(int argc, char* argv[]) {
    QQuickStyle::setStyle("Basic");
    QGuiApplication app(argc, argv);

    micecam::ui::AppController controller;
    controller.refreshCameras();

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("appController", &controller);
    engine.load(QUrl("qrc:/MiceCam/UI/qml/main.qml"));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
```

- [ ] **Step 2: Bind toolbar actions and text**

In `AppToolbar.qml`, add root properties:

```qml
property bool isRecording: false
property string recordText: isRecording ? "Stop" : "Record"
signal recordClicked()
```

Replace local `recordBtn.isRecording` usage with `root.isRecording` and `root.recordText`.

Change click handler:

```qml
onClicked: root.recordClicked()
```

- [ ] **Step 3: Wire main toolbar behavior**

In `main.qml`, set:

```qml
AppToolbar {
    id: toolbar
    isRecording: appController.isRecording
    recordText: appController.recordButtonText
    onRecordClicked: {
        if (appController.isRecording) {
            appController.stopRecording()
        } else if (!appController.startRecording()) {
            preflightModal.open()
        }
    }
}
```

Keep existing settings/fullscreen handlers.

- [ ] **Step 4: Bind sidebar model**

In `AppSidebar.qml`, replace:

```qml
model: CameraModel {}
```

with:

```qml
model: appController.cameraModel
```

Keep the existing delegate layout.

- [ ] **Step 5: Bind camera grid model**

In `CameraGridView.qml`, replace hardcoded card rows with a `Repeater` over `appController.cameraModel`. Preserve the current two-row visual proportions by using the existing `ColumnLayout` and cards inside a `Flow`:

```qml
Flow {
    Layout.fillWidth: true
    Layout.fillHeight: true
    spacing: 12
    Repeater {
        model: appController.cameraModel
        delegate: CameraCard {
            width: index < 2 ? (parent.width - 12) / 2 : (parent.width - 24) / 3
            height: index < 2 ? root.height / 2 - 22 : root.height / 2 - 22
            cameraName: model.name
            fps: model.fps
            drops: model.dropCount
            status: model.status
            isRecording: model.isRecording
            onContextFullscreen: root.cardFullscreen(cameraName, fps, drops, isRecording, status)
            onContextMenuRequested: function(gx, gy) {
                root.showContextMenu(cameraName, fps, drops, isRecording, status, gx, gy)
            }
        }
    }
}
```

This is a binding-only change. Do not alter card colors, fonts, or spacing tokens.

- [ ] **Step 6: Bind status bar text**

In `AppStatusBar.qml`, replace hardcoded labels with properties:

```qml
property string elapsedText: "00:00:00"
property string cameraCountText: "0 cameras"
property string totalFramesText: "0 frames"
property string averageFpsText: "0.00 fps avg"
property string bytesWrittenText: "0 B"
property string diskRemainingText: "Disk unknown"
property bool recording: false
```

Bind each `StatusSegment.labelText` to these properties.

In `main.qml`:

```qml
AppStatusBar {
    id: statusBar
    elapsedText: appController.elapsedText
    cameraCountText: appController.cameraCountText
    totalFramesText: appController.totalFramesText
    averageFpsText: appController.averageFpsText
    bytesWrittenText: appController.bytesWrittenText
    diskRemainingText: appController.diskRemainingText
    recording: appController.isRecording
}
```

- [ ] **Step 7: Bind preflight modal items**

Expose `property var items: []` in `PreflightModal.qml`. Replace fixed three failure rectangles with a `Repeater` over `root.items`. Each item uses existing rectangle styles based on `severity`.

In `main.qml`, before opening:

```qml
preflightModal.items = appController.preflightItems()
preflightModal.open()
```

- [ ] **Step 8: Bind notification popup model**

In `NotificationPopup.qml`, add:

```qml
property var alertModel: null
```

Set `ListView.model: root.alertModel`.

In `AppToolbar.qml`, pass alert model into popup:

```qml
property var alertModel: null
NotificationPopup {
    id: notifyPopup
    alertModel: root.alertModel
}
```

In `main.qml`, set:

```qml
alertModel: appController.alertModel
```

- [ ] **Step 9: Build and run smoke**

Run:

```bash
cmake --build build --target micecam_ui -j
build/cmd/micecam_ui/micecam_ui > /tmp/micecam_ui_wiring.log 2>&1 &
sleep 3
cat /tmp/micecam_ui_wiring.log
```

Expected: app launches with no QML errors.

- [ ] **Step 10: Commit**

```bash
git add cmd/micecam_ui/main.cpp cmd/micecam_ui/qml/main.qml cmd/micecam_ui/qml/components/AppToolbar.qml cmd/micecam_ui/qml/components/AppSidebar.qml cmd/micecam_ui/qml/components/CameraGridView.qml cmd/micecam_ui/qml/components/AppStatusBar.qml cmd/micecam_ui/qml/components/PreflightModal.qml cmd/micecam_ui/qml/components/NotificationPopup.qml
git commit -m "feat(ui): bind qml surfaces to backend controller"
```

---

## Task 9: Bind Camera Detail and Settings to Backend Options

**Files:**
- Modify: `cmd/micecam_ui/qml/main.qml`
- Modify: `cmd/micecam_ui/qml/components/CameraDetailView.qml`
- Modify: `cmd/micecam_ui/qml/components/EncodingSettings.qml`
- Modify: `cmd/micecam_ui/qml/components/AlertsSettings.qml`
- Modify: `cmd/micecam_ui/qml/components/LoggingSettings.qml`
- Modify: `cmd/micecam_ui/AppController.h`
- Modify: `cmd/micecam_ui/AppController.cpp`
- Modify: `cmd/micecam_ui/AppSettings.h`
- Modify: `cmd/micecam_ui/AppSettings.cpp`
- Test: `tests/unit/test_app_controller.cpp`

- [ ] **Step 1: Add failing selected camera test**

Append to `tests/unit/test_app_controller.cpp`:

```cpp
TEST(AppController, SelectedCameraExposesCapabilityOptions) {
    micecam::ui::AppController controller(micecam::ui::BackendMode::MockOnly);
    controller.refreshCameras();

    auto camera = controller.cameraAt(0);
    ASSERT_FALSE(camera.isEmpty());
    EXPECT_TRUE(camera["resolutionOptions"].toStringList().contains("1920 x 1080"));
    EXPECT_TRUE(camera["framerateOptions"].toStringList().contains("30 fps"));
    EXPECT_TRUE(camera["formatOptions"].toStringList().contains("rgb24"));
}
```

- [ ] **Step 2: Run the test to verify it fails**

Run:

```bash
cmake --build build --target test_app_controller -j
build/tests/test_app_controller --gtest_filter=AppController.SelectedCameraExposesCapabilityOptions
```

Expected: failure until `cameraAt()` returns option lists.

- [ ] **Step 3: Implement `cameraAt()` capability map**

In `AppController.cpp`, return:

```cpp
QVariantMap AppController::cameraAt(int row) const {
    return cameraModel_.get(row);
}
```

Ensure `AppCameraModel::get()` includes:

```cpp
map["resolutionOptions"] = row.resolutionLabels;
map["framerateOptions"] = row.framerateLabels;
map["formatOptions"] = row.formatLabels;
```

- [ ] **Step 4: Bind CameraDetailView option lists**

In `CameraDetailView.qml`, add:

```qml
property var resolutionOptionLabels: []
property var frameRateOptionLabels: []
property var formatOptionLabels: []
signal settingsChanged(string resolution, string frameRate, string pixelFormat)
```

Replace the static `ListModel` usage for the three ComboBoxes with these arrays. On activation, emit `settingsChanged(...)`.

- [ ] **Step 5: Update selected camera in main.qml**

When opening camera detail, fetch:

```qml
var camera = appController.cameraAt(row)
selectedCameraName = camera.name
selectedCameraFps = camera.fps
selectedCameraDrops = camera.dropCount
selectedCameraStatus = camera.status
selectedCameraRecording = camera.isRecording
selectedResolutionOptions = camera.resolutionOptions
selectedFrameRateOptions = camera.framerateOptions
selectedFormatOptions = camera.formatOptions
```

Pass those arrays into `CameraDetailView`.

- [ ] **Step 6: Bind settings controls**

In settings QML:

- Encoding bitrate slider reads/writes `appController.settings.defaultBitrateKbps`.
- Alerts watchdog reads/writes `appController.settings.watchdogTimeout`.
- Alerts thresholds read/write `yellowDropThreshold` and `redDropThreshold`.
- Alerts webhook reads/writes `webhookUrl`.
- Logging level reads/writes `logLevel`.

Keep component visuals unchanged.

- [ ] **Step 7: Run tests and UI build**

Run:

```bash
cmake --build build --target test_app_controller micecam_ui -j
build/tests/test_app_controller
```

Expected: pass.

- [ ] **Step 8: Commit**

```bash
git add cmd/micecam_ui/AppController.h cmd/micecam_ui/AppController.cpp cmd/micecam_ui/AppSettings.h cmd/micecam_ui/AppSettings.cpp cmd/micecam_ui/qml/main.qml cmd/micecam_ui/qml/components/CameraDetailView.qml cmd/micecam_ui/qml/components/EncodingSettings.qml cmd/micecam_ui/qml/components/AlertsSettings.qml cmd/micecam_ui/qml/components/LoggingSettings.qml tests/unit/test_app_controller.cpp
git commit -m "feat(ui): bind camera detail and settings to backend"
```

---

## Task 10: End-to-End Verification and Documentation

**Files:**
- Modify: `project_index`
- Create: `docs/wikis/v2-backend-ui-wiring.md`
- Create: `docs/reports/implements/phase-v2-backend-ui-wiring-05-15.md`

- [ ] **Step 1: Run full build**

Run:

```bash
cmake --build build -j
```

Expected: build exits 0. Warnings must be reviewed and either fixed or listed in the implementation report.

- [ ] **Step 2: Run full tests**

Run:

```bash
ctest --test-dir build --output-on-failure
```

Expected: 100% tests passed.

- [ ] **Step 3: Run UI smoke**

Run:

```bash
build/cmd/micecam_ui/micecam_ui > /tmp/micecam_ui_final_wiring.log 2>&1 &
echo $! > /tmp/micecam_ui_final_wiring.pid
sleep 3
cat /tmp/micecam_ui_final_wiring.log
kill "$(cat /tmp/micecam_ui_final_wiring.pid)"
```

Expected: no QML binding errors, no crash.

- [ ] **Step 4: Verify mock recording output**

Run a UI-triggered mock recording manually or through a Qt test helper. Verify output directory contains:

```text
<session_id>/
  mock_cam_0_0.mp4
  mock_cam_0_0.srt
  _meta.json
  _stats.json
```

Then verify:

```bash
ffprobe -v error -select_streams v:0 -show_entries stream=codec_name -of default=nk=1:nw=1 <path-to-mp4>
```

Expected:

```text
h264
```

- [ ] **Step 5: Update project index**

Add these entries under Current UI Stack:

```markdown
- Qt backend adapter/controller: `cmd/micecam_ui/AppController.h`
- Qt camera model: `cmd/micecam_ui/AppCameraModel.h`
- Qt alert model: `cmd/micecam_ui/AppAlertModel.h`
- Qt settings bridge: `cmd/micecam_ui/AppSettings.h`
```

- [ ] **Step 6: Write wiki**

Create `docs/wikis/v2-backend-ui-wiring.md`:

```markdown
# V2 Backend/UI Wiring

MiceCam v2 keeps QML as a rendering layer. Backend state reaches QML through `AppController`, `AppCameraModel`, `AppAlertModel`, and `AppSettings`.

## Data Flow

1. `CameraManager` discovers OAK, FFmpeg, or mock backends.
2. `AppController::refreshCameras()` flattens backend `DeviceInfo` and `StreamInfo` into `CameraRow`.
3. QML camera surfaces bind to `AppCameraModel`.
4. `AppController::startRecording()` runs detailed preflight and starts `RecordingPipeline`.
5. A capture thread reads `CameraStream` frames and pushes them into `RecordingPipeline`.
6. `RecordingPipeline` encodes raw frames through `TranscodeStage`, writes H264 MP4, SRT, metadata, and stats.
7. `AlertManager` stores alert history for `AppAlertModel`.

## Boundaries

QML must not call `CameraManager`, `RecordingPipeline`, or backend classes directly. It only calls `AppController` and binds to Qt models.

## Visual Rule

The wiring work does not redesign QML. It only replaces mock data and mock click handlers with backend bindings.
```

- [ ] **Step 7: Write implementation report**

Create `docs/reports/implements/phase-v2-backend-ui-wiring-05-15.md` with:

```markdown
# Implementation Report: V2 Backend/UI Wiring

## Summary

Wired the polished QML UI to v2 backend contracts through Qt adapter models and controller services.

## Changed Files

List each changed file and its responsibility.

## Verification

- `cmake --build build -j`
- `ctest --test-dir build --output-on-failure`
- `build/cmd/micecam_ui/micecam_ui` smoke
- `ffprobe` on mock recording output

## Known Issues

List only verified remaining issues. If none remain, write `None known from this verification pass.`

## Rollback

Revert the wiring commits in reverse order. Backend contract extensions are backward compatible with existing tests.
```

- [ ] **Step 8: Commit docs**

```bash
git add project_index docs/wikis/v2-backend-ui-wiring.md docs/reports/implements/phase-v2-backend-ui-wiring-05-15.md
git commit -m "docs: document v2 backend ui wiring"
```

---

## Final Verification Gate

Before claiming the implementation complete, run:

```bash
cmake --build build -j
ctest --test-dir build --output-on-failure
cmake --build build --target micecam_ui -j
```

Then launch smoke:

```bash
build/cmd/micecam_ui/micecam_ui > /tmp/micecam_ui_wiring_final.log 2>&1 &
echo $! > /tmp/micecam_ui_wiring_final.pid
sleep 3
cat /tmp/micecam_ui_wiring_final.log
kill "$(cat /tmp/micecam_ui_wiring_final.pid)"
```

Success requires:

- Build exits 0.
- CTest reports 100% pass.
- UI launch produces no QML errors.
- Mock recording produces H264 `.mp4`, `.srt`, `_meta.json`, and `_stats.json`.
- No QML file contains hardcoded session runtime values such as `00:42:17`, `76,230 frames`, fixed `5 cameras`, fixed alert list, or H.265 text.

## Scope Exclusions

This plan does not change the visual design. It also does not implement hardware-in-the-loop validation for real OAK-D hardware; it creates the backend contract needed for real hardware to populate the same UI models and keeps existing HIL tests as the hardware verification path.

## Self-Review

- Spec coverage: Covers v2 UI needs for discovery, capability selection, preflight, recording, stats, settings, alerts, and output validity.
- Placeholder scan: No undefined task placeholders remain.
- Type consistency: `CameraRow`, `AlertRow`, `AppCameraModel`, `AppAlertModel`, `AppSettings`, and `AppController` names are introduced before use in later tasks.
- Risk note: The riskiest dependency is `RecordingPipeline` output correctness. It is intentionally handled before QML binding.
