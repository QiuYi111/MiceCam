# Feature Spec: Production Hardening

## Metadata

| Field      | Value                          |
|------------|--------------------------------|
| Feature ID | `008-production-hardening`     |
| Branch     | `feat/008-production-hardening`|
| Status     | Draft                          |
| Owner      | `jingyi`                       |
| Date       | `2026-05-22`                   |
| Baseline   | `atlas/CONSOLIDATED_REPORT.md` |

## Summary

Fix 15 P0/P1 defects identified by the `atlas/` semantic audit of MiceCam v2 (59 source files, ~7385 lines). The audit found 28 issues across domain, pipeline, infrastructure, and UI layers. This spec addresses the 10 highest-risk items from Stages A and B of the audit roadmap: data races, thread safety, missing backtracks, stub implementations, and crash bugs that block production deployment.

## User Scenarios

### US-001: Reliable Recording Stop (P0)

**Priority**: P1

**Independent Test**: Start recording with 2 cameras. Signal stop. Verify all MP4 files are complete and playable. Repeat 10 times.

**Acceptance Scenarios**:
- Given recording is active with multiple streams, When user clicks Stop, Then all MP4 files are flushed and finalized without data races.
- Given recording stop is triggered while push_frame() is in progress, Then the pipeline drains remaining frames before closing.

### US-002: Graceful App Shutdown (P0)

**Priority**: P1

**Independent Test**: Start recording. Close the app window. Verify the process exits cleanly without crash or std::terminate.

**Acceptance Scenarios**:
- Given recording is active, When the user closes the app window, Then the capture thread stops cleanly and the app shuts down without crash.
- Given recording is not active, When the user closes the app window, Then the app exits immediately without hanging.

### US-003: Consistent UI Metrics (P0)

**Priority**: P1

**Independent Test**: Record for 60 seconds. Verify `total_frames_`, `bytes_written_`, and per-stream metrics never show impossible values (e.g., decreasing frame count).

**Acceptance Scenarios**:
- Given recording is active, When the UI reads total_frames_ from the main thread while the capture thread writes it, Then the value is consistent (no torn reads).
- Given recording is active with multiple streams, When per-stream counters are updated concurrently, Then each counter is independently consistent.

### US-004: Encoder Info Reflects Reality (P1)

**Priority**: P1

**Independent Test**: Record on macOS (VideoToolbox) and Ubuntu (libx264). Verify the UI encoding panel shows the actual encoder name and bitrate.

**Acceptance Scenarios**:
- Given recording is active on macOS, When the user checks encoding info, Then the encoder name reflects the actual codec (e.g., "H.264 (VideoToolbox)") not a hardcoded string.
- Given recording is active on Linux, When hardware encoding is unavailable, Then the UI shows "H.264 (libx264)" as the fallback.

## Requirements

### Functional Requirements — Stage A (Merge Gate)

- **FR-A1**: `StreamLivenessMonitor::StallCountResetsOnActivity` test passes 10/10 consecutive runs on macOS using `cycle_count_` spin-wait. (Code already implemented; spec doc updated.)
- **FR-A2**: `AppController` destructor must call `stopCaptureLoop()` and join `capture_thread_` before `~QObject`.
- **FR-A3**: `total_frames_`, `bytes_written_`, `stream_frame_counts_`, `stream_drop_counts_` changed to `std::atomic<>` types. `active_streams_` access guarded with its own mutex or made single-thread-only.
- **FR-A4**: `RecordingPipeline::stop()` must drain in-flight `push_frame()` calls before closing writers, preventing cross-thread access to streams_ during flush.
- **FR-A5**: `RecordingPipeline::start()` must clean up any partially-created `StreamPipeline` objects and output directories if a later stream initialization fails.

### Functional Requirements — Stage B (Production Ready)

- **FR-B1**: `FeishuWebhook::send()` must perform an actual HTTP POST with configured webhook URL and JSON payload (alert type, plugin, device, timestamp, diagnostic message).
- **FR-B2**: `current_encoder_name_` and `current_bitrate_` in `AppController` must reflect actual encoder state from `RecordingPipeline` or stream config, not hardcoded strings.
- **FR-B3**: `gRPC NotifyStallFn` stub in `PluginRegistryService` must be connected to the main process stall detection, enabling external plugin crash recovery signalling.
- **FR-B4**: `PluginRegistryService` lock nesting (stall_callback inside registry_mutex_) must be resolved — either by deferring the callback outside the lock scope or using a dedicated mutex.
- **FR-B5**: `ConfigLoader` must be documented as single-thread-only or protected with a mutex, since both UI and worker threads may access it concurrently.

### Non-Functional Requirements

- **NFR-001**: All existing 45 tests must continue to pass after changes.
- **NFR-002**: Thread safety changes must not reduce recording throughput (verified by 60-second recording producing correctly-timed frames).
- **NFR-003**: App must shut down within 3 seconds of window close (vs current behavior where it may hang indefinitely).

## Success Criteria

| #    | Criterion                                    | Measured By          |
|------|----------------------------------------------|----------------------|
| SC-1 | Flaky test passes 10/10 on macOS             | CI run               |
| SC-2 | App closes cleanly without crash             | Manual: close during recording |
| SC-3 | UI metrics never show torn/inconsistent values | ThreadSanitizer or manual 60s test |
| SC-4 | Encoder name/bitrate match actual codec      | Manual check on macOS + Linux |
| SC-5 | Stop recording produces valid MP4 10/10 times| CI integration test   |
| SC-6 | RecordingPipeline start failure cleans up    | Unit test             |
| SC-7 | FeishuWebhook sends real HTTP request        | Integration test with mock endpoint |
| SC-8 | 45/45 existing tests pass                    | CI run               |

## Assumptions

- `cycle_count_` fix in `StreamLivenessMonitor` is already implemented (commit `52c2258`); only spec doc and verification remain.
- `micecam_config.json` contains `feishu_webhook_url` field already parsed by ConfigLoader.
- macOS VideoToolbox encoder name can be queried from FFmpeg codec context.
- gRPC NotifyStallFn integration reuses existing `CameraPlugin::NotifyStallFn` streaming RPC definition.

## Out of Scope

- Full ThreadSanitizer integration in CI (separate spec)
- UI test coverage for recording/crash/disconnect paths (covered by spec 005 HIL)
- PluginRingReader checksum upgrade to CRC32 (Stage C)
- TimestampEngine initialization guard (Stage C)
- SessionMetadata field additions (Stage D)
- AppSettings debounce/write batching (Stage D)
- Windows HIL/CI fork e2e tests (hardware dependency)
- OAK-D hardware validation (waiver, spec 007)

## Risk Notes

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| atomic replacement changes API compatibility | Low | Medium | Use atomic<T> with `.load()`/`.store()` in read paths; verify compile |
| RecordingPipeline drain logic introduces deadlock | Medium | High | Use try_lock or state-based early exit; add timeout |
| FeishuWebhook HTTP send blocks UI thread | Medium | Medium | Run send in background thread or use async HTTP client |
| gRPC NotifyStallFn wiring requires plugin process to be running | High | Medium | Conditional: only connect when plugin process is live |
