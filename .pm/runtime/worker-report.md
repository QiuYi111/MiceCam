# Worker Report: Phase 7 — FFmpeg/OAK Calibrate, CI, fMP4 Smoke Test

## Task

Implement spec 004 Phase 7 (final): FFmpeg/OAK plugin Calibrate RPC, three-platform CI, and fMP4 smoke test.

## Risk Classification

**Branch** — touches plugin RPC implementations and test infrastructure. No core domain or infra changes.

## Changes

### 1. FFmpeg Plugin: Real Calibrate RPC

**File**: `cmd/plugins/micecam_ffmpeg/FFmpegPluginServer.cpp`

- Replaced stub `Calibrate()` with real encoder-based calibration
- Uses `find_encoder_for_calibration()` — same hw detect + fallback logic as recording path
- Encodes test frames (solid YUV420P gray) for `calibration_duration_ms` (default 3000ms)
- Measures I-frame and P-frame latency via `steady_clock`
- Computes `max_sustainable_fps = 1e9 / max(I, P)`
- Computes `recommended_slot_size = max_encoded_frame_size * 1.5`
- Returns all fields: `i_frame_latency_ns`, `p_frame_latency_ns`, `max_sustainable_fps`, `recommended_slot_size`, `actual_encoder_name`, `actual_width`, `actual_height`

### 2. OAK Plugin: Placeholder Calibrate RPC

**File**: `cmd/plugins/micecam_oak/OAKPluginServer.cpp`

- Returns conservative estimates: I=2ms, P=0.5ms, max_fps=30.0
- `recommended_slot_size = width * height * 3/2` (NV12 estimate)
- `actual_encoder_name = "depthai_h264"`
- Includes warning: "Calibration estimated -- no hardware available for testing"

### 3. Three-Platform CI

**File**: `.github/workflows/ci.yml` (new)

- Three jobs: `build-macos`, `build-windows`, `build-linux`
- macOS: Homebrew deps (ffmpeg, protobuf, grpc, spdlog, nlohmann-json, googletest)
- Windows: vcpkg deps, `BUILD_UI=OFF`
- Linux: apt-get deps, `BUILD_UI=OFF`
- All exclude HIL and stress tests via `--exclude-regex`

### 4. fMP4 Smoke Test

**File**: `tests/integration/test_fmp4_smoke.cpp` (new)

- Two tests:
  1. `FileReadableWithoutTrailer` — writes fMP4 with `+frag_keyframe+empty_moov+default_base_moov`, encodes 15 frames via libx264, writes with trailer, verifies readable, frame count matches
  2. `FileReadableAfterCrashNoTrailer` — same setup but closes WITHOUT `av_write_trailer` (crash simulation), verifies file is openable with valid H264 stream descriptor and dimensions

### 5. Test Updates

**File**: `tests/unit/test_ffmpeg_plugin_server.cpp`

- `CalibrateReturnsNotImplemented` → now tests real calibration output
- Uses small resolution (320x240) and 500ms duration for speed
- Verifies: `success=true`, non-zero latencies, positive fps, non-empty encoder name, correct dimensions

**File**: `tests/unit/test_oak_plugin_server.cpp`

- `CalibrateReturnsNotImplemented` → `CalibrateReturnsPlaceholderValues`
- Verifies exact placeholder values and warning message contains "estimated"

### 6. CMakeLists.txt

- Added `add_micecam_test(test_fmp4_smoke ...)` to integration test list

## Verification

```
cmake --build build -j 4  # SUCCESS (0 errors, 0 warnings in changed files)
ctest --test-dir build --output-on-failure  # 37/37 passed
```

## Acceptance Criteria

- [x] FFmpeg Calibrate RPC returns actual encoder latencies from test encoding
- [x] OAK Calibrate RPC returns placeholder values with warning
- [x] CI YAML has three jobs: macOS, Windows, Linux
- [x] Windows CI uses BUILD_UI=OFF
- [x] CI excludes HIL and stress tests
- [x] fMP4 smoke test passes: file readable without trailer
- [x] `cmake --build build -j 4` succeeds
- [x] `ctest --test-dir build --output-on-failure` passes (37/37)
- [x] At least 2 new/modified tests (3 new/modified test files)

## Blockers

None.
