# Phase Plan: Native App Production Integration

**Date**: 2026-03-11
**Status**: Proposed
**Target**: `cmd/micecam_ui`
**Depends On**: `docs/requirements/native-app-production-integration.md`

## Phase Overview

This phase plan translates the native app production requirements into a staged implementation sequence with explicit outcomes and verification targets.

## Phase 0: Worker Runtime Contract Freeze

### Goal

Freeze the worker-process runtime design before additional UI integration work continues.

### Deliverables

- Written ADR for native worker-process runtime
- App-facing state machine definition
- Supervisor-worker IPC contract
- Initial acceptance test matrix

### Tasks

- Extract current `PipelineController` responsibilities into a design inventory.
- Translate the Python worker-process lessons into a native runtime design.
- Decide preview budget, heartbeat, and shutdown semantics.
- Define service/model boundaries for:
  - supervisor service
  - worker launcher
  - camera inventory model
  - capability model
  - structured activity/event model

### Exit Criteria

- Worker-process runtime choice is documented.
- State machine is approved.
- No additional native app integration work proceeds against an unstable supervisor-worker contract.

## Phase 1: Device and Preflight Integration

### Goal

Replace the current thin camera enumeration approach with a production-grade setup and readiness layer.

### Deliverables

- Stable camera inventory model
- Capability-driven resolution/FPS selection
- Preflight validator
- QML readiness model wired into the new UI

### Tasks

- Replace raw `QVariantList` camera snapshots with typed app models.
- Implement device refresh on startup and inventory change.
- Add capability lookup per device/backend.
- Sanitize session names and validate output path creation/writability.
- Prevent `startRecording()` unless preflight succeeds.

### Verification

- Automated tests for empty camera inventory, unsupported setting selection, and invalid output path.
- Manual smoke test with at least one FFmpeg-backed device.

### Exit Criteria

- UI shows real availability and valid capabilities.
- Start is blocked on failed preflight with actionable feedback.

## Phase 2: Supervisor and Worker Lifecycle Hardening

### Goal

Move lifecycle orchestration into an explicit supervisor layer and harden worker launch, stop, decode, and recovery behavior.

### Deliverables

- `RecordingSupervisorService` or equivalent coordinator
- Explicit lifecycle transitions
- Deterministic stop and close behavior
- Safe worker and decode job management

### Tasks

- Move start/stop/decode orchestration out of `PipelineController` into a supervisor service.
- Implement worker launch, handshake, heartbeat, and stop protocols.
- Replace ad hoc background decode handling with managed worker-owned or worker-coordinated async execution.
- Ensure thread-safe state updates back into Qt/QML from supervisor events.
- Define and implement close behavior during:
  - recording
  - recovering
  - stopping
  - decoding
- Add failure mapping for worker startup, camera init, worker crash, stop, and decode failures.

### Verification

- Automated tests for:
  - successful record-stop-complete flow
  - worker startup failure
  - init failure
  - worker crash and recovery
  - decode failure
  - close during decode

### Exit Criteria

- All supervisor and worker lifecycle transitions are explicit and test-covered.
- App remains responsive through failures and shutdown.

## Phase 3: Preview Budget and Metrics Reliability

### Goal

Make preview and runtime metrics production-credible.

### Deliverables

- Preview budget policy
- Rate-limited preview path
- Latest-frame-only preview semantics
- Real throughput and buffer metrics

### Tasks

- Add preview frame rate cap and optional quality/downscale policy.
- Ensure preview work cannot back-pressure recording.
- Ensure preview IPC can be dropped or disabled under recovery and degraded-runtime policies.
- Expose preview enabled/disabled state to QML.
- Replace placeholder throughput reporting with real values.
- Extend structured event reporting for dropped frames and degradation signals.

### Verification

- Automated tests or targeted instrumentation for preview disabled and preview throttled paths.
- Manual performance validation on the target capture hardware.

### Exit Criteria

- Preview behaves as a best-effort operator aid.
- Displayed metrics reflect real runtime behavior.

## Phase 4: Operator Workflow Completion

### Goal

Complete the app-level actions and UX behaviors required for production operation.

### Deliverables

- Reveal/open output action
- Post-recording completion actions
- Structured recent activity feed
- Improved error and warning surfaces

### Tasks

- Add platform-appropriate output reveal/open support.
- Add completion-state affordances for raw session and decoded export.
- Replace raw text log dependence with structured event rendering.
- Improve messaging for worker recovery, device loss, dropped frames, and decode completion.

### Verification

- Manual validation of full operator flow from setup to export review.
- UI smoke checks across wide and compact layouts.

### Exit Criteria

- A user can complete the full recording workflow without needing hidden logs or ad hoc shell intervention.

## Phase 5: Quality Gate and Release Readiness

### Goal

Make the native app shippable through a reliable verification pipeline.

### Deliverables

- Native app test suite integrated into CI/local verification
- Updated `make verify` contract or equivalent
- Packaging checklist
- Release smoke-test checklist

### Tasks

- Enable and document test builds for the native app path.
- Add acceptance/integration tests for app lifecycle flows.
- Add worker crash, restart, and recovery tests.
- Add native app launch smoke test to release verification.
- Verify packaging for native Qt runtime dependencies and packaged worker launch.
- Document release blockers and known hardware matrix.

### Verification

- `make verify` or replacement gate runs build, lint, and tests from a clean checkout.
- Packaging artifacts are launch-tested on target platform(s).

### Exit Criteria

- Release process is documented and repeatable.
- Native app can be promoted without relying on manual tribal knowledge.

## Test Strategy Summary

### RED

- Write app-layer acceptance tests first for lifecycle and preflight.

### GREEN

- Implement the minimum service/model code needed to satisfy the tests.

### REFACTOR

- Simplify controller/QML integration after lifecycle tests are stable.

## Recommended Implementation Order

1. Phase 0
2. Phase 1
3. Phase 2
4. Phase 3
5. Phase 4
6. Phase 5

This order is intentional. It prevents the team from continuing to polish the UI surface before the runtime contract is stable.
