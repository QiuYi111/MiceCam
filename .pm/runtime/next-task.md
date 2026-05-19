# Task: Spec 007 Phase 1 — CameraSourceModel Foundation

## Objective

Expand `CameraSourceModel` into the single source-of-truth for camera and plugin UI data (FR-001 through FR-005). Wire real plugin device data into the model, add source ordering, and provide stable camera detail lookup. Keep `AppCameraModel` temporarily available for compatibility tests.

## Baseline

- Current branch: `codex/007-plugin-ui-release`
- Code baseline: `dev` at `ac24012` (3 documentation-only commits ahead)
- Build status: `cmake --build build -j 4` passes cleanly

## Required Harness Process

Full chain for `branch`+ risk: `harness-risk` → `harness-context` → `harness-tdd` → `harness-eval` → `harness-report`

## Required Work

### 1. Expand CameraSourceModel Source Roles (FR-002)

Add these roles to `CmdRoleNames` and `CameraSourceModel`:
- `SourceGroupRole` — source group identifier string
- `PluginVersionRole` — plugin version string
- `ApiVersionRole` — API version string
- `DiagnosticsStateRole` — diagnostic state enum (ok/warning/error)
- `DiagnosticsMessageRole` — user-readable diagnostic message
- `RestartRequiredRole` — boolean
- `AvailableDeviceCountRole` — count of available/enabled devices
- `IsExpandedRole` — source collapsible state

### 2. Expand Device-Level Roles (FR-003)

Add to device rows:
- `PersistentIdRole` — stable device ID string
- `VendorInfoRole` — vendor string
- `SerialNumberRole` — serial string
- Payload badges per stream

Ensure `getDeviceAt(sourceIdx, devIdx)` returns all existing `CameraRow` fields plus the new `DeviceInfo` fields.

### 3. Implement Source Ordering (FR-004, FR-008)

In `populateFromSources()`:
- Order sources: active > available > warning > disabled
- Within each tier: bundled before linked before unknown

### 4. Wire Plugin Device Data (FR-005)

Populate `PluginSource.device_ids` by mapping enumerated devices to their originating plugin source.
Pass real `PluginDeviceInfo` data into `CameraSourceModel::populateFromSources()`.
Remove or update any code that passes an empty `plugin_devices` vector.

### 5. Add getCameraDetail(cameraId)

Add `Q_INVOKABLE getCameraDetail(cameraId)` to `AppController` with stable, index-free lookup that returns correct device and stream data regardless of current source ordering.

Update `refreshCameras()` to preserve `sourceId` and `sourceGroup` on each `CameraRow`.

### 6. Keep AppCameraModel Compatibility

Do not remove `AppCameraModel` yet. Existing tests/test code may still reference it. Deprecation is OK (comments), removal is for a later phase.

## Allowed Files

You may modify:
- `cmd/micecam_ui/CameraSourceModel.h`
- `cmd/micecam_ui/CameraSourceModel.cpp`
- `cmd/micecam_ui/AppController.h`
- `cmd/micecam_ui/AppController.cpp`
- `cmd/micecam_ui/AppCameraModel.h` (deprecation comments only, no behavior change)
- `cmd/micecam_ui/AppCameraModel.cpp` (deprecation comments only, no behavior change)
- `internal/domain/PluginSource.h` (if needed for device_ids mapping)
- `internal/domain/PluginDeviceInfo.h` (if needed for role exposure)
- `internal/domain/DeviceInfo.h` (if needed for role exposure)
- `tests/unit/test_app_models.cpp`
- `tests/unit/test_app_controller.cpp`
- New test files under `tests/unit/` if warranted (e.g., `test_camera_source_model.cpp`)
- `docs/reports/` (implementation report)
- `.pm/runtime/worker-report.md`
- `project_index`

## Forbidden Scope

- **No QML changes.** Phase 2 handles QML.
- **No proto changes.**
- **No CMakeLists.txt changes** unless needed to add a new test file.
- **No new RPC definitions.**
- **No security/auth/sandboxing changes.**
- **Do not remove AppCameraModel** (only add deprecation comments).
- **Do not modify `internal/infrastructure/`** files beyond what's needed for data pass-through.
- **No merges, rebases, or pushes.**

## Verification Commands

Run and record:

```bash
cmake --build build -j 4
ctest --test-dir build --output-on-failure
```

If a new test file requires CMakeLists.txt registration, add it but verify all existing tests still pass.

## Acceptance Criteria

- [ ] `CameraSourceModel` exposes source-level roles: `pluginVersion`, `apiVersion`, `diagnosticsState`, `diagnosticsMessage`, `restartRequired`, `deviceCount`, `availableDeviceCount`, `isExpanded`
- [ ] `CameraSourceModel::getDeviceAt(sourceIdx, devIdx)` returns full device+stream data including persistentId, vendor, serial, payload badges
- [ ] Source ordering follows: active > available > warning > disabled; bundled > linked > unknown within each tier
- [ ] `PluginSource.device_ids` is populated; real plugin device data reaches `CameraSourceModel`
- [ ] `AppController::getCameraDetail(cameraId)` returns correct data via index-free lookup
- [ ] `refreshCameras()` preserves `sourceId` and `sourceGroup` on each `CameraRow`
- [ ] New or expanded unit tests cover source roles, ordering, and device lookup (RED→GREEN)
- [ ] All existing tests pass (no regressions)
- [ ] Build passes: `cmake --build build -j 4`
- [ ] Worker report lists changed files, commands, test results, problems, deviations
- [ ] One git commit with task changes only

## Context

Phase 1 is `branch` (branch-level) risk. The changes are additive to existing model/controller code and do not touch QML. Existing tests protect against regression.
