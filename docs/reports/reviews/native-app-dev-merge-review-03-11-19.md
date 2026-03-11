# Native App Dev Merge Review

**Reviewer**: Codex review agent
**Date**: 2026-03-11 19:00 CST
**Target Branch**: `codex/phase2-native-app-integration`
**Target Integration Branch**: `dev` (requested target, not present in local repo snapshot)
**Decision**: `NOT APPROVED`

## Review Scope

This review answers a release-style question rather than a narrow code-style question:

- Can the current native app branch be merged into `dev` as a feature-complete branch awaiting public beta?
- Is the native app functionally complete and logically coherent?
- Is the native app at least as capable as the Python app in operator-facing behavior and runtime architecture?

Windows testing and packaging were treated as non-blocking per request.

## Evidence Reviewed

- Product requirement: `docs/requirements/native-app-production-integration.md`
- Phase plan: `docs/plan/phase-native-app-integration.md`
- Contribution rules: `CONTRIBUTING.md`
- Native app runtime and UI:
  - `cmd/micecam_ui/RecordingSupervisorService.*`
  - `cmd/micecam_ui/WorkerProcessRuntime.*`
  - `cmd/micecam_ui/NativeWorkerRuntime.*`
  - `cmd/micecam_ui/PipelineController.*`
  - `cmd/micecam_ui/qml/main.qml`
- Python reference app:
  - `cmd/gui/gui/main_window.py`
- Native app tests:
  - `tests/ui/native_app_preflight_test.cpp`
  - `tests/ui/pipeline_controller_test.cpp`
  - `tests/ui/recording_supervisor_service_test.cpp`

## Verification Performed

The following checks passed in the current environment:

- `make test`
- `make lint`
- `./build/micecam_tests --gtest_filter='NativeAppPreflightTest.*:CameraInventoryModelTest.*:RecordingSupervisorServiceTest.*:PipelineControllerTest.*'`

Not performed in this review:

- Real hardware operator smoke for the native UI
- Packaged app launch validation on a clean machine
- Direct diff against an actual `dev` branch, because no local or remote `dev` branch exists in the repository snapshot reviewed on 2026-03-11

## What Is Ready

The branch has real progress and should not be characterized as a superficial UI-only effort.

Implemented strengths:

- Native UI no longer depends on a monolithic controller-only runtime path.
- Worker-process supervision exists and follows the same strategic direction as the Python app.
- Preflight validation, typed camera inventory, structured activity events, and decode-state handling are present.
- Close-during-decode behavior is explicitly modeled and has automated coverage.
- Native app smoke and release-checklist documents now exist.

Architecturally, the branch is directionally stronger than the earlier native UI state and is approaching the PRD's intended production shape.

## Blocking Findings

### 1. Decode failure breaks the operator recovery path

Severity: Blocker

The supervisor precomputes `resolvedExportPath` at recording start, before decode succeeds. If decode later fails, the QML error-state UI still exposes `Open Output`, and the controller always prefers `resolvedExportPath` over the raw session directory.

Impact:

- In a common failure scenario, the operator is sent to a `_decoded` path that may not exist.
- The valid raw recording path is present, but the UI does not prioritize it.
- This violates the PRD requirement that decode/export failures must leave the app controllable with clear recovery behavior.

Relevant code:

- `cmd/micecam_ui/RecordingSupervisorService.cpp`
- `cmd/micecam_ui/PipelineController.cpp`
- `cmd/micecam_ui/qml/main.qml`

## 2. Native app currently regresses the Python app on preview control

Severity: Blocker

The Python app exposes an explicit operator control to disable preview and skips preview updates when disabled. The native app exposes preview status, but no actual operator control to disable preview transport or rendering.

Impact:

- Native app is not yet functionally greater than or equal to the Python app.
- FR-8 explicitly requires the ability to disable preview without affecting recording.
- For public beta, this is an operator-facing capability regression rather than a cosmetic gap.

Relevant code:

- Python reference: `cmd/gui/gui/main_window.py`
- Native app UI: `cmd/micecam_ui/qml/main.qml`

## 3. The branch is not clean enough to merge as an integration branch

Severity: Blocker

The diff is not limited to native app integration. It also includes generated files, binary payloads, and legacy or packaging artifacts that should not be bundled into a branch whose purpose is native app readiness review.

Examples observed in the diff:

- `.DS_Store`
- `build_test/CMakeCache.txt`
- `tools/__pycache__/read_bin.cpython-314.pyc`
- `legacy/MiceCam_SDK/...`
- `release/bin/...`

Impact:

- Reviewability is materially degraded.
- Future rollback and blame become noisy.
- Merge risk increases because branch intent and branch content no longer match.

This is a branch hygiene failure even before considering runtime behavior.

## Non-Blocking Observations

- The native app has good automated coverage for preflight and supervisor behavior, but acceptance coverage is still dominated by fake runtimes rather than true app-level workflow tests.
- The release checklist is present, but public-beta confidence still depends on manual hardware validation on target capture devices.
- The worker handshake smoke check is useful, but it is not a substitute for end-to-end operator flow validation.

## Merge Recommendation

Current recommendation: do not merge `codex/phase2-native-app-integration` into `dev` yet.

This branch is close enough to justify continued hardening on the same line of work, but not close enough to serve as a public-beta candidate.

## Required Exit Criteria Before Re-Review

The branch can be reconsidered for merge once all of the following are true:

1. Decode failure keeps the raw session path actionable, and `Open Output` resolves to a path that actually exists.
2. The native app exposes an operator-level preview disable control, and disabling preview does not affect recording.
3. The branch is cleaned so the review diff contains only intentional native app, build, and documentation changes required for this feature.
4. Manual hardware smoke confirms the full operator path:
   - device selection
   - preflight
   - record
   - stop
   - decode or complete
   - open output
   - close during decode
5. A follow-up review confirms the native app is no worse than the Python app on the public-beta workflow.

## Final Conclusion

The native app branch demonstrates meaningful architectural maturity and should continue forward, but it is not yet merge-ready for `dev` under the standard of "feature complete and awaiting public beta."

The correct status on 2026-03-11 is:

- Native app direction: acceptable
- Native app merge readiness: not acceptable yet
- Public beta readiness: not acceptable yet
