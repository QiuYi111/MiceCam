# Worker Report

## Task summary

Fix behavioral anomalies in FFmpeg plugin server: capability consistency (FR-003), NotifyStreamStall device health check (FR-004), OpenStream double-open guard (FR-010), Calibrate input validation (FR-011), and Windows signal handler (FR-016).

## What was done

- **FR-003**: Added `hasH264Encoder()` private method that checks actual encoder availability via `avcodec_find_encoder_by_name` for h264_videotoolbox, h264_nvenc, h264_qsv, h264_vaapi, libx264. Result is cached after first call. Replaced `#ifdef __APPLE__` guards in `EnumerateDevices()` and `buildCapabilityInfo()` with runtime `hasH264Encoder()` calls.
- **FR-004**: Added `isDeviceAccessible()` private method that checks filesystem path existence via `access(F_OK)` and falls back to cached device list. Updated `NotifyStreamStall()` to check device accessibility before returning recoverable/retry vs device_lost.
- **FR-010**: Added double-open guard in `OpenStream()` that checks for existing streams with same device_id AND same payload_kind, returning `ALREADY_EXISTS` on duplicate.
- **FR-011**: Added input validation at top of `Calibrate()` rejecting non-positive width/height/fps with `INVALID_ARGUMENT`. Removed silent default fallback (1920x1080@30) for invalid params.
- **FR-016**: Added `#ifdef _WIN32` blocks with `console_ctrl_handler` and `SetConsoleCtrlHandler` to both `micecam_ffmpeg/main.cpp` and `micecam_oak/main.cpp`.

## Changed files

- `cmd/plugins/micecam_ffmpeg/FFmpegPluginServer.cpp` — FR-003, FR-004, FR-010, FR-011 behavioral fixes
- `cmd/plugins/micecam_ffmpeg/FFmpegPluginServer.h` — Added `hasH264Encoder()`, `isDeviceAccessible()`, cache members
- `cmd/plugins/micecam_ffmpeg/main.cpp` — FR-016 Windows signal handler
- `cmd/plugins/micecam_oak/main.cpp` — FR-016 Windows signal handler

## Commands run

| Command | Result |
|---------|--------|
| `cmake --build build -j 4` | PASS (0 errors, 0 warnings) |
| `ctest --test-dir build --output-on-failure --exclude-regex '.*hil.*\|.*stress.*'` | PASS (39/39) |
| `grep -n "hasH264Encoder" FFmpegPluginServer.cpp` | Found at lines 108, 437, 479 |
| `grep -n "isDeviceAccessible" FFmpegPluginServer.cpp` | Found at lines 496, 692 |
| `grep -n "ALREADY_EXISTS" FFmpegPluginServer.cpp` | Found at line 274 |
| `grep -n "INVALID_ARGUMENT" FFmpegPluginServer.cpp` | Found at line 575 |
| `grep -n "SetConsoleCtrlHandler" main.cpp (both)` | Found in both files |
| `grep -rn "#ifdef __APPLE__" FFmpegPluginServer.cpp` | 1 remaining: line 506 in `find_encoder_for_calibration` (encoder selection, NOT capability-related) |

## Test results

All 39/39 tests pass. No test files were modified.

## Harness results

- **Risk classification**: leaf (modifying plugin server behavior within existing gRPC methods)
- **TDD**: Not required per task specification
- **Build**: Clean, no warnings
- **Tests**: 39/39 pass

## Acceptance criteria checklist

- [x] 1. `FFmpegPluginServer::hasH264Encoder()` exists and checks actual encoder availability
- [x] 2. `GetCapabilities.supports_h264` is set based on `hasH264Encoder()` result (no `#ifdef __APPLE__` guard)
- [x] 3. `EnumerateDevices` H264 payload uses `hasH264Encoder()` (no `#ifdef __APPLE__` guard)
- [x] 4. `NotifyStreamStall` checks `isDeviceAccessible()` before returning `recoverable=true`
- [x] 5. `OpenStream` returns `ALREADY_EXISTS` when called twice for same device+payload
- [x] 6. `Calibrate` returns `INVALID_ARGUMENT` for non-positive width/height/fps
- [x] 7. `main.cpp` for both plugins has `#ifdef _WIN32` signal handler block
- [x] 8. `cmake --build build -j 4` succeeds on macOS
- [x] 9. `ctest` — all existing tests pass (39/39)
- [x] 10. No `#ifdef __APPLE__` guards remain for capability-related code (1 remains in `find_encoder_for_calibration` which is encoder selection, not capability reporting)

## Problems encountered

- **FR-010 scope conflict**: Task spec says to guard on `device_id` only, but `test_dual_path_keyframe` legitimately opens two streams for the same device with different payloads (H264 + RAW). Guarding on device_id alone would break dual-path support. Resolved by guarding on `device_id + payload_kind` combination, which prevents true duplicate streams while allowing multi-path scenarios.
- **FR-004 device accessibility**: `isDeviceAccessible()` uses `access(F_OK)` for filesystem path check, but synthetic cameras and AVFoundation devices don't have filesystem paths. Added fallback to cached device list check.

## Deviations from task

1. **FR-010 double-open guard**: Task spec checks only `device_id`. Implementation checks `device_id + payload_kind` to preserve dual-path (multi-stream) capability. Rationale: same device with different payloads is not a "silent duplicate" — it's an intentional multi-path design.
2. **FR-004 NotifyStreamStall action string**: Task spec says `action="retry"`, but existing test expects `action="retrying"`. Kept `"retrying"` to match existing test expectations. Test files are forbidden from modification.

## Remaining work

None. All acceptance criteria met.

## Suggested next step

Commit Phase 2 changes and proceed to next phase in the task pipeline.

## Evidence

```
Build output: 0 errors, 0 warnings
Test output: 39/39 passed

git diff stats (plugin files only):
 cmd/plugins/micecam_ffmpeg/FFmpegPluginServer.cpp | 74 +++++++++++++++++++----
 cmd/plugins/micecam_ffmpeg/FFmpegPluginServer.h   |  5 ++
 cmd/plugins/micecam_ffmpeg/main.cpp               | 19 ++++++
 cmd/plugins/micecam_oak/main.cpp                  | 19 ++++++
 4 files changed, 105 insertions(+), 12 deletions(-)
```
