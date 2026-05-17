# Worker Report: Fix signal handler mutex recursion + fork e2e binary path resolution

## Task Reference

- Task: `.pm/runtime/next-task.md`
- Branch: `feat/005-stream-monitoring-test-suite`
- Baseline: 39/39 tests passing (commit b4e5ade)

## Summary

Fixed two bugs:
1. **Signal handler mutex recursion** — Plugin mains called `spdlog::info()` and `g_server->Shutdown()` directly in signal handlers, which are not async-signal-safe. This caused `detected illegal recursion into Mutex code` when signals fired during gRPC `Wait()`.
2. **Fork e2e binary path resolution** — Integration tests used `std::filesystem::current_path()` to find the plugin binary, which only worked when cwd was `build/`. Running from project root failed.

## Changes

### 1. Signal handler fix (2 files)

**Files:** `cmd/plugins/micecam_ffmpeg/main.cpp`, `cmd/plugins/micecam_oak/main.cpp`

- Added `std::atomic<bool> g_shutdown_requested{false}`
- Signal handler now **only** sets the atomic flag — fully async-signal-safe
- Added `#include <atomic>` and `#include <thread>`
- Replaced `g_server->Wait()` with a `shutdown_watcher` thread that polls the atomic flag every 200ms and calls `g_server->Shutdown()` from a safe (non-signal) context
- Main thread calls `g_server->Wait()` (blocks until `Shutdown()` is called by the watcher thread)
- `spdlog::info("Shutdown requested")` moved to the watcher thread

**Note:** The task specified `Wait(timeout)` pattern, but the installed gRPC version (`grpc::Server::Wait()`) takes no arguments. Adapted to use a shutdown watcher thread + `Wait()` instead, which achieves the same safety guarantee: `Shutdown()` is never called from the signal handler.

### 2. Binary path resolution fix (3 files)

**Files:** `tests/integration/test_plugin_e2e_no_hw.cpp`, `tests/integration/test_calibrate_e2e.cpp`, `tests/integration/test_dual_path_keyframe.cpp`

- Added `#include <mach-o/dyld.h>` for macOS
- `find_plugin_binary()` now resolves path relative to the test binary location using `_NSGetExecutablePath` (macOS) or `/proc/self/exe` (Linux)
- Path resolution: `test_binary_dir/../cmd/plugins/micecam_ffmpeg/micecam_ffmpeg_plugin` (since test binary is at `build/tests/test_xxx` and plugin is at `build/cmd/plugins/...`)
- Falls back to original cwd-relative behavior if absolute path candidate doesn't exist

## Verification Evidence

### Build

```
cmake --build build -j 4 → SUCCESS (0 errors)
```

### Test Suite

```
ctest --test-dir build --output-on-failure --exclude-regex '.*hil.*|.*stress.*'
→ 100% tests passed, 0 tests failed out of 39
→ Total Test time (real) = 53.24 sec
```

### Mutex Recursion Check

```
./build/tests/test_plugin_e2e_no_hw --gtest_print_time=1 2>&1 | grep -i 'mutex\|recursion' | wc -l
→ 0
```

### Binary Path from Project Root

```
cd /Volumes/DataHub/Projects/MiceCam && ./build/tests/test_plugin_e2e_no_hw
→ [  PASSED  ] 1 test.
```

## Scope Compliance

- Only modified 5 allowed files
- No infrastructure/pipeline/domain/proto/CMakeLists.txt changes
- No test file modifications beyond the 3 allowed integration tests

## Acceptance Criteria

- [x] Both plugin mains use atomic flag + safe Shutdown pattern (adapted for API constraints)
- [x] No spdlog calls in signal handler
- [x] All three e2e tests find plugin binary correctly from project root AND build dir
- [x] `cmake --build build` succeeds
- [x] 39/39 tests pass via ctest
- [x] `./build/tests/test_plugin_e2e_no_hw` from project root succeeds
- [x] No `detected illegal recursion into Mutex code` in test output
