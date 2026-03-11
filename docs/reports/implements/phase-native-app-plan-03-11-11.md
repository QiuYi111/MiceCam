# Implementation Report: Native App Integration Planning and Architecture Docs

**Date**: 2026-03-11
**Branch**: `codex/native-app-integration-plan`

## Overview

Added the missing planning and architecture documents needed to move the redesigned QML UI toward a production-ready native application.

This pass is documentation-first and turns the current repository analysis into formal requirements, a staged execution plan, and a runtime architecture explanation grounded in both the native QML branch and the legacy Python application.

This document set was later extended in the same branch with an explicit architecture decision selecting a native worker-process runtime for maximum recording stability and recovery support.

## Deliverables

- Added `docs/requirements/native-app-production-integration.md`
  - product goal
  - functional and non-functional requirements
  - acceptance criteria
  - explicit preview and runtime isolation requirements
- Added `docs/plan/phase-native-app-integration.md`
  - phased implementation roadmap
  - verification goals
  - exit criteria per phase
- Added `docs/wikis/native-app-runtime-architecture.md`
  - current native runtime analysis
  - legacy Python runtime comparison
  - preview versus recording performance strategy
  - recommended target architecture
- Updated `project_index`
  - added the new requirements, plan, and architecture docs
  - removed the outdated note claiming those directories do not exist

## Key Decisions Captured

- The native app should no longer be treated as a visual refactor only; it needs a runtime integration phase.
- The Python app's worker isolation and preview budget rules are critical architectural lessons and should not be discarded during the QML migration.
- Preview must be explicitly modeled as best-effort and bounded, rather than allowed to compete with recording implicitly.
- The team should freeze a runtime architecture decision early instead of continuing to accumulate responsibilities inside `PipelineController`.

## Verification

- Cross-checked the new documents against:
  - `cmd/micecam_ui/main.cpp`
  - `cmd/micecam_ui/PipelineController.h`
  - `cmd/micecam_ui/PipelineController.cpp`
  - `cmd/micecam_ui/VideoFrameProvider.cpp`
  - `cmd/gui/gui/main_window.py`
  - `cmd/gui/gui/recorder_thread.py`
  - `cmd/gui/recorder_worker.py`
  - `bindings/python/module.cpp`

## Notes

- No automated tests were run because this task adds planning and architecture documentation only.
- No code changes were made to the capture or UI runtime in this pass.
