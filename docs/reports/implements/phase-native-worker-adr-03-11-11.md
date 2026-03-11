# Implementation Report: Native Worker Runtime Decision

**Date**: 2026-03-11
**Branch**: `codex/native-app-integration-plan`

## Overview

Captured the runtime architecture decision for the native app: recording will move to a native worker-process model supervised by the QML desktop application.

This update changes the planning baseline from "evaluate both runtime directions" to "execute the worker-process architecture" because the product priority is now explicit: recording stability and recovery behavior outrank preview quality and UI simplicity.

## Deliverables

- Added `docs/adr/0001-native-worker-process-runtime.md`
  - decision context
  - chosen architecture
  - consequences and rejected alternative
  - required design implications for preview, recovery, IPC, and packaging
- Updated `docs/requirements/native-app-production-integration.md`
  - made process isolation mandatory
  - added worker crash and recovery expectations to acceptance criteria
- Updated `docs/plan/phase-native-app-integration.md`
  - replaced open runtime choice with explicit worker-runtime contract work
  - expanded lifecycle hardening around launch, heartbeat, crash, and recovery
- Updated `docs/wikis/native-app-runtime-architecture.md`
  - replaced dual-option runtime section with the chosen direction
- Updated `project_index`
  - added ADR directory and worker-runtime ADR reference

## Key Decision

- The QML app will become a supervisor UI.
- Recording-critical runtime work will execute in a separate native worker process.
- Preview is formally treated as a best-effort side channel that may degrade or stop before recording is allowed to destabilize.

## Verification

- Cross-checked the ADR and updated documents against:
  - `cmd/micecam_ui/main.cpp`
  - `cmd/micecam_ui/PipelineController.cpp`
  - `cmd/micecam_ui/VideoFrameProvider.cpp`
  - `cmd/gui/gui/recorder_thread.py`
  - `cmd/gui/recorder_worker.py`

## Notes

- No automated tests were run because this update is documentation-only.
- This decision should be followed by an IPC contract document and acceptance tests for worker crash and recovery behavior.
