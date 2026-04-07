# Windows Development Pitfalls

This page records the concrete Windows issues we already tripped over while stabilizing capture, preview, and packaging.

## 1. USB camera identity must match between UI and worker

Problem:

- Qt can enumerate a USB camera in the UI
- FFmpeg/dshow in the worker may enumerate a different order, or no device at all

Symptom:

- Works on one machine, fails on another
- Worker logs `No Windows dshow video devices found`
- Camera opens by index on one rig but not on test machines

Rule:

- On Windows, use the same FFmpeg-backed enumeration path in the UI and the worker
- Pass a backend-native device identity, not only a UI index

## 2. Do not write worker status files to relative paths from Program Files

Problem:

- Packaged worker used `recorder_status.json` in its current working directory
- Installed app runs from `Program Files`
- Windows denies write/replace operations there

Symptom:

- `WinError 5`
- `recorder_status.json.tmp -> recorder_status.json`
- Worker records frames successfully, then exits with code `1`

Rule:

- Write mutable runtime files to a temp or user-writable location
- Status publishing must never be fatal to recording

## 3. Native preview can be "connected" but still stay blank

Problem:

- Python worker attaches a callback
- UI listens for `PREVIEW_UPDATE`
- But `IngestionPipeline` never dispatches frames to observers

Symptom:

- Recording works
- Preview tiles remain blank forever
- No `PREVIEW_UPDATE:` lines appear even though preview is enabled

Rule:

- If preview is callback-driven, verify `IngestionPipeline` actually calls `dispatcher_.dispatch(...)`

## 4. This machine's default full native configure can fail on XLink

Problem:

- Full configure against the `depthai-core` submodule may fail to resolve `XLink`

Symptom:

- CMake configure fails with missing `XLinkConfig.cmake`

Rule:

- For local Windows validation on this machine, prefer `-DMICECAM_USE_PREBUILT_DEPTHAI=ON`
- Keep the prebuilt DepthAI package available under `3rdParty/depthai-core-v3.4.0-win64`

## 5. `.venv` Python may be runnable but still unusable for CMake bindings

Problem:

- `.venv\Scripts\python.exe` exists
- But CMake cannot find `Development`, `Development.Module`, or `Development.Embed`

Symptom:

- `Could NOT find Python3 (missing: Development Development.Module Development.Embed)`

Rule:

- For binding builds, prefer the full `uv` CPython install:
  - `C:/Users/<user>/AppData/Roaming/uv/python/cpython-3.14.0-windows-x86_64-none/`

## 6. PyInstaller does not reliably bring in DepthAI runtime DLLs

Problem:

- `_micecam.cp314-win_amd64.pyd` depends on `depthai-core.dll`
- PyInstaller may warn but still finish

Symptom:

- Packaged app builds
- OAK runtime fails later on clean machines
- Build warnings mention unresolved `depthai-core.dll`

Rule:

- After PyInstaller, verify these DLLs exist in both app and worker staging directories:
  - `depthai-core.dll`
  - `dynamic_calibration.dll`

## 7. GoogleTest build success is not the same as runnable test discovery

Problem:

- `micecam_tests.exe` can compile
- CMake test discovery may still fail before tests actually run

Symptom:

- `0xc0000135` during `GoogleTestAddTests.cmake`

Rule:

- Separate "compile succeeded" from "tests executed"
- If discovery fails, report it explicitly as a runtime environment issue

## 8. Session name time and recorded start time are not necessarily the same

Problem:

- Session name can be created before recording really starts
- `start_timestamp_ns` now comes from `system_clock` at actual `DiskWriter::start()`

Symptom:

- `session_YYYYMMDD_HHMMSS` differs from JSONL `start_timestamp_ns` by several seconds

Rule:

- Treat JSONL `start_timestamp_ns` as the source of truth for actual recording start time
- Only use session name as a human-friendly label unless the generation point is tightened
