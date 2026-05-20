# Production Readiness Checklist / Closure Gate

> **Formerly**: Feature Spec 004 — Production-Ready Plugin App (implementation scope now **complete** on `dev`)
> **Current purpose**: Track remaining closure items before `dev` → `main` merge and final sign-off.

## Metadata

| Field       | Value                                  |
|-------------|----------------------------------------|
| Document ID | `specs/004-production-ready-plugin-app`|
| Status      | Closure Gate                           |
| Baseline    | `dev` at `ac24012`                     |
| Audit       | `docs/reports/reviews/dev-readiness-audit-05-19.md` |

## Background

Spec 004 described 28 functional requirements for a production-ready plugin application. Implementation was carried out alongside specs 003, 005, and 006 on the `dev` branch. As of `ac24012`, **27/28 FRs are implemented and tested** on Linux (41/41 pass) and macOS (42/43 pass, one flaky). The spec is no longer a forward-looking implementation plan; it is now a closure gate for merging `dev` to `main`.

---

## Checklist: Functional Requirements

### Done (implementation verified)

- [x] **FR-001** — AppController has zero in-process backend references (`grep Backend AppController.cpp` → 0 matches)
- [x] **FR-002** — Plugins enforce `api_version=2`; version <2 rejected at handshake (`kApiVersion = 2` in both plugins)
- [x] **FR-003** — Plugin-encoded H264 bypasses Transcoder; RecordingPipeline dual-path routes encoded packets directly to StreamWriter
- [x] **FR-004** — RAW fallback encoding: Transcoder encodes when `PayloadHeader.kind != H264`
- [x] **FR-005** — `Calibrate` RPC defined in `camera_plugin.proto` (lines 282-340)
- [x] **FR-006** — `StreamConfig.keyframe_interval` field (proto field 10)
- [x] **FR-007** — Preflight Phase 1: `run_phase1_calibration()` calls Calibrate, computes `min_gop`
- [x] **FR-008** — `P_latency >= frame_interval` blocks recording with error code
- [x] **FR-009** — Calibrate failure triggers resolution fallback retry
- [x] **FR-010** — Preflight Phase 2: `run_phase2_stress_test()` with parallel stream drop detection
- [x] **FR-011** — Fragmented MP4: `+frag_keyframe+empty_moov+default_base_moof` in `StreamWriter.cpp:66`
- [x] **FR-012** — Wall time in timestamps: `FrameTimestamp.wall_time_ns`, SRT ISO 8601, `_meta.json` session_start_wall_time
- [x] **FR-013** — Plugin crash recovery: `PluginRegistryService::handle_plugin_crash()` with finalize → shm cleanup → restart → reconnect
- [x] **FR-014** — Per-plugin crash isolation: recovery scoped to affected `plugin_id`
- [x] **FR-015** — Runtime overflow monitoring: spdlog WARNING on ring buffer overflow
- [x] **FR-016** — Device disconnect modal: warning dialog on device loss
- [x] **FR-017** — Plugin management UI list with columns (name, type, version, enabled, status, device count)
- [x] **FR-018** — Plugin import action with manifest + handshake validation
- [x] **FR-019** — Plugin enable/disable toggle (restart required)
- [x] **FR-020** — Recording lock disables all plugin modification controls
- [x] **FR-021** — Plugin detail view displays manifest fields
- [x] **FR-022** — `LinkedPluginConfig` stored as `{path, enabled, added_at}` objects
- [x] **FR-023** — `_stats.json` is JSON object keyed by stream ID (validated by `validate_session_artifacts.py` 44/44)
- [x] **FR-024** — Three-platform CI: macOS-arm64, ubuntu-24.04, windows-2022 matrix (`.github/workflows/ci.yml`)
- [x] **FR-025** — Windows CI uses `BUILD_UI=OFF`; HIL/stress excluded from CI
- [x] **FR-027** — FFmpeg and OAK plugins at `api_version=2` with Calibrate RPC implementation
- [x] **FR-028** — Slot size from `CalibrateResponse.recommended_slot_size`

### Done (non-functional)

- [x] **NFR-001** — fMP4 survives SIGKILL (verified by fMP4 smoke test)
- [x] **NFR-002** — fMP4 overhead negligible
- [x] **NFR-003** — Preflight completes in <5s per phase
- [x] **NFR-004** — Crash recovery reconnects within timeout
- [x] **NFR-005** — Crash recovery events logged at INFO+
- [x] **NFR-006** — Overflow logged as WARNING with stream ID
- [x] **NFR-007** — fMP4 playable in VLC, ffprobe, FFmpeg
- [x] **NFR-008** — Calibrate testable with mock plugin; crash recovery testable with fault injection

### Open

- [ ] **FR-026** — Merge `dev` to `main` (blocked by flaky test below)
- [ ] **Fix/harden** `test_stream_liveness_monitor::StallCountResetsOnActivity` on macOS (flaky timing; PASS on Ubuntu 41/41)

### Deferred

- [ ] **HIL tests** (`test_hil_e2e`, `test_hil_crash_recovery`) — hardware available on `jingyi-lab` (2x USB cameras, RTX 3090); test files not yet created. Track as separate spec or branch.
- [ ] **PM runtime state** — `.pm/runtime/state.yaml`, `acceptance-review.md`, `active-stage.md`, `handoff.md` are stale. Update after merge.
- [ ] **Manual UI sign-off** — if still required, conduct post-merge.

---

## Acceptance Evidence (from audit `ac24012`)

| Platform    | Build | CTest    | Python Validator |
|-------------|-------|----------|-------------------|
| macOS arm64 | PASS  | 42/43    | 44/44 PASS        |
| Ubuntu 24.04| PASS  | 41/41    | (included)        |
| Windows     | CI    | CI green | CI green          |

**Code audits confirmed**: 1 PayloadKind definition, 0 in-process backends in AppController, 0 internal includes from api/, fMP4 movflags present, Calibrate RPC in proto, wall time in all writers, crash recovery wired.

---

## Next Steps (in order)

| Step | Branch | Action |
|------|--------|--------|
| 1 | `fix/flaky-stallcount-test` | Harden `StallCountResetsOnActivity` on macOS |
| 2 | `dev` → `main` | Merge |
| 3 | — | Update `.pm/runtime/` state files |
| 4 | `feat/hil-tests` | Create `test_hil_e2e.cpp` + `test_hil_crash_recovery.cpp` |
| 5 | — | Run HIL gate on `jingyi-lab` |
| 6 | — | Manual UI sign-off (optional) |

This checklist is the **closure gate** for spec 004. No broad feature branch (`feat/004-production-ready`) is needed — all implementation already lives on `dev`.
