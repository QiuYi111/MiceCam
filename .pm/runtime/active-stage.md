# Active Stage

## Stage ID

ui-polish

## Stage goal

Bring the Qt/QML native UI into close alignment with `specs/001-micecam-v2-rewrite/ui-spec.md` and the full `UIDesign/*.png` reference set, following Apple HIG system-level desktop patterns.

## Why this stage matters

The current UI has the broad structure, but several surfaces still read as prototype: Alerts does not match the reference design, Logging was replaced by an over-prominent Output page, theme tokens drifted from the approved navy system, and major recording/preview states are hardcoded. This stage turns the existing QML into a polished laboratory monitoring tool.

## Inputs

- `specs/001-micecam-v2-rewrite/spec.md` — full system spec
- `specs/001-micecam-v2-rewrite/ui-spec.md` — approved Apple HIG UI spec
- `specs/001-micecam-v2-rewrite/UIDesign/*.png` — visual reference screens
- `.pm/stable/product.md` — product definition
- `.pm/stable/ui-direction.md` — approved UI direction

## Allowed work

- Scoped QML UI polish
- UI-facing state/model binding only where needed for visual fidelity
- Build and runtime visual verification after each round
- Focused implementation reports

## Forbidden work

- Backend recording pipeline changes
- Camera hardware/FFmpeg pipeline changes
- Product positioning changes
- CI/CD configuration
- Large unrelated refactors

## Exit criteria

- [ ] EC-001: Alerts and Logging settings match the reference screens
  Evidence: screenshots compared against `alerts.png` and `logging.png`
  Blocking: true
- [ ] EC-002: Camera workspace, toolbar, status bar, context menu, fullscreen, and modals match UI spec
  Evidence: screenshots compared against `home.png`, `notification.png`, `right-click.png`, `enlarge.png`, `preflight.png`
  Blocking: true
- [ ] EC-003: `micecam_ui` builds cleanly after every polish round
  Evidence: `cmake --build build --target micecam_ui -j` output
  Blocking: true
- [ ] EC-004: implementation report produced
  Evidence: `docs/reports/implements/phase-ui-polish-*.md`
  Blocking: true

## Current progress

Round 1 ready: theme/navigation/Alerts/Logging polish.

## Open blockers

None.
