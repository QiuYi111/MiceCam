# Blockers

## BLOCKER-001: PreflightValidator.FullValidationPasses test is ambiguous

**Status**: ACTIVE (test issue, not implementation issue)

**What is blocking**: The test `FullValidationPasses` at `tests/unit/test_preflight.cpp:91` expects `EXPECT_FALSE(result.passed)` but:
1. The test name says "FullValidationPasses" (suggesting expected true)
2. The comment says "disk check may or may not pass depending on actual space"
3. On this machine `/tmp` has sufficient space, so result.passed is true

**Why it blocks**: Per strict TDD rules, I must NOT modify test files during GREEN/REFACTOR phases. The test needs fixing but that's a RED phase change.

**What was tried**: Implementation correctly validates disk space. The test assertion contradicts the test name and actual disk state.

**Options to unblock**:
1. (Recommended) Fix the test: change `EXPECT_FALSE` to `EXPECT_TRUE` and rename to `FullValidationPassesWhenDiskHasSpace`
2. Add a separate test for the disk-full scenario using a path with known limited space (e.g., by mocking)
3. Remove the EXPECT_FALSE assertion and just verify the result has valid structure

**Impact**: 59/60 tests pass. The failing test is a test design issue, not an implementation bug.
