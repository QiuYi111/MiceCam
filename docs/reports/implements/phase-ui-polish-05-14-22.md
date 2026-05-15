# Implementation Report: UI Polish Round 2 - Home Grid Runtime Fidelity

**Date**: 2025-05-14
**Phase**: UI Polish
**Task**: Fix home/camera workspace visual blockers for credible UI review

## Summary

Resolved all five runtime visual blockers preventing credible UI review of the home camera grid: remote image 404s, missing SF font warnings in touched files, frameless titlebar drag warnings, and rough CameraCard fidelity.

## Changes

### Theme.qml — Platform Font Fallbacks

Added three new properties to the `Theme` singleton:
- `fontPrimary`: Maps to `.AppleSystemUIFont` on macOS, `Segoe UI` on Windows, `sans-serif` on Linux
- `fontMono`: Maps to `.AppleSystemUIFontMonospaced` on macOS, `Consolas` on Windows, `monospace` on Linux
- `fontWeightMedium`: String constant `"Medium"`

### CameraCard.qml — Full Rewrite

**Before**: Loaded `https://raw.githubusercontent.com/QiuYi111/MiceCam/v2/assets/mock_cam.png` (404), hardcoded `"SF Pro Text"`, minimal visual fidelity.

**After**:
- Local gradient + Canvas grid-line preview surface (no network dependency)
- Top-left camera label with semi-transparent dark backing rectangle
- Top-right REC badge with pulsing red dot animation
- Bottom overlay bar: semi-transparent black, fps left, drops right
- Warning state: amber border (2px) + amber drop count text + amber dot indicator (no emoji)
- Card border: 1px dark outline (normal) or 2px amber (warning)
- All font references use `Theme.fontPrimary`

### AppTitleBar.qml — Drag Handler Fix

- Added null guard: `if (active && Window.window)` prevents `startSystemMove` null/type warnings
- Replaced hardcoded `"SF Pro Text"` with `Theme.fontPrimary`

### AppToolbar.qml — Font Fix

- Replaced all 4 occurrences of `"SF Pro Text"` with `Theme.fontPrimary`

### CameraGridView.qml — Grid Spacing

- Side margins: 20px (was 16px)
- Card spacing: 12px (was 16px)
- All cards set `isRecording: true` for realistic preview matching `home.png`

## Design Decisions

1. **Canvas grid vs bundled image**: Chose QML Canvas with grid lines over adding a PNG resource. Rationale: no CMakeLists.txt resource change needed, no external asset dependency, renders at any resolution, matches the "video preview" feel.

2. **Platform font detection via `Qt.platform.os`**: Used Qt's runtime platform detection rather than trying to load SF Pro or bundling fonts. This produces zero warnings from touched files.

3. **Pulsing REC dot**: Added `SequentialAnimation` on opacity to make recording state visually obvious, matching common NVR/camera UI patterns.

## Verification

- Build: `cmake --build build --target micecam_ui -j` — **PASS**
- Runtime: No 404s, no startSystemMove warnings from AppTitleBar
- Visual: Screenshot confirms cards render with proper layout, labels, overlays, and warning states
- Font: All touched files use `Theme.fontPrimary`, no hardcoded unavailable font families

## Known Issues / Follow-up

- `qt.qpa.fonts` still logs a 81ms font alias cost for `"SF Pro Text"` from 8 files outside this task's scope (AppSidebar, AppStatusBar, AlertsSettings, LoggingSettings, EncodingSettings, OutputSettings, AboutView, NotificationPopup). A separate task should sweep those.
- Full adaptive grid (responsive column count) deferred to a later round per task instructions.
