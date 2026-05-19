# Roadmap

## Stage 0: Product definition ✅

### Goal

Define what to build, for whom, and why. Establish product contract.

### Exit criteria

- [x] product.md complete
- [x] evidence.md passed (grill session completed with user)
- [x] user-journeys.md complete (8 user scenarios in spec.md)
- [x] ui-direction.md complete (ui-spec.md with Apple HIG)
- [x] value-proposition.md complete
- [x] roadmap.md complete
- [x] state.yaml initialized with readiness flags

---

## Stage 1: Feasibility validation

### Goal

Prove the two riskiest technical assumptions before committing to full build.

### Exit criteria

- [ ] Spike 1: OAK-D H264 hardware encoding — verify DepthAI VideoEncoder(H264) node produces valid H264 stream
- [ ] Spike 2: FFmpeg hardware encoder selection — verify VideoToolbox/NVENC/QSV fallback chain works
- [ ] spike-report.md produced with recommendation: continue
- [ ] feasibility_ready: true in state.yaml

---

## Stage 2: Foundation (Phase 1 from plan)

### Goal

Project compiles and links against all dependencies. Domain model and plugin interfaces defined.

### Exit criteria

- [ ] CMakeLists.txt rewritten with FFmpeg, Qt6, spdlog, nlohmann_json, depthai-core, GTest
- [ ] Domain types: DeviceInfo, StreamConfig, FrameTimestamp, SessionMetadata, StreamStats, AlertRecord
- [ ] Plugin interfaces: ICameraBackend, IDeviceEnumerator, WatchdogObserver
- [ ] Pipeline interfaces: IEncoder, IStreamWriter, IStatsCollector
- [ ] PluginRegistry, TimestampEngine
- [ ] cmake --build build succeeds on macOS
- [ ] ./micecam launches blank QML window

---

## Stage 3: Encoding pipeline (Phase 2 from plan)

### Goal

Prove H264 encoding works end-to-end with mock source. MP4 + SRT + metadata output.

### Exit criteria

- [ ] HardwareEncoderSelector with runtime detection + libx264 fallback
- [ ] FFmpegEncoder wraps avcodec_encode_video2
- [ ] TranscodeStage (passthrough + raw→H264)
- [ ] StreamWriter (MP4 via avformat)
- [ ] SRTWriter (per-frame timestamps)
- [ ] MetadataWriter (_meta.json + _stats.json)
- [ ] Mock frame source → .mp4 playable in VLC; .srt valid

---

## Stage 4: Camera backends (Phase 3 from plan)

### Goal

Real camera hardware producing frames into the pipeline.

### Exit criteria

- [ ] OAKCameraBackend: enumerate 4 cameras, H264 encode via DepthAI
- [ ] FFmpegCameraBackend: enumerate USB, open device, read frames
- [ ] CameraManager with PluginRegistry aggregation
- [ ] PreflightValidator: disk space, capability checks
- [ ] HardwareLock for OAK exclusive access

---

## Stage 5: Recording pipeline + Alerting (Phase 4+5 from plan)

### Goal

Full recording session with monitoring and alerting.

### Exit criteria

- [ ] RecordingPipeline orchestrates per-stream encode + write + stats
- [ ] StatsCollector: per-stream counters, latency, drop rate
- [ ] Watchdog: stall detection, Feishu webhook POST
- [ ] AlertManager: dedup, observer pattern
- [ ] Session lifecycle: start → record → stop → finalize → crash recovery

---

## Stage 6: Qt/QML UI (Phase 6 from plan)

### Goal

Complete desktop application matching ui-spec.md.

### Exit criteria

- [ ] ApplicationWindow with Toolbar + SplitView + StatusBar
- [ ] CameraGridView with adaptive grid layout
- [ ] Sidebar with camera list + status indicators
- [ ] Record/Stop controls with preflight validation flow
- [ ] Settings panel (encoding, alerts, logging)
- [ ] Alert banners, modals, fullscreen view
- [ ] Session history view

---

## Stage 7: Cross-platform CI/CD (Phase 7 from plan)

### Goal

Verified builds and tests on all target platforms.

### Exit criteria

- [ ] GitHub Actions matrix: macOS (arm64), Windows (MSVC), Linux (GCC+Clang)
- [ ] All unit tests green on all platforms
- [ ] Integration tests green on macOS
- [ ] CI artifacts produced per platform

---

## Stage 8: Integration + Hardware tests + Polish (Phase 8 from plan)

### Goal

10/10 success criteria from spec.md pass. Ready for dogfood.

### Exit criteria

- [ ] Mock integration: 5 streams 60s → all outputs valid
- [ ] Fault injection: disconnect, encoder fallback, disk full, stall → handled
- [ ] Hardware-in-loop: real OAK-D + USB, 1 hour, zero drops
- [ ] Performance profiling meets NFR targets
- [ ] All 10 SC from spec.md pass

---

## Stage 9: Dogfood

### Goal

Use the product in real experiments. Find and fix friction.

### Exit criteria

- [ ] At least 3 complete recording sessions with real animal subjects
- [ ] All found issues logged and resolved or deferred
- [ ] Performance acceptable on lab machines
- [ ] Data scientists can open and analyze output without assistance
