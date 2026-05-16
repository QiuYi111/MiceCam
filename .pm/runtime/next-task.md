# Worker Task Packet

## Objective

Run spec `003-camera-plugin-runtime` Phase 6 Ubuntu hardware/stress gate by syncing the current code to `jingyi-lab`, compiling on Ubuntu, and collecting real HIL/stress evidence.

## Stage context

Current stage: `003-phase-6-hardware-gate`.

Phase 6 preparation is accepted at commit `8555131`. The local macOS no-hardware gate passes, but the real hardware/stress gate remains pending. This task is for execution and evidence collection on the Ubuntu lab machine, not feature implementation.

## Read first

- `.pm/runtime/active-stage.md`
- `.pm/runtime/acceptance-review.md`
- `.pm/runtime/handoff.md`
- `docs/plan/phase-camera-plugin-runtime.md`
- `specs/003-camera-plugin-runtime/spec.md`
- `scripts/hil-test.sh`
- `scripts/validate_session_artifacts.py`
- `tests/hil/test_real_camera.cpp`
- `tests/hil/test_e2e_stress.cpp`
- `cmd/stress_1h/main.cpp`
- `CMakeLists.txt`

## Task

1. Confirm local source state before sync.
   - Record branch, commit, and dirty status.
   - Required source commit: `8555131` or a descendant containing it.
   - Do not include local build directories or generated artifacts in sync.
2. Sync code to `jingyi-lab`.
   - Prefer existing SSH alias `jingyi-lab`.
   - Use a clean remote workspace or clean remote git checkout.
   - Verify the remote checkout commit hash after sync.
   - If SSH or remote workspace is unavailable, write a blocker with exact command/output.
3. Capture Ubuntu environment facts.
   - `hostname`
   - `uname -a`
   - `lsb_release -a` or `/etc/os-release`
   - compiler and CMake versions
   - FFmpeg/ffprobe versions
   - GPU/NVENC visibility if applicable (`nvidia-smi` when present)
   - camera visibility (`ls /dev/video*`, `v4l2-ctl --list-devices` when present)
4. Build and run the normal no-hardware Ubuntu gate.
   - Configure a clean Ubuntu build directory.
   - Prefer `-DBUILD_UI=OFF` if Qt UI dependencies are not installed on `jingyi-lab`.
   - Build all non-HIL targets.
   - Run `ctest --test-dir <build-dir> --output-on-failure`.
5. Build and run HIL/stress gates.
   - Configure a separate build with `-DBUILD_HIL=ON -DBUILD_STRESS=ON`.
   - Build HIL and stress targets.
   - Run the HIL CTest suite with output on failure.
   - Run available stress binaries/tests. If the one-hour binary is available and hardware is ready, run it and capture logs.
   - If one-hour stress cannot run because hardware, GPU/NVENC, `/mnt/data`, or permissions are missing, report the precise blocker and still provide all earlier compile/test evidence.
6. Validate artifacts when produced.
   - For any generated `.mp4/.srt/_meta.json/_stats.json` session output, run `python3 scripts/validate_session_artifacts.py <session_dir> --strict`.
   - If current stress outputs do not match validator-required plugin transport fields, report that mismatch as gate evidence rather than editing the validator or weakening checks.
7. Report results.
   - Write `.pm/runtime/worker-report.md`.
   - Include exact remote commands and summarized outputs.
   - Include whether the Ubuntu compile gate passed.
   - Include whether HIL passed, failed, skipped, or was blocked.
   - Include whether one-hour stress passed, failed, skipped, or was blocked.
   - Include artifact paths and validation results.

## Allowed scope

- Remote `jingyi-lab` workspace operations needed to build/test this commit
- `.pm/runtime/worker-report.md`
- `.pm/runtime/blockers.md` if blocked

## Forbidden scope

- Product code changes
- Test changes
- UI/QML changes
- Proto changes
- Changing validation thresholds
- Disabling failing tests
- Claiming hardware/stress success without real command output
- Merging branches
- Editing `.pm/runtime/acceptance-review.md`, `.pm/runtime/state.yaml`, `.pm/runtime/loop-log.md`, `.pm/runtime/handoff.md`, or `.pm/runtime/active-stage.md`

## Acceptance criteria

- [ ] `jingyi-lab` remote checkout is at commit `8555131` or a verified descendant.
- [ ] Ubuntu environment facts are recorded.
- [ ] Ubuntu no-hardware configure/build result is recorded.
- [ ] Ubuntu no-hardware CTest result is recorded.
- [ ] HIL build result is recorded.
- [ ] HIL test result is recorded with pass/fail/blocker evidence.
- [ ] One-hour stress result is recorded with pass/fail/blocker evidence.
- [ ] Any generated artifacts are validated with `scripts/validate_session_artifacts.py --strict`, or absence/mismatch is reported precisely.
- [ ] `.pm/runtime/worker-report.md` contains changed-files, commands-run, test-results, acceptance checklist, problems encountered, and deviations sections.

## Required verification commands

Run local equivalents where useful, but the gate evidence must come from `jingyi-lab`:

```bash
git status --short
git rev-parse --short HEAD
ssh jingyi-lab 'hostname && uname -a'
ssh jingyi-lab 'ls /dev/video* 2>/dev/null || true'
ssh jingyi-lab 'v4l2-ctl --list-devices 2>/dev/null || true'
ssh jingyi-lab 'nvidia-smi || true'
cmake -S . -B build-ubuntu -DCMAKE_BUILD_TYPE=Release -DBUILD_UI=OFF
cmake --build build-ubuntu -j "$(nproc)"
ctest --test-dir build-ubuntu --output-on-failure
cmake -S . -B build-ubuntu-hil -DCMAKE_BUILD_TYPE=Release -DBUILD_UI=OFF -DBUILD_HIL=ON -DBUILD_STRESS=ON
cmake --build build-ubuntu-hil -j "$(nproc)"
ctest --test-dir build-ubuntu-hil --output-on-failure
./build-ubuntu-hil/stress_1h_120fps
python3 scripts/validate_session_artifacts.py <session_dir> --strict
```

Adjust paths only to match the actual remote workspace/build layout. Capture exact commands used.

## Required report file

`.pm/runtime/worker-report.md`

## If blocked

Write `.pm/runtime/blockers.md` and `.pm/runtime/worker-report.md` with the blocker signature and command output. Do not modify implementation code to bypass the gate.
