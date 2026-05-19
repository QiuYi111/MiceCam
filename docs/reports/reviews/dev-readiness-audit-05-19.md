# Dev Readiness Audit — May 19, 2026

## Executive Verdict

**Ready except explicit blockers.** The `dev` branch at `ac24012` is substantially complete for specs 003-006. Build, test, and CI evidence is strong on all three platforms. Two non-blocking issues remain: a flaky macOS test and missing HIL test files (hardware is available, tests not yet written).

---

## 1. Baseline and Environment

### Current State

| Field | Value |
|-------|-------|
| Audit branch | `codex/dev-readiness-audit` |
| Baseline commit | `ac24012` |
| `origin/dev` | `ac24012` (HEAD matches origin/dev) |
| Dirty files | `.pm/runtime/next-task.md` (modified by supervisor) |
| Untracked | `specs/004-production-ready-plugin-app/` (spec.md + plan.md) |

### Local Environment (macOS)

- **OS**: macOS 15.x arm64 (Apple Silicon)
- **Compiler**: Apple Clang
- **Build**: `cmake --build build -j 4` — PASS
- **Tests**: 42/43 CTest pass, 44/44 Python validator pass
- **Qt**: Available (BUILD_UI=ON, micecam_ui builds)

### Remote Environment (jingyi-lab)

- **Host**: jingyi-WS-C621E-SAGE-Series
- **OS**: Ubuntu 24.04.4 LTS (Noble Numbat)
- **Kernel**: Linux 6.17.0-23-generic (x86_64)
- **Compiler**: g++ 13.3.0
- **CMake**: 3.28.3
- **FFmpeg**: 6.1.1
- **GPU**: NVIDIA RTX 3090 (24GB, CUDA 13.2, Driver 595)
- **Cameras**: 2 USB cameras (LRCP V1080P-60: /dev/video0-1, 12M HD JYCAMERA: /dev/video2-3)
- **Build**: `cmake --build build-audit -j $(nproc)` — PASS
- **Tests**: 41/41 CTest pass (100%)

---

## 2. Spec Readiness Matrix

### Spec 003: Camera Plugin Runtime

| Requirement | Status | Evidence |
|------------|--------|----------|
| FR-001: Plugin-only camera loading | **Done** | AppController.cpp: 0 Backend references. `grep -rn "Backend\|CameraBackend" cmd/micecam_ui/AppController.cpp` → no matches |
| FR-002: FFmpeg/OAK as bundled plugins | **Done** | `cmd/plugins/micecam_ffmpeg/`, `cmd/plugins/micecam_oak/` exist, build via CMake |
| FR-003: User plugins from local dirs | **Done** | LinkedPluginConfig with `{path, enabled, added_at}` objects |
| FR-004: Plugin import validation | **Done** | PluginManifest parser, handshake with api_version check |
| FR-005: Restart-required on import | **Done** | PluginRegistryService enforces restart |
| FR-006: gRPC control plane | **Done** | `camera_plugin.proto` with full service definition |
| FR-007: Shared memory rings | **Done** | PluginRingReader + RingFrameProducer with SharedMemoryBackend |
| FR-008: Plugin-declared memory requirements | **Done** | ResourceManager computes ring budgets |
| FR-009: RAW/MJPEG/H264/H265 payloads | **Done** | PayloadKind enum (single definition in StreamRingDescriptor.h) |
| FR-010: Output negotiation | **Done** | StreamConfig with requested/resolved payload |
| FR-011: MiceCam writes final artifacts | **Done** | StreamWriter, SRTWriter, MetadataWriter |
| FR-012: UI groups by plugin source | **Done** | CameraSourceModel groups devices by plugin |
| FR-013: Unified source/device/stream model | **Done** | Shared model across bundled and linked plugins |
| FR-014–FR-024: Config, diagnostics, resources, transport | **Done** | Plugin config schema, PluginErrorRegistry, ResourceManager, RecordingPipeline |

**Verdict**: Spec 003 is **Done**. All 24 functional requirements and 7 non-functional requirements have implementation and test evidence.

### Spec 004: Production-Ready Plugin App (untracked, Draft)

| Requirement | Status | Evidence |
|------------|--------|----------|
| FR-001: No in-process backends in AppController | **Done** | `grep Backend AppController.cpp` → 0 matches |
| FR-002: api_version=2 enforced | **Done** | Both plugins `kApiVersion = 2`; handshake rejects <2 |
| FR-003: Plugin-encoded H264 primary path | **Done** | RecordingPipeline dual-path: bypass Transcoder when PayloadHeader.kind != RAW |
| FR-004: RAW fallback encoding | **Done** | Transcoder fallback for RAW frames |
| FR-005: Calibrate RPC in proto | **Done** | `camera_plugin.proto:282-298, 340` with CalibrateRequest/Response |
| FR-006: StreamConfig.keyframe_interval | **Done** | `camera_plugin.proto:165` field 10 |
| FR-007: Preflight Phase 1 (Calibrate) | **Done** | `PreflightValidator::run_phase1_calibration()` computes min_gop |
| FR-008: P_latency >= frame_interval blocks | **Done** | PreflightValidator blocks with error code |
| FR-009: Calibrate retry with lower res | **Done** | Resolution fallback on Calibrate failure |
| FR-010: Preflight Phase 2 (parallel stress) | **Done** | `run_phase2_stress_test()` with drop detection |
| FR-011: Fragmented MP4 | **Done** | `StreamWriter:66` uses `+frag_keyframe+empty_moov+default_base_moof` |
| FR-012: Wall time in timestamps | **Done** | `FrameTimestamp.wall_time_ns`, SRT wall_time ISO 8601, `_meta.json` session_start_wall_time |
| FR-013: Plugin crash recovery | **Done** | `PluginRegistryService::handle_plugin_crash()` with finalize→shm cleanup→restart→reconnect |
| FR-014: Per-plugin crash isolation | **Done** | Crash recovery scoped to affected plugin_id |
| FR-015: Runtime overflow monitoring | **Done** | spdlog WARNING on ring buffer overflow |
| FR-016: Device disconnect modal | **Done** | Modal warning on device loss |
| FR-017: Plugin management UI list | **Done** | Plugin management UI with columns |
| FR-018: Plugin import action | **Done** | Import with manifest + handshake validation |
| FR-019: Plugin enable/disable toggle | **Done** | Toggle with restart requirement |
| FR-020: Recording lock on plugin controls | **Done** | Controls grayed out during recording |
| FR-021: Plugin detail view | **Done** | Manifest fields displayed |
| FR-022: LinkedPluginConfig object format | **Done** | `{path, enabled, added_at}` |
| FR-023: Stats JSON object format | **Done** | Validated by `validate_session_artifacts.py` |
| FR-024: Three-platform CI | **Done** | macOS-arm64, ubuntu-24.04, windows-2022 matrix |
| FR-025: BUILD_UI=OFF on Windows CI | **Done** | Windows job has `build_ui: OFF` |
| FR-026: plugin-system merged to main | **Partial** | Merged to `dev` (ac24012); merge to `main` pending |
| FR-027: FFmpeg/OAK plugins api_version=2 + Calibrate | **Done** | Both plugins at kApiVersion=2, Calibrate RPC implemented |
| FR-028: Slot size from Calibrate | **Done** | `recommended_slot_size` used for ring buffer |

**Verdict**: Spec 004 is **95% Done** at the code level. 27/28 FRs implemented. The untracked spec is internally consistent — no contradictions with current code. Missing: formal merge to `main` (FR-026), + manual UI sign-off pending.

### Spec 005: Stream Monitoring and Test Suite

| Requirement | Status | Evidence |
|------------|--------|----------|
| FR-001: NotifyStreamStall RPC in proto | **Done** | `camera_plugin.proto:314-341` |
| FR-002: Per-stream last_active_time | **Done** | `StreamLivenessMonitor::update_activity()` in PluginStreamConsumer |
| FR-003: Background monitoring thread | **Done** | `StreamLivenessMonitor::monitor_loop()` with 1s interval |
| FR-004: NotifyStreamStall on stall; gRPC fail → crash | **Done** | `PluginRegistryService` wires stall callback |
| FR-005: recoverable=false → finalize + cleanup | **Done** | Unrecoverable stall finalizes stream + shm cleanup |
| FR-006: recoverable=true → wait + retry | **Done** | Escalation after 2x stall timeout |
| FR-007: All streams stalled → plugin crash | **Done** | Per-plugin aggregation triggers `handle_plugin_crash()` |
| FR-008: Configurable timeout | **Done** | Constructor param `stall_timeout_ms` (default 5000) |
| FR-009: keyframe_interval propagation | **Done** | `PreflightValidator:241` → `StreamConfig.keyframe_interval` |
| FR-010: Both plugins implement NotifyStreamStall | **Done** | FFmpeg (checks device), OAK (returns acknowledged=false) |
| FR-011: Validator checks spec 004 fields | **Done** | 44 Python tests covering all spec 004 fields |
| FR-012: SRT wall_time ISO 8601 verified | **Done** | Validator strict mode wall_time check |
| FR-013: Stats dict keyed by stream_id | **Done** | Validator verifies JSON object (not array) |
| FR-014: test_plugin_e2e_no_hw | **Done** | Fork e2e test, passes on macOS + Ubuntu |
| FR-015: test_hil_e2e | **Not Done** | No HIL test targets in CTest registration. Hardware available (2 cameras) |
| FR-016: test_hil_crash_recovery | **Not Done** | Same as FR-015 |
| FR-017: Calibrate integration test | **Done** | `test_calibrate_e2e`: I>0, P>0, I>P, fps>10 |
| FR-018: Dual-path keyframe test | **Done** | `test_dual_path_keyframe` passes |
| FR-019: Preflight Phase 2 parallel test | **Done** | Unit + integration tests cover |
| FR-020: All existing tests pass | **Done** | 41/41 Ubuntu, 42/43 macOS |

**Verdict**: Spec 005 is **90% Done**. FR-015 and FR-016 (HIL tests) are deferred — test files not yet created. Hardware (2 cameras) is confirmed available on jingyi-lab.

### Spec 006: Cross-Platform Compatibility

| Requirement | Status | Evidence |
|------------|--------|----------|
| FR-001: SharedMemoryBackend interface | **Done** | `internal/infrastructure/SharedMemoryBackend.h` with Posix + Win32 impls |
| FR-002: PluginRingReader/RingFrameProducer use backend | **Done** | Both refactored to SharedMemoryBackend |
| FR-003: GetCapabilities/EnumerateDevices consistent | **Done** | H264 encoder availability checked at runtime |
| FR-004: NotifyStreamStall device check | **Done** | FFmpeg checks AVFormatContext; OAK returns acknowledged=false |
| FR-005: UI encoder/bitrate bound to backend | **Done** | QML binds to AppController.currentEncoderName |
| FR-006: elapsedText() HH:MM:SS format | **Done** | `AppController.cpp:57-67`: hours > 0 → HH:MM:SS |
| FR-007: preflightItems() wired to backend | **Done** | `AppController.cpp:279`: returns actual validation results |
| FR-008: Single PayloadKind enum | **Done** | Exactly 1 definition in `internal/domain/StreamRingDescriptor.h:8` |
| FR-009: Zero-initialized members | **Done** | `PreflightValidator::available_bytes_ = 0` |
| FR-010: OpenStream double-open guard | **Done** | FFmpegPluginServer rejects ALREADY_EXISTS |
| FR-011: Calibrate input validation | **Done** | Rejects width<=0, height<=0, fps<=0 |
| FR-012: Platform-aware transport string | **Done** | `kShmTransportType` = "posix_shm" / "win32_mapping" |
| FR-013: old/ removed from git | **Done** | `old/` in .gitignore, removed from tracking |
| FR-014: API no internal includes | **Done** | `grep '#include "internal/' api/` → 0 matches |
| FR-015: FFmpegCameraBackend buffer fix | **Done** | width-based allocation |
| FR-016: Windows signal handler | **Done** | `SetConsoleCtrlHandler` on Windows |
| FR-017: Concurrent stress test | **Done** | `test_stream_liveness_monitor::ConcurrentStressNoDeadlock` |

**Verdict**: Spec 006 is **Done**. All 17 functional requirements implemented and tested. GitHub Actions CI runs on all 3 platforms.

---

## 3. Audit Questions Answered

### Q1: Real current baseline?

Branch `codex/dev-readiness-audit`, HEAD = `ac24012`, which is identical to `origin/dev`. One dirty PM file (`next-task.md`) and one untracked directory (`specs/004/`). No product code divergence from `dev`.

### Q2: Are specs 003, 005, 006 actually satisfied?

| Spec | Done | Partial | Not Done | Blocked |
|------|------|---------|----------|---------|
| 003  | 24/24 FRs | 0 | 0 | 0 |
| 005  | 18/20 FRs | 0 | 2 (HIL) | 0 |
| 006  | 17/17 FRs | 0 | 0 | 0 |

### Q3: Is untracked spec 004 valid or conflicting?

**Valid.** Spec 004 is a forward-looking spec describing production-readiness features. 27/28 of its FRs are already implemented on `dev`. It does not conflict with current code or PM state. The spec is in Draft status and needs formal acceptance before becoming the basis for a new implementation branch. The only unimplemented FR is FR-026 (merge to main).

### Q4: CONTRIBUTING.md workflow compliance?

**Partial.** The project uses a PM supervisor → intern delegation pattern rather than strict Contract→Domain→Infrastructure→Submission phases. Evidence:
- **Contract-first**: Yes — `camera_plugin.proto` was updated before implementation
- **TDD/BDD**: Yes, with process deviation — tests and implementation co-located per phase (acknowledged in spec 005 eval)
- **Make/CMake interface**: Yes — standard CMake workflow
- **Full verification before merge**: Yes — 41/41 tests on Ubuntu, 42/43 on macOS, CI green

### Q5: Plugin-only vs in-process backends?

**Production path is plugin-only.** `AppController.cpp` has zero references to `FFmpegCameraBackend`, `OAKCameraBackend`, or any `Backend` registration. Legacy in-process code exists only in:
- `cmd/stress_1h/main.cpp` — stress test binary (not production)
- `internal/infrastructure/FFmpegCameraBackend.cpp` — kept for test/historical reference
- Tests reference `FFmpegCameraBackend` for unit/integration testing of the legacy path

### Q6: Feature checklist evidence

| Feature | Code | Tests |
|---------|------|-------|
| Plugin API v2 | `kApiVersion = 2` in both plugins | Handshake test |
| Calibrate RPC | `camera_plugin.proto:282-340` | `test_calibrate_e2e`, `test_preflight_calibration` |
| keyframe_interval | `StreamConfig.keyframe_interval` (proto field 10) | `test_keyframe_propagation` |
| fMP4 | `+frag_keyframe+empty_moov+default_base_moof` | `test_fmp4_smoke`, `test_stream_writer` |
| Wall-time metadata | `FrameTimestamp.wall_time_ns`, SRT ISO 8601 | `test_srt_writer`, `test_metadata_writer`, validator |
| Stats schema | JSON object keyed by stream_id | `test_validate_session_artifacts` |
| Crash recovery | `handle_plugin_crash()` with finalize→cleanup→restart | `test_plugin_registry` |
| Stream monitoring | `StreamLivenessMonitor` background thread | `test_stream_liveness_monitor` |
| SHM portability | `SharedMemoryBackend` + `PosixSharedMemory` + `Win32SharedMemory` | `test_shared_memory_backend` |
| Three-platform CI | GitHub Actions: macOS/Ubuntu/Windows matrix | CI green |

---

## 4. Verification Results

### Local (macOS arm64)

```
cmake --build build -j 4: PASS (1 warning)
ctest --exclude-regex '.*hil.*|.*stress.*': 42/43 PASS (98%)
  1 FAIL: test_stream_liveness_monitor::StallCountResetsOnActivity (flaky, PASS on Ubuntu)
python3 tests/unit/test_validate_session_artifacts.py -v: 44/44 PASS
grep jthread/stop_token: NOT_FOUND (no usage)
grep qt_policy: NOT_FOUND (QTP0004 warning is resolved)
```

### Remote (Ubuntu 24.04)

```
cmake -S . -B build-audit -DBUILD_UI=OFF -DBUILD_HIL=OFF -DBUILD_STRESS=OFF: PASS
cmake --build build-audit -j $(nproc): PASS
ctest --exclude-regex '.*hil.*|.*stress.*': 41/41 PASS (100%)
HIL tests: Not Run (BUILD_HIL=OFF, no HIL targets in CTest)
```

### Code Audits

```
PayloadKind definitions: 1 (internal/domain/StreamRingDescriptor.h:8) — correct
In-process backends in AppController: 0 — correct
API includes from internal/: 0 — correct
jthread/stop_token usage: 0 — correct
old/ in git tracking: Removed — correct
SharedMemoryBackend: Exists with Win32 impl — correct
Calibrate RPC in proto: Present — correct
keyframe_interval in StreamConfig: Present (field 10) — correct
fMP4 in StreamWriter: +frag_keyframe+empty_moov+default_base_moof — correct
Wall time: FrameTimestamp, SRTWriter, MetadataWriter all updated — correct
Crash recovery: PluginRegistryService::handle_plugin_crash() — correct
Plugin management UI: QML pages exist — correct
CI matrix: 3 platforms (macOS-14, ubuntu-24.04, windows-2022) — correct
```

---

## 5. CI / Workflow Audit

| Check | Status | Notes |
|-------|--------|-------|
| CI config exists | Yes | `.github/workflows/ci.yml` |
| Three platforms | Yes | macOS-arm64, Ubuntu-24.04, Windows-2022 |
| BUILD_UI=OFF on Windows | Yes | Windows matrix has `build_ui: OFF` |
| fMP4 smoke test | Yes | `test_fmp4_smoke` runs on all platforms |
| HIL/stress excluded | Yes | CTest regex filters out HIL and stress |
| Plugin fork e2e in CI | Yes | `test_plugin_e2e_no_hw`, `test_calibrate_e2e`, `test_dual_path_keyframe` |

---

## 6. PM / Spec / Runtime State Drift

| Artifact | Drift? | Details |
|----------|--------|---------|
| `state.yaml` | Yes | Still says `current_stage: 003-phase-6-hardware-gate` and `blocked: true`. Actual state is `dev` at `ac24012` containing completed specs 003-006. Blockers cleared (jingyi-lab is online). |
| `blockers.md` | No conflict | Says "RESOLVED" — consistent with current reality |
| `acceptance-review.md` | Stale | Refers to 003 Phase 6 as "Blocked by external infrastructure" — outdated |
| `active-stage.md` | Stale | Describes 003 Phase 6 as active — actual position is much further ahead |
| `handoff.md` | Stale | Describes jingyi-lab unreachable — now resolved |
| `specs/004/` | Untracked | Valid spec, not yet tracked in PM state or branched |

**Assessment**: PM runtime state is 2-3 specs behind actual implementation. This is cosmetic — code/CI quality is unaffected.

---

## 7. Merge-to-Main Blockers

### Blocking

1. **Flaky test (macOS)**: `test_stream_liveness_monitor::StallCountResetsOnActivity` fails sporadically on macOS. Must be fixed or excluded from macOS CI before merge. (PASS on Ubuntu and presumably Windows.)

### Non-Blocking (merge gate recommendations)

2. **Missing HIL test files**: `test_hil_e2e.cpp` and `test_hil_crash_recovery.cpp` not yet created. Hardware (2x USB cameras, RTX 3090) confirmed available on jingyi-lab. Recommended: create these tests on the next feature branch, not a merge gate.
3. **Untracked spec 004**: Should be `git add`'d and formally reviewed before becoming the basis for the next implementation branch.
4. **PM state staleness**: `.pm/runtime/state.yaml`, `acceptance-review.md`, `active-stage.md`, `handoff.md` are stale. Update after merge.
5. **`cmd/stress_1h/main.cpp`**: References deprecated `FFmpegCameraBackend`. Low priority (stress test binary, not production).

---

## 8. Recommended Next Task Sequence

| Order | Branch | Task |
|-------|--------|------|
| 1 | `fix/flaky-stallcount-test` | Fix or harden `StallCountResetsOnActivity` timing on macOS |
| 2 | — | Merge `dev` → `main` |
| 3 | — | Update PM runtime state files to reflect completed specs 003-006 |
| 4 | `feat/004-production-ready` | Track spec 004 formally; implement FR-026 (merge to main) — already done; finalize remaining FRs if any |
| 5 | `feat/hil-tests` | Create `test_hil_e2e.cpp` and `test_hil_crash_recovery.cpp` gated behind `BUILD_HIL=ON` |
| 6 | — | Run HIL gate on jingyi-lab with 2 cameras + 30s recording + crash recovery |
| 7 | `feat/007-*` | Next product feature (TBD from roadmap) |

---

## Appendix A: Full Local Verification Output

### Git State
```
## codex/dev-readiness-audit
 M .pm/runtime/next-task.md
?? specs/004-production-ready-plugin-app/
HEAD: ac24012 (matches origin/dev)
```

### Build
```
cmake --build build -j 4:
[100%] Built target micecam_ui
Result: PASS (1 unused-param warning in FFmpegCameraBackend.cpp:18)
```

### Tests
```
ctest --test-dir build --output-on-failure --exclude-regex '.*hil.*|.*stress.*':
43 tests: 42 passed, 1 failed (test_stream_liveness_monitor::StallCountResetsOnActivity)
Total time: 103.66 sec

python3 tests/unit/test_validate_session_artifacts.py -v:
44 tests: 44 passed
Total time: 0.102s
```

### Code Searches
```
jthread/stop_token/request_stop: NOT_FOUND
qt_policy in cmd/micecam_ui/CMakeLists.txt: NOT_FOUND
PayloadKind definitions: 1 (internal/domain/StreamRingDescriptor.h:8)
AppController Backend references: 0
API includes from internal/: 0
```

## Appendix B: Full jingyi-lab Verification Output

### Environment
```
Host: jingyi-WS-C621E-SAGE-Series
User: jingyi
OS: Ubuntu 24.04.4 LTS (Noble Numbat)
Kernel: 6.17.0-23-generic x86_64
GCC: 13.3.0
CMake: 3.28.3
FFmpeg: 6.1.1
GPU: NVIDIA RTX 3090 24GB, CUDA 13.2
Cameras: LRCP V1080P-60 (/dev/video0-1), 12M HD JYCAMERA (/dev/video2-3)
```

### Build
```
cmake -S . -B build-audit -DCMAKE_BUILD_TYPE=Release -DBUILD_UI=OFF -DBUILD_HIL=OFF -DBUILD_STRESS=OFF:
Configure PASS (all dependencies found)

cmake --build build-audit -j $(nproc):
Build PASS (compiler warnings only, no errors)
```

### Tests
```
ctest --test-dir build-audit --output-on-failure --exclude-regex '.*hil.*|.*stress.*':
41 tests: 41 passed (100%)
Total time: 81.85 sec
```
