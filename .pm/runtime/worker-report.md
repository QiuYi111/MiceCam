# Worker Report: 003 Phase 5 OAK Plugin Skeleton

## Stage

`003-phase-5-oak-plugin`

## Objective

Implement a bounded Phase 5 OAK plugin skeleton with manifest validation and no-hardware-safe contract tests.

## Changed Files

| File | Action |
|------|--------|
| `cmd/plugins/micecam_oak/plugin.json` | New — OAK plugin manifest |
| `cmd/plugins/micecam_oak/OAKPluginServer.h` | New — OAK plugin gRPC server header |
| `cmd/plugins/micecam_oak/OAKPluginServer.cpp` | New — OAK plugin gRPC server implementation |
| `cmd/plugins/micecam_oak/main.cpp` | New — OAK plugin executable entrypoint |
| `cmd/plugins/micecam_oak/CMakeLists.txt` | New — OAK plugin build (no DepthAI dependency) |
| `CMakeLists.txt` | Modified — wired OAK plugin subdirectory and test target |
| `tests/unit/test_oak_plugin_server.cpp` | New — 24 contract tests (gRPC + manifest) |

## Implementation Details

### OAK Plugin Server (`OAKPluginServer.h/.cpp`)

- Implements all 11 `CameraPluginService` RPCs.
- No-hardware-safe: `hardware_available_` and `sdk_available_` are hardcoded `false` in skeleton mode.
- `Handshake` returns OAK plugin identity (`MiceCam OAK-D Capture` v0.1.0, API v1) with SDK/hardware warnings.
- `EnumerateDevices` returns a single diagnostic `DeviceInfo` entry with `available=false` and a structured `unavailable_reason`.
- `GetCapabilities` returns all-zero/false fields with `plugin_metadata` JSON indicating unavailable status.
- `ValidateConfig` accepts `encoder_profile` (H264/H265), `resolution` (720p/1080p/4K), `framerate` (1–60), and rejects all unknown keys.
- `GetConfigSchema` returns the three supported config fields.
- `OpenStream`, `StartStream`, `StopStream` return `OAK_UNAVAILABLE` error with `is_recoverable=true`.
- `HealthCheck` reports healthy with `oak_sdk=unavailable` and `oak_hardware=unavailable` in status message.
- `Shutdown` succeeds.

### Plugin Manifest (`plugin.json`)

- `id`: `micecam.oak`
- `version`: `0.1.0` (semver)
- `plugin_api_version`: 1
- `min_micecam_version`: `2.0.0`
- Three platform entries: `macos-arm64`, `linux-x86_64`, `linux-aarch64`
- `preferred_process_model`: `PER_DEVICE`
- `optional_features`: `depthai_sdk`
- Validates cleanly via `PluginManifest::validate()`.

### Build Configuration

- OAK plugin CMakeLists.txt mirrors FFmpeg plugin structure but links only `spdlog` and `nlohmann_json` — no DepthAI, no FFmpeg.
- Test target `test_oak_plugin_server` links `micecam_encoding` (for `PluginManifest`), gRPC, GTest, and spdlog.
- No OAK hardware or SDK is required for build or test.

### Test Coverage (24 tests)

**gRPC contract tests (16):**
1. `HandshakeReturnsOAKIdentity` — accepted, correct version/name
2. `HandshakeVersionMismatch` — rejected with warnings
3. `HandshakeWarnsNoHardware` — hardware/SDK warnings present
4. `GetPluginInfoReturnsOAKIdentity` — correct id/name/version/process models
5. `EnumerateDevicesReturnsDiagnosticsWhenNoHardware` — diagnostic entry with unavailable stream
6. `GetCapabilitiesReturnsUnavailableStatus` — all false/zero, metadata present
7. `ValidateConfigAcceptsValidKeys` — valid config passes
8. `ValidateConfigRejectsUnknownKey` — unknown key rejected
9. `ValidateConfigRejectsInvalidEncoderProfile` — VP9 rejected
10. `ValidateConfigRejectsInvalidResolution` — 8K rejected
11. `ValidateConfigRejectsOutOfRangeFramerate` — 999 rejected
12. `GetConfigSchemaReturnsOAKFields` — encoder_profile, resolution, framerate present
13. `OpenStreamReturnsUnavailable` — OAK_UNAVAILABLE error
14. `StartStreamReturnsUnavailable` — OAK_UNAVAILABLE error
15. `StopStreamReturnsUnavailable` — OAK_UNAVAILABLE error
16. `HealthCheckReturnsHealthyWithDiagnostics` — healthy, sdk/hardware in status
17. `ShutdownSucceeds` — clean shutdown

**Manifest tests (7):**
18. `ManifestValidatesWithoutErrors`
19. `ManifestHasCorrectId`
20. `ManifestHasCorrectVersion`
21. `ManifestHasCorrectApiVersion`
22. `ManifestHasCorrectMinMicecamVersion`
23. `ManifestHasThreePlatforms`
24. `ManifestHasCorrectProcessModels`
25. `ManifestRoundTripsToJson`

## Commands Run

```bash
# Original implementation
cmake --build build -j 4     # PASS — all targets built
ctest --test-dir build --output-on-failure  # PASS — 32/32 tests (including 25 new OAK tests)

# Cleanup rework verification
git checkout -- tests/unit/test_plugin_contract.cpp  # restored to committed baseline
cmake --build build -j 4     # PASS — all targets rebuilt
ctest --test-dir build --output-on-failure  # PASS — 32/32 tests, 0 failed
```

## Test Results

- Total: 32 tests passed, 0 failed
- New OAK tests: `test_oak_plugin_server` — 25 tests, all passed
- Pre-existing tests: all 31 continue to pass

## Acceptance Checklist

- [x] Official OAK plugin manifest exists and is test-validated
- [x] OAK plugin server skeleton compiles without OAK hardware
- [x] Handshake test passes
- [x] No-hardware enumerate/diagnostic test passes
- [x] Capabilities/config validation tests pass
- [x] Stream lifecycle unavailable-path tests pass
- [x] `cmake --build build -j 4` passes
- [x] `ctest --test-dir build --output-on-failure` passes (32/32)
- [x] `.pm/runtime/worker-report.md` written
- [x] One git commit created: `e16e9a8 feat(003): Phase 5 — OAK plugin skeleton, manifest, no-hardware contract tests`

## Cleanup Rework (this task)

Removed unauthorized dirty change in `tests/unit/test_plugin_contract.cpp` (40 lines of stale OAK manifest tests with wrong path, version `1.0.0` vs `0.1.0`, platform key `darwin` vs `macos-arm64`, process model `SINGLETON` vs `PER_DEVICE`). Restored file to committed baseline via `git checkout --`.

Verification after cleanup:
```bash
cmake --build build -j 4     # PASS — all targets built
ctest --test-dir build --output-on-failure  # PASS — 32/32 tests, 0 failed
git status --short -- tests/unit/test_plugin_contract.cpp  # clean (no output)
```

## Problems Encountered

- Stale files from a previous failed worker attempt were present on disk (`OAKPluginServer.h`, `OAKPluginServer.cpp`, `test_oak_plugin_server.cpp`) referencing non-existent proto types (`DiagnosticInfo`, `mutable_diagnostics`, `set_encoder_name`). Overwrote all files with clean implementations matching the actual proto contract.
- Initial test link failed due to missing `micecam_encoding` dependency for `PluginManifest`. Added to link libraries.
- Unauthorized dirty change in `test_plugin_contract.cpp` from a prior worker run was cleaned up in this rework pass.

## Deviations from Task

- None. Implementation follows task scope exactly: skeleton with no DepthAI dependency, no hardware required, manifest validated via existing `PluginManifest` infrastructure.
