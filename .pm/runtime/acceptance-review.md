# Acceptance Review: Backend/UI Wiring Regression Repair

## Verdict

Accepted for targeted regression scope.

The prior stage-exit claim was invalidated by user screenshots showing mock cameras in no-device mode and misleading idle recording/status UI. This repair pass corrects the production default, controller state contract, toolbar state expression, empty camera grid, and focused controller tests.

## Evidence Reviewed

- OpenCode intern executed `.pm/runtime/next-task.md` and wrote `.pm/runtime/worker-report.md`.
- PM independently reviewed the worker report and current diff.
- PM verified `Theme.statusGreen` and `Theme.statusAmber` exist in `cmd/micecam_ui/qml/theme/Theme.qml`.
- PM independently ran:
  - `cmake --build build --target micecam_ui test_app_controller -j 4` — PASS
  - `ctest --test-dir build --output-on-failure -R test_app_controller` — PASS, 1/1 CTest target
  - short native app smoke via `build/cmd/micecam_ui/micecam_ui` for 4 seconds — no QML stderr output

## Acceptance Checklist

- [x] Production startup no longer defaults to `MockOnly`.
- [x] Production mode registers real FFmpeg/OAK backends rather than mock backend.
- [x] No-camera state exposes `0 cameras`, `canStartRecording == false`, `recordButtonText == "No Device"`, and `preflightMessage == "No cameras detected"`.
- [x] Idle with available devices uses `Start` semantics and green play affordance.
- [x] Recording uses red stop affordance.
- [x] Camera grid shows an explicit no-camera empty state.
- [x] Bottom status bar distinguishes idle readiness/preflight from recording metrics.
- [x] Focused controller tests cover no-camera and Start/Stop state transitions.

## Findings

- The repair is scoped and does not attempt worker-process architecture, full preflight contract, or real hardware validation.
- QML runtime smoke produced no stderr warnings in a 4-second run, but PM has not visually inspected the rendered app after this patch.
- Production backend behavior with actual FFmpeg/OAK hardware remains unvalidated in this pass.

## Next Action

`request_user_decision`: user should visually inspect the patched Production-mode UI. If acceptable, proceed to full verification and commit. If not, provide screenshot feedback and run another narrow repair task.
