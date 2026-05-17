# Feature Spec: Remove UI Mock Data — Wire QML to Real Backend Data

## Metadata

| Field      | Value                          |
|------------|--------------------------------|
| Feature ID | `002-remove-ui-mock-data`      |
| Branch     | `feat/002-remove-mock-data`    |
| Status     | Draft                          |
| Owner      | `jingyi`                       |
| Date       | `2026-05-15`                   |

## Summary

Eliminate all hardcoded mock data from the MiceCam QML UI by wiring every remaining mock string, number, list, and boolean to real C++ backend properties exposed through `AppController`, `AppCameraModel`, `AppAlertModel`, and `AppSettings`. Camera cards enumerate dynamically from backend discovery. Settings panels read/write through `AppSettings`. Logs come from a live log model. Camera detail, fullscreen, and notification views use actual backend stream data, not "CAM_A"/"29.97"/"H.265" placeholders.

## User Scenarios

### US-001: Home Grid Shows Only Discovered Cameras

**Priority**: P1

**Independent Test**: Launch the app in MockOnly mode. Verify the camera grid shows exactly 5 cards (CAM_A, CAM_B, CAM_C, CAM_D, USB-1) — matching the mock backend's `enumerate_devices()` output. Verify each card displays the correct stream label, FPS, and drop count from the backend model. Verify no card shows "CAM_A" as a hardcoded string that would be wrong if the backend returned different device names.

**Acceptance Scenarios**:
- Given MockOnly backend returning 1 device with 5 labeled streams, When the app launches, Then the camera grid shows exactly 5 cards with labels CAM_A through USB-1, NOT hardcoded names
- Given MockOnly backend returning 5 streams, When the sidebar camera list renders, Then it shows the same 5 camera names as the grid, with correct status indicators
- Given MockOnly backend, When user clicks a sidebar camera row, Then the detail page opens with that specific camera's name, fps, resolution options from the backend model — not "CAM_D" with fake 18.45 fps

### US-002: Camera Detail Shows Real Stream Capabilities

**Priority**: P1

**Independent Test**: Open camera detail for any mock camera. Verify the Resolution, Frame Rate, and Pixel Format dropdowns are populated from `AppCameraModel` roles (`ResolutionOptionsRole`, `FramerateOptionsRole`, `FormatOptionsRole`). Verify they are NOT the hardcoded `ListModel` values `"1920×1080"/"1280×720"/"640×480"`, `"15fps"/"30fps"/"60fps"`, `"Mono8"/"BGR"/"NV12"`. Change a dropdown selection and verify the UI updates.

**Acceptance Scenarios**:
- Given a mock camera with resolutions `{1920x1080, 1280x720, 640x480}`, When detail page opens, Then resolution dropdown shows those 3 options from the model, not a separate hardcoded ListModel
- Given a mock camera with framerates `{15, 30, 60}`, When detail page opens, Then framerate dropdown shows "15 fps", "30 fps", "60 fps" from the model
- Given a mock camera with format `{"rgb24"}`, When detail page opens, Then stream mode dropdown shows "rgb24" from the model

### US-003: Elapsed Time Shows Actual Recording Duration

**Priority**: P1

**Independent Test**: Start recording and wait 3 seconds. Verify the elapsed time display (in toolbar, camera cards, fullscreen view, and detail view) shows a changing time value, not the hardcoded "00:42:17". Stop recording and verify the final time is approximately correct.

**Acceptance Scenarios**:
- Given recording is running for 5 seconds, When viewing the toolbar, Then the elapsed time text shows approximately "00:05", not "00:42:17"
- Given recording is running, When viewing any camera card, Then its elapsed time overlay shows the recording duration, not "00:42:17"
- Given fullscreen camera view is open and recording, Then the elapsed time in the top bar shows the recording duration, not "00:42:17"

### US-004: Settings Panels Read and Write Real Configuration

**Priority**: P1

**Independent Test**: Open Encoding settings. Verify the bitrate slider reads from `appController.settings.defaultBitrateKbps`. Change the bitrate slider. Open Alerts settings, verify thresholds read from backend. Change watchdog timeout stepper. Open Logging settings, verify output directory shows the actual configured path from `AppSettings.outputDirectory`.

**Acceptance Scenarios**:
- Given EncodingSettings is open, When bitrate slider position is observed, Then it reflects `appController.settings.defaultBitrateKbps` divided by 1000 Mbps, not a default slider value
- Given OutputSettings is open, When the output directory text field is observed, Then it shows `appController.settings.outputDirectory`, not `/Volumes/Recordings/MiceCam`
- Given AlertsSettings is open, When any threshold stepper is observed, Then it reflects the actual setting value from AppSettings

### US-005: Log Viewer Shows Live Backend Log Entries

**Priority**: P2

**Independent Test**: Start the app. Open the Logging page. Verify the recent log preview area shows actual log entries from the backend (e.g., initialization messages from spdlog), not the 8 hardcoded static lines about "OAK-D-1", "cam_01_2026-05-14.mp4", etc.

**Acceptance Scenarios**:
- Given the app has started, When Logging page is opened, Then the log preview shows log entries from the backend's actual log buffer, not hardcoded placeholder text
- Given a recording is started, When Logging page is observed, Then new log entries appear without reloading the page

### US-006: Fullscreen and Detail Views Use Selected Camera Data

**Priority**: P2

**Independent Test**: Click a camera card's fullscreen action. Verify the fullscreen view shows that specific camera's name, not "CAM_A". Verify the bottom bar shows actual encoder info from the backend, not "H.265 / 1080p".

**Acceptance Scenarios**:
- Given a specific camera card is in fullscreen, When the fullscreen view renders, Then the camera name matches the card that was clicked
- Given fullscreen view is open, When the bottom metrics bar is observed, Then encoder and resolution text comes from the backend model, not "H.265 / 1080p"

## Requirements

### Functional Requirements

- **FR-001**: `CameraDetailView.qml` resolution, framerate, and stream mode dropdowns MUST be populated from `AppCameraModel` roles (`ResolutionOptionsRole`, `FramerateOptionsRole`, `FormatOptionsRole`), not from hardcoded `ListModel` definitions
- **FR-002**: All occurrences of hardcoded elapsed time string `"00:42:17"` MUST be replaced with binding to `appController.elapsedText` — in `CameraCard.qml:115`, `CameraDetailView.qml:230,285`, `FullscreenCameraView.qml:110`, `AppToolbar.qml:80`
- **FR-003**: `main.qml` signal handlers for fullscreen, detail navigation, and camera selection MUST read camera data from `appController.cameraModel` rows, not from hardcoded `"CAM_A"`/`"CAM_D"`/`29.97`/`18.45`/`152` literals
- **FR-004**: `OutputSettings.qml:57` TextField MUST bind to `appController.settings.outputDirectory` instead of hardcoded path `"/Volumes/Recordings/MiceCam"`
- **FR-005**: `LoggingSettings.qml:364-371` 8 hardcoded log entry lines MUST be replaced with a Repeater bound to an `AppController` log model property (`recentLogEntries`)
- **FR-006**: `FullscreenCameraView.qml` default property values and `open()` function fallbacks MUST use data from `appController.cameraModel` rather than `"CAM_A"`/`29.97`/`0`/`true`/`0`
- **FR-007**: `CameraDetailView.qml:9-13` default property values MUST accept empty/zero defaults and populate from camera model when a camera is selected
- **FR-008**: `AppController::elapsedText()` MUST return actual elapsed recording time when recording, counting up from session start, not hardcoded `"00:00"`
- **FR-009**: `AppSettings` MUST add 12 new properties: `keyframeInterval`(int), `encoderPreset`(QString), `hardwareAcceleration`(bool), `previewQuality`(QString), `desktopNotifications`(bool), `soundAlerts`(bool), `verboseDiagnostics`(bool), `createSubfolderPerSession`(bool), `folderNamePrefix`(QString), `namingPattern`(QString), `containerFormat`(QString), `maxFileSizeGB`(int)
- **FR-010**: `AppController` MUST expose a `recentLogEntries` property (QAbstractListModel or QStringList) that provides the last N log lines from the backend
- **FR-011**: All settings switches in `EncodingSettings.qml`, `AlertsSettings.qml`, `LoggingSettings.qml`, `OutputSettings.qml` that are currently hardcoded `checked: true/false` MUST bind to their corresponding `AppSettings` property

### Non-Functional Requirements

- **NFR-001**: Performance — Elapsed time update must not cause visible UI lag; update interval max 1 second
- **NFR-002**: Reliability — Zero QML binding errors at runtime after all changes; verified via empty runtime log
- **NFR-003**: Compatibility — Existing 24/24 tests must still pass; no regression in backend code

## Success Criteria

| #    | Criterion                                    | Measured By          |
|------|----------------------------------------------|----------------------|
| SC-1 | Camera grid dynamically shows exact count from backend, zero hardcoded camera names in grid/sidebar/fullscreen/detail signal handlers | Manual inspection + automated test verifying camera model row count matches grid card count |
| SC-2 | Camera detail dropdown options match `AppCameraModel` roles, zero hardcoded ListModels remaining | XPath/string search for `ListElement` in resolved QML inside CameraDetailView |
| SC-3 | Elapsed time shows dynamic recording duration, no `"00:42:17"` strings remain in any QML file | `grep -r "00:42:17" cmd/micecam_ui/qml/` returns zero results |
| SC-4 | All settings switches react to AppSettings property changes | Manual inspection of Encoding/Alerts/Logging/Output pages |
| SC-5 | Log viewer shows live entries from backend, zero hardcoded log strings | Manual inspection of Logging page |
| SC-6 | Zero QML binding errors at runtime | `cat /tmp/micecam_ui_wiring.log` is empty |
| SC-7 | All 24 existing tests pass | `ctest --test-dir build -j4` shows 100% |
| SC-8 | `main.qml` has no hardcoded camera name string (no `"CAM_A"`, `"CAM_D"`) | `grep -r '"CAM_[A-D]"' cmd/micecam_ui/qml/main.qml` returns zero results |

## Assumptions

- `AppCameraModel` already carries `ResolutionOptionsRole`, `FramerateOptionsRole`, `FormatOptionsRole` strings lists populated by `AppController::refreshCameras()`. These roles just aren't consumed by QML yet.
- `AppSettings` already wraps `ConfigLoader` with getters/setters and a `save()` method. Adding 12 new properties follows the same pattern.
- The MockCameraBackend `enumerate_devices()` returns fixed data (5 streams) and is the authoritative camera source for MockOnly mode.
- Visual design (colors, fonts, spacing, layout) must NOT change — only data bindings are replaced.
- The `elapsedText` timer can be driven by a QML `Timer` or a C++ `QTimer` on AppController side — whichever is simpler.

## Clarifications

- Camera detail metrics grid "H.265 (HEVC)", "12.0 Mbps", "48/64" → MUST wire to real data. `AppController` already holds `RecordingPipeline` and per-stream stats. `StatsCollector::snapshot()` provides `encoder_used`, `bytes_written`, `frames_actual`. Can populate these via new model roles or a `QVariantMap` from `cameraAt(int)`.
- `recentLogEntries` → NOT a new build. spdlog provides `spdlog::sinks::callback_sink_mt` which fires a callback per log entry. Wire it to a ring-buffer `QStringList` exposed as `Q_PROPERTY` on `AppController`.

## Out of Scope

- Video preview canvas content (noise pattern placeholders) — these are visual assets, not data mock. Will be replaced by real camera frames in a separate spec.
- "Remove" action in CameraContextMenu — remains disabled until camera management feature spec.
- AboutView version string, description, and link URLs — cosmetic metadata, low priority.
- Settings page `navigateBack` wiring — already uses `root.parent.parent.currentViewIndex = 0` pattern shared by all pages.
- The `OutputSettings.qml` file is currently not in the StackLayout in `main.qml` — it's a standalone file not rendered. No binding fixes needed unless it gets added to the nav.

## Risk Notes

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| QML binding errors after replacing hardcoded values | Medium | High | Zero-tolerance: runtime log must be empty. Incremental build and smoke test after each file change. |
| AppSettings 12 new properties add complexity | Low | Medium | Follow existing pattern exactly. Each property mirrors an existing getter/setter. |
| Log model (recentLogEntries) requires plumbing spdlog → QML | Medium | Medium | Start with a simple QStringList populated from a ring buffer. Full spdlog integration deferred. |
| Camera model data race: refreshCameras while QML binds | Low | Medium | Use beginResetModel/endResetModel in replaceRows — already implemented. |
