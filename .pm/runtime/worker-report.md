# Worker Report: Phase 2 — StreamLivenessMonitor

## Task Summary

Implemented `StreamLivenessMonitor` class with background thread, per-stream timestamps, per-plugin aggregation, injectable clock, and 6 unit tests.

## Files Changed

| File | Action | Lines |
|------|--------|-------|
| `internal/infrastructure/StreamLivenessMonitor.h` | NEW | ~58 |
| `internal/infrastructure/StreamLivenessMonitor.cpp` | NEW | ~94 |
| `tests/unit/test_stream_liveness_monitor.cpp` | NEW | ~168 |
| `CMakeLists.txt` | MODIFIED | +2 (source + test) |

## Implementation Details

### StreamLivenessMonitor (header + impl)

- **Injectable clock**: `ClockFn` defaults to `std::chrono::steady_clock::now` when nullptr, enabling deterministic testing.
- **Background thread**: Uses `std::jthread` with stop token; `monitor_loop()` sleeps 1s between check cycles.
- **Per-stream tracking**: `last_active_` maps stream_id → timestamp; `stream_to_plugin_` maps stream_id → plugin_id.
- **Stall detection**: Each cycle compares `clock_() - last_active_[sid]` against `stall_timeout_ms_`; fires `StallCallback` per stalled stream.
- **Per-plugin aggregation**: Groups streams by plugin_id; if ALL streams of a plugin are stalled, fires `AllStalledCallback` once per stall event (tracked via `plugins_all_stalled_fired_` set, cleared on `update_activity`).
- **Thread safety**: All map access guarded by `mutex_`.

### Unit Tests (6 cases)

1. **StallCallbackFiresAfterTimeout** — register 1 stream, advance clock past timeout → stall callback fires.
2. **NoStallCallbackBeforeTimeout** — register 1 stream, advance clock but stay within timeout → no callbacks.
3. **PartialPluginStallNoAllStalled** — 3 streams (2 plugin_a, 1 plugin_b), stall only cam1 → no all-stalled callback.
4. **AllStreamsStalledFiresAllStalledCallback** — all streams stalled → all-stalled fires for both plugins.
5. **UnregisterRemovesStream** — unregister before timeout → no callbacks.
6. **UpdateActivityResetsTimer** — update_activity resets the stall timer.

## Verification Evidence

### Build

```
cmake --build build -j 4 → [100%] Built target test_stream_liveness_monitor (success)
```

### New Tests

```
[==========] Running 6 tests from 1 test suite.
[  PASSED  ] 6 tests. (12064 ms total)
```

### Full Suite

```
100% tests passed, 0 tests failed out of 33 (Total Test time: 27.05 sec)
```

- Existing tests: 31 passed (no regressions)
- New tests: 6 passed (test_stream_liveness_monitor)
- Python test: 1 passed (test_validate_session_artifacts)

## Risk Classification

**leaf** — new class, no existing production code modified except CMakeLists.txt (additive only).

## Acceptance Criteria

- [x] `StreamLivenessMonitor.h/.cpp` implemented with injectable clock
- [x] `test_stream_liveness_monitor.cpp` with 6 test cases
- [x] `cmake --build build` succeeds
- [x] All 33 tests pass (31 existing + 2 others, no regressions)
- [x] All 6 new unit tests pass individually
