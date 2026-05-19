# Active Stage: 007 Phase 5 — Notifications, Metrics, Flaky Test Fix

## Stage ID

`007-phase-5-metrics-notifications-flakytest`

## Stage goal

Close backend feedback loops: push live fps/drops from `StatsCollector` to `CameraSourceModel` via C++ timer, surface plugin crash/device disconnect through tiered alert system, and eliminate the flaky `StallCountResetsOnActivity` test by replacing timing-sensitive `sleep_for` with deterministic `cycle_count_` synchronization.

## Why this stage matters

Without live metrics, the UI shows static fps/drop values. Without crash/disconnect notifications, the operator has no feedback when a plugin crashes or a device disconnects. The flaky test breaks CI reliability.

## Inputs

- `specs/007-plugin-ui-integration/spec.md` — FR-024 through FR-027, FR-032, NFR-003, NFR-006
- `specs/007-plugin-ui-integration/plan.md` — Phase 5 actions
- Existing: `internal/pipeline/StatsCollector.*`, `internal/infrastructure/StreamLivenessMonitor.*`
- Existing: `cmd/micecam_ui/AppController.*`, `cmd/micecam_ui/AppAlertModel.*`

## Allowed work

- Add C++ QTimer in `AppController` that polls `StatsCollector` snapshots and pushes fps/drops to `CameraSourceModel` device roles (at most 1Hz per NFR-003)
- Emit `dataChanged` signals so QML binds update without polling
- Plugin crash/device disconnect notification path:
  - Amber banner for recoverable crashes (auto-dismiss on recovery)
  - Red banner/modal for fatal crashes (requires user acknowledge)
  - Warning banner for device disconnect
  - Log all events to activity feed
- Fix `StreamLivenessMonitor`:
  - Add atomic `std::atomic<int> cycle_count_`
  - Increment after each monitor loop iteration
  - Replace `sleep_for` in `StallCountResetsOnActivity` with spin-wait on `cycle_count_`
  - Run test 10 consecutive times on macOS
- Add/expand unit tests for notifications, metrics push, and liveness fix

## Forbidden work

- No QML changes (UI frozen)
- No proto changes
- No new RPC definitions
- No changes to Plugin Management or Plugin Detail QML
- No CameraSourceModel role renames/removals (additive only)
- No AppCameraModel changes

## Exit criteria

- [ ] Live metrics timer pushes fps/drops from StatsCollector to CameraSourceModel at <=1Hz
- [ ] Plugin crash surfaces as banner with recovery status, escalates to modal on failure
- [ ] Device disconnect surfaces as warning banner with device name
- [ ] All plugin events appear in activity feed/logs
- [ ] `StreamLivenessMonitor::cycle_count_` is atomic and incremented per loop cycle
- [ ] `StallCountResetsOnActivity` uses `cycle_count_` spin-wait, not `sleep_for`
- [ ] Flaky test passes 10/10 consecutive runs on macOS
- [ ] All existing tests pass (no regressions)
- [ ] Build passes: `cmake --build build -j 4`
