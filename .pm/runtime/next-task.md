# Task: Spec 007 Phase 8 — Release Candidate Gate

## Objective

Finalize spec 007: run full test suite, lint, independent code review, update documentation and PM state. Prepare merge recommendation. **No QML changes. No new features. Gate-only task.**

## Baseline

- Branch: `codex/007-plugin-ui-release`
- Last commit: `5eadb7d` (Phase 6 accepted)
- Completed: Phases 1-7 (source model, grouped UI, plugin mgmt, plugin detail, live metrics/notifications/flaky test, AppSettings, HIL on jingyi-lab)
- Test baseline: 45/45 pass locally, HIL 4/4 pass on jingyi-lab

## Required Work

### 1. Run Full Automated Test Suite

```bash
cmake --build build -j 4
ctest --test-dir build --output-on-failure
```

All tests must pass. Document any skip/waiver.

### 2. Run Lint/Format Checks

If project has lint/format commands, run them and fix any issues. If no lint setup exists, note it.

### 3. Independent Code Review

Run independent OpenCode `/harness review` against the full diff on this branch:
```bash
opencode run "/harness review Review the full diff on branch codex/007-plugin-ui-release for safety, scope compliance, architecture alignment, and evidence quality."
```

Fix any blocking issues found. Repeat until 0 blocking issues.

### 4. Update Documentation

- Update `project_index` with spec 007 implementation summary
- Update relevant `docs/wikis/` if needed
- Create `docs/reports/implements/spec-007-release-gate-05-19.md` with:
  - Phases completed summary
  - Test evidence (local 45/45, HIL 4/4)
  - Known deferrals: packaging validation, UI screenshots, OAK-D hardware
  - Merge recommendation

### 5. Finalize PM State

Update `.pm/runtime/` files:
- `acceptance-review.md` — final acceptance
- `state.yaml` — mark spec 007 complete
- `loop-log.md` — final iteration entry
- `handoff.md` — handoff for merge

### 6. Prepare Merge Recommendation

- Document all commits on `codex/007-plugin-ui-release` vs `dev`
- Document what tests pass, what is deferred
- Recommend merge to dev, then dev to main
- **Do NOT merge** — user must approve and execute merge

## Allowed Files

- `project_index`
- `docs/reports/implements/spec-007-release-gate-05-19.md`
- `docs/wikis/` (relevant wiki files only)
- `.pm/runtime/worker-report.md`
- `.pm/runtime/acceptance-review.md`
- `.pm/runtime/state.yaml`
- `.pm/runtime/loop-log.md`
- `.pm/runtime/handoff.md`
- `.pm/runtime/loop-control`

## Forbidden Scope

- No QML changes
- No source code changes (C++, CMake, proto)
- No test changes
- No merges, rebases, destructive git
- No force-push
- No self-merge to main

## Verification Commands

```bash
# Full test suite
cmake --build build -j 4 && ctest --test-dir build --output-on-failure

# Independent review
opencode run "/harness review Review the full diff on branch codex/007-plugin-ui-release for safety, scope compliance, architecture alignment, and evidence quality."

# Commit log
git log --oneline dev..HEAD

# Diff summary
git diff --stat dev..HEAD
```

## Acceptance Criteria

- [ ] Full test suite: 45/45 pass locally
- [ ] Independent `/harness review` completes with 0 blocking issues (or blocking issues fixed)
- [ ] `project_index` updated with spec 007 summary
- [ ] Release gate report created at `docs/reports/implements/spec-007-release-gate-05-19.md`
- [ ] PM state files finalized
- [ ] Deferrals documented: packaging validation, UI screenshots, OAK-D hardware
- [ ] Merge recommendation prepared (no merge executed)
- [ ] One git commit for doc/PM state changes only
