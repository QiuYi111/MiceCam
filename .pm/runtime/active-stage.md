# Active Stage

## Stage ID

feasibility

## Stage goal

Prove the two riskiest technical assumptions are viable before committing to full build.

## Why this stage matters

Top 3 user fears: timestamp accuracy, frame drops, silent failures. Both spikes directly address the root causes.
Without feasibility validation, we risk building on unproven foundations.

## Inputs

- `specs/001-micecam-v2-rewrite/spec.md` — full system spec
- `specs/001-micecam-v2-rewrite/plan.md` — implementation plan
- `.pm/stable/product.md` — product definition
- DepthAI SDK documentation
- FFmpeg documentation (hardware encoding)

## Allowed work

- Technical spikes
- Prototyping
- Proof-of-concept experiments
- Dependency evaluation
- Writing spike-report.md

## Forbidden work

- Production implementation
- Schema changes
- UI implementation
- CI/CD configuration

## Exit criteria

- [ ] EC-001: OAK H264 spike complete with valid output
  Evidence: buildable prototype that enumerates OAK-D and produces H264 stream from VideoEncoder node
  Blocking: true
- [ ] EC-002: FFmpeg hardware encoder spike complete with fallback chain verified
  Evidence: buildable prototype that selects platform encoder (VideoToolbox on macOS), encodes test frame, falls back to libx264 on failure
  Blocking: true
- [ ] EC-003: spike-report.md produced with recommendation: continue
  Evidence: spike-report.md in .pm/runtime/
  Blocking: true
- [ ] EC-004: feasibility_ready: true in state.yaml
  Evidence: state.yaml updated
  Blocking: true

## Current progress

Not started.

## Open blockers

None.
