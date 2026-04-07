# Phase Report: Native App USB Enumeration Unification

**Date**: 2026-03-20 12:00
**Scope**: Native app USB camera selection reliability on Windows test machines
**Branch**: `codex/fix-usb-dshow-enumeration`

## Summary

This change removes the split-brain USB camera discovery path where the UI enumerated cameras through Qt while the worker reopened them through FFmpeg/dshow.

The native app now uses the FFmpeg backend as the source of truth for USB camera inventory, and the selected backend-native device name is passed from UI to worker at recording start.

## Implemented Changes

- Added FFmpeg-backed USB inventory discovery through `FFmpegCameraBackend::enumerate_video_devices()`.
- Updated the native UI inventory path to use shared FFmpeg device enumeration for USB cameras.
- Added a backend-native device name field to the app-facing camera descriptor and recording request contract.
- Passed the selected FFmpeg device name through `WorkerProcessRuntime` into `NativeWorkerRuntime` and then into `CameraConfig`.
- Updated the Windows FFmpeg open path to prefer the explicit device name and fall back to index resolution only when needed.
- Added a manual Refresh button in the QML setup rail.
- Preserved camera selection across inventory refresh by matching backend identity instead of blindly reusing the old index.

## Why This Was Needed

Some test machines could display a USB camera in the UI but fail to start recording with FFmpeg because the worker re-enumerated dshow devices independently and could not reliably recover the same camera by index.

That issue is machine- and camera-specific because Windows driver exposure, FFmpeg enumeration order, and process-local device visibility can differ across systems.

## Verification

- Configured a fresh Visual Studio CMake build in `build-codex-usb-enum` with `MICECAM_BUILD_TESTS=ON`.
- Built `micecam_ui` successfully in Debug.
- Built `micecam_tests.exe` successfully in Debug.
- Automated test execution is still blocked in this environment by Windows runtime DLL discovery during GoogleTest auto-discovery (`0xc0000135`), so end-to-end test execution could not be completed here.

## Residual Risks

- FFmpeg device descriptions can still vary across driver stacks, so logs should continue to include the exact resolved device target for field debugging.
- Capability discovery for FFmpeg USB devices still uses the current fallback table rather than per-device FFmpeg capability probing.
- Manual refresh improves operator control, but hot-plug auto-refresh for FFmpeg devices is still not implemented.
