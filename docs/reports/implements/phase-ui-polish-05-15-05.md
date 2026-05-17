# Implementation Report: UI Polish - Camera Card Rounded Corners (Rework)

**Date**: 2026-05-15
**Phase**: UI Polish (Card Corner Rework)
**Branch**: `codex/ui-polish-hig`
**Baseline commit**: `9d284bd`

## Summary

Reworked camera card corner rendering to use explicit layer geometry instead of relying on `root.clip` alone. Prior fix (`9d284bd`) was rejected because `Rectangle { clip: true }` does not reliably clip children to a rounded shape in Qt Quick.

## Root Cause

The prior approach assumed `root.clip: true` with `radius: 12` would clip all child content to rounded corners. In Qt Quick, `clip: true` clips to the rectangular bounding box, not the rounded visual shape. Child content (gradients, canvas, overlays) could still render square in the corners. Each visual layer must explicitly participate in the rounded corner strategy.

## Changes

### CameraCard.qml

1. Added `property int cardRadius: 12` as single source of truth
2. `previewSurface`: Added `radius: root.cardRadius` and `clip: true` — rounds top corners explicitly
3. `bottomBar`: Added `radius: root.cardRadius` and `clip: true` — rounds bottom corners explicitly
4. `bottomBar`: Added same-color child Rectangle (anchored top, height = cardRadius) to square off top edge
5. Warning border overlay: Updated to `radius: root.cardRadius`

## Design Decision

Each visual layer that touches the card boundary must own its own rounded corner geometry:
- `previewSurface` owns the top corners via explicit radius + clip
- `bottomBar` owns the bottom corners via explicit radius + clip, with a top-edge patch to keep its top edge straight
- Warning border traces the outer shape with matching radius
- `root.clip: true` remains as a safety net, not the primary strategy

## Verification

- Build: Clean, zero warnings
- Runtime log: Empty (no QML errors, no warnings)
- Visual: All cards have uniform 12px rounded corners, bottom bar has straight top edge, warning border follows same shape

## Preserved Work

- Camera detail uses ComboBox/dropdown acquisition controls backed by label/value ListModels
- Sidebar has persistent Cameras Home row with icon
- Stop/timer control remains separate and artifact-free
- Right-click Configure/Fullscreen still routed via contextMenuRequested signal

## Known Issues / Follow-up

- History page from `UIDesign/history.png` remains as follow-up work
- Dropdown/ComboBox acquisition controls use label/value ListModels - ready for backend wiring
