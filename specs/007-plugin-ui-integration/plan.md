# Implementation Plan: Plugin UI Integration + Spec 004 Closure

## Inputs

| Source | Reference |
|--------|-----------|
| Spec | `specs/007-plugin-ui-integration/spec.md` |
| Visual contract | `specs/007-plugin-ui-integration/ui-spec.md` |
| Visual anchors | `specs/007-plugin-ui-integration/pluginUI/*.png` |
| Related spec | `specs/004-production-ready-plugin-app/` |
| Architecture decisions | `docs/adr/0001-native-worker-process-runtime.md`, `docs/adr/0002-external-camera-plugin-runtime.md` |
| Project index | `project_index` |

## Technical Context

| Dimension | Value |
|-----------|-------|
| Language | C++20, QML, Python test/support scripts as needed |
| Framework | Qt/QML native app, CMake, GTest, nlohmann_json, gRPC/protobuf plugin contract |
| UI entry | `cmd/micecam_ui/main.cpp`, `cmd/micecam_ui/qml/main.qml` |
| UI bridge | `cmd/micecam_ui/AppController.*`, `cmd/micecam_ui/CameraSourceModel.*` |
| Plugin runtime | `internal/infrastructure/PluginRegistryService.*`, external bundled/linked camera plugins |
| Domain models | `internal/domain/StreamConfig.h`, `internal/domain/PluginSource.h`, `internal/domain/PluginDeviceInfo.h`, `internal/domain/DeviceInfo.h` |
| External dependencies | FFmpeg, DepthAI/OAK-D, Qt, platform packaging/runtime libraries |
| Release targets | macOS arm64, Windows, Ubuntu unless release policy narrows target list |

## Architecture Impact

### DDD Layer Impact

| Layer | Change |
|-------|--------|
| `internal/domain/` | Expand `StreamConfig` with host-side plugin config map; ensure device/source/config terms match plugin runtime concepts; preserve existing typed fields as fallback. |
| `internal/infrastructure/` | Extend plugin registry/service outputs for restart-required, linked removal, import validation diagnostics, config persistence, device diagnostics, liveness monitor cycle synchronization, and FFmpeg config-map consumption. |
| `api/` | No new RPC is planned. Existing `api/micecam/camera_plugin.proto::StreamConfig.config` is the protocol carrier for plugin config values. If implementation finds it missing in generated bindings, regenerate protobuf artifacts without changing RPC shape. |
| `cmd/micecam_ui/` | Migrate QML/controller data flow from flat `AppCameraModel` to `CameraSourceModel`; expand `AppController` invokables for plugin rows, plugin details, camera detail lookup, import validation, remove linked plugin, config validate/apply, and recording lock. |
| `cmd/micecam_ui/qml/` | Rework `AppSidebar.qml`, `CameraGridView.qml`, `PluginManagementPage.qml`, and `PluginDetailPage.qml` to match `pluginUI/` anchors while preserving shell/title/status bar. |
| `tests/` | Add/expand unit, integration, QML contract, HIL, flaky-test, and release smoke coverage. |
| `docs/` | Update implementation report, release report, wikis, and project index after architecture/user-facing behavior is implemented. |

### Contract Impact

- `CameraSourceModel` roles become the UI contract for camera/source rendering.
- `CameraSourceModel::getDeviceAt(sourceIdx, devIdx)` must return full device and stream metadata.
- `AppController::pluginList()` must return per-plugin status, API, type, enabled, restart-required, device count, and toggle policy fields.
- `AppController::importPlugin()` must return structured validation results rather than a bare bool, or add a companion invokable that exposes the structured result while preserving existing tests during migration.
- `AppController::getPluginDetail()` must become plugin-id based or provide stable plugin-id lookup in addition to current path-based lookup.
- Add invokables for linked plugin removal, config schema loading, config validation, config apply, and camera detail lookup.
- Host-side plugin config persists as `{config_dir}/plugin_configs/{plugin_id}.json`.
- Domain `StreamConfig.config` maps host-side settings into existing proto `StreamConfig.config`.

### Data Model Impact

- No database migration.
- New or expanded in-memory UI models:
  - source rows
  - device rows
  - stream rows
  - plugin rows
  - plugin detail/config/diagnostic maps
- New local JSON config files under `{config_dir}/plugin_configs/`.
- Existing `micecam_config.json` loading must be fixed in `AppSettings`.
- Existing flat `AppCameraModel` should be treated as deprecated compatibility surface during migration and removed from primary QML bindings once `CameraSourceModel` is complete.

## Blast Radius Classification

| Field | Value |
|-------|-------|
| Level | `core` |
| Reason | 007 touches domain models, plugin protocol data flow, recording/plugin runtime integration, UI-controller contracts, packaging validation, and HIL release gates. It also absorbs spec 004 closure items. |
| Required Gates | human_spec_review, architecture_review, rollback_plan, security_review, tests, review_agent |

[REQUIRES HUMAN REVIEW]

## Constitution Check

| Check | Pass | Notes |
|-------|------|-------|
| Contract-first | Yes | `spec.md`, `ui-spec.md`, ADR 0002, and existing proto contract define UI/runtime behavior before implementation. |
| DDD direction | Yes | Data flows from plugin/domain/infrastructure into UI models; QML does not read plugin files directly except through controller contracts. |
| TDD/BDD | Yes | Each phase has explicit RED/GREEN/REFACTOR gates and user-scenario acceptance tests from spec US-001 through US-009. |
| Observability | Yes | Plugin import failures, crashes, device disconnects, restart-required state, and config validation must surface through UI banners/logs/activity feed. |
| Security | Needs review | Linked plugin import/removal and path persistence are local filesystem trust boundaries. Human/security review required before release. |

## Implementation Strategy

Implement foundation to UI to release closure. Do not start with visual QML polish until model/controller contracts are tested.

### Phase 0: Planning and Branch Hygiene

Goal: make 007 executable without ambiguity.

Files:

- `specs/007-plugin-ui-integration/spec.md`
- `specs/007-plugin-ui-integration/ui-spec.md`
- `specs/007-plugin-ui-integration/plan.md`
- `project_index`

Actions:

1. Treat `spec.md` as the execution source of truth and `ui-spec.md` as the visual/screen contract.
2. Keep branch `codex/007-plugin-ui-release`.
3. Add implementation reports under `docs/reports/implements/`.
4. Keep spec 004 closure items tracked in 007 until release merge.

Exit gate:

- Plan reviewed by human before core implementation begins.

### Phase 1: Source Model and Controller Contract Foundation

Goal: make `CameraSourceModel` the single camera/source UI data source.

Primary files:

- `cmd/micecam_ui/CameraSourceModel.h`
- `cmd/micecam_ui/CameraSourceModel.cpp`
- `cmd/micecam_ui/AppController.h`
- `cmd/micecam_ui/AppController.cpp`
- `cmd/micecam_ui/AppCameraModel.h`
- `cmd/micecam_ui/AppCameraModel.cpp`
- `internal/domain/PluginSource.h`
- `internal/domain/PluginDeviceInfo.h`
- `internal/domain/DeviceInfo.h`
- `tests/unit/test_app_models.cpp`
- `tests/unit/test_app_controller.cpp`

Actions:

1. Add source roles required by FR-002: plugin version, API version, diagnostics message, restart required, available device count, expanded state.
2. Add device and stream fields required by FR-003 and FR-017.
3. Add source ordering logic: active, available, warning, disabled; bundled before linked before unknown inside each tier.
4. Populate `CameraRow.sourceId` and `sourceGroup` while keeping `AppCameraModel` temporarily available for compatibility tests.
5. Populate `PluginSource.device_ids` and pass real plugin device data into `CameraSourceModel::populateFromSources()`.
6. Add `getCameraDetail(cameraId)` with stable lookup that does not depend on row index.
7. Adjust `cameraCount`, `canStartRecording`, and preflight state to derive from source/device availability.

Test-first tasks:

- RED: model tests for source role names, source ordering, unavailable compact state, and device lookup.
- RED: controller tests proving `refreshCameras()` preserves source identity and `getCameraDetail(cameraId)` returns the expected stream/device.
- GREEN: implement model/controller data wiring.
- REFACTOR: remove duplicate mapping logic between `AppCameraModel` and `CameraSourceModel` where practical.

Exit gate:

- Unit tests for `CameraSourceModel` and `AppController` pass.
- Existing tests using `AppCameraModel` still pass or are intentionally migrated.

### Phase 2: Source-Grouped Camera UI

Goal: replace flat camera sidebar/grid presentation with source-grouped camera browsing.

Primary files:

- `cmd/micecam_ui/qml/components/AppSidebar.qml`
- `cmd/micecam_ui/qml/components/CameraGridView.qml`
- `cmd/micecam_ui/qml/components/CameraCard.qml`
- `cmd/micecam_ui/qml/components/CameraDetailView.qml`
- `cmd/micecam_ui/qml/main.qml`
- `cmd/micecam_ui/qml.qrc`
- `cmd/micecam_ui/qml/resources/icons/*`

Actions:

1. Rebind sidebar from `appController.cameraModel` to `appController.sourceModel`.
2. Render collapsible source headers and nested device rows.
3. Rebind main camera grid to source sections and render device preview cards inside healthy sections.
4. Render warning/disabled/restart-required sources as compact rows below active preview sections.
5. Implement no-plugin/no-device/all-disabled empty states per `pluginUI/no_plugin.png`.
6. Preserve title bar, toolbar, status bar, and current camera card visual language.
7. Ensure text does not overlap or resize layout across desktop and narrow widths.

Test-first tasks:

- RED: QML contract tests or focused UI model tests for source tree rendering.
- RED: snapshot/manual checklist for no-plugin and SDK-missing compact source state.
- GREEN: QML implementation.
- REFACTOR: shared status badge/source row components only if duplication becomes meaningful.

Exit gate:

- Camera Sources screen visually matches `no_plugin.png` and source-grouped behavior in `ui-spec.md`.
- No giant blank preview area for unavailable sources.

### Phase 3: Plugin Management Workflow

Goal: make Plugin Management a complete administrative screen.

Primary files:

- `cmd/micecam_ui/AppController.h`
- `cmd/micecam_ui/AppController.cpp`
- `cmd/micecam_ui/qml/components/PluginManagementPage.qml`
- `internal/infrastructure/PluginRegistryService.h`
- `internal/infrastructure/PluginRegistryService.cpp`
- `internal/infrastructure/LinkedPluginConfig.h`
- `internal/infrastructure/LinkedPluginConfig.cpp`
- `tests/unit/test_app_controller.cpp`
- `tests/unit/test_plugin_registry.cpp`
- `tests/unit/test_linked_plugin_config.cpp`

Actions:

1. Expand `pluginList()` with type, version, API, status, status message, device count, restart-required, can-toggle, can-open-settings, can-remove.
2. Lock bundled plugin toggles with tooltip `Bundled plugin cannot be disabled`.
3. Add linked plugin removal action through `removeLinkedDirectory()`.
4. Add structured import validation result for manifest exists, parseable, id valid, platform entrypoint exists, executable, handshake accepted.
5. Render import failure panel per `pluginUI/plugin_fail.png`.
6. Render restart-required banner and per-row restart-required badges.
7. Enforce recording lock for import, toggle, and remove actions per `pluginUI/recording_lock.png`.

Test-first tasks:

- RED: controller tests for bundled locked policy, linked toggle/remove, structured import validation, restart-required flag.
- RED: registry tests for remove-linked behavior and validation diagnostics.
- GREEN: implement controller/service changes.
- REFACTOR: normalize status mapping to a single helper.

Exit gate:

- Plugin Management normal, import failure, and recording-lock states match visual anchors.

### Phase 4: Plugin Detail, Diagnostics, and Schema Config

Goal: make per-plugin details a real operational screen family.

Primary files:

- `cmd/micecam_ui/AppController.h`
- `cmd/micecam_ui/AppController.cpp`
- `cmd/micecam_ui/qml/components/PluginDetailPage.qml`
- new QML components if needed:
  - `PluginManifestSection.qml`
  - `PluginDiagnosticsSection.qml`
  - `PluginConfigSection.qml`
  - `PluginDeviceSection.qml`
- `internal/domain/StreamConfig.h`
- `internal/infrastructure/PluginRegistryService.*`
- `internal/infrastructure/ConfigLoader.*`
- `internal/infrastructure/FFmpegCameraBackend.*`
- `cmd/plugins/micecam_ffmpeg/*`
- `tests/unit/test_app_controller.cpp`
- `tests/unit/test_ffmpeg_plugin_server.cpp`
- `tests/unit/test_ffmpeg_camera.cpp`
- `tests/integration/test_plugin_e2e_no_hw.cpp`

Actions:

1. Expand `getPluginDetail()` to include manifest, diagnostics, capabilities, devices, streams, status badges, restart-required, and config schema.
2. Add schema-driven config render data for string, integer, float, bool, enum, and path.
3. Add `Validate Config` controller path to call plugin validation and map per-field errors.
4. Add `Apply Changes` controller path to persist JSON config.
5. Add `std::map<std::string, std::string> config` to domain `StreamConfig`.
6. Load stored plugin config before stream open and populate proto `StreamConfig.config`.
7. Update FFmpeg plugin/backend behavior to read config map with typed-field fallback.
8. Implement recording-time field locks: only `runtime-safe` editable during recording.
9. Split visual states to match `plugin_manifest.png`, `plugin_config.png`, `plugin_setting.png`, and `diagnose.png`.

Test-first tasks:

- RED: domain/config tests for JSON persistence and `StreamConfig.config` pass-through.
- RED: plugin detail tests for manifest, diagnostics, capabilities, devices, streams, and schema.
- RED: FFmpeg config-map fallback tests.
- RED: QML/controller tests for recording lock and apply-mode flags.
- GREEN: implement.
- REFACTOR: split QML sections only after tests stabilize.

Exit gate:

- Plugin Detail states match all four plugin detail visual anchors.
- Config values persist and apply on next stream open.

### Phase 5: Notifications, Metrics, and Spec 004 Flaky Test Closure

Goal: close runtime feedback loops and remove known flaky CI risk.

Primary files:

- `cmd/micecam_ui/AppController.*`
- `cmd/micecam_ui/AppAlertModel.*`
- `cmd/micecam_ui/qml/components/NotificationPopup.qml`
- `internal/pipeline/StatsCollector.*`
- `internal/infrastructure/StreamLivenessMonitor.h`
- `internal/infrastructure/StreamLivenessMonitor.cpp`
- `tests/unit/test_stream_liveness_monitor.cpp`
- `tests/unit/test_stats_collector.cpp`

Actions:

1. Add a one-second-or-slower metrics update path from `StatsCollector` snapshots into `CameraSourceModel` device roles.
2. Surface plugin crash and device disconnect through banners, modal escalation, logs, and alert model.
3. Add `StreamLivenessMonitor::cycle_count_` and deterministic wait support for tests.
4. Replace timing-sensitive `sleep_for` assertions in flaky liveness monitor tests.
5. Run `StallCountResetsOnActivity` 10 consecutive times on macOS.

Test-first tasks:

- RED: liveness monitor cycle synchronization test.
- RED: alert model/controller tests for crash and disconnect state.
- GREEN: implement.
- REFACTOR: keep thread synchronization minimal and explicit.

Exit gate:

- Flaky test passes 10/10 locally or in target CI environment.
- Crash/disconnect notification path is covered by unit/fault-injection tests.

### Phase 6: App Settings and Release Packaging Validation

Goal: make release artifacts launchable and persistent across restarts.

Primary files:

- `cmd/micecam_ui/AppSettings.h`
- `cmd/micecam_ui/AppSettings.cpp`
- `cmd/micecam_ui/CMakeLists.txt`
- `CMakeLists.txt`
- `cmd/plugins/micecam_ffmpeg/CMakeLists.txt`
- `cmd/plugins/micecam_oak/CMakeLists.txt`
- packaging scripts or CMake install rules, if present or added
- `docs/wikis/native-app-release-checklist.md`
- `docs/wikis/windows-dev-pitfalls.md`

Actions:

1. Fix `AppSettings` constructor to load `micecam_config.json`.
2. Add/verify tests for settings persistence across restarts.
3. Verify packaged app includes QML, bundled plugins, manifests, runtime libraries, and worker/runtime assets.
4. Verify plugin search paths resolve from packaged app location.
5. Run release smoke launch on supported platforms.

Test-first tasks:

- RED: `AppSettings` load/persist test.
- RED: packaging smoke checklist or script that asserts plugin metadata exists in artifact.
- GREEN: implement packaging fixes.

Exit gate:

- macOS, Windows, and Ubuntu builds produce launchable artifacts or documented target waivers.

### Phase 7: HIL Tests and Lab Validation

Goal: produce hardware evidence for release-critical paths.

Primary files:

- `tests/hil/test_hil_e2e.cpp`
- `tests/hil/test_hil_crash_recovery.cpp`
- `tests/hil/CMakeLists.txt` or top-level test registration
- `docs/reports/implements/phase-007-*.md`
- `docs/reports/reviews/` as needed

Actions:

1. Add HIL tests gated by `BUILD_HIL=ON`.
2. Validate FFmpeg/USB enumeration, preview, recording, stop on `jingyi-lab`.
3. Add deterministic mock plugin crash recovery HIL/fault-injection test.
4. Record hardware model, OS, SDK/runtime versions, and pass/fail evidence.
5. Document OAK-D results or explicit waiver with residual risk.

Test-first tasks:

- RED: HIL tests compile but skip clearly when hardware/env vars are missing.
- GREEN: run on `jingyi-lab` with hardware.

Exit gate:

- FFmpeg/USB HIL and mock crash recovery pass, or release report contains explicit waiver approved by owner.

### Phase 8: Release Candidate Gate

Goal: turn 007 into a shippable release candidate.

Primary files:

- `.pm/runtime/*`
- `project_index`
- `docs/wikis/*`
- `docs/reports/implements/phase-007-*.md`
- release notes file if project convention exists

Actions:

1. Run full automated test suite and lint/format checks.
2. Capture UI screenshots for all visual anchors.
3. Run independent review agent against implementation and tests.
4. Fix review findings and repeat until 0 blocking issues.
5. Update PM runtime state files and implementation report.
6. Prepare final merge recommendation for `dev` to `main`.

Exit gate:

- Human signs off release gate.
- No self-merge into `main` without explicit user approval.

## Test Strategy

### Unit Tests

- `CameraSourceModel` role exposure, source ordering, unavailable source compact state, `getDeviceAt`.
- `AppController` source refresh, camera detail lookup, plugin list fields, structured import validation, bundled toggle lock, linked plugin removal, recording lock.
- `StreamConfig` config map behavior and JSON config persistence.
- FFmpeg config-map fallback behavior.
- `AppSettings` load/persist from `micecam_config.json`.
- `StreamLivenessMonitor` cycle-count deterministic synchronization.
- `StatsCollector` snapshot path feeding UI metrics.

### Integration Tests

- Plugin registry service with bundled and linked plugin directories.
- Import valid fake plugin and invalid plugin directories.
- Plugin detail with manifest, diagnostics, capabilities, devices, streams, and config schema.
- Config validate/apply persists JSON and affects next stream open.
- App launch with bundled plugin discovery and source-grouped camera screen.
- Plugin crash and device disconnect notification path.

### QML/UI Contract Tests

- Sidebar renders source headers and nested devices.
- Camera grid renders source sections and compact unavailable rows.
- No-plugin state matches `pluginUI/no_plugin.png`.
- Plugin Management normal/import-failure/recording-lock states match visual anchors.
- Plugin Detail manifest/config/recording-config/diagnostics states match visual anchors.
- Text remains inside bounds at expected desktop and narrow window sizes.

### HIL Tests

- FFmpeg/USB camera enumeration.
- FFmpeg/USB preview, recording, stop.
- Mock plugin crash during recording and recovery/clean termination.
- OAK-D SDK-missing diagnostic state.
- OAK-D hardware path if hardware is available; otherwise waiver.

### Multi-Platform Checks

- macOS arm64 release build launches native UI.
- Windows release build launches native UI.
- Ubuntu release build launches native UI.
- Bundled plugin manifests and binaries are present in packaged artifacts.
- Linked plugin path persistence works across app restarts.

### Edge Cases

- No plugins registered.
- All plugins disabled.
- SDK missing source with zero devices.
- Linked plugin path removed after import.
- Invalid plugin manifest JSON.
- Entrypoint missing or not executable.
- Handshake fails due to incompatible API.
- Recording active while user attempts import/toggle/remove/config edit.
- Unknown future config field type.
- Plugin process unresponsive while UI is open.
- Device lacks persistent id.
- `StreamConfig.config` empty, requiring typed-field fallback.

## Rollback Plan

Because this plan is `core`, rollback must be phase-scoped and concrete.

1. **Source model rollback**
   Restore QML bindings to `appController.cameraModel`, keep `CameraSourceModel` changes behind unused compatibility roles, and revert `AppController` camera count/start logic to flat model derivation.

2. **Plugin Management rollback**
   Disable linked remove and structured import UI, keep existing bool `importPlugin` behavior, and hide restart-required/detail fields that are not fully wired.

3. **Plugin Detail/config rollback**
   Disable config editing/apply controls in QML, retain manifest-only detail rendering, stop loading plugin config JSON, and keep typed `StreamConfig` fields as the only stream-open source.

4. **Protocol/domain rollback**
   If `domain::StreamConfig.config` introduces regression, ignore the map at runtime and leave proto `StreamConfig.config` unused while preserving typed-field fallback. Remove JSON config reads from stream-open path.

5. **Notification/metrics rollback**
   Stop the UI metrics timer and crash/disconnect banners while leaving logs/activity feed as the minimal observability path.

6. **Flaky test rollback**
   Keep `cycle_count_` as an additive test helper if harmless. If it causes thread-safety issues, remove helper and quarantine the flaky test with an explicit bug report rather than weakening assertions silently.

7. **Packaging rollback**
   Revert packaging path changes while keeping source builds functional. Release remains blocked until packaging is repaired.

8. **HIL rollback**
   Keep HIL tests gated by `BUILD_HIL=ON`; if unstable, do not run them in default CI and document waiver/risk in release report.

## Security and Safety Review Points

- Linked plugin import reads arbitrary local directories; validation must not execute untrusted code until manifest/path checks pass.
- Handshake validation may start plugin processes; errors must be contained and logged without crashing UI.
- Linked plugin removal must only remove registry references, not delete user directories.
- Plugin config JSON must be written under app-controlled config directory, not plugin install directories.
- Path fields in schemas must not silently grant file write/delete permissions; they only store user-selected paths.
- Bundled plugins are locked from disable to reduce accidental no-camera release states.

## Review and Gate Checklist

| Gate | Required Evidence |
|------|-------------------|
| Human spec review | Approval of `spec.md`, `ui-spec.md`, and this plan |
| Architecture review | Review of `CameraSourceModel` single-source migration and config-map pass-through |
| Security review | Linked plugin import/remove/config persistence review |
| RED tests | Failing tests committed or documented before each implementation phase |
| GREEN implementation | Tests pass with minimal implementation |
| REFACTOR | Tests remain green after cleanup |
| Review agent | Independent review with 0 blocking issues |
| Release evidence | Build logs, screenshots, HIL logs, implementation report, release notes |

## Complexity Tracking

| Field | Value |
|-------|-------|
| Estimated | High |
| Rationale | The work spans domain config, plugin runtime, UI controller contracts, QML screens, packaging, HIL, and release gating. The risk is manageable only if phased through model contracts before visual work. |
