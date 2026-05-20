# Feature Spec: Plugin UI Integration + Spec 004 Closure

## Metadata

| Field      | Value |
|------------|-------|
| Feature ID | `007-plugin-ui-integration` |
| Branch     | `codex/007-plugin-ui-release` |
| Status     | Grilled draft / release-candidate plan |
| Owner      | `jingyi` |
| Date       | `2026-05-19` |
| Baseline   | `dev` at `ac24012` |
| Related    | `specs/003-camera-plugin-runtime/`, `specs/004-production-ready-plugin-app/` |
| Visual     | `specs/007-plugin-ui-integration/pluginUI/*.png` |
| Companion  | `specs/007-plugin-ui-integration/ui-spec.md` (visual design + screen contracts) |

## Summary

Spec 007 is the final work round before MiceCam release. It closes the gap between the fully implemented plugin backend runtime (specs 003/004/005/006) and the native Qt/QML desktop UI, and absorbs the remaining open and deferred items from spec 004. The work replaces the flat camera model with a single source-grouped data model, wires all plugin-facing QML screens to real runtime data, implements full schema-driven configuration with apply-mode enforcement, adds structured import validation and plugin lifecycle management, and validates the product through multi-platform builds and hardware-in-the-loop testing.

## Design Decisions

The following decisions were resolved through structured interrogation and are binding for implementation:

| # | Decision | Resolution |
|---|----------|------------|
| Q1 | Data model architecture | Deprecate `AppCameraModel`; `CameraSourceModel` becomes the single data source for all camera and plugin UI |
| Q2 | Sidebar data source | Bind to `sourceModel`; render two-level source-device tree |
| Q3 | Grid live data | Merge fps/drops/recording into `CameraSourceModel` roles; eliminate cross-model joins |
| Q4 | Config schema depth | Full schema-driven UI: 6 control types, 4 apply modes, validate/apply actions, recording lock |
| Q5 | Device data granularity | Expose full `DeviceInfo` + `StreamInfo` from `EnumerateDevices` RPC |
| Q6 | Live metrics update | C++ timer pushes from `StatsCollector` to `CameraSourceModel` via `dataChanged` |
| Q7 | CameraDetail data path | `Q_INVOKABLE getCameraDetail(cameraId)` with index-free lookup |
| Q8 | Restart-required granularity | Per-plugin `restartRequired` field in `pluginList()` and `getPluginDetail()` |
| Q9 | Bundled plugin disable policy | Not allowed; toggle locked with tooltip "Bundled plugin cannot be disabled" |
| Q10 | Linked plugin removal | Add remove action (calls `removeLinkedDirectory()`); disabled during recording |
| Q11 | Config apply mechanism | JSON persistence (`{config_dir}/plugin_configs/{plugin_id}.json`) + `StreamConfig.config` map pass-through; no new gRPC RPC needed |
| Q12 | Crash/disconnect notifications | Tiered: banner for runtime events, modal for critical failures, page-level warnings for management issues, all in logs/activity feed |
| Q13 | AppSettings load bug | Fix in 007: call `config_.load()` in `AppSettings` constructor |
| Q14 | Flaky test fix | Replace `sleep_for` with `cycle_count_` atomic synchronization in `StreamLivenessMonitor` |
| Q15 | HIL test scope | Full coverage including mock plugin crash recovery; OAK hardware as waiver |
| Q16 | Import error display | `importPlugin()` returns `QVariantList` of `{check, passed, detail}` per validation step |

## User Scenarios

### US-001: Browse Cameras Grouped by Source

**Priority**: P1

**Independent Test**: Launch MiceCam with FFmpeg plugin enabled and two USB cameras connected. Verify the sidebar and main grid render a collapsible "FFmpeg Official" source group containing both cameras. Verify clicking a source header scrolls to that group in the grid. Verify clicking a device opens Camera Detail.

**Acceptance Scenarios**:
- Given multiple plugin sources report devices, when the camera screen opens, then the sidebar and grid group devices by source with collapsible headers.
- Given a source has no available devices (SDK missing, disabled, error), when the camera screen renders, then the source appears as a compact row below active groups without reserving a preview card.
- Given the user clicks a source header in the sidebar, when the grid scrolls, then the corresponding source section is focused in the main area.
- Given the user clicks a device in the sidebar or grid, when the detail view opens, then `getCameraDetail(cameraId)` returns correct device data and the camera detail page renders.

### US-002: Manage Plugins

**Priority**: P1

**Independent Test**: Open Plugin Management from sidebar. Verify the plugin table shows bundled FFmpeg with locked toggle, lists name/type/version/status/device count. Verify clicking a plugin row opens Plugin Settings. Verify the "Import Plugin" button opens a folder picker.

**Acceptance Scenarios**:
- Given the user navigates to Plugin Management, when the page renders, then each plugin row shows name, type badge, version, API version, status dot, device count, and enable/disable toggle.
- Given a plugin is bundled type, when the user views its toggle, then the toggle is locked with tooltip "Bundled plugin cannot be disabled".
- Given a plugin is linked type and not recording, when the user toggles enable/disable, then the plugin status updates and `restartRequired` is set for that plugin.
- Given a linked plugin row, when the user clicks remove and confirms, then the plugin is removed and `restartRequired` is set.
- Given recording is active, when the user opens Plugin Management, then import, toggle, and remove actions are disabled with tooltip "Not available while recording".

### US-003: Import Plugin with Structured Validation

**Priority**: P1

**Independent Test**: Create a fake plugin directory missing `plugin.json`. In Plugin Management, click Import and select the directory. Verify the UI shows a structured error panel listing each validation check (manifest exists, parseable, ID valid, entrypoint exists, entrypoint executable, handshake) with individual pass/fail status.

**Acceptance Scenarios**:
- Given the user imports a valid linked plugin directory, when validation succeeds, then the plugin appears in the table with "Restart required" status.
- Given the user imports an invalid directory, when validation fails, then the UI shows an inline error panel with per-check pass/fail rows and a retry action.
- Given import succeeds, when the user views the plugin table, then a restart-required banner appears showing the count of plugins requiring restart.

### US-004: Configure Plugin Through Schema

**Priority**: P1

**Independent Test**: Open Plugin Settings for the FFmpeg plugin. Verify config fields render from schema: width (integer), height (integer), framerate (integer), pixel_format (enum), payload_kind (enum). Verify each field shows its apply-mode chip. Change a value and click "Validate Config". Verify validation result appears inline.

**Acceptance Scenarios**:
- Given a plugin returns a config schema via `GetConfigSchema`, when the Plugin Settings config section renders, then each field shows label, control matching its type, current value, apply-mode chip, and optional help text.
- Given the user edits a field, when "Validate Config" is clicked, then `ValidateConfig` RPC is called and errors appear inline per field.
- Given validation passes and there are changes, when "Apply Changes" is clicked, then config is persisted to `{config_dir}/plugin_configs/{plugin_id}.json` and fields with `requires-restart` mode set the plugin's `restartRequired` flag.
- Given recording is active, when the config section renders, then `runtime-safe` fields remain editable and all other fields are locked with a lock icon.

### US-005: View Plugin Details and Diagnostics

**Priority**: P1

**Independent Test**: Import a fake plugin with a diagnostic error (e.g., incompatible API). Open Plugin Settings. Verify the diagnostics section shows the error with code, severity, user message, technical detail, and suggested action. Verify the manifest section shows plugin ID, version, API version, author, path. Verify the devices section lists enumerated devices with status and stream info.

**Acceptance Scenarios**:
- Given a plugin has active diagnostics, when Plugin Settings renders, then the diagnostics section shows each issue with code, severity, message, suggested action, and recoverability.
- Given a plugin manifest is loaded, when the manifest section renders, then plugin ID, display name, version, API version, source type, process model, author, path, required features, and optional features are displayed.
- Given a plugin has enumerated devices, when the devices section renders, then each device shows display name, persistent ID, status, stream count, and payload badges.

### US-006: Receive Plugin Crash and Device Disconnect Alerts

**Priority**: P1

**Independent Test**: Start recording with a mock plugin. Kill the plugin process externally. Verify a crash alert banner appears in the UI with the plugin name and recovery status. Verify the banner updates when recovery completes or fails.

**Acceptance Scenarios**:
- Given a plugin crashes during operation, when the crash is detected, then a red alert banner appears showing the plugin name and "Attempting recovery...".
- Given crash recovery succeeds, when the plugin reconnects, then the banner updates to show success and auto-dismisses after a few seconds.
- Given crash recovery fails, when the retry limit is reached, then a modal dialog appears requiring user acknowledgment.
- Given a device disconnects, when the disconnect is detected, then a warning banner appears with the device name and suggested action.

### US-007: Fix Flaky Test and Complete Spec 004 Closure

**Priority**: P1

**Independent Test**: Run `test_stream_liveness_monitor::StallCountResetsOnActivity` on macOS 10 times. Verify all runs pass with the `cycle_count_` synchronization mechanism.

**Acceptance Scenarios**:
- Given the `StreamLivenessMonitor` runs its check cycle, when the cycle completes, then `cycle_count_` is incremented atomically.
- Given a test waits for `cycle_count_` to reach a target, when the target is reached, then the test proceeds without `sleep_for` timing dependencies.
- Given all 007 work is complete, when the branch is proposed for release, then `dev` is merged to `main` with all tests passing on macOS and Ubuntu.

### US-008: Validate Release on Multiple Platforms

**Priority**: P1

**Independent Test**: Build MiceCam on macOS and verify the packaged app launches, shows the plugin source-grouped camera screen, and all plugin management flows work.

**Acceptance Scenarios**:
- Given release builds are requested, when the build completes on each supported platform, then the app launches to the camera screen with plugin runtime loaded.
- Given the packaged app runs, when plugin search paths are resolved, then bundled plugins are discovered and linked plugins persist across restarts.
- Given `AppSettings` is saved, when the app restarts, then settings are loaded correctly from `micecam_config.json`.

### US-009: Hardware-in-the-Loop Validation

**Priority**: P1

**Independent Test**: On `jingyi-lab`, run HIL tests against two USB cameras via FFmpeg plugin. Verify device enumeration, preview, recording, and stop pass. Run crash recovery test by killing a mock plugin during recording.

**Acceptance Scenarios**:
- Given FFmpeg/USB cameras are connected, when HIL tests run, then device enumeration, preview, recording, and stop all pass.
- Given a mock plugin is killed during recording, when crash recovery runs, then the plugin reconnects and recording resumes or terminates cleanly.
- Given OAK-D hardware is unavailable, when the release report is written, then OAK results are documented as an explicit waiver with residual risk.

## Requirements

### Functional Requirements

**Source Data Model (Phase 1)**

- **FR-001**: Deprecate `AppCameraModel`; all camera and source data flows through `CameraSourceModel` as the single data source.
- **FR-002**: `CameraSourceModel` must expose source-level roles: `sourceId`, `sourceName`, `sourceType`, `pluginVersion`, `pluginApiVersion`, `enabled`, `diagnosticsState`, `diagnosticsMessage`, `restartRequired`, `deviceCount`, `availableDeviceCount`, `isExpanded`.
- **FR-003**: `CameraSourceModel::getDeviceAt(sourceIdx, devIdx)` must return device-level data including all existing `CameraRow` fields (cameraId, name, fps, dropCount, status, isRecording, resolution/framerate/format options) plus `DeviceInfo` fields (persistentId, vendor, serial, status, streams with their payloads and capabilities).
- **FR-004**: `refreshCameras()` must populate `CameraRow.sourceId` and `sourceGroup` from the originating plugin's source data.
- **FR-005**: `PluginSource.device_ids` must be populated by mapping enumerated devices back to their originating plugin source.

**Source-Grouped Camera Screen (Phase 2)**

- **FR-006**: The sidebar must render a two-level tree: collapsible source headers with nested device rows, replacing the flat `DEVICES` list.
- **FR-007**: The main camera grid must render source-section headers followed by device preview cards, with unavailable sources as compact rows below active groups.
- **FR-008**: Source ordering must follow: active > available > warning > disabled, with bundled before linked before unknown within each tier.
- **FR-009**: Empty states must show contextual messages: no plugins registered, all plugins disabled, restart required, with a link to Plugin Management.

**Plugin Management (Phase 3)**

- **FR-010**: The plugin table must show per-plugin `restartRequired` status; a banner must display the count of plugins requiring restart when > 0.
- **FR-011**: Bundled plugins must have their enable/disable toggle locked with tooltip "Bundled plugin cannot be disabled".
- **FR-012**: Linked plugins must expose a remove action that calls `removeLinkedDirectory()` and sets `restartRequired`.
- **FR-013**: `importPlugin()` must return `QVariantList` where each element is `{check: string, passed: bool, detail: string}` for every validation step (manifest exists, parseable, ID valid, entrypoint exists, executable, handshake accepted).
- **FR-014**: Import failure must render an inline error panel above the plugin table showing per-check results with a retry action.
- **FR-015**: Recording lock must disable import, toggle, and remove controls with tooltip "Not available while recording".

**Plugin Settings (Phase 4)**

- **FR-016**: Plugin Settings must render manifest, diagnostics, capabilities, devices, and config sections as full-width operational areas.
- **FR-017**: Device list must show full `DeviceInfo` + `StreamInfo` data per device with persistent ID, status, stream count, and payload badges.
- **FR-018**: `GetConfigSchema` RPC must be called and its fields rendered as typed controls: text field (string), stepper/numeric (integer), slider+numeric (float), checkbox/switch (boolean), dropdown (enum), path field+picker (path).
- **FR-019**: Each config field must display its apply-mode chip: `runtime-safe`, `pre-open`, `pre-record`, or `requires-restart`.
- **FR-020**: "Validate Config" must call `ValidateConfig` RPC and display per-field validation errors inline.
- **FR-021**: "Apply Changes" must persist config to `{config_dir}/plugin_configs/{plugin_id}.json` and set `restartRequired` for any `requires-restart` fields.
- **FR-022**: During recording, config fields with apply mode other than `runtime-safe` must be disabled with a lock icon; `runtime-safe` fields remain editable.
- **FR-023**: `getPluginDetail()` must return full data: manifest metadata, diagnostics, capabilities, devices (with streams), and config schema.

**Live Data and Notifications**

- **FR-024**: A C++ timer must push per-stream fps and drop count from `StatsCollector` to `CameraSourceModel` device roles during recording, emitting `dataChanged` signals.
- **FR-025**: Plugin crash alerts must surface as a top banner (amber for recoverable, red for fatal) with auto-dismiss on recovery; unrecoverable crashes must show a modal dialog.
- **FR-026**: Device disconnect must surface as a warning banner with device name and suggested action.
- **FR-027**: All plugin events must appear in the logs/activity feed.

**Config Infrastructure**

- **FR-028**: The domain `StreamConfig` struct must include a `std::map<std::string, std::string> config` field.
- **FR-029**: When opening a stream, stored config values must be read from `{config_dir}/plugin_configs/{plugin_id}.json` and populated in the proto `StreamConfig.config` map.
- **FR-030**: FFmpeg plugin `OpenStream` must read `config.config()` map and apply values (width, height, framerate, pixel_format, payload_kind) instead of using only typed proto fields.
- **FR-031**: `AppSettings` must call `config_.load("micecam_config.json")` in its constructor so settings persist across restarts.

**Spec 004 Inherited Items**

- **FR-032**: `StreamLivenessMonitor` must expose an atomic `cycle_count_` incremented after each monitor loop iteration; tests must spin-wait on `cycle_count_` instead of `sleep_for`.
- **FR-033**: `test_hil_e2e.cpp` must validate FFmpeg/USB enumeration, preview, recording, and stop on real hardware.
- **FR-034**: `test_hil_crash_recovery.cpp` must validate plugin crash recovery by killing a mock plugin process during recording.
- **FR-035**: OAK-D HIL results are documented as an explicit waiver in the release report when hardware is unavailable.

### Non-Functional Requirements

- **NFR-001**: Source-grouped rendering must not introduce measurable latency compared to the flat camera list for up to 4 sources with 8 devices.
- **NFR-002**: Config schema rendering must handle schemas with up to 20 fields without UI lag.
- **NFR-003**: Live metrics timer must update fps/drops at most once per second to avoid excessive signal traffic.
- **NFR-004**: Plugin Management and Plugin Settings must remain responsive when a plugin process is unresponsive or crashed.
- **NFR-005**: Multi-platform builds (macOS arm64, Windows, Ubuntu) must produce launchable artifacts that reach the camera screen with plugin runtime loaded.
- **NFR-006**: The flaky test `StallCountResetsOnActivity` must pass 10/10 consecutive runs on macOS CI.

## Success Criteria

| # | Criterion | Measured By |
|---|-----------|-------------|
| SC-1 | Camera grid groups devices by source/plugin | Manual UI check against `pluginUI/no_plugin.png` and `pluginUI/plugin_manage.png` visual anchors |
| SC-2 | Unavailable sources render as compact rows without preview cards | Manual UI check with SDK-missing OAK plugin |
| SC-3 | Plugin Management shows per-plugin status, restart banner, and structured import errors | Manual UI check with valid and invalid plugin imports |
| SC-4 | Plugin Settings renders schema-driven config with all 6 control types and apply-mode enforcement | Unit test + manual check against FFmpeg plugin schema |
| SC-5 | Recording lock disables non-runtime-safe config fields and plugin modification controls | Manual check during active recording |
| SC-6 | Plugin crash shows banner notification with recovery status | Fault injection test |
| SC-7 | Flaky test passes 10/10 on macOS | CI run |
| SC-8 | HIL tests pass for FFmpeg/USB path and mock crash recovery | `jingyi-lab` test run |
| SC-9 | Multi-platform builds launch to camera screen | Build + smoke test per platform |
| SC-10 | `dev` merged to `main` with all tests green | Git merge + CI |
| SC-11 | AppSettings persist across restarts | Automated test |
| SC-12 | Config values persist and apply on next stream open | Integration test |

## Assumptions

- Both bundled plugins (FFmpeg, OAK) have only `PRE_OPEN` apply-mode config fields; `RUNTIME_SAFE` fields do not exist yet but the UI must support the apply-mode display for future plugins.
- Plugin config is stored on the host side as JSON; plugins read config values via the `StreamConfig.config` map at stream open time.
- Application restart after plugin import/toggle/remove remains acceptable (no hot reload).
- `jingyi-lab` has 2 USB cameras available for HIL testing; OAK-D hardware is not required.
- The existing `CameraSourceModel` implementation provides the correct grouping logic and only needs data pipeline fixes and role expansion.
- The existing `PluginManagementPage.qml` and `PluginDetailPage.qml` are functional and need enhancement, not rewrite.

## Clarifications

- None currently. All design decisions were resolved through structured interrogation (Q1–Q16).

## Out of Scope

- Custom plugin-rendered UI controls.
- Plugin marketplace or remote plugin distribution.
- Hot plugin reload without application restart.
- Plugin signing, sandboxing, or malware scanning UI.
- Deep OCT domain-specific visualization beyond generic plugin config/device/diagnostic support.
- Redesigning the entire MiceCam shell beyond the plugin integration path.
- Adding new gRPC RPCs (e.g., `SetConfig`); config flows through existing `StreamConfig.config` map and host-side JSON persistence.
- OAK-D hardware testing when hardware is unavailable (waiver required).

## Risk Notes

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| `CameraSourceModel` expansion breaks existing sidebar/grid bindings | Medium | High | Incremental migration: expand roles first, then rebind QML one component at a time |
| Config schema rendering for unknown future field types | Low | Medium | Render unknown types as read-only text; schema version check |
| `StreamConfig.config` map passthrough breaks existing FFmpeg plugin behavior | Low | High | FFmpeg plugin must fall back to typed proto fields when config map is empty |
| Multi-platform build failures due to plugin path resolution | Medium | Medium | Test packaged app plugin discovery on each platform before merge |
| HIL crash recovery test timing sensitivity | Medium | Medium | Use deterministic mock plugin with known crash points; avoid real-time dependencies |
