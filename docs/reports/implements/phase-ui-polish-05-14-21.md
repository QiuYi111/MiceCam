# Phase: UI Polish Round 1 Rework - Alerts/Logging Fidelity

**Date**: 2025-05-14 21:00
**Status**: Round 1 accepted with follow-up issues

## Summary

Reworked UI polish for Alerts and Logging settings pages plus Sidebar to match approved Apple HIG reference screens. Fixed all fidelity blockers identified in supervisor review.

## Changes

### AppSidebar.qml
- Changed sidebar background from dark navy (`Theme.navyDark`) to light (`Theme.bgSecondary`)
- Changed text colors from light-on-dark to dark-on-light
- Changed selection highlight from dark navy to light navy tint (`Theme.navyTint`)
- Changed divider color from dark to `Theme.divider`
- Navigation items: Encoding, Alerts, Logging, About (Output removed from navigation)

### AlertsSettings.qml
- Restructured `SettingRow` from vertical column layout to horizontal row layout (left label/description + right-aligned controls)
- Fixed watchdog default: 30 -> 3 seconds
- Fixed yellow threshold default: 0.5 -> 0.1
- Fixed red threshold default: 2.0 -> 1.0
- Replaced emoji eye icon with `AppIcon` component
- Replaced emoji play button with `AppIcon` component
- Changed saved text from "Saved" to "All changes saved automatically"
- Added `AppIcon` check icon next to saved text
- Added range text for sliders: "Range: 0.0% - 5.0%" and "Range: 0.0% - 10.0%"

### LoggingSettings.qml
- Changed log preview from dark terminal (`#1E1E1E`) to light panel (`#FAFAFA`) with subtle border
- Replaced emoji folder icon with `AppIcon` component
- Replaced dark monospace text colors with spec-appropriate colors (navy for INFO, amber for WARN)
- Changed saved text to "All changes saved automatically" with AppIcon check
- Restructured to use horizontal RowLayout matching Alerts page consistency

## Design Decisions

1. **SpinBox for watchdog**: Used Qt's built-in SpinBox instead of custom stepper rectangles for better platform integration and accessibility.

2. **Row layout consistency**: Both Alerts and Logging now use the same `RowLayout` pattern (left label + right control) matching the reference screenshots.

3. **Light sidebar**: Reference screens show a light gray sidebar, not dark navy. Changed to match.

## Known Issues

- SF Pro Text font warning at launch (pre-existing, harmless)
- CameraCard 404 for mock images (pre-existing, out of scope)
- Titlebar DragHandler can warn when the frameless window is dragged/clicked.
- Further visual tuning is still needed for exact spacing and alignment against `UIDesign/alerts.png`.

## Verification

- Build: PASS
- Runtime: PASS for Alerts/Logging QML after second rework.
- Screenshot/probe confirms the previous black controls and collapsed sliders are fixed.
