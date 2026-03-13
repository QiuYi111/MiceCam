# Implementation Report: Windows OAK Runtime Isolation

**Date**: 2026-03-13 15:00
**Branch**: `codex/minimize-oak-invalid-key`
**Scope**: isolate Windows OAK runtime behavior, verify the prebuilt DepthAI path, and record the backend boundary needed for native app work

## Summary

This work established that the supplied Windows prebuilt DepthAI package can drive the OAK hardware successfully, but the project's in-process backend integration was unstable until the runtime boundary was simplified.

The most important outcome is architectural rather than cosmetic:

- the OAK path should be isolated behind a dedicated runtime boundary

## What Changed

### Build system

- Added prebuilt-DepthAI support to `CMakeLists.txt`.
- Added optional runtime DLL deployment for prebuilt DepthAI targets.
- Split OAK code away from the generic camera backend path.
- Introduced:
  - `micecam_oak_runtime`
  - `micecam_oak`

### OAK runtime experiments

- Added standalone probes:
  - `cmd/examples/oak_backend_inline_probe.cpp`
  - `cmd/examples/oak_backend_probe.cpp`
  - `cmd/examples/oak_runtime_probe.cpp`
- Added isolated runtime session code:
  - `internal/infrastructure/oak_runtime_session.h`
  - `internal/infrastructure/oak_runtime_session.cpp`
- Reworked `internal/infrastructure/oak_camera_backend.cpp` into a thin adapter over the runtime session.

### Windows stability fixes

- Fixed the backend initialization path so it does not unconditionally `stop()` before first initialization.
- Fixed Windows file finalization so unbuffered writes are followed by a buffered final truncate to the exact logical size.

## Key Findings

### Verified good path

The following worked through standalone probe programs:

- OAK device boot
- four camera enumeration
- synchronized quad MJPEG message-group production

### Verified historical bad path

Observed failures during debugging included:

- `invalid unordered_map<K, T> key`
- `Invalid Member Count (buildPipeline)`
- `Two nodes with same ID were tried to be placed onto pipeline`
- `Couldn't open stream`
- `Failed to find device after booting`
- `X_LINK_DEVICE_NOT_FOUND`

### Root cause conclusion

The original `Invalid Member Count (buildPipeline)` failure was not caused by OAK hardware or by the pipeline shape alone.

It was caused by the integration context around the backend compilation unit.

Reducing project header exposure and ensuring that DepthAI headers were included before project headers removed that specific failure mode.

The later `Couldn't open stream` failure in the backend adapter path had a narrower cause:

- `OAKCameraBackend::initialize()` called `stop()` unconditionally, even during first-time initialization
- on Windows with the verified prebuilt DepthAI runtime, that premature stop path could poison the first stream open

Changing the adapter to stop only when an existing session or running distributor thread is present removed that failure.

The final Windows recording shutdown error:

- `Error truncating file padding: 87`

was caused by trying to truncate a `FILE_FLAG_NO_BUFFERING` handle to a non-sector-aligned logical size. The fix was to close the unbuffered handle first and then reopen the file with a normal handle for the final precise truncate.

## Verification

Validated manually with these executable targets:

- `oak_diagnostic`
- `oak_backend_inline_probe`
- `oak_backend_probe`
- `oak_runtime_probe`
- `oak_quad_recorder`

Important result:

- `oak_backend_probe` and `oak_runtime_probe` succeeded sequentially
- `oak_quad_recorder --duration 1/2` succeeded and produced four output streams
- the Windows file finalization warning disappeared after the truncate fix

## Risks / Follow-up

- The Windows USB / boot path can still become unhealthy after failed runs, so startup health checks and recovery handling are still needed.
- Native app work can resume more safely now, but only if it consumes the isolated OAK runtime boundary rather than rebuilding the old mixed backend shape.
- Build output folders, downloaded OpenCV artifacts, and local trace files should stay out of normal commits.
