# Context: Remove UI Mock Data — Wire QML to Real Backend Data

## Feature

- **Spec**: `specs/002-remove-ui-mock-data/spec.md`
- **Plan**: (not yet created — spec FR list serves as plan)
- **Blast radius**: branch

## Must Read

- `specs/002-remove-ui-mock-data/spec.md` — full spec with FR-001 through FR-011, user scenarios, acceptance criteria
- `cmd/micecam_ui/qml/main.qml` — main wiring, signal handlers with hardcoded camera data (lines 18-28, 57, 91-96)
- `cmd/micecam_ui/qml/components/CameraDetailView.qml` — 15+ hardcoded items: ListModels, metrics, encoder/bitrate, uptime
- `cmd/micecam_ui/qml/components/FullscreenCameraView.qml` — hardcoded defaults, uptime, encoder text
- `cmd/micecam_ui/qml/components/LoggingSettings.qml` — 8 hardcoded mock log lines
- `cmd/micecam_ui/AppController.h` — current Q_PROPERTY surface, needs elapsedTime fix + log model + camera model enrichment
- `cmd/micecam_ui/AppSettings.h` — current 7 properties, needs 12 more
- `cmd/micecam_ui/AppCameraModel.h` — current CameraRow struct, needs encoder/bitrate roles
- `internal/infrastructure/ConfigLoader.h` — the backing config, understand what save/load keys exist

## Read If Relevant

- `cmd/micecam_ui/qml/components/CameraCard.qml` — if fixing elapsed time overlay (line 115)
- `cmd/micecam_ui/qml/components/AppToolbar.qml` — if fixing toolbar elapsed time (line 80)
- `cmd/micecam_ui/qml/components/OutputSettings.qml` — only if added to nav (currently not rendered)
- `cmd/micecam_ui/qml/components/EncodingSettings.qml` — settings bindings (hardwareAccel, keyframeInterval, encoderPreset, previewQuality)
- `cmd/micecam_ui/qml/components/AlertsSettings.qml` — settings bindings (desktopNotifications, soundAlerts)
- `internal/pipeline/StatsCollector.h` — for encoder_used, bytes_written from snapshot()
- `internal/infrastructure/ConfigLoader.cpp` — if adding new persistable config keys

## Forbidden Context

- `cmd/micecam_ui/qml/theme/Theme.qml` — visual design, no changes needed
- `internal/pipeline/RecordingPipeline.*` — encoding pipeline, no changes needed
- `internal/infrastructure/MockCameraBackend.*` — already returns correct data
- `internal/domain/*` — domain types unchanged
- `tests/unit/*` — existing tests must still pass but no new tests required for this feature
- `cmd/micecam_ui/MockCameraModel.*` — legacy file, not used anymore

## Domain Language

| Term | Definition | Code Location |
|------|-----------|---------------|
| CameraRow | QML-facing camera struct with name, fps, drops, resolution/framerate/format labels | `cmd/micecam_ui/AppCameraModel.h:11-22` |
| AppSettings | QObject wrapping ConfigLoader with Q_PROPERTYs for settings panel | `cmd/micecam_ui/AppSettings.h:10-53` |
| AppController | Main controller owning models/settings/pipeline, exposes Q_PROPERTYs to QML | `cmd/micecam_ui/AppController.h:24-109` |
| MockOnly | BackendMode where only MockCameraBackend is registered | `cmd/micecam_ui/AppController.h:22` |
| refreshCameras() | Enumerates all backends, flattens streams to CameraRow, replaces model | `cmd/micecam_ui/AppController.cpp:70-101` |
| StatsCollector | Per-stream stats: frames_actual, bytes_written, encoder_used | `internal/pipeline/StatsCollector.h` |

## Zoom-Out

This feature is the final step of the backend-ui-wiring stage. The backend (Tasks 1-7) produced:
- Mock backend with 5 labeled streams + full capability data
- Preflight with detailed failure items
- Recording pipeline that encodes through TranscodeStage
- ConfigLoader with save/load round-trip
- AlertManager with queryable history
- AppCameraModel, AppAlertModel, AppSettings, AppController adapters
- Capture pump thread

Task 8 did initial QML binding (Repeater over cameraModel, status bar, toolbar). This feature removes the remaining 20+ hardcoded mock values that Task 8 didn't touch — mostly in signal handlers, detail/fullscreen views, settings switches, and log viewer.

The implementation pattern for all fixes is identical: find the hardcoded literal → replace with a binding to an existing or new backend property. No visual design changes.
