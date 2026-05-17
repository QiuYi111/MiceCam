# Feature Spec: MiceCam v2 Rewrite

## Metadata

| Field     | Value                          |
|-----------|--------------------------------|
| Feature ID | `001-micecam-v2-rewrite`       |
| Branch    | `feat/v2-rewrite`              |
| Status    | Draft                          |
| Owner     | `jingyi`                       |
| Date      | `2026-05-13`                   |

## Summary

Complete rewrite of MiceCam into a cross-platform Qt/QML desktop application. Replaces the legacy Python/C++ hybrid architecture with a unified C++ pipeline centered on hardware-accelerated H264 video recording with per-frame timestamps. Introduces a plugin-based camera backend system, unified monitoring via spdlog logging and watchdog-based alerting, and Apple HIG-compliant UI design. Outputs standard `.mp4` + `.srt` + session metadata for direct consumption by data scientists.

## User Scenarios

### US-001: Record Multi-Camera Session

**Priority**: P1

**Independent Test**: Connect OAK-D 4-camera device and 1 USB camera. Launch application. Verify all 5 cameras appear in preview grid. Click Record. After 60 seconds, click Stop. Verify output directory contains 5 `.mp4` files, 5 `.srt` timestamp tracks, `_meta.json`, and `_stats.json`. Play back any `.mp4` in VLC/FFmpeg and confirm SRT timestamps align with video frames.

**Acceptance Scenarios**:
- Given OAK-D with 4 cameras and 1 USB camera connected, When user clicks Record, Then all 5 streams begin recording simultaneously with synchronized wall-clock anchors
- Given a recording in progress, When camera CAM_C disconnects mid-session, Then CAM_C `.mp4` is finalized with valid header, remaining 4 streams continue uninterrupted, and UI displays red alert
- Given CAM_C reconnects after disconnect, When recording is still active, Then a new `_reconnect_N.mp4` file is created and recording resumes for that stream
- Given any camera stream, When frame is corrupted or unreadable, Then that frame is skipped, `.srt` marks `skipped: true`, and session `_stats.json` counts it
- Given 4 OAK streams recording at 1080p30, When session runs for 1 hour, Then final stats show drop_rate < 0.001 (one frame dropped per 1000)

### US-002: Camera Plugin Discovery and Configuration

**Priority**: P1

**Independent Test**: Launch application with no cameras connected. Verify empty state message appears. Connect OAK-D — verify 4 streams appear in grid within 3 seconds. Connect USB camera — verify stream appears. Disconnect all — verify grid clears.

**Acceptance Scenarios**:
- Given no cameras connected, When application starts, Then UI displays empty state message with "No cameras detected" and a link to settings
- Given OAK-D connected, When application scans backends, Then DepthAI plugin discovers 4 cameras (CAM_A through CAM_D) with model, serial, and supported resolutions
- Given USB camera connected, When application scans, Then FFmpeg backend discovers the device with vendor/product name, supported formats, and resolutions
- Given a camera backend fails to initialize (e.g., DepthAI runtime missing), When application starts, Then that backend is disabled silently, other backends continue to function, and a log entry is recorded

### US-003: Hardware-Accelerated H264 Encoding

**Priority**: P1

**Independent Test**: Start recording on macOS (Apple Silicon). Verify `_stats.json` reports `encoder: h264_videotoolbox`. On Linux with NVIDIA, verify `encoder: h264_nvenc`. On Intel iGPU, verify `encoder: h264_qsv`. Verify output `.mp4` is valid H264 (ffprobe confirms codec).

**Acceptance Scenarios**:
- Given macOS platform, When recording starts, Then VideoToolbox hardware encoder is selected and confirmed in stats
- Given hardware encoder fails to initialize, When recording starts, Then fallback to libx264 software encoding is used and stats record `encoder_fallback: true`
- Given OAK-D hardware, When recording starts, Then OAK's internal H264 hardware encoder (DepthAI VideoEncoder node) is used for all 4 OAK streams, bypassing host CPU entirely
- Given any active encoder, When average encode time exceeds frame_interval/2, Then system emits Yellow alert; when it exceeds frame_interval, Then Red alert fires

### US-004: Frame-Level Timestamps

**Priority**: P1

**Independent Test**: Record 60s session. Parse `_meta.json` for wall clock anchor. Parse `.srt` for per-frame timestamps. Verify all timestamps are monotonically increasing with steady_clock intervals. Verify no timestamp drift > 1ms from expected frame interval.

**Acceptance Scenarios**:
- Given any recording session, When session starts, Then a single wall-clock anchor (`system_clock::now()`) is captured and written to `_meta.json`
- Given active recording, When each frame arrives, Then `steady_clock::now()` is captured as frame timestamp and stored as session-relative microsecond offset
- Given OAK-D cameras, When DepthAI hardware PTS is available, Then hardware PTS is used to correct frame interval precision; steady_clock is used as fallback
- Given 1080p30 recording for 60 seconds, When session ends, Then max frame interval deviation from expected 33333us is less than 1ms (measured by max/min deviation in `_stats.json`)

### US-005: Watchdog and Alerting

**Priority**: P1

**Independent Test**: Start recording. Kill the writer thread (simulate stall). Verify within 3 seconds, watchdog fires and sends webhook to Feishu. Verify session stats record the alert.

**Acceptance Scenarios**:
- Given active recording, When pipeline loop feeds watchdog at configurable interval, Then watchdog remains satisfied and no alert fires
- Given active recording, When pipeline loop fails to feed watchdog for > 3 seconds (configurable), Then Red alert fires: status bar shows "Pipeline stalled," Feishu webhook is POSTed with session ID and timestamp
- Given Feishu webhook is configured, When any alert fires (camera disconnect, high drop rate, encoder fallback, disk full, pipeline stall), Then webhook payload includes alert type, severity, session ID, stream identifier, and Unix timestamp
- Given Feishu webhook URL is not configured, When alert fires, Then alert is logged locally but no HTTP request is attempted

### US-006: Session Observability and Statistics

**Priority**: P2

**Independent Test**: Record 60s session. Open `_stats.json`. Verify file contains per-stream data: expected/actual frames, drop rate, encoder name, per-frame timing stats, disk bytes written, and alert log. Verify JSON is valid and all numeric fields are present.

**Acceptance Scenarios**:
- Given any recording session, When session ends, Then `_stats.json` is written to output directory containing per-stream timing statistics (min/max/mean frame interval, encode latency distribution)
- Given a session with 0 frame drops, When session ends, Then stats show `drop_rate: 0.0000` for each stream
- Given a session that was terminated by user, When Stop is clicked, Then stats and all output files are finalized with complete metadata

### US-007: Preflight Resource Check

**Priority**: P2

**Independent Test**: Set output directory to a nearly-full disk. Click Record. Verify warning message appears. Free up space. Click Record. Verify recording starts normally.

**Acceptance Scenarios**:
- Given output directory selected, When user clicks Record, Then preflight checks available disk space against estimated session size (bitrate × expected duration × stream count) and warns if < 2x headroom
- Given selected camera configuration, When preflight runs, Then system checks that all requested resolutions and frame rates are supported by each camera backend
- Given preflight passes all checks, When user clicks Record, Then recording starts within 1 second
- Given preflight fails any check, When user clicks Record, Then recording is blocked and specific failure reason is displayed

### US-008: Logging Infrastructure

**Priority**: P2

**Independent Test**: Enable debug logging. Start/stop recording. Verify spdlog outputs structured logs with timestamps, log level, and module identifier to both console and file sink.

**Acceptance Scenarios**:
- Given spdlog configured with async sink, When any log message is emitted at DEBUG level or higher, Then message is timestamped, tagged with module name and thread ID
- Given runtime log level toggle, When log level is changed from INFO to TRACE, Then previously suppressed TRACE messages immediately begin appearing without restart
- Given a crash or unexpected exit, When application terminates, Then crash log is written to disk with final buffer contents (spdlog flush_on crash handler)

## Requirements

### Functional Requirements

- **FR-001**: Plugin camera backend system with `IDeviceEnumerator` interface — each backend implements `enumerate() -> vector<DeviceInfo>` independent of others
- **FR-002**: Unified `CameraStream` abstraction — each stream provides frames to the pipeline; OAK delivers pre-encoded H264 from hardware; USB delivers raw frames for host-side hardware encoding
- **FR-003**: Transcoding stage before disk write — ensures all output streams are H264 regardless of source format; FFmpeg hardware encoder selection (VideoToolbox, NVENC, QSV, VAAPI, AMF) with libx264 software fallback
- **FR-004**: Multi-stream MP4 output — each camera stream produces one `.mp4` file; OAK 4 streams produce 4 separate files; metadata per stream
- **FR-005**: SRT subtitle track per `.mp4` — each frame entry with session-relative microsecond offset, sequence number, and skipped/corrupt flag
- **FR-006**: Session metadata JSON — wall clock anchor, camera parameters, stream list, recording duration, encoding configuration
- **FR-007**: Session statistics JSON — per-stream frame counts, drop rates, encode latencies, timing deviations, disk metrics, alert log
- **FR-008**: Timestamp system — `system_clock::now()` as session wall-clock anchor; `steady_clock::now()` per-frame for monotonic interval measurement; OAK hardware PTS where available for interval correction
- **FR-009**: Watchdog mechanism — configurable timeout (default 3s); triggers Feishu webhook POST on timeout; continues monitoring after alert
- **FR-010**: Alert types — camera disconnect, camera reconnect, high drop rate (>0.1% yellow, >1% red), encode stall, encoder fallback, disk full, pipeline stall
- **FR-011**: Preflight validation — disk space estimate, resolution/framerate support check per backend, encoder availability check
- **FR-012**: spdlog async logging — per-module loggers, runtime level toggle, file + console sinks, crash-safe flush
- **FR-013**: Fault recovery — camera disconnect: finalize .mp4 for affected stream, continue other streams; camera reconnect: new .mp4 file; corrupted frame: skip and mark in .srt
- **FR-014**: Hardware resource lock — OAK device exclusively locked by first instance; subsequent instances detect lock and fail with clear message
- **FR-015**: Cross-platform build — CMake + vcpkg toolchain; macOS (arm64/x86_64), Windows (x86_64 MSVC), Linux (x86_64 GCC/Clang)

### Non-Functional Requirements

- **NFR-001**: Performance — 4× OAK 1080p30 + 1× USB 1080p30 simultaneously encoding to H264 with < 1% frame drop rate on target hardware (M1 Max, RTX 3090, i7-13700K)
- **NFR-002**: Encoding latency — per-frame encode time < 50% of frame interval for all streams (e.g., < 16.5ms for 30fps)
- **NFR-003**: Timestamp precision — steady_clock frame interval measurement with < 1ms max deviation from expected at 30fps
- **NFR-004**: Startup time — application cold start to camera grid visible < 5 seconds on target hardware
- **NFR-005**: UI responsiveness — preview grid updates at display refresh rate; recording start/stop responds within 500ms
- **NFR-006**: Memory — idle application < 200MB; recording 5 streams < 1GB (buffer + encoder overhead)
- **NFR-007**: Reliability — 1 hour continuous recording with 0 unrecoverable errors; graceful degradation on individual stream failure
- **NFR-008**: SRT accuracy — `.srt` frame timestamps correctly map to video frames within ±1 frame tolerance
- **NFR-009**: Disk throughput — maximum write throughput < 25MB/s for all streams combined at 10Mbps bitrate

## Success Criteria

| #  | Criterion | Measured By |
|----|-----------|-------------|
| SC-1 | OAK 4路 + USB 1路 1080p30 录制1小时零丢帧 | `_stats.json` drop_rate = 0 for all streams |
| SC-2 | 输出 `.mp4` 可被 FFmpeg/VLC 直接播放 | ffprobe validates codec and container |
| SC-3 | `.srt` 时间戳帧间隔误差 < 1ms | Parse `.srt`, compute interval deviation |
| SC-4 | OAK 断连后其他3路继续录制不中断 | Manual disconnect test, verify 3 files complete |
| SC-5 | 看门狗3秒不喂 → 飞书推送到达 | Inject stall, verify webhook receipt within 5s |
| SC-6 | 三平台(GitHub Actions)编译通过 + 测试绿 | CI matrix: macOS, Windows, Linux |
| SC-7 | 硬件编码器 fallback 到 libx264 正常工作 | Disable hardware encoder, verify libx264 used |
| SC-8 | 预检: 磁盘不足时阻止录制并提示 | Set output to small tmpfs, verify block |
| SC-9 | Apple HIG 合规: SF字体、SF Symbols、系统级观感 | Visual review against HIG checklist |
| SC-10 | 多实例互斥: OAK被占用时第二实例报错 | Two instances, one grabs OAK, second fails cleanly |

## Assumptions

- All target machines have FFmpeg with libavcodec/libavformat/libavdevice installed or statically linked
- DepthAI SDK (depthai-core) supports H264 hardware encoding via VideoEncoder node on OAK-D
- Target machines have hardware GPU encoder available (Apple Media Engine / NVENC / QSV / VAAPI)
- OAK-D device provides 4 synchronized camera streams (CAM_A through CAM_D) at configurable resolution/framerate
- Feishu bot webhook URL is pre-configured by user; application does not provision the bot
- All cameras are visible light (no thermal/FLIR in initial scope)
- Output directory is local storage; network storage not tested or guaranteed
- Single-user desktop application (local Qt process, no remote access or multi-user support)
- Operating system provides monotonic steady_clock with microsecond resolution
- High-bitrate H264 (5-10 Mbps per stream) is acceptable for scientific quality

## Clarifications

None — all design decisions resolved during grill session.

## Out of Scope

- **Remote viewing / streaming** — local desktop application only; no network preview or remote access
- **Thermal/FLIR camera support** — plugin architecture supports it in future, but not in v2 initial release
- **Python bindings** — pybind11 layer and all `bindings/python/` code is removed
- **Legacy Python UI** — `cmd/gui/` and all PyQt code deleted
- **Legacy `.bin` + `.jsonl` format** — no backward compatibility; v2 uses `.mp4` only
- **HDF5 export** — stub converter removed; not needed with `.mp4` output
- **GPU MJPEG decoder** — removed; H264 pipeline makes it unnecessary
- **USB camera backend via OpenCV** — removed; FFmpeg backend handles all USB camera scenarios
- **Cloud storage or NAS integration** — output is always local filesystem
- **Multi-machine synchronization** — no external sync signals (genlock, PTP, NTP)
- **Audio recording** — video only; no audio track in `.mp4`
- **User authentication or access control** — single-user local app, no login
- **Automatic file cleanup or rotation** — user manages disk space manually
- **Data encryption at rest** — standard filesystem; no application-level encryption

## Risk Notes

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| OAK H264 encoder quirks (DepthAI VideoEncoder node maturity) | Medium | High | Early prototype to validate H264 output; fallback to OAK MJPEG + host transcode if H264 node is buggy |
| Cross-platform FFmpeg hardware encoder inconsistencies | Medium | Medium | CI test matrix covers all 3 platforms; runtime encoder availability check with graceful fallback to libx264 |
| Timestamp precision gaps (OAK hardware PTS not reliable, steady_clock jitter on some platforms) | Medium | High | Validate with 1-hour recording on each target platform; compare `.srt` intervals against expected; if unacceptable, investigate platform-specific monotonic clock APIs |
| Silent frame drops in OAK pipeline (undetectable at application level) | Low | High | OAK hardware side: validate by feeding known frame rate test pattern; compare expected vs actual frame count |
| AVPacket::pts unreliable on some USB camera drivers | Medium | Medium | If pts jitter exceeds threshold, fall back completely to steady_clock timestamps for that stream; log encoder fallback event |
| macOS AVFoundation format negotiation issues (regression from v1) | Low | High | Use libavdevice format override to request MJPEG explicitly; if forced to UYVY422, hardware encode handles the bandwidth |
| Windows dshow camera name resolution issues (regression from v1) | Medium | Medium | Use FFmpeg dshow device enumeration with known working name formats; add camera name sanitization learned from v1 |
