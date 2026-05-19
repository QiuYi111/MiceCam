# Active Stage: 007 Phase 1 — Source Model and Controller Contract Foundation

## Stage ID

`007-phase-1-source-model`

## Stage goal

Make `CameraSourceModel` the single camera/source UI data source. Expand roles, add ordering logic, wire real plugin device data, add stable camera detail lookup. Keep `AppCameraModel` temporarily available for compatibility.

## Why this stage matters

Current `CameraSourceModel` exists but is largely unpopulated (no real device data, empty plugin_devices vector). `AppCameraModel` still drives sidebar and grid QML bindings. Phase 1 establishes the data contract foundation before any QML visual changes.

## Inputs

- `specs/007-plugin-ui-integration/spec.md` — FR-001 through FR-005
- `specs/007-plugin-ui-integration/plan.md` — Phase 1 actions
- Existing: `cmd/micecam_ui/CameraSourceModel.*`, `AppController.*`, `AppCameraModel.*`
- Existing: `internal/domain/PluginSource.h`, `PluginDeviceInfo.h`, `DeviceInfo.h`

## Allowed work

- Add source roles to `CameraSourceModel`: pluginVersion, apiVersion, diagnosticsMessage, restartRequired, availableDeviceCount, expanded state
- Add device/stream fields: persistentId, vendor, serial, stream payloads/capabilities
- Add source ordering: active > available > warning > disabled; bundled > linked > unknown
- Populate `PluginSource.device_ids` and pass real plugin device data into `CameraSourceModel::populateFromSources()`
- Add `getCameraDetail(cameraId)` with stable index-free lookup
- Add/expand model and controller tests
- Keep `AppCameraModel` temporarily available for compatibility

## Forbidden work

- No QML changes
- No proto changes
- No build system changes (CMakeLists.txt)
- No new RPC definitions
- No changes to `internal/` domain/infrastructure files beyond those needed to pass PluginDeviceInfo
- No security/auth changes

## Exit criteria

- [ ] `CameraSourceModel` exposes all source roles from FR-002
- [ ] `CameraSourceModel::getDeviceAt()` returns full device+stream data per FR-003
- [ ] Source ordering follows spec (active > available > warning > disabled; bundled > linked > unknown)
- [ ] `getCameraDetail(cameraId)` uses stable index-free lookup
- [ ] Unit tests for `CameraSourceModel` pass
- [ ] Unit tests for `AppController` source/device contracts pass
- [ ] Existing tests using `AppCameraModel` still pass or are intentionally migrated
- [ ] Build passes: `cmake --build build -j 4`
