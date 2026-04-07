# ADR 0001: Use a Native Worker Process for Recording Runtime

**Date**: 2026-03-11
**Status**: Accepted
**Decision Makers**: Product and implementation planning for `micecam_ui`

## Context

The redesigned native QML UI is visually ready to become the product-facing desktop application, but the runtime architecture is not yet aligned with the reliability requirements of a production recording tool.

The project has two broad architecture options:

1. Hardened in-process runtime
2. Native worker-process runtime

The product requirement is explicit:

- recording stability is the highest priority
- preview may be degraded or disabled
- UI responsiveness may be sacrificed before recording stability is sacrificed
- the system should support restart and automatic recovery behavior where feasible

The legacy Python app already demonstrates one valuable property: the UI supervises a separate recording worker instead of directly owning the runtime.

## Decision

The native app will use a worker-process architecture.

The QML desktop application will act as a supervisor UI. Recording, capture runtime, and other critical background operations will run in a separate native worker process.

## Decision Drivers

- Maximum recording stability is more important than preview smoothness or UI simplicity.
- Fault containment is more important than lower implementation complexity.
- Recovery behavior is easier to reason about when the recording engine has a distinct process boundary.
- The existing Python app already validated the basic value of supervisor/worker separation.
- The current in-process native design places too much risk in one process boundary.

## Consequences

### Positive

- Worker crashes do not automatically terminate the UI.
- The UI can remain available to surface errors, recovery state, and operator actions.
- Preview can be aggressively downgraded or disabled without changing the recording engine architecture.
- Restart and supervised recovery become first-class runtime behaviors.
- Long-running recording logic is easier to isolate from QML and UI lifecycle concerns.

### Negative

- IPC design is now mandatory.
- Packaging becomes more complex because both supervisor app and worker executable must ship correctly.
- Preview transport must be explicitly designed rather than shared through direct in-memory ownership.
- There is more lifecycle surface area to test: launch, handshake, crash, restart, stop, shutdown, stale worker detection.

## Architectural Shape

The target architecture is:

- Native UI process
  - QML scene
  - UI-facing adapter/view-model
  - supervisor service
  - recovery policy
- Native worker process
  - capture runtime
  - device initialization
  - recording pipeline
  - decode/export jobs if assigned to the worker boundary
  - metrics and structured event emission

## Required Design Implications

### 1. Preview Policy

Preview is a low-priority channel.

The worker must support:

- disabled preview mode
- capped preview rate
- latest-frame-only semantics
- lossy preview acceptable

If preview threatens recording stability, preview must degrade or stop first.

### 2. Recovery Policy

The supervisor must distinguish at minimum:

- intentional stop
- worker startup failure
- worker crash during recording
- device initialization failure
- device loss after recording started
- decode failure

The app must define which cases support:

- automatic restart
- operator confirmation
- terminal failure

### 3. State Model

The UI-visible state machine must include worker-aware states, such as:

- `launching_worker`
- `worker_ready`
- `recording`
- `recovering`
- `stopping`
- `decoding`
- `completed`
- `error`

### 4. IPC Contract

The supervisor and worker must exchange structured messages for:

- lifecycle state
- stats
- warnings/errors
- preview metadata or preview payloads
- stop requests
- health heartbeat

### 5. Packaging

Release packaging must include both:

- native UI application
- native worker executable

The release gate must verify that the UI can launch and communicate with the packaged worker.

## Rejected Alternative

### Hardened In-Process Runtime

This option was rejected because it does not align with the stated priority order.

Even if carefully engineered, an in-process model keeps UI and recording runtime in the same failure domain. That is the wrong default for a product that prioritizes recording stability, restart behavior, and recovery above preview and UI fidelity.

## Next Steps

1. Update requirements and phase plan to make worker-process runtime mandatory.
2. Define the supervisor-worker IPC contract.
3. Decide whether decode runs inside the same worker process or a separate worker role.
4. Add acceptance tests that exercise worker crash and recovery behavior.
