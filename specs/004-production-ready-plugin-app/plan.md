# Closure Plan: Production-Ready Plugin App

> **Status**: Closure plan only. All phased implementation (Phases 1-7 from the original plan) was completed alongside specs 003/005/006 on `dev` at `ac24012`.

## Summary

The original 7-phase implementation plan is **done**. The remaining work is a short sequence of closure tasks, not an implementation branch. No multi-file code changes are needed — only a test timing fix, a merge, PM state updates, and optional HIL test creation.

## Closure Task Sequence

### C-1: Fix Flaky macOS Test

| Field       | Value |
|-------------|-------|
| Branch      | `fix/flaky-stallcount-test` |
| Priority    | Blocking merge |
| Scope       | `tests/unit/test_stream_liveness_monitor.cpp` (StallCountResetsOnActivity) |
| Problem     | `std::this_thread::sleep_for` timing flake on macOS scheduler causes test to see `stall_count == 2` instead of expected `1` |
| Fix         | Relax assertion or add retry/wait loop |
| Verification| `ctest --test-dir build --output-on-failure -R StallCountResetsOnActivity` → 100/100 repeat runs |

### C-2: Merge `dev` to `main`

| Field       | Value |
|-------------|-------|
| Branch      | `dev` → `main` (no intermediate branch) |
| Priority    | Gate |
| Scope       | Git merge only |
| Pre-conditions | C-1 complete, CI green on both branches |
| Verification| `cmake --build build && ctest` on macOS post-merge; CI green |

### C-3: Update PM Runtime State

| Field       | Value |
|-------------|-------|
| Branch      | Update directly on `main` or in a `chore/update-pm-state` branch |
| Priority    | Medium |
| Scope       | `.pm/runtime/state.yaml`, `active-stage.md`, `handoff.md`, `acceptance-review.md` |
| Action      | Set `current_stage` to post-004, clear `blocked: true`, update handoff to reflect jingyi-lab availability |

### C-4: Create HIL Tests (track separately)

| Field       | Value |
|-------------|-------|
| Branch      | `feat/hil-tests` |
| Priority    | Non-blocking |
| Scope       | `tests/hil/test_hil_e2e.cpp`, `tests/hil/test_hil_crash_recovery.cpp` |
| Hardware    | `jingyi-lab`: 2x USB cameras (LRCP V1080P-60, 12M HD JYCAMERA), RTX 3090 NVENC |
| Gate        | `BUILD_HIL=ON` in CMake; excluded from CI |

### C-5: Manual UI Sign-Off (optional)

| Field       | Value |
|-------------|-------|
| Priority    | Low |
| Action      | User confirms Plugin Management UI, recording flow, and crash recovery UX are acceptable |

## What Does NOT Need a New Branch

- Proto changes — already done on `dev`
- fMP4 / wall time / SRT — already done on `dev`
- Preflight two-phase — already done on `dev`
- RecordingPipeline dual path — already done on `dev`
- Plugin crash recovery — already done on `dev`
- Plugin-only migration — already done on `dev`
- Plugin management UI — already done on `dev`
- Plugin upgrades to api_version=2 — already done on `dev`
- Three-platform CI — already done on `dev`

**No `feat/004-production-ready` branch is needed. The feature is done.**

## Rollback

Not applicable — this is a closure plan, not a code change plan. If any closure task fails, revert that specific task's branch; the `dev` baseline remains intact.
