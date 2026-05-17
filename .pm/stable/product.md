# Product: MiceCam v2

## One-sentence summary

Cross-platform native desktop application for multi-camera laboratory animal monitoring with hardware-accelerated H264 recording and per-frame timestamps.

## Problem

The current MiceCam (v1) stores raw MJPEG/UYVY frames in custom `.bin` format at 120MB/s on macOS, causing frame drops. Python/C++ hybrid architecture requires duplicate maintenance across two runtimes. Frame timestamps use `system_clock::now()` with no hardware PTS integration, making them unreliable for scientific analysis.

## Target users

- **Laboratory researchers** running behavioral experiments on mice/rats in multi-camera setups (OAK-D 4-camera spatial AI device + USB cameras)
- **Data scientists** analyzing recorded video with frame-precise timestamps for behavioral scoring and movement tracking

## Jobs to be done

1. Start recording all cameras with one click, stop with confidence that all data is saved
2. Monitor live camera feeds during experiments to verify animal activity
3. Retrieve frame-precise timestamped video for analysis in standard tools (FFmpeg, Python/OpenCV)
4. Receive immediate alerts if recording fails (camera disconnect, disk full, pipeline stall) via Feishu

## Core use cases

1. Multi-camera recording: OAK-D 4 streams + USB cameras → simultaneous H264 `.mp4` output
2. Live monitoring: adaptive grid preview during recording with real-time frame stats
3. Post-session analysis: open `.mp4` in standard player, `.srt` for frame timestamps, `_stats.json` for session metrics
4. Alerting: watchdog monitors pipeline health, pushes alerts to Feishu webhook

## Non-goals

- Remote viewing / streaming over network
- Thermal/FLIR camera support (plugin architecture supports future, not v2)
- Cloud storage or NAS integration
- Audio recording
- Multi-user access control
- Automatic file cleanup/rotation

## Product pillars

1. **Data integrity** — zero silent failures; every frame accounted for in `.srt`; every alert surfaced
2. **Performance** — hardware-accelerated encoding; 5+ streams at 1080p30 with < 1% frame drop
3. **Simplicity** — one-click record; clean Apple HIG UI; standard output formats
4. **Observability** — per-session stats JSON; watchdog with webhook; spdlog structured logging
5. **Extensibility** — plugin camera backend system; new camera types without touching pipeline core

## MVP boundary

### Included

- Qt/QML desktop UI (macOS + Windows + Linux)
- OAK-D 4-camera backend (DepthAI H264 hardware encoding)
- FFmpeg USB camera backend (hardware-accelerated H264 encoding)
- Per-stream `.mp4` + `.srt` + `_meta.json` + `_stats.json` output
- Timestamp system: wall clock anchor + steady_clock per-frame
- Watchdog with Feishu webhook alerting
- Preflight validation (disk space, camera capability check)
- spdlog structured logging

### Excluded

- Python bindings (pybind11)
- Legacy Python UI (PyQt)
- Legacy `.bin` storage format
- HDF5 export
- GPU MJPEG decoder
- OpenCV USB backend

## Success criteria

See `specs/001-micecam-v2-rewrite/spec.md` Success Criteria section — 10 measurable criteria including zero-drop 1-hour recording, `.mp4` validity, timestamp precision < 1ms, CI green on 3 platforms.

## Anti-goals

- Over-engineering the plugin system before validating with 2 real backends (OAK + FFmpeg)
- Premature optimization of UI animations before functionality works
- Repeating the Python/C++ split that created maintenance debt in v1
- Building for hypothetical users before satisfied lab researchers

## Open questions

- [x] OAK H264 encoder maturity → spike first
- [x] Cross-platform FFmpeg hardware encoder consistency → spike + CI matrix
- [x] Timestamp precision with mixed hardware/no-hardware PTS → steady_clock fallback strategy defined
