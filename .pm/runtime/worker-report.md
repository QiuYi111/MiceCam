# Worker Report: Phase 3 — Preflight Two-Phase (Calibrate + Stress Test)

## Task
Spec 004 Phase 3: two-phase preflight — Phase 1 per-stream Calibrate RPC integration (min_gop computation, blocking, retry-lower-res) + Phase 2 multi-stream parallel stress test with drop detection.

## Risk classification
**Branch** — multi-file pipeline change, requires tests. No infra/core changes.

## Files changed

| File | Action | Description |
|------|--------|-------------|
| `internal/domain/CalibrationResult.h` | NEW | Domain type with stream_id, latencies, min_gop, degraded_resolution, warnings |
| `internal/pipeline/PreflightValidator.h` | MODIFIED | Added ICalibrationClient, IStreamTestController interfaces, StressTestResult, PreflightResult.calibration_results, new validate() overload, run_phase1_calibration(), run_phase2_stress_test(), compute_min_gop() |
| `internal/pipeline/PreflightValidator.cpp` | MODIFIED | Implemented Phase 1 calibration with retry-lower-res, Phase 2 stress test with drop detection, integrated into validate() flow |
| `tests/unit/test_preflight_calibration.cpp` | NEW | 16 tests covering min_gop computation, blocking, retry, drop detection, full integration |
| `CMakeLists.txt` | MODIFIED | Added test_preflight_calibration target |

## Implementation details

### Phase 1: run_phase1_calibration()
- For each stream config, calls `ICalibrationClient::calibrate()`
- Computes `min_gop = ceil(I_latency / (frame_interval - P_latency))` via static `compute_min_gop()`
- If `P_latency >= frame_interval`: blocks recording (min_gop = -1, success = false)
- If initial calibration fails: retries with 50% width/height, sets `degraded_resolution = true`
- Returns `map<stream_id, CalibrationResult>`

### Phase 2: run_phase2_stress_test()
- Opens all streams simultaneously via `IStreamTestController::openStream()`
- Sleeps for configurable duration (default 3000ms)
- Queries drop counts per stream via `getDropCount()`
- Closes all streams
- Phase 2 warnings are non-blocking (recording proceeds)

### Integration: validate() overload
- Runs existing disk space check first
- Then Phase 1 calibration (if client provided) — blocks on failure
- Then Phase 2 stress test (if controller provided) — warnings attached but non-blocking
- Backward compatible: original validate() signature unchanged

## Test evidence

16 new tests, all passing:

| Test | Description |
|------|-------------|
| MinGopComputation_BasicCase | I=2ms, P=7ms, fps=30 → min_gop=1 |
| MinGopComputation_HigherI | I=50ms, P=5ms, fps=30 → min_gop=2 |
| MinGopComputation_LargeGop | I=100ms, P=5ms → matches ceil formula |
| BlocksWhenPExceedsFrameInterval | P=34ms > frame_interval=33.3ms → blocked |
| BlocksWhenPIsExactlyFrameInterval | P == frame_interval → blocked |
| BlocksWhenFpsZero | fps=0 → blocked |
| Phase1SuccessfulCalibration | Happy path calibration |
| Phase1RetryLowerResolution | First fails, retry at 50% succeeds |
| Phase1RetryStillFails | Both attempts fail → blocked |
| Phase1BlocksOnHighPLatency | Calibration succeeds but P too high → blocked |
| Phase2DropDetection | Drops detected → warning with stream ID and count |
| Phase2NoDrops | No drops → passed, no warnings |
| Phase2OpensAndClosesAllStreams | Verifies open/close lifecycle |
| FullValidateWithCalibrationAndStress | End-to-end with both phases |
| FullValidateFailsWhenPhase1Blocked | Phase 1 failure stops validation |
| FullValidateWithPhase2Warnings | Phase 2 warnings attached, recording proceeds |

## Verification commands

```
cmake --build build -j 4 2>&1 | tail -5
→ [100%] Built target test_preflight_calibration

ctest --test-dir build --output-on-failure 2>&1 | tail -5
→ 100% tests passed, 0 tests failed out of 36
```

## Acceptance criteria

- [x] CalibrationResult domain type exists with all specified fields
- [x] Phase 1 calls Calibrate RPC per stream and computes min_gop correctly
- [x] Phase 1 blocks recording when P_latency >= frame_interval
- [x] Phase 1 retries with lower resolution on Calibrate failure
- [x] Phase 2 starts all streams in parallel for configurable duration and detects drops
- [x] Phase 2 generates warnings on drops, no warnings on clean run
- [x] Existing PreflightValidator checks (disk space, etc.) still pass
- [x] cmake --build succeeds
- [x] ctest passes (36/36)
- [x] 16 new tests (minimum 4 required)
