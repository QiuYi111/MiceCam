# Implementation Report: Native App Phase 2+ Supervisor Integration

**Date**: 2026-03-11
**Branch**: `codex/phase2-native-app-integration`

## Overview

Implemented the Phase 2 supervisor lifecycle hardening slice for `cmd/micecam_ui`, and carried adjacent Phase 4/5 work needed to make the native app usable as an integrated application rather than a visual shell.

The native app now launches a headless worker mode of `micecam_ui`, supervises it from the UI process, maps worker lifecycle transitions into QML-facing state, and exposes structured recent activity plus output-opening actions.

## Changes

- Added `RecordingSupervisorService` to centralize:
  - lifecycle transitions
  - stop / decode / close semantics
  - worker exit and error mapping
  - automatic worker relaunch after unexpected exit when safe
  - automatic session resume attempt by replaying the last start request after worker relaunch
  - structured activity accumulation
- Added `ActivityEventModel` for typed operator-facing recent activity.
- Added `WorkerProcessRuntime` to supervise a worker process over JSONL IPC.
- Added `NativeWorkerRuntime` and wired `main.cpp --worker` to run the recording runtime in a headless process.
- Added worker-side preview IPC with:
  - 5 FPS budget
  - latest-frame-only semantics
  - downscaled JPEG transport into the UI image provider
- Refactored `PipelineController` into a thinner QML adapter that now owns:
  - camera inventory refresh
  - capture setup and preflight
  - runtime request assembly
  - supervisor state projection into QML properties
- Updated QML workflow behavior:
  - app close is blocked while the worker is still stopping/decoding
  - completed/error states expose an output-open action
  - recent activity is now backed by supervisor-generated structured events
- Added acceptance-style app lifecycle tests in `tests/ui/recording_supervisor_service_test.cpp`.
  - includes worker relaunch success/failure paths after crash
  - includes session resume failure fallback after worker restart
- Updated `project_index` and `docs/wikis/native-app-runtime-architecture.md` to reflect the new runtime shape.
- Added `scripts/smoke_native_app.sh` and `docs/wikis/native-app-release-checklist.md` as the initial native app release gate artifacts.

## Verification

- Built tests:
  - `cmake --build build --target micecam_tests -j4`
- Built native app:
  - `cmake --build build --target micecam_ui -j4`
- Ran focused native app tests:
  - `./build/micecam_tests --gtest_filter='NativeAppPreflightTest.*:CameraInventoryModelTest.*:RecordingSupervisorServiceTest.*'`
- Ran full discovered test suite:
  - `ctest --test-dir build --output-on-failure`
- Ran lint gate:
  - `make lint`
- Smoke-tested worker mode:
  - `printf '{"type":"shutdown"}\n' | ./build/micecam_ui --worker`
  - `scripts/smoke_native_app.sh`

## Review Notes

Independent post-implementation review focused on:

- supervisor state coverage against `docs/plan/phase-native-app-integration.md`
- failure mapping for launch, stop, decode, and unexpected worker exit
- close behavior while decode is active
- build/test/lint regression risk in the native app path

No blocking issues were found after the final verification pass.

## Remaining Gaps

- Preview frames now cross the worker boundary through a capped latest-frame JPEG path, but the preview channel is still intentionally best-effort rather than recording-critical.
- Automatic recovery now attempts worker relaunch and session restart, but it does not resume partial captured data from the exact interruption point.
- Release packaging validation still needs a packaged-app launch check rather than only an in-tree executable smoke test.
