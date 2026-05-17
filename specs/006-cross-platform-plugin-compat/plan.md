# Implementation Plan: Cross-Platform Compatibility, Code Health, and Test Gap Closure

## Inputs

| Source            | Reference                                                      |
|-------------------|----------------------------------------------------------------|
| Spec              | `specs/006-cross-platform-plugin-compat/spec.md`              |
| PRD               | N/A (derived from post-spec-005 cross-platform audit)         |
| Related Contracts | `api/micecam/camera_plugin.proto` (no changes — FR-003/FR-004 modify server behavior only) |
| Dependency        | Spec 005 branch must be merged to `plugin-system` first       |

## Technical Context

| Dimension             | Value                                                    |
|-----------------------|----------------------------------------------------------|
| Language              | C++20, QML/JavaScript, Python 3.10+                      |
| Framework             | CMake, GTest, Qt 6 / PySide6                             |
| Storage               | POSIX shared memory (ring buffers), Windows named file mappings |
| External Dependencies | FFmpeg (libavcodec/libavformat), gRPC, protobuf, spdlog  |
| Platforms             | macOS (clang), Linux (gcc), Windows (MSVC 2019+)         |

## Architecture Impact

### DDD Layer Impact

| Layer             | Change                                                                                                      |
|-------------------|-------------------------------------------------------------------------------------------------------------|
| `domain/`         | No changes. `PayloadKind` canonical location stays in `StreamRingDescriptor.h`.                            |
| `infrastructure/` | **NEW**: `SharedMemoryBackend` interface + `PosixSharedMemory` impl in `internal/infrastructure/`. Refactor `PluginRingReader.cpp` to use backend. Fix `FFmpegCameraBackend.cpp` linesize bug. |
| `pipeline/`       | Remove duplicate `PayloadKind` from `RecordingPipeline.h` (use `using micecam::domain::PayloadKind`). Zero-init `PreflightValidator::available_bytes_`. |
| `api/`            | No proto changes. Fix `api/micecam/ICameraBackend.h` include hygiene (remove `internal/` includes).         |
| `cmd/` (plugins)  | Refactor `RingFrameProducer.cpp` to use `SharedMemoryBackend`. Fix `FFmpegPluginServer.cpp` capability consistency, Calibrate validation, OpenStream double-open guard, NotifyStreamStall device check. Platform-aware transport string. Windows signal handler. |
| `cmd/` (UI)       | Fix `AppController.cpp`: `elapsedText()` HH:MM:SS, `preflightItems()` wiring. Fix `CameraDetailView.qml` hardcoded encoder/bitrate. |
| `tests/`          | NEW: `test_shared_memory_backend.cpp`, `test_negative_plugin_rpcs.cpp`, concurrent stress test additions. |

### Contract Impact

- **`SharedMemoryBackend`** (NEW interface): `open(name, size) -> int`, `map(fd, size) -> void*`, `unmap(ptr, size)`, `unlink(name)`, `close(fd)`. Two impls: `PosixSharedMemory`, `Win32SharedMemory`.
- **No proto changes** — all behavioral fixes are within existing gRPC method implementations.
- **`FFmpegPluginServer` behavioral contracts change**: `GetCapabilities.supports_h264` now reflects real encoder availability; `Calibrate` rejects invalid params; `OpenStream` rejects duplicate device opens; `NotifyStreamStall` performs real device check.
- **`RecordingPipeline.h`**: Removes duplicate `PayloadKind` enum. Adds `using micecam::domain::PayloadKind`.

### Data Model Impact

- **`_meta.json` / `_stats.json`**: No structural change.
- **`StreamRingDescriptor`**: `platform_handle_type` field values change from hardcoded `"posix_shm"` to platform-aware `"posix_shm"` / `"win32_mapping"`.

### Cross-Process Wire Compatibility

The `SharedMemoryBackend` abstraction must produce identical ring buffer layout on all platforms. The `RingHeader` (64 bytes) and `PayloadHeader` (44 bytes) are byte-layout types with `static_assert` guards. The abstraction replaces OS-specific calls (`shm_open`/`mmap` vs `CreateFileMapping`/`MapViewOfFile`) but does NOT change the ring buffer data layout.

## Blast Radius Classification

| Field          | Value                                                                                                     |
|----------------|-----------------------------------------------------------------------------------------------------------|
| Level          | **core**                                                                                                  |
| Reason         | Modifies foundational IPC layer (SHM abstraction used by both host and plugin processes). Changes plugin server behavioral contracts (Calibrate, OpenStream, GetCapabilities, NotifyStreamStall). Touches domain + infrastructure + pipeline + api + cmd layers. |
| Required Gates | spec, plan, tests, review_agent, human_spec_review, architecture_review, rollback_plan, security_review   |

**[REQUIRES HUMAN REVIEW]** — SHM IPC abstraction affects the cross-process ring buffer contract. Plugin server behavioral changes (Calibrate validation, OpenStream guard) alter the gRPC API semantics.

## Constitution Check

| Check          | Pass | Notes                                                                                     |
|----------------|------|-------------------------------------------------------------------------------------------|
| Contract-first | Yes  | `SharedMemoryBackend` interface defined in header before impl. Plugin behavioral changes documented in spec FRs. |
| DDD direction  | Yes  | Platform abstraction in `infrastructure/` (correct layer). Domain types unchanged. Pipeline uses `using` to import domain type (downward dependency). |
| TDD/BDD        | Yes  | Phase 1-2 produce implementations; Phase 5 adds new tests. Negative test coverage is a primary deliverable. |
| Observability  | Yes  | No change — existing spdlog logging covers all error paths. New Calibrate validation errors logged via spdlog. |
| Security       | Yes  | Plugin untrusted assumption unchanged. `SharedMemoryBackend` uses platform security attributes (POSIX: 0600 perms; Win32: default DACL). Input validation on Calibrate prevents resource abuse. |

## Implementation Strategy

Build foundation-first: platform abstraction → type cleanup → behavioral fixes → UI fixes → tech debt → tests.

### Phase 1: SharedMemoryBackend Abstraction (FR-001, FR-002)

**Goal**: Create platform-agnostic SHM interface so PluginRingReader and RingFrameProducer compile on Windows.

**New files**:
- `internal/infrastructure/SharedMemoryBackend.h` — interface: `open()`, `map()`, `unmap()`, `unlink()`, `close()`
- `internal/infrastructure/PosixSharedMemory.h/.cpp` — macOS/Linux impl: `shm_open`, `mmap`, `shm_unlink`
- `internal/infrastructure/Win32SharedMemory.h/.cpp` — Windows impl: `CreateFileMapping`, `MapViewOfFile`, `CloseHandle`

**Modified files**:
- `internal/infrastructure/PluginRingReader.cpp` — replace `shm_open`/`mmap`/`munmap`/`shm_unlink`/`::close` with `SharedMemoryBackend` calls
- `cmd/plugins/micecam_ffmpeg/RingFrameProducer.cpp` — same replacement on plugin side
- `cmd/plugins/micecam_ffmpeg/FFmpegPluginServer.cpp` — replace `shm_unlink` in cleanup paths
- `CMakeLists.txt` — add new source files, conditional compilation (`if(WIN32)` for Win32 impl)

**Design**:
```cpp
namespace micecam::infrastructure {
class SharedMemoryBackend {
public:
    virtual ~SharedMemoryBackend() = default;
    virtual int open(const std::string& name, size_t size) = 0;
    virtual void* map(int fd, size_t size) = 0;
    virtual void unmap(void* ptr, size_t size) = 0;
    virtual void unlink(const std::string& name) = 0;
    virtual void close(int fd) = 0;
};

std::unique_ptr<SharedMemoryBackend> create_shared_memory_backend();
}
```

Factory `create_shared_memory_backend()` returns `PosixSharedMemory` on POSIX, `Win32SharedMemory` on Windows. Header-only factory via `#ifdef _WIN32`.

**Verification**: macOS/Linux: existing 39 ctest pass. Windows: `cmake --build build` succeeds.

### Phase 2: Type Cleanup (FR-008, FR-009, FR-012)

**Goal**: Remove duplicate PayloadKind enum. Zero-init uninitialized members. Platform-aware transport string.

**Modified files**:
- `internal/pipeline/RecordingPipeline.h` — remove duplicate `enum class PayloadKind`. Add `#include "domain/StreamRingDescriptor.h"` and `using PayloadKind = micecam::domain::PayloadKind;`. Update all internal uses from `PayloadKind::H264` (unchanged since `using` brings it into scope).
- `internal/pipeline/PreflightValidator.h` — change `uint64_t available_bytes_;` to `uint64_t available_bytes_ = 0;`
- `internal/infrastructure/PluginStreamConsumer.h` — change `std::string transport = "posix_shm";` to use platform constant
- `internal/infrastructure/PluginStreamConsumer.cpp` — same: replace `"posix_shm"` literal with platform constant
- `cmd/plugins/micecam_ffmpeg/RingFrameProducer.cpp` — same: `desc.platform_handle_type` uses platform constant

**Platform constant** (in a shared header, e.g., `internal/infrastructure/SharedMemoryBackend.h`):
```cpp
#ifdef _WIN32
inline constexpr const char* kShmTransportType = "win32_mapping";
#else
inline constexpr const char* kShmTransportType = "posix_shm";
#endif
```

**Verification**: `grep -r "enum class PayloadKind"` returns exactly 1 result. All 39 ctest pass.

### Phase 3: Plugin Behavioral Fixes (FR-003, FR-004, FR-010, FR-011, FR-016)

**Goal**: Fix capability consistency, Calibrate validation, OpenStream guard, NotifyStreamStall device check, Windows signal handler.

**Modified files**:
- `cmd/plugins/micecam_ffmpeg/FFmpegPluginServer.cpp`:
  - **FR-003 (GetCapabilities/EnumerateDevices)**: In `buildCapabilityInfo()`, check for actual encoder availability via `avcodec_find_encoder_by_name` for h264_nvenc/h264_videotoolbox/h264_qsv/libx264. Set `supports_h264` based on result. In `EnumerateDevices()`, replace `#ifdef __APPLE__` guard with runtime encoder availability check (same function). Add private helper: `bool hasH264Encoder() const`.
  - **FR-010 (OpenStream double-open)**: Before creating new stream, check `streams_` map for existing entry with same `device_id`. If found, return `grpc::Status(grpc::StatusCode::ALREADY_EXISTS, "device already has active stream: " + device_id)`.
  - **FR-011 (Calibrate validation)**: At top of `Calibrate()`, if `req->width() <= 0 || req->height() <= 0 || req->fps() <= 0`, return `grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "width, height, and fps must be positive")`. Remove silent default fallback.
  - **FR-004 (NotifyStreamStall device check)**: After checking `streams_` map, verify the capture device is still accessible. For FFmpeg: check if the `AVFormatContext` associated with the stream is still valid (non-null `fmt_ctx_`). If device is gone, return `recoverable=false, action="device_lost"`. Add private helper: `bool isDeviceAccessible(const ActiveStream& stream) const`.
- `cmd/plugins/micecam_ffmpeg/main.cpp` — FR-016: Add `#ifdef _WIN32` block with `SetConsoleCtrlHandler` registering `g_shutdown_requested` flag. Keep existing `std::signal(SIGINT/SIGTERM, signal_handler)` for POSIX.
- `cmd/plugins/micecam_oak/main.cpp` — FR-016: Same Windows signal handler addition.

**Verification**: Unit tests: `test_ffmpeg_plugin_server.cpp` updated with new test cases. All existing tests pass (new error paths are additive).

### Phase 4: UI Fixes (FR-005, FR-006, FR-007)

**Goal**: Bind encoder/bitrate to backend properties, fix elapsed timer, wire preflight items.

**Modified files**:
- `cmd/micecam_ui/AppController.cpp`:
  - **FR-006 (elapsedText)**: Replace `%02d:%02d` format with hour-aware logic:
    ```cpp
    int hr = static_cast<int>(total_sec / 3600);
    int min = static_cast<int>((total_sec % 3600) / 60);
    int sec = static_cast<int>(total_sec % 60);
    if (hr > 0) return QString::asprintf("%02d:%02d:%02d", hr, min, sec);
    return QString::asprintf("%02d:%02d", min, sec);
    ```
  - **FR-007 (preflightItems)**: Wire to `PreflightValidator` or `CameraManager`. Create `QVariantList` from actual device detection + disk space results. Add `Q_INVOKABLE` property for preflight data.
- `cmd/micecam_ui/AppController.h` — add properties: `QString currentEncoderName`, `QString currentBitrate`, `QVariantList preflightItems` (actual implementation)
- `cmd/micecam_ui/qml/components/CameraDetailView.qml`:
  - **FR-005**: Replace hardcoded `"H.265 (HEVC)"` with `appController.currentEncoderName || "—"` binding. Replace `"12.0 Mbps"` with `appController.currentBitrate || "—"`. Apply at both locations (metrics grid L267-268 and encoder section L938/954).

**Verification**: Manual UI verification on macOS. Unit test for `elapsedText()` with mock time.

### Phase 5: Tech Debt (FR-013, FR-014, FR-015)

**Goal**: Remove `old/` from git, fix API include hygiene, fix FFmpegCameraBackend buffer allocation.

**Modified files**:
- `.gitignore` — add `old/` entry
- `old/` — `git rm -r --cached old/` (remove from tracking, keep on disk)
- `api/micecam/ICameraBackend.h` — replace `#include "internal/domain/DeviceInfo.h"`, `#include "internal/domain/StreamConfig.h"`, `#include "internal/domain/Capabilities.h"` with forward declarations or new `api/micecam/` header copies
- `internal/infrastructure/FFmpegCameraBackend.cpp` — FR-015: In `decode_raw_frame()`, allocate `out_data` using `width * height` (not `linesize * height`):
  ```cpp
  int y_size = dec_ctx_->width * dec_ctx_->height;
  int u_size = dec_ctx_->width * dec_ctx_->height / 4;
  int v_size = dec_ctx_->width * dec_ctx_->height / 4;
  out_data.resize(y_size + u_size + v_size);
  ```

**Verification**: `git ls-files old/` returns empty. `grep -r '#include "internal/' api/` returns empty. Unit test: `test_ffmpeg_camera.cpp` updated to verify `out_data.size()`.

### Phase 6: Test Gap Closure (FR-017, US-009)

**Goal**: Add missing test coverage for concurrency, negative RPCs, and edge cases.

**New test files**:
- `tests/unit/test_shared_memory_backend.cpp` — NEW: test `PosixSharedMemory` open/map/write/read/unlink cycle. Test cleanup on destruction. Test name collision handling.
- `tests/unit/test_negative_plugin_rpcs.cpp` — NEW:
  - Calibrate with `width=0, height=0, fps=-1` → INVALID_ARGUMENT
  - Calibrate with `fps=0` → INVALID_ARGUMENT
  - OpenStream twice for same device → ALREADY_EXISTS
  - OpenStream with nonexistent device → error
  - NotifyStreamStall with disconnected device → `recoverable=false`
- **Extend existing** `tests/unit/test_stream_liveness_monitor.cpp` — add concurrent stress test: 4 threads calling `register_stream`/`unregister_stream`/`update_activity` for 5 seconds, no deadlock or crash
- **Extend existing** `tests/unit/test_app_controller.cpp` — add `elapsedText()` test: 90s → `"01:30"`, 65min → `"01:05:00"`, 3h7m5s → `"03:07:05"`

**CMake changes**:
- `CMakeLists.txt` — add `add_micecam_test(test_shared_memory_backend ...)` and `add_micecam_test(test_negative_plugin_rpcs ...)`

**Verification**: All new tests pass on macOS and Linux. Windows: tests compile and pass (SHM backend test runs POSIX path on POSIX, Win32 path on Windows).

### Phase 7: CI + Final Verification

- Verify Windows CI builds with zero errors and all platform-appropriate tests pass
- Verify macOS and Linux CI: all existing + new tests pass
- Run clang-tidy `cppcoreguidelines-init-variables` on modified files
- Run `grep -r "enum class PayloadKind"` — exactly 1 result
- Run `git ls-files old/` — empty
- Run `grep -r '#include "internal/' api/` — empty

## Test Strategy

### Unit Tests

| Test File                               | What It Tests                                                       | Layer           |
|-----------------------------------------|---------------------------------------------------------------------|-----------------|
| `test_shared_memory_backend`            | SHM open/map/write/read/unlink, cleanup, name collision            | infrastructure  |
| `test_negative_plugin_rpcs`             | Invalid Calibrate, double OpenStream, device_lost NotifyStall      | cmd (plugin)    |
| `test_stream_liveness_monitor` (extend) | Concurrent register/unregister stress (4 threads, 5s)              | infrastructure  |
| `test_app_controller` (extend)          | `elapsedText()` format: 90s→MM:SS, 65min→HH:MM:SS                  | cmd (UI)        |
| `test_ffmpeg_camera` (extend)           | `out_data.size()` matches width-based allocation, not linesize      | infrastructure  |
| `test_ffmpeg_plugin_server` (extend)    | `GetCapabilities.supports_h264` matches EnumerateDevices payloads   | cmd (plugin)    |

### Integration Tests

No new integration test files. Existing fork e2e tests (`test_plugin_e2e_no_hw`, `test_calibrate_e2e`, `test_dual_path_keyframe`) continue to pass. The negative RPC tests use existing in-process gRPC test fixtures (no fork needed).

### Edge Cases

| Edge Case                                     | Test                                              |
|-----------------------------------------------|---------------------------------------------------|
| Calibrate with `width=0`                      | `test_negative_plugin_rpcs` → INVALID_ARGUMENT    |
| Calibrate with `fps=-1`                       | `test_negative_plugin_rpcs` → INVALID_ARGUMENT    |
| OpenStream same device twice                  | `test_negative_plugin_rpcs` → ALREADY_EXISTS      |
| NotifyStreamStall device disconnected         | `test_negative_plugin_rpcs` → recoverable=false   |
| Concurrent register/unregister (4 threads)    | `test_stream_liveness_monitor` stress extension   |
| `elapsedText()` at 59:59 → 1:00:00 boundary   | `test_app_controller` boundary test               |
| `elapsedText()` at exactly 3600 seconds        | `test_app_controller` boundary test               |
| SHM open with existing name                   | `test_shared_memory_backend` → re-open succeeds   |
| SHM unlink of non-existent name               | `test_shared_memory_backend` → no crash           |
| Windows SHM cleanup on process exit           | `test_shared_memory_backend` destructor test       |
| `hasH264Encoder()` returns true on Linux+NVENC | `test_ffmpeg_plugin_server` with mock encoder list |

## Rollback Plan

This change is on feature branch `feat/006-cross-platform-compat` branched from `plugin-system`. Rollback strategy:

1. **Phase 1 (SHM abstraction)**: Revert to direct POSIX calls. The abstraction is a passthrough — removing it restores byte-identical behavior. No data format change.
2. **Phase 2 (Type cleanup)**: Restore duplicate `PayloadKind` in `RecordingPipeline.h`. Restore `"posix_shm"` literals. Restore uninitialized member. These are all mechanical reverts.
3. **Phase 3 (Behavioral fixes)**: Revert to current behavior: `supports_h264=false`, silent Calibrate defaults, no double-open guard, always-recoverable NotifyStall. No data migration, no persistent state.
4. **Phase 4 (UI fixes)**: Revert QML bindings to hardcoded strings. Revert `elapsedText()` to MM:SS. Revert `preflightItems()` to empty list. No persistent state.
5. **Phase 5 (Tech debt)**: `git checkout HEAD -- old/` restores `old/` tracking. `git checkout HEAD -- api/micecam/ICameraBackend.h` restores includes. Revert buffer allocation in `FFmpegCameraBackend.cpp`.
6. **Phase 6 (Tests)**: Delete new test files. Revert test extensions. No production impact.
7. **Full revert**: `git revert` the entire branch merge. No schema change, no proto change, no data migration, no persistent state to clean up.

**Critical: No proto changes in this spec.** The ring buffer wire format is unchanged. Cross-process compatibility is preserved — the `SharedMemoryBackend` abstraction only changes how memory is opened/mapped, not the data layout within it.

## Complexity Tracking

| Field     | Value      |
|-----------|------------|
| Estimated | **Medium** |
| Rationale | Touches 5 DDD layers but no proto changes, no new gRPC RPCs, no new background threads. The SHM abstraction is a straightforward Strategy pattern with ~200 LOC total. Behavioral fixes are localized to individual gRPC method bodies. UI fixes are binding-level changes. Type cleanup is mechanical. The main risk is the SHM abstraction correctness across 3 platforms, mitigated by identical ring buffer layout and `static_assert` guards. |

## Phase Summary

| Phase | Description                                  | Files Changed | New Files | Risk   |
|-------|----------------------------------------------|---------------|-----------|--------|
| 1     | SharedMemoryBackend abstraction              | 4 + CMake     | 4         | High   |
| 2     | Type cleanup (PayloadKind, init, transport)  | 5             | 0         | Low    |
| 3     | Plugin behavioral fixes                      | 4             | 0         | Medium |
| 4     | UI fixes                                     | 4             | 0         | Low    |
| 5     | Tech debt (old/, API hygiene, buffer)        | 3 + .gitignore | 0         | Low    |
| 6     | Test gap closure                             | 4 (extend) + CMake | 2    | Low    |
| 7     | CI + final verification                      | 0             | 0         | Low    |

**Total new files**: ~6 (4 SHM impl + 2 test files)
**Total modified files**: ~20 (4 infra + 5 pipeline+domain refs + 4 cmd/plugins + 4 cmd/ui + .gitignore + 2 CMake + test extensions)
