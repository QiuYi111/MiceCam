# Spec 004 Checklist Conversion Report — May 19, 2026

## Summary

Converted `specs/004-production-ready-plugin-app/` from a large implementation feature spec (28 FRs, 7 phases, 25+ files) into a production readiness checklist and closure plan. The implementation work described in the original spec was **already completed** on `dev` at `ac24012` alongside specs 003, 005, and 006.

## Why 004 Was Downgraded

The original spec described a forward-looking feature branch (`feat/004-production-ready`). However:

- **27/28 FRs are implemented** on the current `dev` baseline
- **41/41 tests pass** on Ubuntu, **42/43** on macOS (one flaky)
- **Code audits confirm** every requirement: plugin-only mode, Calibrate RPC, fMP4, wall time, crash recovery, CI matrix
- The specs 003, 005, and 006 carried this work as part of their implementation scopes on `dev`

The spec no longer needs a feature branch. It needs closure, not implementation.

## What Changed

| File | Change |
|------|--------|
| `specs/004-production-ready-plugin-app/spec.md` | Rewritten as **Production Readiness Checklist / Closure Gate**. All 27 done FRs marked with evidence pointers. Remaining 1 FR (merge to main) and deferred items (HIL tests, PM state, UI sign-off) listed as open/deferred. |
| `specs/004-production-ready-plugin-app/plan.md` | Rewritten as **Closure Plan** with 5 small closure tasks (C-1 through C-5). Original 7 phases demoted/dropped. States explicitly: no `feat/004-production-ready` branch needed. |
| `project_index` | Added entry referencing `specs/004-production-ready-plugin-app/` as the production readiness closure checklist. |
| `docs/reports/implements/spec-004-checklist-conversion-05-19.md` | This file. |

## Remaining Open Gates

| # | Item | Priority | Branch |
|---|------|----------|--------|
| 1 | Fix flaky `StallCountResetsOnActivity` on macOS | Blocking | `fix/flaky-stallcount-test` |
| 2 | Merge `dev` → `main` | Gate | — |
| 3 | Update stale `.pm/runtime/` state files | Medium | `chore/update-pm-state` |
| 4 | Create HIL tests (`test_hil_e2e`, `test_hil_crash_recovery`) | Non-blocking | `feat/hil-tests` |
| 5 | Manual UI sign-off | Low | — |

## Recommended Next Branches

1. `fix/flaky-stallcount-test` — timing fix in `test_stream_liveness_monitor::StallCountResetsOnActivity`
2. Merge `dev` to `main`
3. Update PM runtime state
4. `feat/hil-tests` — HIL test files gated behind `BUILD_HIL=ON`

## Deviations

None. Only allowed documentation files modified. No product code, tests, CI, or build changes.
