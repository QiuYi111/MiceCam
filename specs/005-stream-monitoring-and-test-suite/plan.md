# Implementation Plan: Stream-Level Monitoring and Comprehensive Test Suite

## Inputs

| Source            | Reference                                                      |
|-------------------|----------------------------------------------------------------|
| Spec              | `specs/005-stream-monitoring-and-test-suite/spec.md`          |
| PRD               | N/A (derived from spec 004 grill session findings)            |
| Related Contracts | `api/micecam/camera_plugin.proto` (add `NotifyStreamStall`)   |

## Technical Context

| Dimension             | Value                                                    |
|-----------------------|----------------------------------------------------------|
| Language              | C++20, Python 3.10+                                      |
| Framework             | gRPC/protobuf, GTest, CMake                              |
| Storage               | POSIX shared memory (ring buffers), filesystem (MP4/SRT) |
| External Dependencies | FFmpeg (libavcodec/libavformat), gRPC, protobuf, spdlog  |

## Architecture Impact

### DDD Layer Impact

| Layer             | Change                                                                                                      |
|-------------------|-------------------------------------------------------------------------------------------------------------|
| `domain/`         | New types: `StreamLivenessState` (enum), `StreamStallEvent` (struct). No changes to existing domain types. |
| `infrastructure/` | New `StreamLivenessMonitor` class with background thread. Modify `PluginStreamConsumer` to update timestamps. Wire `NotifyStreamStall` call into `PluginRegistryService`. |
| `api/`            | Add `NotifyStreamStall` RPC + messages to `camera_plugin.proto`.                                            |
| `cmd/`            | Implement `NotifyStreamStall` in `FFmpegPluginServer` and `OAKPluginServer`. Propagate `keyframe_interval` in AppController wiring. |
| `pipeline/`       | Propagate `CalibrationResult.min_gop` as `StreamConfig.keyframe_interval` through RecordingPipeline to OpenStream. |
| `scripts/`        | Update `validate_session_artifacts.py` with spec 004 fields.                                                |

### Contract Impact

- **`camera_plugin.proto`**: Add `NotifyStreamStall` RPC to `CameraPluginService` service. Add `NotifyStreamStallRequest` message (`stream_id: string`, `stall_duration_ms: int32`). Add `NotifyStreamStallResponse` message (`acknowledged: bool`, `recoverable: bool`, `action: string`, `message: string`).
- **`StreamLivenessMonitor`** (new interface): `start()`, `stop()`, `register_stream(stream_id, plugin_id)`, `unregister_stream(stream_id)`, `update_activity(stream_id)`, `set_stall_callback(fn)`, `set_all_stalled_callback(fn)`. Injectable clock for testing.
- **No existing interface changes** — all modifications are additive.

### Data Model Impact

- **`_meta.json`**: No structural change (spec 004 fields already written by existing code). Validator gains awareness.
- **`_stats.json`**: No structural change. Already written as `map<stream_id, StreamStats>` by `MetadataWriter::write_stats`.
- **New in-memory state**: `unordered_map<string, SteadyClock::time_point> last_active_time_` per `StreamLivenessMonitor`.

## Blast Radius Classification

| Field          | Value                                                                                                     |
|----------------|-----------------------------------------------------------------------------------------------------------|
| Level          | **core**                                                                                                  |
| Reason         | Modifies the plugin protocol (`camera_plugin.proto`) — adding an RPC. Changes crash recovery behavior in `PluginRegistryService`. Touches domain + infrastructure + api + cmd + pipeline layers. |
| Required Gates | human_spec_review, architecture_review, rollback_plan, security_review, all existing 37+ tests pass       |

**[REQUIRES HUMAN REVIEW]** — Plugin protocol change and crash recovery logic modification require human sign-off.

## Constitution Check

| Check          | Pass | Notes                                                                                     |
|----------------|------|-------------------------------------------------------------------------------------------|
| Contract-first | Yes  | NotifyStreamStall RPC defined in proto before implementation. New StreamLivenessMonitor interface defined in header before .cpp. |
| DDD direction  | Yes  | New domain types (`StreamLivenessState`, `StreamStallEvent`) in `domain/`. Monitor in `infrastructure/`. No upward dependency from domain to infrastructure. |
| TDD/BDD        | Yes  | Each phase has explicit test requirements (FR-014 through FR-019). Unit tests with mock time; integration tests with fork; HIL with real hardware. |
| Observability  | Yes  | NFR-004: all stall/recovery events logged via spdlog at INFO+. Per-stream stall events visible in logs. |
| Security       | Yes  | Plugin untrusted: NotifyStreamStall calls have 2s gRPC deadline. Failure → crash recovery. No new attack surface (local-only gRPC on ephemeral port). |

## Implementation Strategy

Build bottom-up: proto → domain → infrastructure → pipeline wiring → plugin servers → tests → scripts → HIL.

### Phase 1: Proto + Domain Types (Foundation)

**Goal**: Add the NotifyStreamStall RPC to the proto and add domain-level types for stream monitoring.

**Files changed**:
- `api/micecam/camera_plugin.proto` — add `NotifyStreamStallRequest`, `NotifyStreamStallResponse` messages and `NotifyStreamStall` RPC
- `internal/domain/StreamLivenessState.h` — NEW: enum `StreamLivenessState { ACTIVE, STALLED, FINALIZED }` and struct `StreamStallEvent`
- Regenerate proto stubs (CMake `protobuf_generate_cpp` + gRPC plugin)

**Verification**: Build succeeds. Existing 37 tests pass.

### Phase 2: Stream Liveness Monitor (Infrastructure Core)

**Goal**: Implement the background monitoring thread with per-stream timestamps and injectable clock.

**Files changed**:
- `internal/infrastructure/StreamLivenessMonitor.h` — NEW: class with `register_stream()`, `unregister_stream()`, `update_activity()`, `set_stall_callback()`, `set_all_stalled_callback()`, injectable `Clock` concept
- `internal/infrastructure/StreamLivenessMonitor.cpp` — NEW: background `std::jthread`, check loop every 1s, per-plugin aggregation, stall detection, all-stalled detection

**Design**:
- `using Clock = std::function<std::chrono::steady_clock::time_point()>;` — injectable for testing (default: `std::chrono::steady_clock::now`)
- `using StallCallback = std::function<void(const std::string& stream_id, const std::string& plugin_id, uint64_t stall_duration_ms)>;`
- `using AllStalledCallback = std::function<void(const std::string& plugin_id)>;`
- Monitor maintains `unordered_map<string, Clock::time_point> last_active_` and `unordered_map<string, string> stream_to_plugin_`
- Background thread: iterate all streams, check `now - last_active[stream] > stall_timeout`, fire callbacks
- Per-plugin aggregation: group stalled streams by plugin_id, if all streams for a plugin are stalled → `AllStalledCallback`

**Verification**: Unit test with mock clock that advances past threshold → stall callback fires.

### Phase 3: Wire PluginStreamConsumer → Monitor + NotifyStreamStall Call

**Goal**: `PluginStreamConsumer::consumerLoop()` calls `monitor->update_activity(stream_id)` on each frame. Stall callback calls `NotifyStreamStall` gRPC. All-stalled callback triggers `handle_plugin_crash()`.

**Files changed**:
- `internal/infrastructure/PluginStreamConsumer.h` — add `StreamLivenessMonitor*` member, accept in constructor or via setter
- `internal/infrastructure/PluginStreamConsumer.cpp` — call `monitor_->update_activity(config_.stream_id)` after each successful frame read in `consumerLoop()`
- `internal/infrastructure/PluginRegistryService.h/.cpp` — add `set_grpc_stub_factory()` method. Wire stall callback to call `NotifyStreamStall` via gRPC stub. Wire all-stalled callback to call `handle_plugin_crash()`.
- `internal/infrastructure/StreamLivenessMonitor.h` — add `set_grpc_notify_fn()` or handle in PluginRegistryService

**Stall notification flow**:
1. Monitor detects `stream_id` stall → calls `StallCallback`
2. `StallCallback` (in PluginRegistryService) calls `NotifyStreamStall` on the plugin's gRPC stub with 2s deadline
3. If `recoverable=false` → call `RecordingPipeline::finalize_stream(stream_id)`, `shm_unlink`, emit alert
4. If `recoverable=true` → wait, re-check after 2x stall timeout
5. If gRPC fails → treat as plugin crash (all-stalled path)

**All-stalled flow**:
1. Monitor detects all streams from `plugin_id` stalled → calls `AllStalledCallback`
2. `AllStalledCallback` calls existing `handle_plugin_crash(plugin_id)`

**Verification**: Unit test with mock gRPC stub. Integration test with real gRPC server.

### Phase 4: keyframe_interval Propagation

**Goal**: `PreflightValidator::run_phase1_calibration` computes `min_gop`. This value must reach `RecordingPipeline::create_stream_pipeline` and be passed as `StreamConfig.keyframe_interval` to the `OpenStream` gRPC call.

**Files changed**:
- `internal/pipeline/RecordingPipeline.cpp` — in `create_stream_pipeline()`, when building `StreamConfig` for OpenStream, read `cal.min_gop` from `config_.calibration_results` and set it on the proto `StreamConfig.keyframe_interval`
- `internal/pipeline/RecordingPipeline.h` — no structural change needed (calibration_results already in SessionConfig)
- **Gap analysis**: The current `RecordingPipeline` does NOT call `OpenStream` gRPC directly — that's done by `AppController` or `PreflightValidator`. Need to trace where OpenStream is called and ensure keyframe_interval is propagated.

**Verification**: Unit test that validates `create_stream_pipeline` reads min_gop from calibration_results and sets fallback_gop_size (already done at RecordingPipeline.cpp:89). Separate test that the proto StreamConfig sent to OpenStream contains the keyframe_interval.

### Phase 5: Plugin Server NotifyStreamStall Implementation

**Goal**: Both plugin servers handle the new RPC.

**Files changed**:
- `cmd/plugins/micecam_ffmpeg/FFmpegPluginServer.h/.cpp` — add `NotifyStreamStall` override. Check if stream_id exists in `streams_` map. Check device accessibility (call `AVFoundationEnumerator::is_device_accessible(device_id)` or similar).
- `cmd/plugins/micecam_oak/OAKPluginServer.h/.cpp` — add `NotifyStreamStall` override. Return `acknowledged=false` (no hardware).
- `cmd/plugins/micecam_ffmpeg/AVFoundationEnumerator.h` — add `is_device_accessible(device_id)` method if not present.

**FFmpeg NotifyStreamStall logic**:
```
if stream_id not in streams_:
    return acknowledged=false
if device is accessible:
    return acknowledged=true, recoverable=true, action="retrying"
else:
    return acknowledged=true, recoverable=false, action="device_lost"
```

**Verification**: Unit test for each plugin server using existing gRPC test fixture (FFmpegPluginServerTest, OAKPluginServerTest pattern).

### Phase 6: Unit Tests

**New test files**:
- `tests/unit/test_stream_liveness_monitor.cpp` — NEW: test with injectable clock, register/unregister, stall detection, per-plugin aggregation, all-stalled detection, configurable timeout
- `tests/unit/test_notify_stream_stall.cpp` — NEW: test FFmpeg and OAK NotifyStreamStall via gRPC (extend existing plugin server test fixtures)
- `tests/unit/test_keyframe_propagation.cpp` — NEW: test that CalibrationResult.min_gop reaches StreamConfig.keyframe_interval

**CMake changes**:
- `CMakeLists.txt` — add `add_micecam_test(test_stream_liveness_monitor ...)` and `add_micecam_test(test_keyframe_propagation ...)`
- Plugin test block — add `test_notify_stream_stall` (requires gRPC linking, like test_ffmpeg_plugin_server)

**Coverage**:
- StreamLivenessMonitor: register 3 streams from 2 plugins, advance clock past timeout for 1 stream → stall callback fires for that stream only. Advance past timeout for all streams of plugin A → all-stalled callback fires for plugin A.
- NotifyStreamStall: valid stream_id → acknowledged=true. Unknown stream_id → acknowledged=false. gRPC failure → treated as crash.
- keyframe_interval: Calibrate returns min_gop=15 → OpenStream receives StreamConfig.keyframe_interval=15.

### Phase 7: Integration Tests

**New test files**:
- `tests/integration/test_plugin_e2e_no_hw.cpp` — NEW: fork real `micecam_ffmpeg` binary, execute Handshake → Calibrate → OpenStream → Shutdown via gRPC. No hardware needed.
- `tests/integration/test_calibrate_e2e.cpp` — NEW: fork real plugin binary, call Calibrate with real encoder, validate I > P > 0, max_sustainable_fps > 10, actual_encoder_name non-empty.
- `tests/integration/test_dual_path_keyframe.cpp` — NEW: create TestRing with H264 frames at known keyframe intervals, feed through RecordingPipeline, verify output MP4 has keyframes at expected positions.

**Fork e2e test design** (test_plugin_e2e_no_hw):
1. Locate plugin binary: `getenv("MICECAM_FFMPEG_PLUGIN")` or fall back to `CMAKE_BINARY_DIR/cmd/plugins/micecam_ffmpeg/micecam_ffmpeg`
2. Fork + exec with `--port <random_port>`
3. Wait for gRPC server readiness (retry connect with 100ms backoff, max 5s)
4. Execute RPC lifecycle: Handshake → GetCapabilities → EnumerateDevices → Calibrate(duration_ms=500) → OpenStream(test config) → Shutdown
5. Verify: all RPCs return OK, Calibrate returns success=true with valid fields, OpenStream returns valid ring descriptor
6. Teardown: Shutdown RPC → waitpid with SIGTERM + 2s timeout → SIGKILL → cleanup shm

**CMake changes**:
- Add `add_micecam_test(test_plugin_e2e_no_hw ...)` with `LABELS "no-hardware"`
- Add `add_micecam_test(test_calibrate_e2e ...)` with `LABELS "no-hardware"`
- Add `add_micecam_test(test_dual_path_keyframe ...)` with `LABELS "no-hardware"`

### Phase 8: Update validate_session_artifacts.py

**Goal**: Add spec 004 field checks.

**Files changed**:
- `scripts/validate_session_artifacts.py`:
  - Add `SPEC004_META_FIELDS` strict-mode check list: `session_start_wall_time`, `keyframe_interval`, `i_frame_latency_ms`, `p_frame_latency_ms`, `crash_window_sec`, `actual_encoder_name`, `calibration_duration_ms`, `parallel_test_passed`, `requested_streams`
  - In `validate_meta_json()` strict mode: check spec 004 fields exist and have correct types
  - In `validate_srt()` strict mode: check `wall_time=<ISO 8601>` format regex `\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6}`
  - In `validate_stats_json()`: verify JSON is dict (not array), keyed by stream_id
  - In `validate_mp4()` strict mode: verify fragmented MP4 (fMP4) structure via ffprobe

**Verification**: New test `tests/python/test_validate_session_artifacts.py` (synthetic data, all fields → PASS, missing field → FAIL, wrong type → FAIL, array stats → FAIL).

### Phase 9: HIL Tests (jingyi-lab only)

**New test files**:
- `tests/hil/test_hil_e2e.cpp` — NEW: full recording lifecycle with 2 real cameras (30s). Validate output with ffprobe. Run validator subprocess.
- `tests/hil/test_hil_crash_recovery.cpp` — NEW: start recording, kill -9 plugin process after 10s, verify crash recovery, fMP4 playable, reconnect file created.

**These tests are gated by `BUILD_HIL=ON` CMake flag** — excluded from CI by `--exclude-regex '.*hil.*|.*stress.*'`.

### Phase 10: CI + Final Verification

- Verify fork e2e no-hw test runs on all 3 CI platforms (macOS, Windows, Linux)
- Update `.github/workflows/ci.yml` if needed (ensure `BUILD_PLUGINS=ON` is set)
- Run full test suite: 37 existing + new tests all pass

## Test Strategy

### Unit Tests

| Test File                               | What It Tests                                                       | Layer           |
|-----------------------------------------|---------------------------------------------------------------------|-----------------|
| `test_stream_liveness_monitor`          | Stall detection, per-plugin aggregation, configurable timeout, mock clock | infrastructure  |
| `test_notify_stream_stall`              | FFmpeg/OAK NotifyStreamStall RPC (acknowledged, recoverable, unknown stream) | cmd (plugin)    |
| `test_keyframe_propagation`             | CalibrationResult.min_gop → StreamConfig.keyframe_interval flow    | pipeline        |

**Coverage target**: All new code paths in StreamLivenessMonitor, NotifyStreamStall handlers, and keyframe propagation chain.

### Integration Tests

| Test File                         | What It Tests                                                                    | Environment |
|-----------------------------------|----------------------------------------------------------------------------------|-------------|
| `test_plugin_e2e_no_hw`           | Fork real plugin binary, full RPC lifecycle via gRPC                             | CI all platforms |
| `test_calibrate_e2e`              | Real encoder Calibrate: I > P > 0, fps > 10, slot_size > 0, encoder name valid  | CI all platforms |
| `test_dual_path_keyframe`         | H264 passthrough + RAW fallback keyframe positions match keyframe_interval      | CI all platforms |
| `test_hil_e2e`                    | 2-camera full recording + artifact validation                                   | jingyi-lab only |
| `test_hil_crash_recovery`         | Kill plugin → crash recovery → valid fMP4 + reconnect file                      | jingyi-lab only |

### Edge Cases

| Edge Case                                     | Test                                              |
|-----------------------------------------------|---------------------------------------------------|
| Stream stall timeout exactly at boundary      | Mock clock at timeout - 1ns → active, at timeout → stalled |
| NotifyStreamStall gRPC deadline exceeded      | Plugin hangs → deadline fires → crash recovery    |
| NotifyStreamStall UNAVAILABLE                 | Plugin killed mid-call → crash recovery           |
| All streams stall simultaneously              | Kill plugin → all streams timeout at once → single crash recovery |
| One stream stalls, others active              | Stall callback for one, others continue           |
| Register/unregister stream during monitoring  | Thread safety: no crash on concurrent modification |
| Stats JSON is array not dict                  | Validator reports FAIL                            |
| SRT has no wall_time entries in strict mode   | Validator reports WARN                            |
| Empty session directory                       | Validator reports FAIL                            |
| Fork plugin binary not found                  | Test SKIP (not FAIL) with informative message     |
| Plugin exits with non-zero code               | Fork test detects and reports failure             |

## Rollback Plan

This change is on a feature branch `feat/005-stream-monitoring-test-suite` branched from `plugin-system`. Rollback strategy:

1. **Phase 1-2 (proto + domain)**: Revert proto change. Since proto is generated, delete generated stubs and re-run cmake. No runtime impact — proto is only used at build time.
2. **Phase 3 (wiring)**: The StreamLivenessMonitor is a standalone class with no side effects if not instantiated. Revert the PluginStreamConsumer changes — remove the `monitor_->update_activity()` call. The monitor pointer defaults to nullptr and is not called.
3. **Phase 4 (keyframe)**: keyframe_interval is already in StreamConfig proto (field 10). Reverting the propagation code just means the field stays at its default (0), which the plugin handles by using its own GOP sizing. No regression.
4. **Phase 5 (plugin servers)**: Revert the NotifyStreamStall override. Unknown RPC returns UNIMPLEMENTED — callers handle this as gRPC failure → crash recovery.
5. **Phase 6-8 (tests + scripts)**: Tests and scripts have no production impact. Delete test files and revert script changes.
6. **Full revert**: `git revert` the entire branch merge. No data migration, no schema change, no persistent state to clean up.

## Complexity Tracking

| Field     | Value    |
|-----------|----------|
| Estimated | **High** |
| Rationale | Touches 5 DDD layers (domain, infrastructure, api, cmd, pipeline). Adds a new gRPC RPC (proto + 2 server implementations). Introduces a background monitoring thread with concurrency concerns. Creates 5+ new test files including fork-based process tests. Updates a Python validation script. 20 functional requirements across monitoring, recovery, propagation, and testing. However, each individual piece is well-bounded and the integration points are clean interfaces. |

## Phase Summary

| Phase | Description                                  | Files Changed | New Files | Risk   |
|-------|----------------------------------------------|---------------|-----------|--------|
| 1     | Proto + domain types                         | 1             | 1         | Low    |
| 2     | StreamLivenessMonitor                        | 0             | 2         | Medium |
| 3     | Wire consumer → monitor → NotifyStreamStall  | 3             | 0         | High   |
| 4     | keyframe_interval propagation                | 1             | 0         | Low    |
| 5     | Plugin server NotifyStreamStall              | 4             | 0         | Medium |
| 6     | Unit tests                                   | 1 (CMake)     | 3         | Low    |
| 7     | Integration tests (no-hw)                    | 1 (CMake)     | 3         | Medium |
| 8     | Update validate_session_artifacts.py         | 1             | 0         | Low    |
| 9     | HIL tests                                    | 1 (CMake)     | 2         | Medium |
| 10    | CI verification                              | 1             | 0         | Low    |

**Total new files**: ~12 (2 infra + 1 domain + 3 unit tests + 3 integration tests + 2 HIL tests + 1 plan)
**Total modified files**: ~12 (proto + 4 infra + 2 cmd + 1 pipeline + 1 script + 2 CMake + 1 CI)
