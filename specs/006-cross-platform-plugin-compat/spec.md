# Feature Spec: Cross-Platform Compatibility, Code Health, and Test Gap Closure

## Metadata

| Field      | Value                                        |
|------------|----------------------------------------------|
| Feature ID | `006-cross-platform-plugin-compat`           |
| Branch     | `feat/006-cross-platform-compat`             |
| Status     | Draft                                        |
| Owner      | `jingyi`                                     |
| Date       | `2026-05-17`                                 |

## Summary

Fix all cross-platform, code health, and test gap issues identified during the post-spec-005 audit of specs 001-005. The audit covered 4 areas: (1) platform abstraction gaps that prevent Windows compilation, (2) behavioral contradictions between APIs, (3) UI stubs and hardcoded values, and (4) missing test coverage for edge cases. This spec resolves 4 HIGH (compilation failures / UB), 8 MEDIUM (behavioral anomalies / UI issues), 4 LOW (tech debt), and 5 test gaps. The goal: all code compiles and all tests pass on macOS, Linux, and Windows CI without exclusion hacks.

## User Scenarios

### US-001: Windows Build Compiles Cleanly

**Priority**: P1

**Independent Test**: Run `cmake --build build` on Windows with MSVC. Verify zero compilation errors. Run `ctest --test-dir build --exclude-regex '.*e2e.*'` and verify all platform-appropriate tests pass.

**Acceptance Scenarios**:
- Given a Windows CI runner with MSVC, When `cmake --build build` runs, Then zero compilation errors occur
- Given a Windows CI runner, When `ctest` runs with platform exclusions, Then all unit tests pass
- Given the SHM abstraction layer is in place, When `PluginRingReader` and `RingFrameProducer` are compiled on Windows, Then they use `CreateFileMapping`/`MapViewOfFile` instead of `shm_open`/`mmap`
- Given the SHM abstraction layer is in place, When compiled on macOS or Linux, Then behavior is identical to before (POSIX SHM path)
- Given the plugin is running on Windows, When a CTRL_C_EVENT or CTRL_CLOSE_EVENT signal is received, Then the plugin's `SetConsoleCtrlHandler` callback triggers graceful stream shutdown

### US-002: Capability Reporting Is Internally Consistent

**Priority**: P1

**Independent Test**: Start the FFmpeg plugin. Call `GetCapabilities`. Verify `supports_h264` matches whether `EnumerateDevices` advertises H264 payloads. Verify H264 is advertised on all platforms where hardware encoders are available (not just macOS).

**Acceptance Scenarios**:
- Given the FFmpeg plugin is running, When `GetCapabilities` returns `supports_h264=true`, Then `EnumerateDevices` includes `H264` in `supported_payloads` for at least one device
- Given the FFmpeg plugin is running on Linux with NVENC available, When `EnumerateDevices` is called, Then H264 is included in supported payloads
- Given the FFmpeg plugin is running on macOS with VideoToolbox available, When `EnumerateDevices` is called, Then H264 is included in supported payloads

### US-003: NotifyStreamStall Checks Device Health

**Priority**: P2

**Independent Test**: Open a stream on the FFmpeg plugin. Call `NotifyStreamStall` with the stream's ID. Verify the plugin checks whether the capture device is still accessible before responding with `recoverable`. Simulate device disconnection and verify the response changes.

**Acceptance Scenarios**:
- Given a stream is active on a connected device, When `NotifyStreamStall` is called, Then the plugin checks device accessibility and returns `recoverable=true` if the device is present
- Given a stream's device has been disconnected, When `NotifyStreamStall` is called, Then the plugin returns `recoverable=false` with `action="device_lost"`
- Given a stream_id is valid but the device state is unknown, When `NotifyStreamStall` is called, Then the plugin returns a conservative response (not always `recoverable=true`)

### US-004: UI Displays Real Encoder and Bitrate

**Priority**: P2

**Independent Test**: Start a recording session on a real camera. Navigate to the camera detail view. Verify the encoder name and bitrate shown match the actual encoder selected by `Calibrate` and the configured bitrate — not hardcoded strings.

**Acceptance Scenarios**:
- Given recording is active with an h264_nvenc encoder, When the camera detail view is displayed, Then the encoder field shows the actual encoder name from `Calibrate` result
- Given recording is active with a configured bitrate of 8 Mbps, When the camera detail view is displayed, Then the bitrate field shows "8.0 Mbps"
- Given recording has not started, When the camera detail view is displayed, Then encoder and bitrate show placeholder values (e.g., "—")

### US-005: Elapsed Timer Shows Hours for Long Sessions

**Priority**: P2

**Independent Test**: Start a recording session. Mock the session start time to be 65 minutes ago. Verify the elapsed time displays `01:05:00` (HH:MM:SS), not `65:00` (overflowed MM:SS).

**Acceptance Scenarios**:
- Given a session has been recording for 90 seconds, When `elapsedText()` is called, Then it returns `"01:30"` (MM:SS format, hours == 0)
- Given a session has been recording for 65 minutes, When `elapsedText()` is called, Then it returns `"01:05:00"` (HH:MM:SS format)
- Given a session has been recording for 3 hours 7 minutes 5 seconds, When `elapsedText()` is called, Then it returns `"03:07:05"`

### US-006: Preflight Items Wire to Real Validation

**Priority**: P2

**Independent Test**: Start the application with a camera connected. Navigate to the preflight check screen. Verify that `preflightItems()` returns a non-empty list with actual validation results (device detection, disk space, calibration status).

**Acceptance Scenarios**:
- Given a camera is connected and disk space is sufficient, When `preflightItems()` is called, Then it returns a list with at least one item showing a pass status
- Given no camera is connected, When `preflightItems()` is called, Then it returns a list with at least one item showing a fail status
- Given `preflightItems()` is called from QML, Then the returned items populate the preflight modal with real validation results

### US-007: Duplicate PayloadKind Eliminated

**Priority**: P1

**Independent Test**: Search the codebase for `enum class PayloadKind`. Verify exactly one definition exists (in `micecam::domain`). Verify all consumers use `domain::PayloadKind` or a `using`-declaration.

**Acceptance Scenarios**:
- Given the codebase, When `enum class PayloadKind` is searched, Then exactly one definition exists in `internal/domain/StreamRingDescriptor.h`
- Given `RecordingPipeline.h`, When it needs `PayloadKind`, Then it uses `using micecam::domain::PayloadKind` or includes the domain header
- Given all existing tests, When compiled after the change, Then all 39 ctest pass

### US-008: Uninitialized Members Are Zero-Initialized

**Priority**: P1

**Independent Test**: Run a static analyzer (clang-tidy `cppcoreguidelines-init-variables` or similar) on `PreflightValidator.h`. Verify no uninitialized scalar members remain.

**Acceptance Scenarios**:
- Given `PreflightValidator.h`, When `available_bytes_` is declared, Then it has an in-class initializer `= 0`
- Given any class with scalar members, When a static analyzer checks for uninitialized members, Then zero warnings are produced

### US-009: Negative Test Coverage for Plugin RPCs

**Priority**: P2

**Independent Test**: Run the new negative tests. Verify they cover: invalid Calibrate params (zero width/height, negative fps), double OpenStream on same device, concurrent register/unregister streams, and recoverable=false device_lost flow.

**Acceptance Scenarios**:
- Given `Calibrate` is called with `width=0, height=0, fps=-1`, When the test executes, Then the server returns an error or well-defined defaults (not UB)
- Given `OpenStream` is called twice for the same device_id, When the second call executes, Then the server returns an error or well-defined behavior (not silent duplicate streams)
- Given `register_stream` and `unregister_stream` are called concurrently from 4 threads, When the test executes, Then no deadlock or data corruption occurs
- Given a device_lost scenario, When `NotifyStreamStall` returns `recoverable=false, action="device_lost"`, Then the stream is finalized and SHM cleaned

### US-010: old/ Directory Removed from Git Tracking

**Priority**: P3

**Independent Test**: Verify `old/` directory is in `.gitignore` and removed from git tracking. Verify the repo size is reduced by ~200 MB.

**Acceptance Scenarios**:
- Given `old/` is in `.gitignore`, When `git status` runs, Then `old/` is not tracked
- Given `old/` was removed from tracking, When `git log` is checked, Then a commit removes the directory

### US-011: API Layer Does Not Include from internal/

**Priority**: P2

**Independent Test**: Search `api/micecam/` headers for `#include "internal/`. Verify zero matches.

**Acceptance Scenarios**:
- Given `api/micecam/ICameraBackend.h`, When includes are checked, Then no `#include "internal/...` directives exist
- Given the API layer needs domain types, When a consumer includes an API header, Then only public headers are transitively included

### US-012: FFmpegCameraBackend Buffer Allocation Consistent

**Priority**: P3

**Independent Test**: Capture a frame from a 640x480 camera. Verify `out_data.size()` equals `width * height * 3 / 2` (YUV420P) — not a larger value from linesize over-allocation.

**Acceptance Scenarios**:
- Given a 640x480 YUV420P frame is decoded, When `out_data.size()` is checked, Then it equals `640 * 480 * 3 / 2 = 460800`
- Given frame data is written to the buffer, When the copy loop finishes, Then the actual bytes written matches `out_data.size()`

## Requirements

### Functional Requirements

- **FR-001**: A `SharedMemoryBackend` interface MUST be created with methods: `open(name, size) -> handle`, `map(handle, size) -> void*`, `unmap(ptr, size)`, `unlink(name)`, `close(handle)`. Two implementations MUST be provided: `PosixSharedMemory` (macOS/Linux, using `shm_open`/`mmap`/`shm_unlink`) and `Win32SharedMemory` (Windows, using `CreateFileMapping`/`MapViewOfFile`/`CloseHandle`). A factory function MUST select the correct implementation at compile time.
- **FR-002**: `PluginRingReader` and `RingFrameProducer` MUST use `SharedMemoryBackend` instead of direct POSIX SHM calls. All `#include <sys/mman.h>`, `#include <fcntl.h>` MUST be moved behind the platform abstraction.
- **FR-003**: `FFmpegPluginServer::GetCapabilities` MUST set `supports_h264` to `true` if and only if `EnumerateDevices` would advertise H264 for at least one device. The H264 payload advertisement in `EnumerateDevices` MUST NOT be gated by `#ifdef __APPLE__` alone — it MUST check for actual encoder availability (VideoToolbox, NVENC, VAAPI, QSV, or libx264).
- **FR-004**: `FFmpegPluginServer::NotifyStreamStall` MUST perform an actual device health check before responding with `recoverable=true`. For FFmpeg-based capture, this means verifying the underlying `AVFormatContext` is still readable or the device path is still accessible.
- **FR-005**: `CameraDetailView.qml` MUST display encoder name and bitrate from backend properties (bound to `Calibrate` result and encoder config), not hardcoded strings. The encoder field MUST show `"—"` when no session is active.
- **FR-006**: `AppController::elapsedText()` MUST return `HH:MM:SS` format when hours > 0, and `MM:SS` format when hours == 0. Minutes MUST NOT overflow past 59.
- **FR-007**: `AppController::preflightItems()` MUST wire to `PreflightValidator` (or equivalent backend) and return actual validation results: device detection, disk space check, calibration readiness.
- **FR-008**: The duplicate `PayloadKind` enum in `micecam::pipeline` namespace (`RecordingPipeline.h`) MUST be removed. `RecordingPipeline.h` MUST use `micecam::domain::PayloadKind` via `using`-declaration or direct include.
- **FR-009**: `PreflightValidator::available_bytes_` MUST be zero-initialized with an in-class initializer: `uint64_t available_bytes_ = 0;`. Any other uninitialized scalar members in the codebase MUST also be fixed.
- **FR-010**: `FFmpegPluginServer::OpenStream` MUST guard against opening the same device_id twice. If the device already has an active stream, the RPC MUST return an error status (ALREADY_EXISTS or INVALID_ARGUMENT).
- **FR-011**: `FFmpegPluginServer::Calibrate` MUST validate input parameters. If `width <= 0` or `height <= 0` or `fps <= 0`, the RPC MUST return an error status with a descriptive message (not silently default to 1920x1080@30).
- **FR-012**: The `"posix_shm"` transport string in `PluginStreamConsumer` and `RingFrameProducer` MUST be replaced with a platform-aware constant or function that returns `"posix_shm"` on POSIX and `"win32_mapping"` on Windows.
- **FR-013**: The `old/` directory MUST be added to `.gitignore` and removed from git tracking via `git rm -r --cached old/`. The directory contents MAY remain on disk for local reference.
- **FR-014**: `api/micecam/ICameraBackend.h` MUST NOT include headers from `internal/`. Domain types used in the API MUST either be forward-declared, duplicated in `api/micecam/`, or moved to a shared public header.
- **FR-015**: `FFmpegCameraBackend::decode_raw_frame()` MUST allocate `out_data` using `width` (not `linesize`) for buffer size calculation, so that `out_data.size()` matches the actual frame data size.
- **FR-016**: Plugin signal handlers on Windows MUST use `SetConsoleCtrlHandler` in addition to `std::signal(SIGTERM, ...)` to ensure graceful shutdown on Windows console events.
- **FR-017**: A concurrent stress test MUST be added for `StreamLivenessMonitor` that exercises 4+ threads calling `register_stream`/`unregister_stream` simultaneously with no deadlock or data corruption.

### Non-Functional Requirements

- **NFR-001**: All changes MUST compile on macOS (clang), Linux (gcc), and Windows (MSVC) with zero errors and zero warnings.
- **NFR-002**: All existing 39 ctest + 44 Python tests MUST continue to pass after all changes.
- **NFR-003**: The `SharedMemoryBackend` abstraction MUST have zero runtime overhead compared to direct POSIX calls on macOS/Linux (inline delegation or compile-time selection).
- **NFR-004**: New tests added in this spec MUST run on all CI platforms (no hardware required).
- **NFR-005**: No new dependencies introduced. `SharedMemoryBackend` uses only OS APIs.

## Success Criteria

| #    | Criterion | Measured By |
|------|-----------|-------------|
| SC-1 | Windows CI builds with zero errors | Green check on Windows CI matrix job |
| SC-2 | `SharedMemoryBackend` compiles and passes tests on all 3 platforms | 3/3 CI matrix jobs green |
| SC-3 | `GetCapabilities` and `EnumerateDevices` are consistent | Unit test: `supports_h264` matches H264 in payloads |
| SC-4 | `NotifyStreamStall` checks device health | Unit test: disconnected device → `recoverable=false` |
| SC-5 | UI encoder/bitrate bound to backend | Manual verification: detail view shows real values |
| SC-6 | `elapsedText()` shows HH:MM:SS after 60 minutes | Unit test with mock time: 65 min → `"01:05:00"` |
| SC-7 | `preflightItems()` returns non-empty results | Unit test: returns ≥1 item |
| SC-8 | Exactly one `PayloadKind` definition | `grep -r "enum class PayloadKind"` returns 1 result |
| SC-9 | No uninitialized scalar members | clang-tidy `cppcoreguidelines-init-variables`: 0 warnings |
| SC-10 | Negative tests pass: invalid Calibrate, double OpenStream, concurrent ops (FR-017), device_lost | New test file: all assertions pass |
| SC-11 | `old/` not in git tracking | `git ls-files old/` returns empty |
| SC-12 | API headers have no `internal/` includes | `grep -r '#include "internal/' api/` returns empty |
| SC-13 | `out_data.size()` matches frame size | Unit test: 640x480 → size = 460800 |
| SC-14 | All 39+ existing ctest pass | `ctest --test-dir build`: 100% pass |

## Assumptions

- The `feat/005-stream-monitoring-test-suite` branch has been merged to `plugin-system` before this spec's branch is created.
- Windows CI runner has MSVC 2019+ with C++20 support.
- `CreateFileMapping`/`MapViewOfFile` semantics are sufficiently equivalent to POSIX `shm_open`/`mmap` for ring buffer IPC (both support named shared memory with read/write access).
- FFmpeg encoder availability can be checked via `avcodec_find_encoder_by_name` at runtime (not just compile-time platform checks).
- The `old/` directory is legacy code no longer needed by any active code path (confirmed: 0 imports/references from active source).
- UI binding to backend properties uses existing QML property bindings via `AppController` (no new QML architecture needed).

## Clarifications

None — all findings confirmed via automated code audit with line-level evidence.

## Audit Finding Disposition

| Finding ID | Description | Disposition | FR/US |
|------------|-------------|-------------|-------|
| HIGH-1 | PluginRingReader POSIX SHM | FR-001/FR-002, US-001 | Fixed |
| HIGH-2 | RingFrameProducer POSIX SHM | FR-001/FR-002, US-001 | Fixed |
| HIGH-3 | Uninitialized `available_bytes_` | FR-009, US-008 | Fixed |
| HIGH-4 | Duplicate `PayloadKind` enum | FR-008, US-007 | Fixed |
| MEDIUM-1 | `supports_h264` contradicts EnumerateDevices | FR-003, US-002 | Fixed |
| MEDIUM-2 | NotifyStreamStall always `recoverable=true` | FR-004, US-003 | Fixed |
| MEDIUM-3 | Hardcoded H.265/12.0 Mbps in QML | FR-005, US-004 | Fixed |
| MEDIUM-4 | `elapsedText()` MM:SS overflow | FR-006, US-005 | Fixed |
| MEDIUM-5 | `preflightItems()` returns empty | FR-007, US-006 | Fixed |
| MEDIUM-6 | Hardcoded `"posix_shm"` | FR-012, US-001 | Fixed |
| MEDIUM-7 | SIGTERM handler Windows-incompatible | FR-016, US-001 | Fixed (downgraded to LOW) |
| MEDIUM-8 | H264 only advertised on macOS | FR-003, US-002 | Fixed (same as MEDIUM-1) |
| LOW-1 | `old/` directory (340 files, 205 MB) | FR-013, US-010 | Fixed |
| LOW-3 | API layer includes from `internal/` | FR-014, US-011 | Fixed |
| LOW-4 | FFmpegCameraBackend linesize vs width | FR-015, US-012 | Fixed |
| LOW-5 | SHM Windows compat | FR-001/FR-002, US-001 | Fixed (same as HIGH-1/HIGH-2) |
| TEST-GAP-1 | No concurrent register/unregister test | FR-017, US-009 | Fixed |
| TEST-GAP-2 | No negative Calibrate params test | FR-011, US-009 | Fixed |
| TEST-GAP-3 | No OpenStream double-open test | FR-010, US-009 | Fixed |
| TEST-GAP-4 | No recoverable=false device_lost E2E test | FR-004, US-009 | Fixed |
| TEST-GAP-5 | No HIL test files | **Deferred** — hardware required, see Out of Scope | N/A |
| LOW-2 | TODO/FIXME in active code | **False positive** — audit confirmed 0 in active source; all 7 in `old/` only | N/A |

## Out of Scope

- **OAK hardware validation** — OAK plugin remains `acknowledged=false` for `NotifyStreamStall`.
- **Full App layer gRPC wiring** — `PluginRegistryService::notify_stall_fn_` real gRPC stub wiring is a separate large task.
- **v4l2 device enumeration** — Linux device enumeration beyond synthetic fallback is a separate task.
- **HIL test creation** — `test_hil_e2e.cpp` and `test_hil_crash_recovery.cpp` remain deferred to hardware availability.
- **UI redesign** — Only data binding fixes; no visual redesign of any QML components.
- **Audio recording** — Video only per spec 001.
- **Plugin sandboxing/signing** — Trusted local code assumption per spec 004.

## Risk Notes

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Windows SHM semantics differ from POSIX (e.g., no `shm_unlink` equivalent) | Medium | High | `Win32SharedMemory` uses named file mapping with `PAGE_READWRITE`; cleanup on process exit; add explicit `CloseHandle` in destructor |
| `CreateFileMapping` name collisions across sessions | Low | Medium | Namespaced with `Global\MiceCam_<session_id>_<ring_id>` pattern |
| FFmpeg encoder availability check at runtime is slow | Low | Low | Cache result after first `GetCapabilities` call; encoder list doesn't change during session |
| Removing `old/` from git loses history | Very Low | Very Low | History preserved in git log; `git rm --cached` only removes from tracking |
| QML property binding change breaks existing UI | Low | Medium | Test with manual UI verification on macOS |
| FR-005 (UI encoder/bitrate) has no automated regression test | Low | Medium | Only manual verification (SC-5); consider adding QML unit test in future spec |

## Implementation Order

1. **Phase 1 — Compilation blockers (HIGH)**:
   - FR-008: Remove duplicate `PayloadKind` enum (US-007)
   - FR-009: Zero-initialize `PreflightValidator::available_bytes_` (US-008)
   - FR-001/FR-002: Create `SharedMemoryBackend` abstraction; refactor `PluginRingReader` and `RingFrameProducer` (US-001)
   - Verify: Windows CI builds with zero errors

2. **Phase 2 — Behavioral fixes (MEDIUM)**:
   - FR-003: Fix `GetCapabilities`/`EnumerateDevices` consistency (US-002)
   - FR-004: Implement real device health check in `NotifyStreamStall` (US-003)
   - FR-010: Add double-open guard in `OpenStream` (US-009)
   - FR-011: Add Calibrate input validation (US-009)
   - FR-012: Platform-aware transport string (US-001)
   - FR-016: Windows signal handler (US-001)
   - Verify: All existing tests pass + new negative tests pass

3. **Phase 3 — UI fixes (MEDIUM)**:
   - FR-005: Bind encoder/bitrate to backend properties (US-004)
   - FR-006: Fix `elapsedText()` to show HH:MM:SS (US-005)
   - FR-007: Wire `preflightItems()` to backend (US-006)
   - Verify: Manual UI verification on macOS

4. **Phase 4 — Tech debt (LOW)**:
   - FR-013: Remove `old/` from git tracking (US-010)
   - FR-014: Fix API layer include hygiene (US-011)
   - FR-015: Fix `FFmpegCameraBackend` buffer allocation (US-012)
   - Verify: `git ls-files old/` empty; `grep '#include "internal/' api/` empty

5. **Phase 5 — Test gaps**:
   - FR-017: Add concurrent register/unregister test for `StreamLivenessMonitor` (US-009)
   - Add negative Calibrate params test (US-009)
   - Add double OpenStream test (US-009)
   - Add recoverable=false device_lost E2E test (US-009)
   - Add `elapsedText()` hour-format test (US-005)
   - Add `SharedMemoryBackend` unit tests for both platforms (US-001)
   - Verify: All new tests pass on all CI platforms

6. **Phase 6 — Final verification**:
   - Run full CI on all 3 platforms
   - Run clang-tidy static analysis
   - Verify `ctest` 100% pass rate
   - Update `project_index` and relevant docs
