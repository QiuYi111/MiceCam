# Implementation Report: Native App Phase 1 Device and Preflight Integration

**Date**: 2026-03-11
**Branch**: `codex/native-app-phase1`

## Overview

Implemented the first execution slice of the native app integration plan for `cmd/micecam_ui`.

This pass moves the native UI from QML-side form inference to controller-owned inventory, capability, and preflight logic. It does not yet introduce the worker-process supervisor from the accepted ADR, but it removes several Phase 1 blockers so the UI can reason about real setup state through stable app-facing interfaces.

## Changes

- Added typed native app setup helpers:
  - `cmd/micecam_ui/RecordingSetup.h`
  - `cmd/micecam_ui/RecordingSetup.cpp`
- Added a typed camera inventory model for QML:
  - `cmd/micecam_ui/CameraInventoryModel.h`
  - `cmd/micecam_ui/CameraInventoryModel.cpp`
- Refactored `PipelineController` to own:
  - camera inventory refresh
  - selected device / resolution / FPS state
  - controller-side preflight validation
  - sanitized session naming
  - capability-driven resolution and FPS exposure
  - startup-time and hot-plug inventory refresh via `QMediaDevices`
- Updated `cmd/micecam_ui/qml/main.qml` to consume:
  - typed camera model
  - controller-owned capabilities
  - controller-owned readiness messaging
  - FPS selection from supported values instead of free-form entry
- Added native app unit coverage in `tests/ui/native_app_preflight_test.cpp`.
- Repaired existing C++ test/build drift so the full `micecam_tests` target works with the repository's current JSONL metadata contract.
- Isolated the repository's own test toggle from third-party subprojects by introducing `MICECAM_BUILD_TESTS`.

## Verification

- Configured with:
  - `cmake -S . -B build -DMICECAM_BUILD_TESTS=ON`
- Built native UI:
  - `cmake --build build --target micecam_ui -j4`
- Built tests:
  - `cmake --build build --target micecam_tests -j4`
- Ran focused native app tests:
  - `./build/micecam_tests --gtest_filter='NativeAppPreflightTest.*:CameraInventoryModelTest.*'`
- Ran full discovered test suite:
  - `ctest --test-dir build --output-on-failure`
- Ran UI smoke command:
  - `./build/micecam_ui --help`

## Remaining Gaps

- Recording still runs in-process; Phase 2 still needs the supervisor/worker split required by `docs/adr/0001-native-worker-process-runtime.md`.
- The activity feed is still string-based rather than a structured event model.
- Preview budget policy is still implicit in `VideoFrameProvider`, not yet exposed as an explicit runtime contract.
