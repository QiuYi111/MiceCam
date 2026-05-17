# Feature Spec: Stream-Level Monitoring and Comprehensive Test Suite

## Metadata

| Field      | Value                                    |
|------------|------------------------------------------|
| Feature ID | `005-stream-monitoring-and-test-suite`   |
| Branch     | `feat/005-stream-monitoring-test-suite`  |
| Status     | Draft                                    |
| Owner      | `jingyi`                                 |
| Date       | `2026-05-17`                             |

## Summary

Close the monitoring and testing gaps revealed by the spec 004 grill session. Add per-stream liveness monitoring with a background watchdog thread that detects stalled streams and crashed plugins via ring buffer inactivity. Add a new `NotifyStreamStall` gRPC RPC so the main process can ask a plugin whether a stalled stream is recoverable. Fill the missing implementation gaps (crash detection trigger, keyframe_interval propagation to OpenStream, validator spec 004 fields). Build a comprehensive test suite in four tiers: unit (no hardware, CI all platforms), integration/in-process (no hardware, CI all platforms), integration/fork-e2e-no-hw (plugin binary, CI all platforms), and HIL (jingyi-lab only, real cameras). Update `validate_session_artifacts.py` to check all spec 004 output fields. The goal is maximum early problem exposure before merge to main.

## User Scenarios

### US-001: Per-Stream Stall Detection

**Priority**: P1

**Independent Test**: Start recording two streams from the same plugin. Simulate one stream's ring buffer going silent (no new frames for 5 seconds). Verify the main process detects the stall, calls `NotifyStreamStall` on the plugin, and logs a warning. Verify the other stream continues normally.

**Acceptance Scenarios**:
- Given recording is active with stream A and stream B, When stream A's ring buffer has no new frames for the configured timeout (default 5s), Then the main process detects stream A as stalled
- Given stream A is detected as stalled, When the main process calls `NotifyStreamStall(stream_id=A)`, Then the plugin receives the notification and responds with `acknowledged=true` and either `recoverable=true` or `recoverable=false`
- Given `NotifyStreamStall` returns `recoverable=false` with `action="device_lost"`, When the main process receives this, Then it finalizes stream A's output files and emits an alert, while stream B continues recording
- Given `NotifyStreamStall` returns `recoverable=true` with `action="retrying"`, When the main process receives this, Then it waits for the plugin to resume producing frames on stream A's ring buffer
- Given the gRPC call to `NotifyStreamStall` fails (UNAVAILABLE, DEADLINE_EXCEEDED), When the main process cannot reach the plugin, Then it treats this as a plugin crash and initiates plugin crash recovery

### US-002: Plugin Crash Detection via Stream Inactivity

**Priority**: P1

**Independent Test**: Start recording with one plugin producing two streams. Kill the plugin process. Verify that after the stall timeout, the main process detects all streams from that plugin as stalled, classifies this as a plugin crash, and executes the crash recovery sequence (finalize → shm cleanup → restart → reconnect).

**Acceptance Scenarios**:
- Given a plugin produces streams A and B, When the plugin process is killed, Then both stream A and stream B stop producing frames
- Given all streams from a plugin are stalled, When the main process detects this, Then it classifies the event as a plugin crash (not individual stream stalls)
- Given a plugin crash is detected via stream inactivity, When crash recovery executes, Then all streams from that plugin are finalized, shared memory is cleaned up, and the plugin is restarted
- Given a plugin crash is detected, When other plugins' streams are active, Then those streams continue recording without interruption

### US-003: NotifyStreamStall RPC Contract

**Priority**: P1

**Independent Test**: Call `NotifyStreamStall` on a running FFmpeg plugin server with a known stream_id. Verify the plugin responds with `acknowledged=true`. Call with an unknown stream_id. Verify the plugin responds with `acknowledged=false`.

**Acceptance Scenarios**:
- Given `NotifyStreamStallRequest` with a valid `stream_id` and `stall_duration_ms`, When the plugin receives it, Then it responds with `acknowledged=true` and a non-empty `action` string
- Given `NotifyStreamStallRequest` with an unknown `stream_id`, When the plugin receives it, Then it responds with `acknowledged=false`
- Given the plugin is a FFmpeg plugin, When `NotifyStreamStall` is called, Then the plugin checks the underlying capture device and responds with `recoverable=true` if the device is still present, or `recoverable=false` with `action="device_lost"` if the device has been disconnected

### US-004: keyframe_interval Propagation to OpenStream

**Priority**: P1

**Independent Test**: Run preflight calibration (mock or real) to compute `min_gop=15`. Verify that when `OpenStream` is called, the `StreamConfig.keyframe_interval` field is set to 15. Verify the plugin uses this value for GOP sizing.

**Acceptance Scenarios**:
- Given preflight Phase 1 computes `min_gop=15` for a stream, When `OpenStream` is called for that stream, Then `StreamConfig.keyframe_interval` is set to 15
- Given a plugin receives `StreamConfig.keyframe_interval=15`, When `StartStream` begins encoding, Then the plugin marks every 15th frame as a keyframe via `PayloadHeader.keyframe=true`

### US-005: Calibrate End-to-End with Real Encoder

**Priority**: P1

**Independent Test**: Fork the FFmpeg plugin server process. Call `Calibrate` with resolution 640x480, fps=30, `calibration_duration_ms=1000`. Verify the response contains non-zero `i_frame_latency_ns` and `p_frame_latency_ns` where I > P. Verify `max_sustainable_fps > 10`. Verify `recommended_slot_size > 0`. Compute `min_gop` from the returned latencies and verify it is >= 1.

**Acceptance Scenarios**:
- Given the FFmpeg plugin is running as a real process, When `Calibrate` is called with valid parameters, Then `i_frame_latency_ns > 0` and `p_frame_latency_ns > 0`
- Given calibration returns I and P latencies, When `i_frame_latency_ns` and `p_frame_latency_ns` are compared, Then `i_frame_latency_ns > p_frame_latency_ns` (I-frames are always slower)
- Given calibration completes, When `max_sustainable_fps` is checked, Then it is > 10 (reasonable lower bound)
- Given calibration completes, When `recommended_slot_size` is checked, Then it is > 0 and < 10MB (reasonable upper bound)
- Given calibration completes, When `actual_encoder_name` is checked, Then it is non-empty and contains either "nvenc", "videotoolbox", "qsv", or "libx264"

### US-006: Plugin E2E No-Hardware (Handshake → Calibrate → OpenStream → Shutdown)

**Priority**: P1

**Independent Test**: Fork the FFmpeg plugin server. Execute the full RPC lifecycle: Handshake → GetCapabilities → EnumerateDevices → Calibrate → OpenStream → Shutdown. Verify all RPCs succeed. This test runs on all platforms in CI without a camera.

**Acceptance Scenarios**:
- Given the plugin binary exists and is executable, When the test forks and launches it, Then the gRPC server starts and accepts connections within 5 seconds
- Given the plugin is running, When `Handshake` is called with `micecam_version="2.0.0"`, Then it returns `api_version=2` and `accepted=true`
- Given Handshake succeeds, When `Calibrate` is called with `calibration_duration_ms=500`, Then it returns `success=true` with valid latency fields
- Given Calibrate succeeds, When `OpenStream` is called with a valid device config, Then it returns a `StreamRingDescriptor` with valid ring_id and shared memory parameters
- Given OpenStream succeeds, When `Shutdown` is called, Then the plugin exits cleanly (exit code 0)
- Given the plugin exits, When shared memory is checked, Then all ring buffers created by the plugin are cleaned up

### US-007: HIL Multi-Device Full Recording with Artifact Validation

**Priority**: P1

**Independent Test**: On jingyi-lab with 2 USB cameras, run the complete recording flow: Preflight (Phase 1 Calibrate + Phase 2 parallel stress) → Record 30 seconds → Stop → Validate all output artifacts. Run `validate_session_artifacts.py --strict` on the output directory and verify all checks PASS.

**Acceptance Scenarios**:
- Given jingyi-lab has 2 USB cameras detected by the FFmpeg plugin, When `EnumerateDevices` is called, Then exactly 2 devices are returned
- Given 2 devices are available, When Preflight Phase 1 runs, Then Calibrate is called for each device and returns valid latencies
- Given Phase 1 passes, When Preflight Phase 2 runs, Then both streams start simultaneously, run for 3-5 seconds, and detect zero drops
- Given preflight passes, When recording runs for 30 seconds, Then the main process produces per-stream: `.mp4`, `.srt`, `_meta.json`, `_stats.json`
- Given recording completes, When `validate_session_artifacts.py --strict` is run, Then all checks PASS including spec 004 fields: `session_start_wall_time`, `keyframe_interval`, `i_frame_latency_ms`, `p_frame_latency_ms`, `crash_window_sec`, `actual_encoder_name`, `calibration_duration_ms`, `parallel_test_passed`, `requested_streams`
- Given SRT files are produced, When wall_time entries are checked, Then each entry contains `wall_time=<ISO 8601>` with correct format
- Given `_stats.json` is produced, When parsed, Then it is a JSON object keyed by stream ID (not a JSON array)

### US-008: HIL Plugin Crash Recovery via kill

**Priority**: P1

**Independent Test**: On jingyi-lab, start recording with the FFmpeg plugin on 2 cameras. After 10 seconds, `kill -9` the plugin process. Verify: (1) main process detects crash via stream inactivity timeout, (2) both streams' MP4 files are playable (fMP4 crash safety), (3) shared memory is cleaned up, (4) plugin is restarted, (5) reconnect file `_reconnect_1.mp4` is created, (6) `_meta.json` has `crash_recovery_wall_time`.

**Acceptance Scenarios**:
- Given recording is active with 2 streams, When the plugin process is killed with SIGKILL, Then both streams stop producing frames within the stall timeout
- Given the plugin crash is detected, When crash recovery executes, Then both pre-crash MP4 files are playable in ffprobe (valid codec, frames, duration)
- Given crash recovery cleans up shared memory, When `shm_open` is called on the old ring buffer names, Then they return ENOENT (segments unlinked)
- Given the plugin restart succeeds, When recording resumes, Then a new file `_reconnect_1.mp4` is created and receives frames
- Given crash recovery completes, When `_meta.json` for the reconnect file is checked, Then `crash_recovery_wall_time` is present in ISO 8601 format
- Given crash recovery completes, When the SRT file for the reconnect recording is checked, Then wall_time entries resume with a gap reflecting the crash duration

### US-009: Updated Artifact Validator

**Priority**: P1

**Independent Test**: Create a synthetic session directory with all spec 004 fields populated. Run `validate_session_artifacts.py --strict`. Verify all checks PASS. Remove `session_start_wall_time`. Verify the validator reports it as missing.

**Acceptance Scenarios**:
- Given `_meta.json` contains all spec 004 fields, When `--strict` validation runs, Then all checks PASS
- Given `_meta.json` is missing `session_start_wall_time`, When validation runs, Then it reports a FAIL for that field
- Given `_stats.json` is a JSON array instead of object, When validation runs, Then it reports a FAIL
- Given an SRT file contains `wall_time=<ISO 8601>` entries, When validation runs, Then wall_time format is verified as `YYYY-MM-DDTHH:MM:SS.ffffff`
- Given an SRT file has no wall_time entries, When `--strict` validation runs, Then it reports a WARN
- Given an MP4 is fragmented (fMP4), When validation runs, Then ffprobe reports valid codec, frames, and duration without errors

## Requirements

### Functional Requirements

- **FR-001**: A `NotifyStreamStall` RPC MUST be added to `camera_plugin.proto` with `NotifyStreamStallRequest` (stream_id, stall_duration_ms) and `NotifyStreamStallResponse` (acknowledged, recoverable, action, message). The `action` field values are: "retrying", "device_lost", "unknown".
- **FR-002**: The main process MUST maintain per-stream `last_active_time` timestamps. `PluginStreamConsumer` MUST update this timestamp each time a frame is successfully read from the ring buffer.
- **FR-003**: A background monitoring thread MUST periodically (every 1 second) check all active stream timestamps. If a stream's `last_active_time` exceeds the configurable stall timeout (default 5 seconds), the stream is classified as stalled.
- **FR-004**: When a stream is detected as stalled, the main process MUST call `NotifyStreamStall` on the plugin. If the gRPC call fails, it MUST be treated as a plugin crash.
- **FR-005**: If `NotifyStreamStall` returns `recoverable=false`, the main process MUST finalize the stalled stream's output files, clean up its ring buffer shared memory, and emit an alert. The plugin process is NOT restarted.
- **FR-006**: If `NotifyStreamStall` returns `recoverable=true`, the main process MUST wait for the plugin to resume producing frames. If the stall timeout is exceeded again, the main process MUST retry `NotifyStreamStall` once. If still recoverable but no frames arrive, escalate to stream finalization after 2x stall timeout.
- **FR-007**: If all streams from a single plugin are simultaneously stalled, the main process MUST classify this as a plugin crash and execute the full crash recovery sequence (finalize → shm cleanup → restart → reconnect).
- **FR-008**: Stream stall timeout MUST be configurable (default 5000ms). The monitoring check interval MUST be configurable (default 1000ms).
- **FR-009**: `PreflightValidator` MUST propagate the computed `keyframe_interval` from `CalibrationResult` to `RecordingPipeline`, and `RecordingPipeline` MUST pass it in the `StreamConfig` during the `OpenStream` gRPC call.
- **FR-010**: Both plugin servers (FFmpeg and OAK) MUST implement `NotifyStreamStall` RPC. The FFmpeg plugin MUST check whether the underlying capture device is still accessible before responding.
- **FR-011**: `validate_session_artifacts.py` MUST be updated to check spec 004 fields in `--strict` mode: `session_start_wall_time`, `keyframe_interval`, `i_frame_latency_ms`, `p_frame_latency_ms`, `crash_window_sec`, `actual_encoder_name`, `calibration_duration_ms`, `parallel_test_passed`, `requested_streams`.
- **FR-012**: `validate_session_artifacts.py` MUST verify SRT `wall_time=<ISO 8601>` format in `--strict` mode.
- **FR-013**: `validate_session_artifacts.py` MUST verify `_stats.json` is a JSON object keyed by stream ID (not a JSON array).
- **FR-014**: A test `test_plugin_e2e_no_hw` MUST be created that forks the real plugin binary, executes Handshake → Calibrate → OpenStream → Shutdown via gRPC, and verifies all RPCs succeed. This test MUST run on all CI platforms without hardware.
- **FR-015**: A test `test_hil_e2e` MUST be created that runs on jingyi-lab only, executes the full recording lifecycle with 2 real cameras (Preflight → Record → Stop → Validate), and verifies all output artifacts pass `validate_session_artifacts.py --strict`.
- **FR-016**: A test `test_hil_crash_recovery` MUST be created that runs on jingyi-lab only, kills the plugin process during recording, and verifies crash recovery produces valid output files and reconnect recordings.
- **FR-017**: An integration test MUST verify the Calibrate RPC with a real FFmpegEncoder produces non-zero I/P latencies where I > P, reasonable max_sustainable_fps, and valid recommended_slot_size.
- **FR-018**: An integration test MUST verify dual-path encoding (H264 passthrough + RAW fallback) with mixed streams produces correct output: keyframe positions in MP4 match the configured keyframe_interval, SRT entries have monotonically increasing wall_time.
- **FR-019**: An integration test MUST verify Preflight Phase 2 with real plugin processes running streams for 2+ devices in parallel detects drops and reports warnings.
- **FR-020**: All existing tests (37+ ctest + Python validator) MUST continue to pass after all changes.

### Non-Functional Requirements

- **NFR-001**: Performance - The stream monitoring thread's per-check overhead MUST be less than 1ms (no blocking operations in the check loop).
- **NFR-002**: Performance - `NotifyStreamStall` RPC MUST complete in under 2 seconds (plugin device check timeout).
- **NFR-003**: Reliability - The monitoring thread MUST NOT deadlock or crash if a plugin is unresponsive. All gRPC calls MUST have deadlines.
- **NFR-004**: Observability - All stream stall detection, notification, recovery, and finalization events MUST be logged via spdlog at INFO level or higher.
- **NFR-005**: Testability - Stream monitoring MUST be testable with mock time (injectable clock) for deterministic unit tests. Crash recovery MUST be testable with fault injection (kill subprocess).

## Success Criteria

| #    | Criterion | Measured By |
|------|-----------|-------------|
| SC-1 | `NotifyStreamStall` RPC defined in proto and implemented in both plugins | Proto file contains RPC; both plugin tests pass |
| SC-2 | Per-stream liveness monitoring detects stalled streams within timeout | Unit test with mock time: injectable clock advances past threshold → stall detected |
| SC-3 | Plugin crash detected when all streams stall | Integration test: kill plugin → all streams timeout → crash recovery triggers |
| SC-4 | keyframe_interval propagated from Calibrate through OpenStream | E2E test: Calibrate returns min_gop → OpenStream receives matching keyframe_interval |
| SC-5 | Calibrate with real encoder returns valid latencies (I > P, both > 0) | Fork e2e test on macOS + HIL on jingyi-lab |
| SC-6 | Fork e2e no-hw test passes on macOS, Windows, Linux CI | Green check on all 3 CI matrix jobs |
| SC-7 | HIL multi-device full recording passes with artifact validation | jingyi-lab: 2 cameras, 30s, validate_session_artifacts.py --strict → all PASS |
| SC-8 | HIL crash recovery produces valid pre-crash files + reconnect file | jingyi-lab: kill plugin → fMP4 playable → reconnect file created |
| SC-9 | validate_session_artifacts.py covers all spec 004 fields | Unit test: synthetic data with all fields → PASS; missing field → FAIL |
| SC-10 | All existing 37+ ctest + Python tests continue to pass | `ctest --test-dir build --output-on-failure` + `python3 scripts/validate_session_artifacts.py` |
| SC-11 | Dual-path encoding produces correct keyframe positions | Integration test: 60 frames, keyframe_interval=15 → IDR at frames 0, 15, 30, 45 |
| SC-12 | Preflight Phase 2 detects drops with real multi-device parallel streams | HIL test: 2 devices, verify zero drops in 3-5s stress test |

## Assumptions

- Spec 004 Phase 1-7 code is present on the `plugin-system` branch and all 37 tests pass.
- The FFmpeg plugin binary can be located in the build tree for fork-based tests (CMake target `micecam_ffmpeg` produces the executable).
- `shm_unlink` on macOS and Linux behaves identically (POSIX semantics).
- jingyi-lab has exactly 2 USB cameras accessible to the FFmpeg plugin via AVFoundation/Video4Linux.
- The stream monitoring background thread does not need to survive `SIGSTOP` of the main process (out of scope).
- `NotifyStreamStall` is a best-effort notification; if the plugin is in an undefined state, the gRPC call will fail and be treated as a crash.
- OAK plugin `NotifyStreamStall` implementation returns `acknowledged=false` (no hardware to check).

## Clarifications

None — all design decisions resolved during the grill session with the user.

## Out of Scope

- **OAK hardware validation** — OAK plugin implements NotifyStreamStall but returns `acknowledged=false`; testing deferred to hardware availability.
- **UI testing** — All testing is backend/logic only. UI visual verification remains manual with user sign-off.
- **Windows/Linux HIL** — HIL tests run only on jingyi-lab (Ubuntu). Windows CI covers build + unit tests only.
- **Plugin process sandboxing/signing** — Trusted local code assumption per spec 004.
- **Audio recording** — Video only per spec 001.
- **Network streaming** — Local-only per product definition.
- **Concurrent multi-user recording** — Single user, single machine per spec 001.
- **Automatic file cleanup or archival** — User manages all recorded data.
- **Disk space monitoring during recording** — Preflight checks disk space before recording; runtime monitoring is out of scope.
- **Thermal throttling detection** — Calibrate measures instantaneous performance; long-running degradation is covered by runtime overflow monitoring (FR-015 from spec 004).
- **Rate limiting on NotifyStreamStall calls** — One call per stall event; no repeated polling.
- **Per-stream reconnect** — Reconnect files are only created when the entire plugin restarts. Individual stream stalls that are finalized do not produce reconnect files.

## Risk Notes

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Stream monitoring thread deadlock under heavy load | Low | High | Non-blocking check loop; injectable clock for testing; deadlock detection in CI |
| NotifyStreamStall gRPC call hangs if plugin is in undefined state | Medium | Medium | All gRPC calls have 2s deadline; timeout treated as plugin crash |
| Fork-based tests flaky on CI (process cleanup, port conflicts) | Medium | Medium | Random port assignment; SIGTERM + SIGKILL + waitpid in teardown; retry logic |
| jingyi-lab camera availability during test runs | Medium | Medium | HIL tests gated by BUILD_HIL flag; non-HIL tests don't require cameras |
| Shared memory leaks if test crashes before cleanup | Low | Medium | Test fixtures use RAII shm_unlink; CI runs cleanup script |
| Calibrate returns misleading latencies on CI runners (shared hardware, no GPU) | Medium | Low | CI Calibrate test uses libx264 fallback; validates structure not absolute values |
| fMP4 fragment structure varies across FFmpeg versions on different platforms | Low | Medium | CI runs fMP4 smoke test on all platforms; validator uses ffprobe which handles version differences |

## Implementation Order

1. Proto: add `NotifyStreamStall` RPC to `camera_plugin.proto`
2. Domain: add stream monitor types (`StreamLivenessMonitor`, `StreamLivenessState`)
3. Infrastructure: implement background monitoring thread with per-stream timestamps
4. Infrastructure: wire `NotifyStreamStall` call into stall detection flow
5. Infrastructure: wire crash detection trigger (all streams stalled → plugin crash)
6. Infrastructure: propagate `keyframe_interval` from PreflightValidator through RecordingPipeline to OpenStream
7. Plugin: implement `NotifyStreamStall` in FFmpeg and OAK plugin servers
8. Tests: unit tests for stream monitor, NotifyStreamStall, keyframe_interval propagation
9. Tests: integration tests for Calibrate e2e, dual-path keyframe verification
10. Tests: fork e2e no-hw test (Handshake → Calibrate → OpenStream → Shutdown)
11. Scripts: update `validate_session_artifacts.py` with spec 004 fields
12. HIL: multi-device full recording test on jingyi-lab
13. HIL: crash recovery kill test on jingyi-lab
14. CI: verify fork e2e no-hw test runs on all 3 platforms
