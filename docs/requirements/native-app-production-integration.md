# Product Requirements: Native App Production Integration

**Date**: 2026-03-11
**Status**: Draft
**Target**: `cmd/micecam_ui`
**Related Branch**: `codex/native-app-integration-plan`

## 1. Background

The current native Qt/QML application already delivers a strong visual redesign, but it is not yet a production-ready desktop application.

The main gaps observed in the current branch are:

- UI state and runtime orchestration are still concentrated inside `PipelineController`.
- Camera selection is visible in QML, but the device/capability model is still too thin for production use.
- Readiness validation is mostly UI-side form validation, not a true preflight contract.
- Runtime isolation is weaker than the legacy Python application, which uses a worker-process model.
- Preview and metrics behavior are not yet governed by an explicit runtime budget.
- Native app behavior is not protected by acceptance-level automated tests.

This document defines the product requirements for integrating the redesigned native UI into a production-ready application.

## 2. Product Goal

Deliver a native desktop application that preserves the new high-quality visual experience while providing recording reliability, operator trust, recoverable failure handling, and a release-ready runtime architecture.

## 3. Primary Users

- Lab or field operators running single-camera or OAK-based capture sessions.
- Engineers validating capture quality and session outputs.
- Deployment owners packaging and distributing the desktop app to production machines.

## 4. Core User Stories

1. As an operator, I can select a real device and supported capture settings without guessing whether the session will start.
2. As an operator, I can trust that live preview will not materially degrade recording stability.
3. As an operator, I can start, monitor, stop, and export a session with clear state transitions and actionable failure messages.
4. As an operator, I can recover from device errors, invalid output paths, and decode failures without losing application control.
5. As a deployment owner, I can build, verify, package, and smoke-test the native app consistently.

## 5. Scope

### In Scope

- Native app runtime integration for the QML UI.
- Camera inventory, selection, capability loading, and readiness validation.
- Recording lifecycle orchestration.
- Preview pipeline strategy and performance budget.
- Decode/export workflow and operator actions.
- Native app observability, testing, packaging, and release gates.

### Out of Scope

- Major redesign of the capture core inside `internal/` beyond integration-driven changes.
- New media formats unrelated to current `micecam_core` and `micecam_camera` behavior.
- Cloud sync, remote control, or multi-user session management.

## 6. Functional Requirements

### FR-1 Session Lifecycle

The app must expose an explicit lifecycle:

- `idle`
- `preflight`
- `recording`
- `stopping`
- `decoding`
- `completed`
- `error`

Each state must drive:

- primary and secondary actions
- visible copy
- readiness messaging
- progress/error surfaces
- allowed operator inputs

### FR-2 Device Model

The app must provide a stable device model rather than a raw `QVariantList` snapshot.

The device model must support:

- device identifier stable within the running app session
- backend type
- human-readable name
- capability query
- availability changes after app startup
- empty-state behavior when no device is available

### FR-3 Capability-Driven Capture Setup

The app must derive selectable resolutions, formats, and FPS values from actual backend capability data or an explicitly versioned fallback capability table.

The operator must never be asked to enter or select unsupported values without being told they are unsupported.

### FR-4 Preflight Validation

Starting a recording must require a successful preflight result covering at minimum:

- selected device exists and is currently available
- output directory exists or can be created
- output directory is writable
- session name is valid and sanitized for filesystem use
- requested capture settings are supported
- application is not already stopping or decoding

### FR-5 Recording and Stop Behavior

The app must:

- prevent duplicate start and stop actions
- surface failure to initialize camera or pipeline as a first-class UI error
- ensure resource shutdown order is deterministic
- preserve the completed session identity after stop
- support safe app close during recording and decoding

### FR-6 Decode and Export Workflow

If auto-decode is enabled, the app must:

- transition into a dedicated decode state
- show decode progress and completion/error status
- expose the resolved export path
- allow the operator to reveal/open the output location

If auto-decode is disabled, the app must still provide a clear post-recording completion state with raw session path visibility.

### FR-7 Activity and Error Model

The app must replace raw UI-only log presentation with a structured event model suitable for:

- recent activity list
- warning and error banners
- future diagnostics export

At minimum the event model must classify:

- info
- warning
- error
- session event

### FR-8 Preview Strategy

Preview must be explicitly treated as a best-effort operator aid, not as a critical path dependency.

The preview subsystem must support:

- configurable or fixed capped update rate
- latest-frame semantics instead of backlog growth
- optional downscaling or reduced-fidelity preview path
- ability to disable preview without affecting recording
- clear offline or unavailable state

### FR-9 Runtime Isolation

The native app must use process-isolated worker execution with a UI supervisor architecture.

The supervisor must remain responsive when the worker:

- fails to start
- crashes during recording
- loses device access
- enters recovery

The runtime design must support automatic restart and recovery policies where safe.

### FR-10 Release Engineering

The native app must be releasable through documented build and verification steps including:

- build
- lint
- automated tests
- native app smoke test
- packaging validation

## 7. Non-Functional Requirements

### NFR-1 Reliability

- Preview failure must not terminate a recording session.
- Recoverable runtime failures must leave the UI responsive.
- Background decode must not leave dangling threads or corrupt state on exit.
- Recording and critical capture work must run outside the UI process boundary.

### NFR-2 Performance

- Recording throughput must remain prioritized over preview fidelity.
- Preview update work must be bounded by an explicit CPU and memory budget.
- Metrics surfaced to the UI must be derived from real runtime data, not placeholders.

### NFR-3 Observability

- No production behavior should depend on ad hoc `print`-style UI logging alone.
- Critical lifecycle events must be observable in a structured way.

### NFR-4 Testability

- Core native app lifecycle behavior must be testable with fake camera and fake decoder dependencies.
- Critical user flows must have acceptance-level integration coverage.

### NFR-5 Maintainability

- QML should bind to stable app-facing models and services, not accumulate orchestration logic.
- State transitions must be centralized and reviewable.

## 8. Acceptance Criteria

The native app can be considered production-ready only when all of the following are true:

1. The operator can complete device selection, preflight, recording, stop, and export end-to-end from the native UI.
2. Unsupported settings, invalid output paths, and missing devices are blocked before recording begins.
3. Preview remains responsive or degrades gracefully without introducing recording instability.
4. Device initialization failure, runtime device loss, stop failure, and decode failure all produce clear UI feedback and leave the app controllable.
5. The app can close safely from idle, recording, and decoding states.
6. Worker startup failure, worker crash, and worker recovery behavior are explicitly handled and validated.
7. The native app layer has automated tests for the key lifecycle scenarios.
8. Packaging and smoke-test instructions exist and are part of the release gate.

## 9. Reference Architectural Insight from the Python App

The legacy Python app provides two critical lessons that must be preserved in the native app:

1. Runtime isolation: the Python UI supervises a worker process instead of directly owning the recording runtime.
2. Preview budget discipline: preview is capped, downsampled, latest-frame-only, and explicitly allowed to drop frames.

These are not incidental implementation details. They are production behaviors that protect recording stability and must be reflected in the native app architecture.

The native app has now chosen the same strategic direction at the architecture level: a native supervisor process paired with a native recording worker.
