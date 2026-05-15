# Task: Backend/UI Wiring Regression Repair

## Objective

Fix the regressions exposed by manual Production-mode smoke testing:

- no physical cameras must render as a true empty device state, not mock camera cards;
- idle/no-device must not present a red recording affordance;
- the bottom status bar must communicate readiness/session state instead of meaningless idle zeros;
- tests must lock the no-device and Start/Stop semantics.

## Stage Connection

Current stage: `backend-ui-wiring`.

This is a re-opened repair pass after user screenshots invalidated the prior "stage exit reached" claim.

## Allowed Scope

- `cmd/micecam_ui/main.cpp`
- `cmd/micecam_ui/AppController.h`
- `cmd/micecam_ui/AppController.cpp`
- narrowly required camera backend files under `internal/infrastructure/`
- QML files under `cmd/micecam_ui/qml/`
- focused unit tests under `tests/unit/`

## Forbidden Scope

- Product positioning changes
- Broad visual redesign
- CI/CD configuration changes
- New major UI surfaces
- Worker-process architecture refactor
- Reverting unrelated user or worker changes

## Acceptance Criteria

- Production startup with no cameras shows `0 cameras` and no camera cards.
- Production/no-camera state disables the recording action and exposes an actionable no-device message.
- Idle with available devices presents a green play/start affordance.
- Recording presents a red stop affordance.
- Bottom status bar distinguishes idle readiness from recording metrics.
- MockOnly remains available for explicit tests/demo mode only.
- Targeted build and tests pass.

## Parallel Work Slices

- Worker A: backend/controller production wiring.
- Worker B: QML state and empty-state expression.
- Worker C: regression tests and coverage audit.

## Verification Commands

```bash
cmake --build build --target micecam_ui test_app_controller -j 4
ctest --test-dir build --output-on-failure -R test_app_controller
```

## Required Report Content

Each worker must report:

- changed files
- findings
- commands run
- acceptance checklist
- remaining risks or blockers
