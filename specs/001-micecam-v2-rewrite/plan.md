# Implementation Plan: MiceCam v2 Rewrite

## Inputs

| Source | Reference |
|---|---|
| Spec | `specs/001-micecam-v2-rewrite/spec.md` |
| UI Spec | `specs/001-micecam-v2-rewrite/ui-spec.md` |
| Related Contracts | None (greenfield) |

## Technical Context

| Dimension | Value |
|---|---|
| Language | C++20 |
| Framework | Qt 6.6+ (QML), CMake 3.20+, vcpkg |
| Encoding | FFmpeg 6.0+ (libavcodec, libavformat, libavdevice) |
| Camera | DepthAI SDK (depthai-core), FFmpeg libavdevice |
| Logging | spdlog 1.x (header-only) |
| Testing | GTest (C++), Qt Test |
| Storage | Local filesystem (`.mp4` + `.srt` + `.json`) |
| External Dependencies | FFmpeg, depthai-core, Qt6, spdlog, nlohmann_json |

## Architecture Impact

### DDD Layer Impact

| Layer | Change |
|---|---|
| `internal/domain/` | Complete replacement: `CameraStream`, `DeviceInfo`, `FrameTimestamp`, `SessionMetadata`, `StreamStats`, `AlertRecord`, `EncoderConfig`, `PluginRegistry` |
| `internal/infrastructure/` | Complete replacement: `FFmpegEncoder`, `HardwareEncoderSelector`, `DiskWriter`, `SRTWriter`, `MetadataWriter`, `StatsCollector`, `Watchdog`, `FeishuWebhook`, `SpdlogManager`, camera backends (`OAKBackend`, `FFmpegCameraBackend`) |
| `internal/pipeline/` | Complete replacement: `RecordingPipeline`, `TranscodeStage`, `PreflightValidator`, `TimestampEngine` |
| `api/` | New: `ICameraBackend`(plugin interface), `IDeviceEnumerator`(plugin interface), `CameraPluginDescriptor` |
| `cmd/` | Complete replacement: Qt/QML application replacing both `cmd/micecam_ui/` and `cmd/gui/` |
| `bindings/python/` | **Removed** — no Python bindings in v2 |
| `legacy/` | **Removed** — all legacy code deleted |

### Contract Impact

- **New**: `ICameraBackend` interface — `enumerate() -> vector<DeviceInfo>`, `open(DeviceInfo, StreamConfig) -> CameraStream`, `close()`, `getCapabilities() -> Capabilities`
- **New**: `IDeviceEnumerator` interface — `enumerate() -> vector<DeviceInfo>`
- **New**: `WatchdogObserver` interface — `onAlert(AlertRecord)`
- **Removed**: `PyPipeline` pybind11 bindings, `IFrameObserver` callback pattern
- **Removed**: JSON stdin/stdout IPC protocol between UI and worker process

### Data Model Impact

- **New**: `CameraStream` — owns per-stream H264 encoder, feeds into transcoding stage
- **New**: `SessionMetadata` (JSON schema) — wall_clock_anchor, camera_configs[], encoding_config, session_id
- **New**: `StreamStats` (JSON schema) — per-stream frame_count, drop_count, encode_latency_us[], timing_deviation_us
- **New**: `AlertRecord` — timestamp, severity, stream_id, alert_type, message
- **Removed**: `.bin` file format, JSONL metadata stream, `FrameInfo` (old), `HDF5ConversionConfig`

## Blast Radius Classification

| Field | Value |
|---|---|
| Level | **infra** |
| Reason | Complete rewrite of all layers: domain model, infrastructure, API contracts, UI, build system. Touches CMakeLists.txt (build infra), removes Python bindings, replaces storage format, changes CI/CD requirements |
| Required Gates | dry_run, explicit_human_approval, rollback_plan, security_review |

[REQUIRES HUMAN REVIEW] — complete greenfield rewrite; all architectural decisions require explicit approval before implementation begins.

## Constitution Check

| Check | Pass | Notes |
|---|---|---|
| Contract-first | Yes | `ICameraBackend` and `IDeviceEnumerator` interfaces defined before any backend implementation |
| DDD direction | Yes | Domain layer defines `CameraStream`, `DeviceInfo`, `SessionMetadata`; infrastructure implements backends, encoders, writers; domain never references infrastructure |
| TDD/BDD | Yes | RED-GREEN-REFACTOR per task; integration tests for camera discovery → recording → output pipeline |
| Observability | Yes | spdlog with async sink, `_stats.json` per session, watchdog with Feishu webhook, per-frame timing |
| Security | Yes | Local-only app; no network exposure except outbound Feishu webhook (user-configurable URL, HTTPS only); no secrets in code or logs |

## Implementation Strategy

Phase order: Foundation → Core Pipeline → Camera Backends → UI → Polish.

### Phase 1: Foundation (Build System + Domain Model + Interfaces)

**Rationale**: No code compiles without CMake. Domain model and plugin interfaces are the contracts every other phase depends on.

**Steps**:
1. Rewrite `CMakeLists.txt` — FFmpeg finder, vcpkg manifest for Qt6+spdlog+nlohmann_json+depthai-core, cross-platform encoder detection, GTest setup, `.clang-format` preserved
2. Define domain types in `internal/domain/` — `DeviceInfo`, `StreamConfig`, `FrameTimestamp`, `SessionMetadata`, `StreamStats`, `AlertRecord`, `EncoderConfig`, `Capabilities`, `PluginDescriptor`
3. Define plugin interfaces in `api/` — `ICameraBackend`, `IDeviceEnumerator`, `WatchdogObserver`
4. Define internal pipeline interfaces — `IEncoder`, `IStreamWriter`, `IStatsCollector`
5. `PluginRegistry` — scans registered backends, manages lifecycle, implements camera discovery aggregation
6. `TimestampEngine` — captures system_clock anchor at session start; accepts per-frame steady_clock timestamp; combines with optional hardware PTS; outputs session-relative microsecond offset
7. Skeleton `main.cpp` with Qt application bootstrap, empty QML window, spdlog initialization
8. Verify: `cmake --build build` succeeds on macOS; `./micecam` launches blank window; no crashes

### Phase 2: Encoding Infrastructure

**Rationale**: H264 encoding is the performance-critical core; all camera backends depend on it.

**Steps**:
1. `HardwareEncoderSelector` — runtime detection: VideoToolbox (macOS), NVENC (NVIDIA), QSV (Intel), VAAPI (Linux), AMF (AMD); fallback to libx264
2. `FFmpegEncoder` — wraps FFmpeg `avcodec_encode_video2`, handles `AVFrame` input, outputs `AVPacket` H264 NAL units; configurable bitrate, CRF, keyframe interval; buffer management with `AVCodecContext`
3. `TranscodeStage` — accepts frames from any source (raw UYVY422, MJPEG, pre-encoded H264 from OAK); if already H264: passthrough; if raw: decode → hardware encode → H264; unified H264 output stream
4. `StreamWriter` — writes H264 `AVPacket`s to `.mp4` via `avformat_write_header` / `av_interleaved_write_frame` / `av_write_trailer`; one writer per stream
5. `SRTWriter` — generates SRT subtitle track from per-frame timestamps; SRT format: index, start_time --> end_time, session_offset_us; appended per-frame
6. `MetadataWriter` — writes `_meta.json` (session header on start, session footer on stop, full JSON valid at any time); `_stats.json` (per-stream counters, latency histogram, alert log)
7. Verify: mock frame source → encode → `.mp4` file playable in VLC; `.srt` embedded; `_meta.json` valid JSON

### Phase 3: Camera Backends

**Rationale**: Backends are the input; without them, pipeline is untestable with real hardware.

**Steps**:
1. `OAKCameraBackend` — implements `ICameraBackend` + `IDeviceEnumerator`; uses DepthAI SDK to discover device, enumerate 4 cameras (CAM_A..CAM_D); configures `VideoEncoder(H264)` node per camera; extracts `getTimestampDevice()` for hardware PTS; outputs pre-encoded H264 frames as `CameraStream`
2. `FFmpegCameraBackend` — implements `ICameraBackend` + `IDeviceEnumerator`; uses `avdevice` to enumerate video devices (AVFoundation on macOS, dshow on Windows, v4l2 on Linux); opens device with format negotiation; reads `AVPacket` via `av_read_frame`; outputs raw frames (UYVY422/MJPEG) for TranscodeStage
3. `CameraManager` — owns PluginRegistry; manages active camera list; handles hot-plug detection (poll or platform events); routes camera connect/disconnect to pipeline
4. `PreflightValidator` — before recording starts: checks disk space (estimate bitrate × expected duration × stream_count × 1.2 safety); checks each camera resolution/framerate against reported Capabilities; checks encoder availability
5. `HardwareLock` — OAK device exclusive lock (filesystem lockfile or DepthAI API check); second instance detects lock and fails gracefully
6. Verify: OAK enumeration on macOS with real device; USB camera enumeration on macOS via AVFoundation; mock frame throughput test (5 streams × 1080p30 → encode → write)

### Phase 4: Recording Pipeline

**Rationale**: Orchestrates all phases into a coherent recording session.

**Steps**:
1. `RecordingPipeline` — owns per-stream `TranscodeStage` + `StreamWriter` + `SRTWriter` + `StatsCollector`; start(StreamConfigs) → begin session → run → stop → finalize; per-frame loop: read CameraStream → TimestampEngine → TranscodeStage → StreamWriter + SRTWriter + StatsCollector
2. `StatsCollector` — per-stream counters (frames_expected based on duration × fps, frames_actual), drop rate = (expected - actual) / expected; encode latency per frame (timer around FFmpegEncoder::encode); timing deviation (abs(frame_interval - expected_interval)); alert tracking (threshold crossings)
3. `Watchdog` — separate thread, waits on condition_variable; pipeline feeds dog via `feed()` every frame; if timeout (default 3s) expires without feed, Watchdog fires `onAlert(ALERT_PIPELINE_STALL)`; continues monitoring after alert
4. Session lifecycle — start: MetadataWriter writes session header, TimestampEngine captures wall anchor; during: loop as above; stop: finalize all writers, MetadataWriter writes session footer with totals; crash recovery: on unexpected exit, MetadataWriter writes partial footer on next launch detecting incomplete session
5. Verify: mock 5-stream source → full pipeline → 60s recording → 5 `.mp4` + 5 `.srt` + `_meta.json` + `_stats.json`; verify frame counts match; verify timestamps monotonic; ffprobe validates `.mp4` codec/container

### Phase 5: Alerting System

**Rationale**: Alerting is the observability contract; decoupled from pipeline via observer pattern.

**Steps**:
1. `AlertManager` — singleton, holds list of `AlertObserver`; `emit(AlertRecord)` → notify all observers; alert dedup (suppress repeated alerts of same type within cooldown window)
2. `FeishuWebhook` — implements `AlertObserver`; POST JSON payload to configured webhook URL; payload: `{"alert_type": "...", "severity": "yellow|red", "session_id": "...", "stream_id": "...", "timestamp": ..., "message": "..."}`; connection timeout 5s; retry 1× on failure; user-agent: `MiceCam/2.0`
3. Threshold configurability — read from config file (JSON or QSettings): watchdog_timeout_s, drop_rate_yellow_pct, drop_rate_red_pct, encode_latency_ratio_yellow, encode_latency_ratio_red, alert_cooldown_s
4. Verify: inject stall → Feishu POST received; duplicate suppression (fire same alert twice in cooldown → only first POSTs); no webhook config → alert logged, no crash

### Phase 6: Qt/QML UI

**Rationale**: UI is the user-facing component; implemented last to validate against fully functional pipeline.

**Steps**:
1. QML application shell — `ApplicationWindow` with Toolbar, SplitView (Sidebar + MainContentArea), StatusBar; theme engine: color tokens, font tokens, spacing tokens, shadow tokens all via Qt `qtquickcontrols2.conf` + singleton QML object
2. CameraGridView — `Repeater` over camera model; adaptive grid layout (1→full, 2→side-by-side, 3→1+2, 4→2×2, 5+→responsive); card component (16:9 aspect, rounded corners, light shadow, overlay bar); preview via `VideoOutput` + `ImageProvider` (decoded keyframe for preview)
3. Sidebar — `ListView` of `CameraListItem` (icon, name, status dot); click-to-scroll behavior; resizable (minimum 180pt, default 240pt)
4. Toolbar + RecordButton — context-sensitive: idle → "Record" (red accent), recording → "Stop" (filled dark); settings gear; alert bell with badge count; fullscreen toggle
5. StatusBar — 1Hz update timer; displays elapsed time (QElapsedTimer), total frames, average fps, total file size (human-readable), disk gauge (percentage bar)
6. Settings panel — replace sidebar content: Encoding section (bitrate slider 3-10, keyframe interval stepper); Alerts section (webhook URL field, watchdog timeout stepper, threshold sliders); Logging section (level picker, open log folder button); About section (version, dependencies)
7. Modals — PreflightFailure (icon, message, adjust button); StopConfirmation (stop & save / continue); EncoderError (error detail, fallback notification); CloseWhileRecording (stop & exit / continue recording)
8. Fullscreen view — double-click card → expand with zoom transition → thumbnail strip at bottom → double-click/Escape to exit
9. Alert banners — slide-in from top, color-coded, auto-dismiss 5s or click-to-dismiss; stack max 3 visible
10. Verify: launch with mock camera model → grid renders correct layout; click Record → cards show recording indicator, status bar updates; click Stop → confirmation modal; settings persist between launches; resize window → grid reflows

### Phase 7: Cross-Platform CI/CD

**Rationale**: Must compile and test on all three target platforms.

**Steps**:
1. CMake cross-platform hardening — `if(APPLE)` VideoToolbox, `if(WIN32)` dshow + NVENC detect, `if(LINUX)` VAAPI; vcpkg triplets for each platform; FFmpeg finder with required components list
2. GitHub Actions matrix — `ubuntu-24.04` (GCC 14 + Clang 18), `macos-14` (arm64 Apple Silicon), `windows-2022` (MSVC 2022); each: vcpkg install → cmake configure → cmake build → ctest
3. CI artifacts — compiled binary per platform; test results XML; compiler warnings log
4. Hardware-in-loop — separate workflow/runner on Linux machine with OAK-D attached; runs integration tests from Phase 8 against real hardware; not blocking (soft requirement for CI, hard requirement for release)
5. Verify: PR triggers CI matrix; all three platforms green; ctest reports 100% pass

### Phase 8: Integration and Hardware Tests

**Rationale**: Full-system validation against success criteria.

**Steps**:
1. Mock camera source — `MockCameraBackend` implementing `ICameraBackend`; generates synthetic frames (color bars + sequence number overlay) at configurable framerate; injects controlled frame drops, corruption, or disconnect for failure testing
2. End-to-end test: 5 mock streams 1080p30 60s → `RecordingPipeline` → verify 5 `.mp4` valid, frame counts match, timestamps monotonic, `_stats.json` accurate
3. Fault injection tests — simulated camera disconnect mid-session, encoder fallback, disk full, watchdog stall; verify alert emitted, `.mp4` finalized, remaining streams continue
4. Hardware-in-loop — real OAK-D 4 cameras + 1 USB camera, 1 hour recording, verify SC-1 (zero drops), SC-2 (`.mp4` valid), SC-3 (timing < 1ms), SC-4 (disconnect recovery), SC-5 (webhook fires)
5. Performance profiling — benchmark per-frame encode latency, memory usage, disk throughput against NFR targets
6. Verify: all 10 Success Criteria from spec.md pass

## Test Strategy

### Unit Tests

| Module | Test File | What It Verifies |
|---|---|---|
| `TimestampEngine` | `tests/unit/timestamp_engine_test.cpp` | wall anchor → steady offset conversion; monotonic guarantee; hardware PTS correction; boundary: 0 offset, max int64 |
| `FFmpegEncoder` | `tests/unit/ffmpeg_encoder_test.cpp` | H264 encode from raw AVFrame; output AVPacket validity; bitrate config; keyframe interval; error: null frame, bad dimensions |
| `TranscodeStage` | `tests/unit/transcode_stage_test.cpp` | passthrough (H264→H264), raw→H264 conversion; format negotiation; error: unsupported input format |
| `StreamWriter` | `tests/unit/stream_writer_test.cpp` | MP4 header validity; AVPacket interleave; trailer finalization; file on disk playable |
| `SRTWriter` | `tests/unit/srt_writer_test.cpp` | SRT format correctness; timestamp mapping; error: duplicate sequence, gap |
| `MetadataWriter` | `tests/unit/metadata_writer_test.cpp` | JSON schema compliance; header/footer write; partial write recovery |
| `HardwareEncoderSelector` | `tests/unit/encoder_selector_test.cpp` | correct encoder for platform; fallback when primary unavailable; encoder name in stats |
| `Watchdog` | `tests/unit/watchdog_test.cpp` | feed → no alert; timeout → alert fires; post-alert recovery; configurable timeout |
| `StatsCollector` | `tests/unit/stats_collector_test.cpp` | counter accuracy; drop rate calc; latency stats (min/max/mean/p99); threshold crossing detection |
| `PreflightValidator` | `tests/unit/preflight_validator_test.cpp` | disk space check; resolution/fps support check; pass/fail result |
| `AlertManager` | `tests/unit/alert_manager_test.cpp` | emit → observer notified; dedup within cooldown; multiple observers |
| `PluginRegistry` | `tests/unit/plugin_registry_test.cpp` | register/enumerate; aggregate discovery; disabled backend handling |
| `OAKCameraBackend` | `tests/unit/oak_backend_test.cpp` | enumerate returns 4 cameras; device info populated; open/close lifecycle |
| `FFmpegCameraBackend` | `tests/unit/ffmpeg_camera_backend_test.cpp` | device enumeration; format list; open with config |

### Integration Tests

| Test | What It Verifies |
|---|---|
| `CameraDiscoveryIntegration` | Mock OAK + Mock FFmpeg backends → aggregate enumeration → 5 DeviceInfo entries |
| `FullPipelineIntegration` | Mock 5 streams 1080p30 → full RecordingPipeline → 60s → verify all output files, timestamps, stats |
| `FaultRecoveryIntegration` | Inject disconnect on stream 3 mid-recording → verify stream 3 finalizes, streams 1/2/4/5 continue, alert fires |
| `EncoderFallbackIntegration` | Disable hardware encoder → verify libx264 used → output valid MP4 → stats show fallback flag |
| `WatchdogEndToEnd` | Start recording → stall pipeline thread → watchdog fires → Feishu webhook called (mock HTTP server) |
| `MultiInstanceLock` | Instance 1 grabs OAK → instance 2 attempts → verify instance 2 fails with clear message |

### Edge Cases

- Empty camera list → recording blocked, empty state UI
- 1 camera only → grid 1×1, no other cards
- Maximum cameras (test with 16 mock cameras) → grid 4×4, no performance degradation
- 0fps camera (simulated stall) → watchdog should fire
- Very high framerate (120fps) → verify encoding keeps up or gracefully throttles
- Min/Max bitrate (1Mbps / 50Mbps) → verify encoder accepts, output valid
- Session duration 0 seconds → no files written
- Session duration exceeding disk space → preflight should catch before start
- Camera hot-unplug during preflight → race condition handled gracefully
- Webhook URL unreachable → timeout, retry once, log failure, no crash
- Unicode camera names → correctly written in JSON metadata and SRT
- Power-of-2 and non-power-of-2 resolutions (e.g., 1920×1080, 1280×960)

## Rollback Plan

This is a complete rewrite. Rollback strategy: **maintain v1 branch as fallback**.

1. **Pre-rewrite**: Tag current `main` as `v1-final`; create `v2-rewrite` branch for all new work
2. **During development**: v1 remains on `main` branch; critical v1 bugfixes can be cherry-picked to `main` without blocking v2
3. **If v2 fails**: Delete `v2-rewrite` branch; `main` is untouched; zero rollback cost
4. **If v2 partially fails**: Individual phases are independently revertible:
   - Phase 1 (foundation): revert CMake commit, restore old `CMakeLists.txt`
   - Phase 3 (backends): revert backend commits, keep domain interfaces
   - Phase 6 (UI): revert QML commits, keep pipeline functional (testable headless)
5. **v2 release**: Merge `v2-rewrite` into `main`; old v1 code archived in `legacy/` (already exists); final v1 binary preserved as `micecam-v1-legacy`

## Complexity Tracking

| Field | Value |
|---|---|
| Estimated | **High** |
| Rationale | Complete greenfield rewrite across all DDD layers. New build system, new domain model, new infrastructure, new UI framework, new storage format, new test suite. 8 phases with inter-dependencies. Cross-platform support adds multiplicative complexity. Hardware-dependent tests require physical device access. Estimated 4-6 weeks for single developer. |

## File Change Summary

### New Files (estimate ~60 files)

```
CMakeLists.txt                              (rewritten)
api/micecam/ICameraBackend.h                (new)
api/micecam/IDeviceEnumerator.h             (new)
api/micecam/WatchdogObserver.h              (new)
api/micecam/PluginDescriptor.h              (new)
internal/domain/DeviceInfo.h                (new)
internal/domain/StreamConfig.h              (new)
internal/domain/FrameTimestamp.h            (new)
internal/domain/SessionMetadata.h           (new)
internal/domain/StreamStats.h               (new)
internal/domain/AlertRecord.h               (new)
internal/domain/EncoderConfig.h             (new)
internal/domain/Capabilities.h              (new)
internal/domain/PluginRegistry.h            (new)
internal/domain/TimestampEngine.h           (new)
internal/infrastructure/FFmpegEncoder.h      (new)
internal/infrastructure/FFmpegEncoder.cpp    (new)
internal/infrastructure/HardwareEncoderSelector.h (new)
internal/infrastructure/HardwareEncoderSelector.cpp (new)
internal/infrastructure/StreamWriter.h      (new)
internal/infrastructure/StreamWriter.cpp    (new)
internal/infrastructure/SRTWriter.h         (new)
internal/infrastructure/SRTWriter.cpp       (new)
internal/infrastructure/MetadataWriter.h    (new)
internal/infrastructure/MetadataWriter.cpp  (new)
internal/infrastructure/OAKCameraBackend.h  (new)
internal/infrastructure/OAKCameraBackend.cpp (new)
internal/infrastructure/FFmpegCameraBackend.h (new)
internal/infrastructure/FFmpegCameraBackend.cpp (new)
internal/infrastructure/CameraManager.h     (new)
internal/infrastructure/CameraManager.cpp   (new)
internal/infrastructure/AlertManager.h      (new)
internal/infrastructure/AlertManager.cpp    (new)
internal/infrastructure/FeishuWebhook.h     (new)
internal/infrastructure/FeishuWebhook.cpp   (new)
internal/infrastructure/Watchdog.h          (new)
internal/infrastructure/Watchdog.cpp        (new)
internal/infrastructure/ConfigLoader.h      (new)
internal/infrastructure/ConfigLoader.cpp    (new)
internal/pipeline/RecordingPipeline.h       (new)
internal/pipeline/RecordingPipeline.cpp     (new)
internal/pipeline/TranscodeStage.h          (new)
internal/pipeline/TranscodeStage.cpp        (new)
internal/pipeline/StatsCollector.h          (new)
internal/pipeline/StatsCollector.cpp        (new)
internal/pipeline/PreflightValidator.h      (new)
internal/pipeline/PreflightValidator.cpp    (new)
cmd/micecam/main.cpp                        (rewritten)
cmd/micecam/qml/main.qml                    (new)
cmd/micecam/qml/Theme.qml                   (new)
cmd/micecam/qml/CameraGridView.qml          (new)
cmd/micecam/qml/CameraCard.qml              (new)
cmd/micecam/qml/Sidebar.qml                 (new)
cmd/micecam/qml/StatusBar.qml               (new)
cmd/micecam/qml/Toolbar.qml                 (new)
cmd/micecam/qml/SettingsPanel.qml           (new)
cmd/micecam/qml/AlertBanner.qml             (new)
cmd/micecam/qml/modals/*.qml                (new, ~4 files)
cmd/micecam/ApplicationController.h         (new)
cmd/micecam/ApplicationController.cpp       (new)
cmd/micecam/CameraListModel.h               (new)
cmd/micecam/CameraListModel.cpp             (new)
tests/unit/*.cpp                            (new, ~14 files)
tests/integration/*.cpp                     (new, ~6 files)
tests/mock/MockCameraBackend.h              (new)
tests/mock/MockCameraBackend.cpp            (new)
```

### Removed Files

```
cmd/gui/                                    (entire directory — legacy Python UI)
bindings/python/                            (entire directory — pybind11 bindings)
internal/infrastructure/hdf5_converter.cpp  (stub)
api/micecam/pipeline/hdf5_converter.h       (stub)
api/micecam/gpu/gpu_jpeg_decoder.h          (header-only design, unused)
internal/infrastructure/usb_camera_backend.cpp (OpenCV-based, compiled out)
internal/domain/frame.cpp                   (empty stub)
internal/domain/ring_buffer.*               (replaced by per-stream encoder buffers)
cmd/micecam_ui/WorkerProcessRuntime.*       (Python subprocess obsolete)
cmd/micecam_ui/RecordingSupervisorService.* (replaced by RecordingPipeline)
cmd/micecam_ui/PipelineController.*         (replaced by ApplicationController)
cmd/micecam/main.cpp                        (stub that prints version)
```
