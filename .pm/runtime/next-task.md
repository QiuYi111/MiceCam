# Next Task: Bind Existing QML to AppController Without Visual Polish (Task 8/8)

## Objective

Replace all hardcoded mock values in existing QML files with bindings to `appController` properties and models. The visual design must NOT change — only data sources and click actions are replaced.

## Required Harness Process

`harness-context` → `harness-eval` → `harness-report`

Risk classification: **BRANCH** — 11 QML files modified, integration surface, no backend code changes.

## Allowed Scope

You may edit:
- `cmd/micecam_ui/main.cpp` — replace MockCameraModel registration with AppController
- `cmd/micecam_ui/qml/main.qml` — wire toolbar/sidebar/grid/statusbar/preflight/notification to controller
- `cmd/micecam_ui/qml/components/AppToolbar.qml` — add properties for isRecording, recordText, recordClicked, alertModel
- `cmd/micecam_ui/qml/components/AppSidebar.qml` — replace CameraModel {} with appController.cameraModel
- `cmd/micecam_ui/qml/components/CameraGridView.qml` — replace hardcoded cards with Repeater over cameraModel
- `cmd/micecam_ui/qml/components/AppStatusBar.qml` — add properties for each status metric
- `cmd/micecam_ui/qml/components/PreflightModal.qml` — add items property, replace hardcoded items with Repeater
- `cmd/micecam_ui/qml/components/NotificationPopup.qml` — add alertModel property, bind ListView.model
- `.pm/runtime/worker-report.md`

## Forbidden Scope — CRITICAL

- **Do NOT change any colors, fonts, spacing, sizes, or layout tokens.** This is binding-only.
- **Do NOT change any visual design.** The existing QML appearance must remain exactly as-is.
- Do NOT edit backend files (domain, infrastructure, pipeline)
- Do NOT edit Qt adapter files (AppCameraModel, AppAlertModel, AppSettings, AppController)
- Do NOT delete the build directory
- Do NOT use image-analysis MCP tools
- Do NOT use `#pragma region` or `#pragma mark`

## Background — Read ALL QML Files First

Read every file in the allowed scope before making any changes. Understand the current hardcoded values, property names, ids, and structure.

## Implementation Steps

### Step 1: main.cpp

Replace the MockCameraModel registration with AppController. Read `cmd/micecam_ui/main.cpp` and change:
- Replace `#include "MockCameraModel.h"` with `#include "AppController.h"`
- Replace `MockCameraModel` instance with `micecam::ui::AppController controller;` (or appropriate constructor)
- Call `controller.refreshCameras();`
- Set context property: `engine.rootContext()->setContextProperty("appController", &controller);`

### Step 2: AppToolbar.qml

Read the current file. Find where `isRecording` is set (hardcoded to true) and the record button click handler. 

Add root-level properties:
```qml
property bool isRecording: false
property string recordText: isRecording ? "Stop" : "Record"
property var alertModel: null
signal recordClicked()
```

Replace the existing `isRecording` assignments with `root.isRecording`. Replace the record button text with `root.recordText`. Replace the click handler with `root.recordClicked()`.

Pass alertModel into NotificationPopup (if present in toolbar).

### Step 3: main.qml

Read the current file. Find `AppToolbar`, `AppSidebar`, `AppStatusBar`, `PreflightModal`.

Wire toolbar:
```qml
AppToolbar {
    id: toolbar
    isRecording: appController.isRecording
    recordText: appController.recordButtonText
    alertModel: appController.alertModel
    onRecordClicked: {
        if (appController.isRecording) {
            appController.stopRecording()
        } else if (!appController.startRecording()) {
            preflightModal.items = appController.preflightItems()
            preflightModal.open()
        }
    }
}
```

Wire sidebar: replace `model: CameraModel {}` with `model: appController.cameraModel`.

Wire status bar:
```qml
AppStatusBar {
    id: statusBar
    elapsedText: appController.elapsedText
    cameraCountText: appController.cameraCountText
    totalFramesText: appController.totalFramesText
    averageFpsText: appController.averageFpsText
    bytesWrittenText: appController.bytesWrittenText
    diskRemainingText: appController.diskRemainingText
    recording: appController.isRecording
}
```

Wire preflight: before opening, set `preflightModal.items = appController.preflightItems()`.

### Step 4: AppSidebar.qml

Read the current file. Find the model assignment. Replace `model: CameraModel {}` or equivalent with `model: appController.cameraModel`. Keep the existing delegate layout unchanged.

### Step 5: CameraGridView.qml

Read the current file. Find the hardcoded 5 camera cards. Replace with a Repeater over `appController.cameraModel`:

```qml
Flow {
    Layout.fillWidth: true
    Layout.fillHeight: true
    spacing: 12
    Repeater {
        model: appController.cameraModel
        delegate: CameraCard {
            // Use existing sizing logic from original cards
            width: index < 2 ? (parent.width - 12) / 2 : (parent.width - 24) / 3
            height: index < 2 ? root.height / 2 - 22 : root.height / 2 - 22
            cameraName: model.name
            fps: model.fps
            drops: model.dropCount
            status: model.status
            isRecording: model.isRecording
        }
    }
}
```

Keep existing CameraCard visual styling completely unchanged. Only replace the data bindings. Keep existing fullscreen/context-menu signal connections if present.

### Step 6: AppStatusBar.qml

Read the current file. Find hardcoded labels like "00:42:17", "5 cameras", "76,230 frames", etc.

Add root-level properties:
```qml
property string elapsedText: "00:00:00"
property string cameraCountText: "0 cameras"
property string totalFramesText: "0 frames"
property string averageFpsText: "0.00 fps avg"
property string bytesWrittenText: "0 B"
property string diskRemainingText: "Disk unknown"
property bool recording: false
```

Bind each existing `StatusSegment` or label's `labelText` (or equivalent property) to these root properties. Keep the exact same control IDs, layout structure, and styling.

### Step 7: PreflightModal.qml

Read the current file. Find the hardcoded failure/warning items (typically 3 Rectangle items).

Add root property:
```qml
property var items: []
```

Replace the hardcoded failure items with a `Repeater`:
```qml
Repeater {
    model: root.items
    delegate: Rectangle {
        // Use existing rectangle style from original items
        // Bind modelData.severity, modelData.title, modelData.message
    }
}
```

### Step 8: NotificationPopup.qml

Read the current file. Add property:
```qml
property var alertModel: null
```

Set `ListView.model: root.alertModel`. Keep existing delegate styling unchanged.

### Step 9: Other QML Files

Check every QML file in `cmd/micecam_ui/qml/components/` for hardcoded demo data:
- `CameraDetailView.qml` — if it has hardcoded camera info, bind to cameraModel or cameraAt()
- `EncodingSettings.qml` — if it has hardcoded settings, bind to appController.settings
- `AlertsSettings.qml` — if it has hardcoded values, bind to appController.settings
- `LoggingSettings.qml` — same

## Verification Commands

```bash
cmake -B build -S . -DBUILD_UI=ON
cmake --build build --target micecam_ui -j

# Runtime smoke test
pkill -f micecam_ui 2>/dev/null || true
./build/cmd/micecam_ui/micecam_ui > /tmp/micecam_ui_wiring.log 2>&1 &
sleep 3
cat /tmp/micecam_ui_wiring.log
# Verify: no QML errors, no binding errors
kill %1 2>/dev/null || true
```

## Acceptance Criteria

- [ ] `cmake --build build --target micecam_ui -j` builds successfully
- [ ] Runtime log has NO QML errors (no "ReferenceError", "TypeError", "Cannot assign" messages)
- [ ] AppToolbar uses `appController.isRecording` and `appController.recordButtonText`
- [ ] AppSidebar model is `appController.cameraModel`
- [ ] CameraGridView uses Repeater over `appController.cameraModel`
- [ ] AppStatusBar metric strings are bound to `appController.*` properties
- [ ] PreflightModal items are populated from `appController.preflightItems()`
- [ ] NotificationPopup ListView model is `alertModel`
- [ ] Visual appearance is preserved (no color/font/spacing/layout changes)
- [ ] No existing tests break
- [ ] Worker report has correct commit hash and all required sections

## Commit

Create one commit on top of current HEAD with ALL QML binding changes in ONE commit:
```
feat(ui): wire qml views to appcontroller models
```
