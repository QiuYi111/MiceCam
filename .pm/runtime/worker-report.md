# Worker Report

## Task summary
Extended `DeviceInfo`/`StreamInfo`/`Capabilities` and `ICameraBackend` with UI-ready fields (resolution options, labels, availability) and implemented per-stream capability lookup in all three camera backends.

## What was done
- Added `ResolutionOption` struct with `width`, `height`, `label` to `DeviceInfo.h`
- Extended `StreamInfo` with `label`, `resolutions` (vector of `ResolutionOption`), `available`, `unavailable_reason`
- Added `ICameraBackend::get_capabilities(device_id, stream_index)` overload with default impl of delegating to zero-arg variant
- Declared override in `MockCameraBackend.h`
- Rewrote `MockCameraBackend::enumerate_devices()` to return 1 mock device with 5 labeled streams (CAM_A..CAM_D, USB-1), each with 3 resolution options, rgb24 format, 15/30/60 fps
- Added `MockCameraBackend::open_stream()` validation: rejects non-mock device IDs
- Implemented `MockCameraBackend::get_capabilities(device_id, stream_index)` returning per-stream resolution/formats/fps
- Populated `StreamInfo` label/resolutions/available fields in OAKCameraBackend (both WITH_DEPTHAI and stub branches)
- Populated `StreamInfo` label/resolutions/available fields in FFmpegCameraBackend (enumerate_devices and get_capabilities)
- Created `tests/unit/test_backend_ui_contract.cpp` with 2 tests (RED→GREEN flow)
- Registered `test_backend_ui_contract` in `CMakeLists.txt`
- Verified all 20 tests pass (100%)

## Changed files
- `internal/domain/DeviceInfo.h` — added `ResolutionOption`, extended `StreamInfo`
- `api/micecam/ICameraBackend.h` — added 2-arg `get_capabilities` overload
- `internal/infrastructure/MockCameraBackend.h` — declared override
- `internal/infrastructure/MockCameraBackend.cpp` — 5-stream mock, per-stream caps, device ID validation
- `internal/infrastructure/OAKCameraBackend.cpp` — populated UI fields in both code paths
- `internal/infrastructure/FFmpegCameraBackend.cpp` — populated UI fields
- `tests/unit/test_backend_ui_contract.cpp` — new test file (2 tests)
- `CMakeLists.txt` — registered new test target

## Commands run
| Command | Result |
|---|---|
| `cmake -B build -S . -DBUILD_UI=ON` | Configured OK |
| `cmake --build build --target test_backend_ui_contract -j` (RED) | 4 compile errors as expected |
| `cmake --build build -j` (GREEN) | Built all targets OK |
| `build/tests/test_backend_ui_contract` | 2/2 tests passed |
| `ctest --test-dir build --output-on-failure -j4` | 20/20 tests passed (100%) |

## Test results
```
[==========] Running 2 tests from 1 test suite.
[----------] 2 tests from BackendUiContract
[ RUN      ] BackendUiContract.MockDiscoveryProvidesUiReadyCameraRows
[       OK ] BackendUiContract.MockDiscoveryProvidesUiReadyCameraRows (0 ms)
[ RUN      ] BackendUiContract.MockCapabilitiesArePerStreamAndSelectable
[       OK ] BackendUiContract.MockCapabilitiesArePerStreamAndSelectable (0 ms)
[----------] 2 tests from BackendUiContract (0 ms total)
[==========] 2 tests from 1 test suite ran. (0 ms total)
[  PASSED  ] 2 tests.
```

Full suite: **20/20 tests passed (100%)**, no regressions.

## Harness results
- Risk classification: **BRANCH** — multi-file changes across domain types, API interfaces, backend implementations, and tests
- TDD RED→GREEN flow executed:
  - RED: 4 compile errors confirmed (missing `label`, `resolutions`, `available` fields; missing 2-arg `get_capabilities`)
  - GREEN: All 4 errors resolved, 2 new tests pass, 0 regressions

## Acceptance criteria checklist
- [x] `test_backend_ui_contract.cpp` compiles and all tests pass
- [x] `MockDiscoveryProvidesUiReadyCameraRows` passes: devices have `id`, `name`, `type`, `streams[].label`, `resolutions`, `supported_framerates`, `supported_formats`, `available`
- [x] `MockCapabilitiesArePerStreamAndSelectable` passes: per-stream capability lookup returns >=2 resolutions, >=2 framerates, >=1 format, non-empty encoder name
- [x] Mock `enumerate_devices()` returns 5 streams with distinct labels (CAM_A, CAM_B, CAM_C, CAM_D, USB-1)
- [x] `open_stream()` accepts mock stream IDs (validates `mock_cam_` prefix)
- [x] No existing tests break (20/20 pass)
- [x] `cmake --build build -j` (including micecam_ui) builds successfully
- [x] Worker report written

## Problems encountered
None.

## Deviations from task
- `open_stream()` validates `mock_cam_` prefix rather than exact match on `mock_cam_0`..`mock_cam_4` to maintain backward compatibility with existing tests that pass `"mock_cam_0"` as device ID.
- OAK and FFmpeg backends populate `label` from hardware context (CAM_A..CAM_D for OAK, device name for FFmpeg) and set `available=true` unconditionally (real availability detection is a future concern).

## Remaining work
None for this task.

## Suggested next step
Task 2/8: Wire Qt models and QML views to the UI contract defined here.

## Evidence
- Git diff: 11 files changed, 302 insertions(+), 121 deletions(-) (including worker report)
- Build: all targets including `micecam_ui` build clean with no errors
- Tests: 20/20 pass (100%)
- CTest output confirming full suite pass (10.61s total)
