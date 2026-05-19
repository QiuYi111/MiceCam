# UI Spec: Plugin Runtime Integration

## Metadata

| Field | Value |
|-------|-------|
| Spec ID | `007-plugin-ui-integration` |
| Document | `ui-spec.md` |
| Status | Release-candidate scope (+ spec 004 closure inherited) |
| Target | Native Qt/QML UI, plugin runtime wiring, multi-platform release validation, spec 004 closure items |
| Baseline | `dev` at `ac24012` |
| Related | `specs/003-camera-plugin-runtime/`, `specs/004-production-ready-plugin-app/`, `docs/requirements/plugin-camera-backend-system.md` |
| Visual anchors | `specs/007-plugin-ui-integration/pluginUI/*.png` |

## Purpose

The backend plugin runtime is now largely implemented, but the native UI is still only partially adapted to it. This spec defines how the plugin system should appear in MiceCam without disrupting the current successful UI structure.

Additionally, spec 004 (`production-ready-plugin-app`) has been downgraded to a closure gate — its 27/28 FRs are implemented, but several open and deferred items remain. Spec 007 absorbs these items and becomes the **single closing spec** for both 004 and 007.

The primary rule: **do not collapse camera operation, plugin management, and plugin configuration into one screen**. They are separate workflows and must remain separate screens.

The visual drafts in `pluginUI/` are the canonical design anchors for this spec. Text in this document clarifies behavior, state rules, and data contracts; where layout intent is ambiguous, prefer the visual draft over older prose.

## Release Scope

Spec 007 is the final work round before release. It is not only a visual UI task. It closes the remaining gap between the solid backend plugin runtime and a shippable desktop product.

Release scope:

1. **Plugin UI design completion**
   Implement the visual and interaction model defined by `pluginUI/` without a broad redesign of the existing app shell.

2. **Plugin UI wiring**
   Connect the native QML screens to real plugin/source/device data instead of demo or flat camera models.

3. **Plugin workflow hardening**
   Complete import, enable/disable, restart-required, diagnostics, recording-lock, and schema-config flows.

4. **Multi-platform build validation**
   Produce and verify release builds for the supported desktop platforms before declaring the branch releasable.

5. **Hardware-in-the-loop validation**
   Run HIL checks against the supported camera/plugin matrix, including FFmpeg/USB and OAK-D paths where hardware is available.

6. **Release gate**
   007 is complete only when UI, wiring, automated tests, multi-platform builds, HIL results, docs, and release notes are all ready for final user review.

Non-goals for 007:

- new plugin marketplace
- remote plugin distribution
- plugin signing/sandboxing UX
- large product redesign outside the plugin runtime integration path
- deep OCT-specific visualization beyond generic plugin config/device/diagnostic support

## Inherited from Spec 004

Spec 004 (`production-ready-plugin-app`) is a closure gate with 27/28 FRs already implemented on `dev`. The following open and deferred items are inherited into 007 and must be resolved before or alongside the 007 release gate.

### Blocking

| # | Item | Origin | Action |
|---|------|--------|--------|
| I-1 | Fix flaky `StallCountResetsOnActivity` on macOS | FR-026 blocker | Harden timing in `test_stream_liveness_monitor`; branch `fix/flaky-stallcount-test` |
| I-2 | Merge `dev` to `main` | FR-026 | Execute after I-1 passes; this is the final gate |

### Non-blocking / Deferred

| # | Item | Origin | Action |
|---|------|--------|--------|
| I-3 | Create HIL tests (`test_hil_e2e`, `test_hil_crash_recovery`) | Deferred | Gated behind `BUILD_HIL=ON`; branch `feat/hil-tests` |
| I-4 | Update stale `.pm/runtime/` state files | Deferred | `state.yaml`, `acceptance-review.md`, `active-stage.md`, `handoff.md` |
| I-5 | Manual UI sign-off | Deferred | Post-merge visual validation against `pluginUI/` anchors |

### Integration into 007 Phases

These inherited items map to 007 implementation phases as follows:

- **Phase 1–4** (UI work): No dependency on inherited items.
- **I-1** (flaky test fix): Should land early, ideally before Phase 5, so all subsequent CI runs are clean.
- **I-2** (merge dev→main): Final step of Phase 8 (Release Candidate Gate).
- **I-3** (HIL tests): Maps to Phase 7 (HIL Closure). Create test files and run on `jingyi-lab`.
- **I-4** (PM state update): Maps to Phase 8. Update after all work is complete.
- **I-5** (UI sign-off): Maps to Phase 5 (UI Smoke and Sign-Off). Compare screenshots against visual anchors.

## Visual Design Inputs

| File | Canonical state/screen | Required interpretation |
|------|------------------------|-------------------------|
| `pluginUI/no_plugin.png` | Camera Sources empty state | No camera plugin is registered; sidebar and main canvas both show a plugin-oriented empty state with a direct path to Plugin Management. |
| `pluginUI/plugin_manage.png` | Plugin Management normal state | Separate administrative screen with source-grouped sidebar preserved, import/refresh actions, restart hint, status table, device counts, and enable toggles. |
| `pluginUI/plugin_fail.png` | Plugin Management import failure | Import errors appear inline above the plugin table with structured validation failures and a retry/folder-selection action. |
| `pluginUI/recording_lock.png` | Plugin Management while recording | Add/remove/toggle/settings actions are disabled during recording; lock banner and tooltips explain the restriction. |
| `pluginUI/plugin_setting.png` | Per-plugin configuration while recording | Plugin config page remains reachable; only runtime-safe fields are editable during active recording, other fields are visibly locked. |
| `pluginUI/plugin_config.png` | Per-plugin configuration idle state | Schema-driven config rows show controls, apply-mode chips, validation state, help text, and footer actions. |
| `pluginUI/plugin_manifest.png` | Per-plugin manifest/details | Manifest, capabilities, and devices are shown as a plugin detail screen reached from Plugin Management. |
| `pluginUI/diagnose.png` | Per-plugin diagnostics | Diagnostics show recoverability, user message, technical detail, suggested action, and collapsed related sections. |

Visual invariants from the drafts:

- Keep the existing app shell: title bar, record controls, top-right utility buttons, left navigation, and bottom session status bar.
- Use the left sidebar as persistent navigation. Plugin pages do not remove Camera Sources; they only change the main content area.
- Source grouping remains visible even while viewing plugin management/detail pages.
- Plugin Management and each plugin detail/config/diagnostic view are separate main-content screens, not stacked into one dense all-in-one page.
- Warning, restart-required, and lock states use inline banners instead of modal-first flows.
- Tables and section rows should be dense and operational; avoid marketing-style cards or oversized explanatory panels.

## Design Principles

1. **Keep the camera screen operational**
   The camera screen is for choosing cameras, checking previews, and starting/stopping recording. It must not become a plugin administration dashboard.

2. **Group cameras by source, not by implementation detail**
   Operators need to understand which cameras came from FFmpeg, OAK, or linked plugins. They do not need to see raw gRPC/process details during recording.

3. **Unavailable plugins should be compact**
   A source with no devices, missing SDK, disabled state, or restart-required state must not allocate a large empty preview area. It should collapse into a small row at the bottom of the camera screen.

4. **Plugin management is a separate tool surface**
   Importing, enabling, disabling, and inspecting plugins belongs on a dedicated `Plugins` screen.

5. **Plugin settings are per-plugin detail screens**
   Manifest details, diagnostics, capabilities, and schema-driven configuration should live on a plugin detail/settings page reached from Plugin Management.

6. **Avoid large UI rewrites**
   The integration should extend the existing layout:
   - existing title bar and toolbar stay
   - existing sidebar stays
   - existing camera grid visual language stays
   - existing settings stack/navigation model stays

## Current UI Gap

Current implementation has useful pieces but is not fully wired:

- `PluginManagementPage.qml` exists and calls `appController.pluginList()`.
- `PluginDetailPage.qml` exists and calls `appController.getPluginDetail()`.
- `CameraSourceModel` exists.
- `AppController::sourceModel()` exists.

But:

- `CameraGridView.qml` still renders flat `appController.cameraModel`.
- `AppSidebar.qml` still renders flat `appController.cameraModel`.
- `AppController::refreshCameras()` clears `sourceId` and `sourceGroup` on each `CameraRow`.
- `CameraSourceModel::populateFromSources()` is currently passed an empty `plugin_devices` vector.
- The primary camera workflow does not yet show source grouping, plugin diagnostics, conflict/lock state, or restart-required source state.

## Information Architecture

The plugin UI has three top-level screen families. Plugin Settings may expose multiple detail subviews, but each subview still occupies the main content area as a distinct screen/state.

### Screen 1: Camera Sources

Primary operational screen. Replaces the flat camera list/grid with source-grouped camera browsing while preserving the existing home screen experience.

Contains:

- top toolbar with Record, Alerts, Fullscreen, Settings
- left sidebar with source-grouped camera navigation
- main camera preview grid grouped by source/plugin
- compact collapsed rows for unavailable sources at bottom
- bottom status bar with recording/session summary

Does not contain:

- plugin import button
- enable/disable toggles
- full manifest tables
- schema configuration forms

### Screen 2: Plugin Management

Administrative screen reached from sidebar `Plugins`.

Contains:

- plugin table/list
- import plugin action
- enable/disable toggles
- restart-required banner
- recording lock state
- plugin diagnostics summary
- row click to open Plugin Settings

Does not contain:

- camera previews
- stream selection
- recording controls except passive lock/read-only state

### Screen 3: Plugin Settings / Detail

Detail/config screen for one plugin, reached from Plugin Management.

Contains:

- back navigation to Plugin Management
- manifest summary
- diagnostics
- capabilities
- device list for this plugin
- schema-driven config controls
- apply-mode rules
- validate/apply/disable actions

Does not contain:

- global camera preview grid
- unrelated plugins
- recording dashboard metrics except lock hints

Plugin Settings includes these canonical subviews:

- manifest/detail view, anchored by `plugin_manifest.png`
- configuration view, anchored by `plugin_config.png` and `plugin_setting.png`
- diagnostics view, anchored by `diagnose.png`

## Screen 1: Camera Sources

### Sidebar Structure

Replace the flat `DEVICES` list with source groups.

Example:

```text
CAMERA SOURCES

▾ FFmpeg Official        ●
  USB Microscope         ●
  JYCAMERA 12M           ●

▸ OAK-D Official         ▲
  SDK missing

▸ Linked Plugins
  Lab OCT Adapter        restart

SETTINGS
  Encoding
  Alerts
  Logging
  Plugins
  About
```

Rules:

- Source rows are collapsible.
- Healthy source rows show a green status dot.
- Warning/error rows show amber/red status dot.
- Device rows are indented under sources.
- Device rows show status dot and short device name.
- If a source has no available devices, show one compact reason line under the source.
- Clicking a source header scrolls/focuses that source group in the main area.
- Clicking a device opens the existing Camera Detail view for that stream/device.

### Main Area Layout

The main camera area is grouped by source.

Each source section:

- source header
- preview cards for available streams/devices
- optional compact diagnostic row

Healthy source section:

```text
FFmpeg Official  API v2  2 devices  OK
[USB Microscope preview card] [JYCAMERA 12M preview card]
```

Unavailable source section:

```text
OAK-D Official  SDK missing  0 devices
```

Unavailable source sections must be compact and placed below all sections with active preview cards.

### Preview Card Rules

Preview cards continue using the existing MiceCam style:

- image/video preview is dominant
- top-left stream/camera label
- bottom overlay with fps and drops
- optional status badges

New plugin metadata should be subtle:

- source name as a small pill or subtitle
- payload badge: `RAW`, `MJPEG`, `H264`, `H265`
- optional lock/conflict icon
- optional diagnostic icon

Do not put full manifest/config data inside preview cards.

### Source Ordering

Order source groups:

1. sources with selected/active devices
2. sources with available devices
3. sources with warnings but no devices
4. disabled/missing/restart-required sources

Within each group:

1. bundled official plugins
2. linked plugins
3. unknown/invalid sources

### Empty States

Global no-camera state:

- If no plugins are registered: show "No camera plugins registered" with a button/link to Plugins screen.
- If plugins exist but no devices: show compact list of source diagnostics.
- If all plugins are disabled: show "All camera plugins are disabled" with link to Plugins screen.
- If restart is required: show "Restart required to load plugin changes".

No state should create a giant blank preview card.

`pluginUI/no_plugin.png` is the anchor for the no-plugin state:

- sidebar Camera Sources area shows a small camera/plugin icon and short helper text
- main content shows a centered empty-state panel with the same diagnosis and `Open Plugins` primary action
- secondary link may point to plugin setup documentation
- below the main empty state, show compact source-health rows such as plugin discovery, camera connections, and active sources
- bottom session status bar remains visible even when no cameras exist

## Screen 2: Plugin Management

### Navigation

Sidebar item:

```text
Plugins
```

Clicking it opens Plugin Management as its own screen in the existing stack.

### Page Layout

Top:

- title: `Plugin Management`
- primary action: `Import Plugin`
- secondary action: `Refresh` when discovery can be rerun
- optional restart hint

Banner area:

- show only when relevant
- examples:
  - `Plugin changes take effect after restart`
  - `Plugin changes are locked while recording`
  - `1 plugin failed validation`

Main table columns:

| Column | Meaning |
|--------|---------|
| Name | Display name |
| Type | `Bundled` or `Linked` |
| Version | Plugin semantic version |
| API | Plugin API version |
| Status | OK, Disabled, Missing, Error, Incompatible, Restart required |
| Devices | Current device count |
| Enabled | Toggle |

Row click opens Plugin Settings.

`pluginUI/plugin_manage.png` is the normal-state anchor. Preserve these details:

- main content starts below the existing toolbar, not as a modal or overlay
- plugin status table uses compact rows with icon, plugin name, type, version, API badge, status badge, device count, and enable toggle
- sidebar source groups show live plugin/source health at the same time as the management table
- restart-required status is represented both in the table and in the source group summary when applicable

### Status Semantics

Status labels:

- `OK`: plugin loaded and healthy
- `Disabled`: explicitly disabled, not launched
- `Missing`: linked path no longer exists
- `Error`: manifest/entrypoint/handshake/runtime failure
- `Incompatible`: API version unsupported
- `Restart required`: config changed but runtime not reloaded

Color mapping:

- green: OK
- gray: Disabled
- amber: Missing, Restart required, SDK missing
- red: Error, Incompatible

### Import Plugin Flow

User flow:

1. Click `Import Plugin`.
2. Folder picker opens.
3. User selects directory.
4. UI validates:
   - `plugin.json` exists
   - manifest is parseable
   - plugin id is valid
   - platform entrypoint exists
   - entrypoint is executable
   - handshake accepts API version
5. On success:
   - row appears in table
   - status shows `Restart required`
   - linked path is visible in detail page
6. On failure:
   - no row is added unless partial diagnostics are intentionally tracked
   - inline error explains the reason
   - recent log receives structured event

`pluginUI/plugin_fail.png` is the import-failure anchor:

- error is an inline red-tinted panel above the plugin table
- top line states the failed operation and user-readable cause
- each failed validation check is shown as a discrete row/tag with a short explanation
- include an action to choose another folder
- preserve the plugin table below the error panel so the user can continue managing existing plugins

### Recording Lock

While recording:

- Import button disabled.
- Enable/disable toggles disabled.
- Destructive actions disabled.
- Detail/settings can be opened with field-level locks.
- Runtime-safe fields on Plugin Settings may remain editable only if explicitly supported.

Tooltip text:

```text
Not available while recording
```

`pluginUI/recording_lock.png` is the management lock anchor:

- show a red lock banner: plugin changes are locked while recording
- import button and row actions are visibly disabled
- disabled controls expose the tooltip `Not available while recording`
- a footer hint repeats that all plugin management actions are disabled during recording
- do not hide plugins or settings routes; show them disabled/read-only so the operator understands state

### Bundled Plugin Rules

Bundled official plugins may be disabled only if product policy allows it.

If allowed:

- disabling official FFmpeg can leave the app with no cameras
- UI must warn before disabling the last healthy camera source

If not allowed:

- show toggle locked
- tooltip: `Bundled plugin cannot be disabled`

This policy must be decided before implementation.

## Screen 3: Plugin Settings / Detail

### Entry

Opened by clicking a row in Plugin Management.

Header:

```text
< Back to Plugins
Lab OCT Adapter
Linked · API v2 · Restart required
```

Visual anchors:

- `pluginUI/plugin_manifest.png`: detail/manifest-oriented plugin view
- `pluginUI/plugin_config.png`: idle configuration view
- `pluginUI/plugin_setting.png`: recording-active configuration view
- `pluginUI/diagnose.png`: diagnostics-oriented plugin view

Header rules:

- show back navigation to Plugins
- show plugin icon, display name, and compact status badges
- status badges may include `Linked`, `Bundled`, `API v2`, `Restart required`, `0 devices`, or health state
- plugin documentation link is allowed when supplied by manifest
- a chevron/overflow affordance may expose less common plugin-level actions

### Page Sections

Use full-width operational sections, not nested decorative cards. The visual drafts use bordered sections/tables; match that density and avoid placing cards inside cards.

Sections:

1. Manifest
2. Diagnostics
3. Capabilities
4. Devices
5. Configuration
6. Actions

### Manifest Section

Fields:

- plugin id
- display name
- version
- API version
- minimum MiceCam version
- source type
- plugin path
- platform entrypoint
- process model
- required features
- optional features

Long paths should truncate in the middle and show full value in tooltip/copy action.

`pluginUI/plugin_manifest.png` is the manifest anchor:

- manifest rows are key/value rows with copy affordances for identifiers and paths
- required and optional features render as compact chips
- capabilities appear as a compact horizontal section below manifest
- devices appear below capabilities with serial/interface/firmware details and per-device menu
- rescan devices action belongs in the device section, not the global toolbar

### Diagnostics Section

Show latest structured diagnostic:

- code
- severity
- user message
- technical detail
- affected device/stream if any
- suggested action
- recoverable flag

Examples:

```text
SDK_MISSING · Warning
DepthAI SDK is not available. OAK-D plugin can be installed but cannot enumerate hardware.
Suggested action: install DepthAI runtime or disable plugin.
```

```text
API_INCOMPATIBLE · Error
Plugin API v1 is not supported by this MiceCam build.
Suggested action: update plugin to API v2.
```

`pluginUI/diagnose.png` is the diagnostics anchor:

- diagnostics can be the primary visible plugin subview when a plugin has active health issues
- top section shows issue count and plugin identity
- each issue is expandable/collapsible
- each expanded issue shows user message, technical detail, suggested action, and recoverable state
- recoverable warnings use amber; incompatible/crash/import failures use red
- related sections such as Capabilities and Devices may be collapsed summary rows under Diagnostics

### Capabilities Section

Show compact capability chips:

- payloads: `RAW`, `MJPEG`, `H264`, `H265`
- config schema available
- runtime-safe config available
- device hotplug supported
- process models: singleton / per-device / per-stream
- shared memory transport supported

Do not expose raw proto field names unless in an advanced details disclosure.

### Devices Section

List devices reported by this plugin:

| Field | Meaning |
|-------|---------|
| Display name | Human-readable device name |
| Persistent id | Stable id when available |
| Status | Available, Busy, Missing, Error |
| Streams | Stream count |
| Payloads | RAW/MJPEG/H264/H265 |
| Conflict | Exclusive resource warning |

If persistent id is missing:

```text
Configuration cannot be persisted for this device because the plugin did not provide a stable persistent id.
```

### Configuration Section

Render plugin schema using standard controls only.

Supported controls:

| Schema Type | UI Control |
|-------------|------------|
| string | text field |
| integer | stepper or numeric input |
| float | slider + numeric input |
| boolean | checkbox or switch |
| enum | dropdown |
| path | path field + picker button |

Apply modes:

- `runtime-safe`: editable while recording
- `pre-open`: editable only before stream open
- `pre-record`: editable only before recording
- `requires-restart`: editable when idle, shows restart-required marker

Each configurable row should show:

- label
- value control
- apply-mode chip
- validation state
- optional help text
- optional lock/help icons when the current recording/open state prevents editing

Example:

```text
Exposure       [------|----]  12.4 ms       runtime-safe
Gain           [-] 4 [+]                    pre-record
Codec          [H264 v]                     pre-open
Output path    [/data/session] [folder]     requires-restart
```

`pluginUI/plugin_config.png` is the idle configuration anchor:

- configuration rows use a left label/description, middle control, apply-mode chip, validation icon, and right help text
- sliders pair with numeric inputs for precise values
- steppers are used for small integer ranges
- dropdowns are used for enums such as codec
- path fields include a folder picker icon
- footer actions are `Validate Config`, `Apply Changes`, `Reset`, and `Disable Plugin`

`pluginUI/plugin_setting.png` is the recording-active configuration anchor:

- show a red/amber recording-active banner at the top of the content area
- runtime-safe rows remain editable and visually active
- pre-record, pre-open, and requires-restart rows are disabled and show lock icons
- footer copy explains that only runtime-safe changes are applied while recording
- apply/validate actions remain available only for editable runtime-safe changes

### Validation and Apply

Actions:

- `Validate Config`
- `Apply Changes`
- `Reset`
- `Disable Plugin`

Rules:

- `Validate Config` calls plugin `ValidateConfig`.
- Invalid values show inline errors.
- `Apply Changes` is enabled only when there are changes and validation passes.
- `requires-restart` changes set global restart-required state.
- Runtime-safe changes can apply during recording only if backend supports them.

## Alerts and Diagnostics Integration

Plugin UI must connect to the existing alert model.

Events that should surface:

- plugin import success/failure
- plugin missing path
- incompatible API
- plugin startup failure
- plugin crash
- device disconnected
- device busy/exclusive lock
- restart required
- config validation failure

Alert display rules:

- critical runtime failures appear as banner/modal depending on severity
- management-only warnings appear in Plugin Management and Plugin Settings
- all plugin events should appear in logs/activity feed

## Data Model Requirements for UI

### Camera Row

Each camera row/card needs:

- `cameraId`
- `name`
- `sourceId`
- `sourceGroup`
- `pluginType`
- `persistentDeviceId`
- `streamIndex`
- `status`
- `diagnostics`
- `payloads`
- `exclusiveResourceId`
- `isSelectable`
- `isRecording`
- `resolutionLabels`
- `framerateLabels`
- `formatLabels`

### Source Row

Each source row needs:

- `sourceId`
- `sourceName`
- `sourceType`
- `pluginVersion`
- `pluginApiVersion`
- `enabled`
- `diagnosticsState`
- `diagnosticsMessage`
- `restartRequired`
- `deviceCount`
- `availableDeviceCount`
- `isExpanded`

### Plugin Row

Each plugin management row needs:

- `pluginId`
- `name`
- `type`
- `version`
- `apiVersion`
- `status`
- `statusMessage`
- `deviceCount`
- `enabled`
- `restartRequired`
- `path`
- `canToggle`
- `canOpenSettings`

## Navigation Rules

Recommended stack indices or routes:

| Route | Screen |
|-------|--------|
| `cameras` | Camera Sources |
| `encoding` | Encoding settings |
| `alerts` | Alerts settings |
| `logging` | Logging settings |
| `plugins` | Plugin Management |
| `pluginDetail/:pluginId` | Plugin Settings |
| `pluginDetail/:pluginId/manifest` | Plugin manifest/detail subview |
| `pluginDetail/:pluginId/config` | Plugin configuration subview |
| `pluginDetail/:pluginId/diagnostics` | Plugin diagnostics subview |
| `cameraDetail/:cameraId` | Camera Detail |

Back behavior:

- Plugin Settings back returns to Plugin Management.
- Camera Detail back returns to Camera Sources.
- Settings pages back return to previous top-level view or Camera Sources.

## Implementation Boundaries

### In Scope

- Source-grouped sidebar.
- Source-grouped camera grid.
- Plugin Management as separate screen.
- Plugin Settings as separate screen.
- Basic schema-driven config controls.
- Diagnostics and restart-required UI.
- Recording lock behavior.
- End-to-end QML/controller wiring for plugin sources, plugin rows, plugin details, diagnostics, config validation, and apply actions.
- Multi-platform release build verification.
- Hardware-in-the-loop validation for release-critical camera/plugin paths.
- Release notes, implementation report, and final sign-off checklist.

### Out of Scope

- Custom plugin-rendered UI.
- Plugin marketplace.
- Hot plugin reload.
- Remote plugin download.
- Plugin signing/sandboxing UI.
- Deep OCT domain-specific visualization.
- Redesigning the entire MiceCam shell.

## Acceptance Criteria

### AC-001: Camera Sources Screen Is Grouped

Given multiple plugin sources exist, when the camera screen opens, then the sidebar and main preview grid group devices by source/plugin.

### AC-002: Unavailable Sources Are Compact

Given a source has no available devices, when the camera screen renders, then the source appears as a compact row after active preview groups and does not reserve a preview card.

### AC-003: Flat Camera UI Is Removed from Primary Workflow

Given `sourceModel` is populated, when QML renders the camera screen, then the UI uses source grouping and does not present all cameras as one undifferentiated list.

### AC-004: Plugin Management Is Separate

Given the user clicks `Plugins` in the sidebar, when the view opens, then the user sees plugin management only and no camera preview cards.

### AC-005: Plugin Detail Is Separate

Given the user clicks a plugin row, when the detail view opens, then the user sees manifest, diagnostics, capabilities, devices, and config controls for that plugin only.

### AC-005a: Plugin Detail Subviews Match Visual Anchors

Given a plugin has manifest, config, and diagnostics data, when the user navigates among plugin detail subviews, then the UI can reproduce the layout intent of `plugin_manifest.png`, `plugin_config.png`, and `diagnose.png` without combining them into one overloaded page.

### AC-006: Import Plugin Flow Is User-Visible

Given the user imports a valid linked plugin directory, when validation succeeds, then the plugin appears with `Restart required` status and a restart hint.

### AC-007: Import Failure Is Structured

Given the user imports an invalid plugin directory, when validation fails, then the UI shows the diagnostic reason without crashing or adding an invalid source as healthy.

### AC-008: Recording Lock Is Enforced

Given recording is active, when the user opens Plugin Management, then import and enable/disable controls are disabled with a tooltip.

### AC-009: Schema Config Honors Apply Mode

Given a plugin schema field is `runtime-safe`, when recording is active, then the field remains editable. Given a field is `pre-record` or `requires-restart`, then it is disabled during recording.

### AC-010: Diagnostics Are Visible

Given a plugin is missing, incompatible, disabled, or crashed, when the user views Camera Sources or Plugin Management, then the status and suggested action are visible without hidden logs.

### AC-011: UI Uses Real Plugin Runtime Data

Given the plugin runtime reports sources, devices, diagnostics, and plugin rows, when the native UI opens, then Camera Sources, Plugin Management, and Plugin Settings render from those runtime models without relying on placeholder data.

### AC-012: Multi-Platform Builds Pass

Given release packaging is requested, when 007 is ready for review, then supported desktop platform builds complete and their artifacts launch to the native UI entry point.

### AC-013: HIL Matrix Passes or Has Explicit Waivers

Given release-critical hardware is available, when HIL validation runs, then FFmpeg/USB and OAK-D/plugin paths pass the documented scenarios. If hardware is unavailable, the release report must include an explicit waiver and residual risk.

### AC-014: Release Gate Is Complete

Given all 007 work is complete, when the branch is proposed for release merge, then tests, screenshots, build logs, HIL evidence, docs, and release notes are attached or linked from the implementation report.

## Test Plan

### Model Tests

- `CameraSourceModel` groups devices under matching sources.
- `CameraSourceModel` exposes diagnostics and restart-required state.
- `AppCameraModel` includes `sourceId` and `sourceGroup` populated from plugin data.
- `pluginList()` returns status, restart-required, and toggle policy fields.

### QML Contract Tests

- Sidebar renders source headers and nested devices.
- CameraGrid renders source headers and compact unavailable rows.
- Plugin Management table renders rows from `pluginList()`.
- Plugin Settings renders manifest and diagnostics from `getPluginDetail()`.
- Recording lock disables plugin modification controls.

### Manual UI Checks

- Compare implementation against all files in `specs/007-plugin-ui-integration/pluginUI/`.
- Launch with no plugins.
- Launch with bundled FFmpeg healthy.
- Launch with OAK SDK missing.
- Import valid fake plugin.
- Import invalid plugin.
- Toggle linked plugin disabled/enabled.
- Open plugin detail.
- Start recording and confirm plugin controls lock.

### Multi-Platform Build Checks

- macOS native build launches `cmd/micecam_ui`.
- Windows native build launches `cmd/micecam_ui`.
- Linux native build launches `cmd/micecam_ui` if Linux is an intended release target.
- Packaged app includes bundled plugins, plugin manifests, runtime libraries, and QML assets.
- Plugin search paths resolve correctly from the packaged app location.
- Importing a linked plugin uses platform-native folder selection and path persistence.

### HIL Checks

- FFmpeg/USB camera enumerates under `FFmpeg Official`.
- FFmpeg/USB camera preview starts, records, and stops.
- OAK-D source reports SDK missing cleanly when DepthAI runtime is absent.
- OAK-D source enumerates, previews, records, and stops when DepthAI runtime and hardware are available.
- Linked plugin import succeeds for a valid test plugin and surfaces restart-required state.
- Invalid linked plugin import reports structured validation failures.
- Recording lock disables plugin management actions while preserving runtime-safe config behavior.

## Recommended Implementation Phases

### Phase 1: Source Data Wiring

- Populate `CameraRow.sourceId` and `CameraRow.sourceGroup`.
- Pass real `PluginDeviceInfo` into `CameraSourceModel`.
- Add restart-required and diagnostics fields to exposed models.

### Phase 2: Source-Grouped Camera Screen

- Refactor sidebar device list to grouped source tree.
- Refactor camera grid to source sections.
- Add compact unavailable source rows.

### Phase 3: Plugin Management Hardening

- Improve table status, restart banner, toggle policy, and import failure diagnostics.
- Add tests for recording lock.

### Phase 4: Plugin Settings Page

- Render manifest, diagnostics, capabilities, devices.
- Add schema-driven controls for basic field types.
- Implement validate/apply behavior.

### Phase 5: UI Smoke and Sign-Off

- Run QML/browser screenshot checks.
- Capture screenshots for Camera Sources empty, Plugin Management normal, Plugin Management import failure, Plugin Management recording lock, Plugin manifest, Plugin config idle, Plugin config recording-active, and Plugin diagnostics.
- Fix layout overflow and text clipping.

### Phase 6: Multi-Platform Build Closure

- Run clean release builds on supported platforms.
- Verify packaging includes plugin runtime assets and bundled plugin metadata.
- Launch each artifact and execute a minimal plugin UI smoke path.
- Record build commands, artifact locations, and failures in the implementation report.

### Phase 7: Hardware-In-The-Loop Closure

- Run the HIL matrix for FFmpeg/USB and OAK-D/plugin paths.
- Capture hardware model, OS, SDK/runtime versions, and pass/fail evidence.
- Convert unavailable hardware into explicit waivers with owner-approved residual risk.

### Phase 8: Release Candidate Gate

- Run full automated test suite and lint/format checks.
- Attach final UI screenshots against all `pluginUI/` anchors.
- Update `project_index`, relevant `docs/wikis/`, and implementation report.
- Produce release notes and final merge recommendation.
