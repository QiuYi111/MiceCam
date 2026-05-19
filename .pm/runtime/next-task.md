# Task: Recheck Spec 004 Coverage and Convert to Release Checklist

## Objective

Re-audit `specs/004-production-ready-plugin-app/` against current `dev` baseline and convert it from a large implementation spec/plan into a production-readiness checklist that can be tracked in the repository.

This is a documentation/spec maintenance task. Do not change product code, tests, build files, or workflows.

## Baseline

- Current branch: `codex/dev-readiness-audit`
- Code baseline: `dev` / `origin/dev` at `ac24012`
- Existing audit report: `docs/reports/reviews/dev-readiness-audit-05-19.md`
- Existing 004 files are currently untracked:
  - `specs/004-production-ready-plugin-app/spec.md`
  - `specs/004-production-ready-plugin-app/plan.md`

## Required Work

1. Recheck spec 004 coverage against current code and audit evidence.
   - Read `docs/reports/reviews/dev-readiness-audit-05-19.md`.
   - Read `specs/004-production-ready-plugin-app/spec.md` and `plan.md`.
   - Spot-check key code paths only as needed to verify the matrix:
     - `api/micecam/camera_plugin.proto`
     - `cmd/micecam_ui/AppController.cpp`
     - `internal/pipeline/PreflightValidator.cpp`
     - `internal/pipeline/RecordingPipeline.cpp`
     - `cmd/plugins/micecam_ffmpeg/`
     - `cmd/plugins/micecam_oak/`
     - `.github/workflows/ci.yml`
     - `tests/` references for Calibrate/fMP4/stats/crash recovery/stream liveness
2. Rewrite `specs/004-production-ready-plugin-app/spec.md` so it is no longer a large implementation feature spec.
   - Rename its purpose in the document to **Production Readiness Checklist / Closure Gate**.
   - Preserve useful acceptance criteria as checklist items.
   - Mark already-covered items as `Done` with evidence pointers.
   - Mark remaining items as `Open` or `Deferred`.
   - Explicitly state that implementation scope was mostly completed by specs 003/005/006 on `dev`.
   - Keep the remaining open items clear:
     - merge `dev` to `main`
     - fix/harden macOS flaky `StallCountResetsOnActivity`
     - add formal HIL tests (`test_hil_e2e`, `test_hil_crash_recovery`) or track them separately
     - update stale PM runtime state after merge
     - manual UI sign-off if still required
3. Rewrite `specs/004-production-ready-plugin-app/plan.md` into a closure plan.
   - Remove or demote phased implementation work that is already done.
   - Replace it with a small sequence of closure tasks and branch recommendations.
   - Make clear that this spec should not spawn a broad `feat/004-production-ready` implementation branch.
4. Include/nest 004 into project tracking.
   - Update `project_index` to mention `specs/004-production-ready-plugin-app/` as the production readiness closure checklist.
   - If there is a better existing index/tracking doc, update that instead or in addition, but keep scope documentation-only.
5. Write a short implementation report:
   - `docs/reports/implements/spec-004-checklist-conversion-05-19.md`
   - Include what changed, why 004 was downgraded, remaining open gates, and next recommended branches.
6. Update `.pm/runtime/worker-report.md` with a truthful report.

## Allowed Files

You may modify/create only:

- `specs/004-production-ready-plugin-app/spec.md`
- `specs/004-production-ready-plugin-app/plan.md`
- `project_index`
- `docs/reports/implements/spec-004-checklist-conversion-05-19.md`
- `.pm/runtime/worker-report.md`

## Forbidden Scope

- No product code changes.
- No test changes.
- No CI/workflow changes.
- No build system changes.
- No dependency changes.
- No commits.
- No merges, rebases, pushes, or destructive git commands.
- Do not modify `.pm/runtime/state.yaml`, `.pm/runtime/handoff.md`, `.pm/runtime/acceptance-review.md`, or `.pm/runtime/loop-log.md`.

## Verification Commands

Run and record:

```bash
git status --short --branch
git diff --name-status
test -f specs/004-production-ready-plugin-app/spec.md
test -f specs/004-production-ready-plugin-app/plan.md
test -f docs/reports/implements/spec-004-checklist-conversion-05-19.md
rg -n "Production Readiness Checklist|Closure Gate|Done|Open|Deferred" specs/004-production-ready-plugin-app docs/reports/implements/spec-004-checklist-conversion-05-19.md
rg -n "specs/004-production-ready-plugin-app|production readiness" project_index
```

No full build is required because this task is documentation-only. If you run no build/test, state that explicitly.

## Acceptance Criteria

- [ ] Spec 004 is rewritten as a production readiness checklist / closure gate, not a broad implementation feature.
- [ ] Spec 004 plan is rewritten as a closure plan with small next branches.
- [ ] 004 is included in project tracking/index.
- [ ] A concise implementation report exists.
- [ ] Worker report exists and lists changed files, commands, verification, problems, deviations.
- [ ] Only allowed files changed.
- [ ] No commit created.
