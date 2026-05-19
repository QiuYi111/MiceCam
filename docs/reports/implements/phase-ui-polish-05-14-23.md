# UI Polish Round 3 - Font Sweep, Status Bar, Preview Fidelity

**Date**: 2026-05-15
**Phase**: UI Polish (post-Round 2)

## Summary

Fixed three remaining visual/runtime issues in `micecam_ui`:
1. Eliminated all hardcoded `"SF Pro Text"` / `"SF Mono"` font references across 8 QML files.
2. Fixed `Theme.qml` platform font resolution (was using unresolved macOS aliases).
3. Improved status bar segment widths and text visibility.
4. Replaced dark grid-only camera card placeholder with noise+grid+timestamp preview.

## Changes

### Theme.qml - Platform font resolution

The previous Round 2 used `.AppleSystemUIFont` and `.AppleSystemUIFontMonospaced` as macOS font aliases. Qt on macOS could not resolve these, producing `qt.qpa.fonts` warnings every launch.

Fixed to use `Helvetica Neue` (primary, matching macOS system appearance) and `Menlo` (monospace, widely available).

### Global font sweep (8 files)

Replaced all 48 occurrences of hardcoded font families:

| File | `"SF Pro Text"` -> `Theme.fontPrimary` | `"SF Mono"` -> `Theme.fontMono` |
|------|------|------|
| AppSidebar.qml | 3 | 0 |
| AlertsSettings.qml | 10 | 0 |
| LoggingSettings.qml | 8 | 9 |
| EncodingSettings.qml | 7 | 0 |
| OutputSettings.qml | 3 | 0 |
| AboutView.qml | 4 | 0 |
| NotificationPopup.qml | 4 | 0 |
| **Total** | **39** | **9** |

### AppStatusBar.qml - Segment sizing

- Widened segment widths: 100-170px with `Layout.minimumWidth` to prevent text clipping.
- Reduced icon-to-text spacing from 12px to 8px for better density.
- Added `clip: false` on segment row and text to ensure visibility.

### CameraCard.qml - Preview fidelity

Replaced the dark grid-only placeholder with a more realistic camera feed simulation:
- **Pixel noise pattern**: 32x24 grid of random-dark pixels simulating sensor noise.
- **Rule-of-thirds overlay**: Faint grid lines at 1/3 and 2/3 positions.
- **Radial vignette**: Darker edges simulating lens falloff.
- **Timestamp overlay**: Monospace `00:42:17` in top-right, matching real camera feeds.

## Verification

- Build: clean, zero warnings.
- Runtime log: empty (no font warnings, no 404s).
- Screenshot at `/tmp/micecam_home_after_round3.png` confirms all fixes.

## Known Issues

None identified in this round.
