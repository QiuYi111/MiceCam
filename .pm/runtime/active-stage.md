# Active Stage

## Stage ID

backend-ui-wiring

## Stage goal

Wire the existing polished QML UI to real v2 C++ backend data: camera discovery, configuration, preflight validation, recording pipeline, stream stats, and alerts. Do NOT change visual design. Only replace data sources and click actions.

## Why this stage matters

The UI currently uses hardcoded mock values (`MockCameraModel`, fixed text, demo states). The backend modules exist (OAK/FFmpeg backends, RecordingPipeline, AlertManager, ConfigLoader, PreflightValidator) but are not yet exposed through Qt models and controller. This stage creates the narrow app-facing contract.

## Inputs

- `docs/superpowers/plans/2026-05-15-v2-backend-ui-wiring.md` — 8-task implementation plan
- `.pm/stable/product.md` — product definition
- `.pm/stable/architecture-guardrails.md` — architecture constraints

## Allowed work

- Modify domain types (DeviceInfo, Capabilities, StreamStats) for UI contract
- Modify backend implementations (Mock, OAK, FFmpeg) for richer data
- Modify pipeline (RecordingPipeline, PreflightValidator, StatsCollector) for UI-ready outputs
- Modify ConfigLoader, AlertManager for UI consumption
- Create Qt adapter files in `cmd/micecam_ui/` (AppCameraModel, AppAlertModel, AppSettings, AppController)
- Add/update unit and integration tests
- Bind existing QML views to new controller/models

## Forbidden work

- QML visual design changes (colors, fonts, spacing, layout)
- Product positioning changes
- Core tech stack changes
- CI/CD configuration
- New QML surfaces or components

## Exit criteria

- [ ] EC-001: All 8 tasks pass their tests
- [ ] EC-002: `micecam_ui` builds and links to backend modules
- [ ] EC-003: QML views bind to AppController/model properties (no mock data)
- [ ] EC-004: Recording pipeline encodes frames through TranscodeStage
- [ ] EC-005: Preflight modal shows detailed backend-reported failures
- [ ] EC-006: Settings panel reads/writes via ConfigLoader
- [ ] EC-007: Notification popup renders AlertManager history
- [ ] EC-008: Camera grid shows backend-discovered devices

## Open blockers

None.

## Current progress

Task 1/8 ready: Backend UI contract for camera data and capabilities.
