# Next Task: CameraCard Canvas Rounded Clip Fix

## Objective

Actually fix the video card rounded corners. The last rework still did not change the visible result.

User evidence, 2026-05-15:
- All video cards still look square/hard-cornered.
- CAM_D still has amber border radius that does not match the visual content.

## Root Cause

`CameraCard.qml` uses a `Canvas` that fills the whole card and paints square pixels to the full rectangular bounds. Adding `radius`/`clip` to surrounding `Rectangle` layers is not enough, because the visible canvas content is still square.

The fix must make the actual painted preview respect a rounded rectangle.

## Allowed Scope

You may edit only:
- `cmd/micecam_ui/qml/components/CameraCard.qml`
- `.pm/runtime/worker-report.md`
- `docs/reports/implements/phase-ui-polish-05-15-05.md`

Do not edit:
- Other QML files.
- Backend/C++.
- Git history.
- New UI features.

## Required Fix

In `CameraCard.qml`, ensure the actual video card visual content is rounded.

Required approach:
- Implement a rounded-rectangle clipping path in `previewCanvas.onPaint` before painting noise/grid/vignette.
- Use `root.cardRadius` as the single radius source.
- The canvas must not paint outside that rounded rect.
- The bottom bar must still have rounded bottom corners and a straight top edge.
- The warning border must trace the same rounded rect.
- All cards must show rounded corners, not only CAM_D.

Suggested Canvas pattern:

```qml
function roundedRectPath(ctx, x, y, w, h, r) {
    ctx.beginPath()
    ctx.moveTo(x + r, y)
    ctx.lineTo(x + w - r, y)
    ctx.quadraticCurveTo(x + w, y, x + w, y + r)
    ctx.lineTo(x + w, y + h - r)
    ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h)
    ctx.lineTo(x + r, y + h)
    ctx.quadraticCurveTo(x, y + h, x, y + h - r)
    ctx.lineTo(x, y + r)
    ctx.quadraticCurveTo(x, y, x + r, y)
    ctx.closePath()
}
```

Then in `onPaint`:
- Clear whole canvas.
- Save context.
- Create rounded path and `ctx.clip()`.
- Paint all preview noise/grid/vignette inside the clip.
- Restore context.

Important:
- Do not claim success from QML build alone.
- Do not use image-analysis MCP tools.
- If a visual screenshot still shows square corners, report failure.

## Verification

Run:

```bash
cmake -B build -S . -DBUILD_UI=ON
cmake --build build --target micecam_ui -j
pkill -f micecam_ui 2>/dev/null || true
(./build/cmd/micecam_ui/micecam_ui > .pm/runtime/micecam_canvas_corner_fix_runtime.log 2>&1 & echo $! > .pm/runtime/micecam_canvas_corner_fix.pid)
sleep 3
screencapture -x .pm/runtime/micecam_canvas_corner_fix_home.png
kill $(cat .pm/runtime/micecam_canvas_corner_fix.pid) 2>/dev/null || true
cat .pm/runtime/micecam_canvas_corner_fix_runtime.log
```

## Acceptance Criteria

- [ ] `micecam_ui` builds successfully.
- [ ] Runtime log is clean.
- [ ] `previewCanvas.onPaint` clips preview drawing to a rounded rectangle path.
- [ ] All video cards have visible rounded top corners.
- [ ] Bottom overlay has rounded bottom corners and straight top edge.
- [ ] CAM_D amber border matches card/content radius.
- [ ] No right-edge or bottom-corner mismatch remains.
- [ ] Worker report has the actual final commit hash.

## Commit

Create one commit on top of `07da9d9`.
