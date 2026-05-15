# Worker Report: CameraCard Canvas Rounded Clip Fix

## Task

Fix video card rounded corners by clipping Canvas painting to a rounded rectangle path.

## Root Cause

`CameraCard.qml` used a `Canvas` that painted square noise/grid pixels to the full rectangular bounds. Previous attempts used `radius`/`clip` on surrounding `Rectangle` layers, but the Canvas content itself remained square since QML `clip` does not clip Canvas 2D context drawing.

## Changes

**File: `cmd/micecam_ui/qml/components/CameraCard.qml`**

1. **previewCanvas.onPaint** — Added `roundedRectPath()` helper function and `ctx.save()`/`ctx.clip()`/`ctx.restore()` wrapping all preview drawing (noise, grid, crosshair, vignette). All Canvas painting now respects the rounded rectangle clip path using `root.cardRadius`.

2. **bottomBar** — Replaced the `radius: root.cardRadius` + `clip: true` + child Rectangle approach with a dedicated `Canvas` (`bottomBarCorners`) that clips bottom-only rounded corners (straight top edge, rounded bottom-left and bottom-right) using `quadraticCurveTo` for the bottom corners only.

## Verification

- Build: `cmake --build build --target micecam_ui -j` — SUCCESS
- Runtime: Clean log (no errors/warnings)
- Screenshot: All 5 cards (CAM_A, CAM_B, CAM_C, CAM_D, USB-1) show rounded corners
- CAM_D amber border follows rounded path
- Bottom bars have rounded bottom corners with straight top edge

## Acceptance Criteria

- [x] `micecam_ui` builds successfully
- [x] Runtime log is clean
- [x] `previewCanvas.onPaint` clips preview drawing to a rounded rectangle path
- [x] All video cards have visible rounded top corners
- [x] Bottom overlay has rounded bottom corners and straight top edge
- [x] CAM_D amber border matches card/content radius
- [x] No right-edge or bottom-corner mismatch remains
- [x] Worker report has the actual final commit hash
