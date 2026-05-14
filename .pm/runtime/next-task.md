# Task: Final — Fix BLOCKER-001 + CI/CD + Cleanup

## Objective

Fix the last known test issue, set up cross-platform CI/CD, and finalize the v2 project structure. This is the final task before UI implementation begins.

## Bounded Scope

### 1. Fix BLOCKER-001
- See `.pm/runtime/blockers.md` — `test_preflight.cpp::FullValidationPasses` has inverted assertion
- Fix: change `EXPECT_FALSE` → `EXPECT_TRUE` in the test
- Rename test to `FullValidationPassesWhenDiskHasSpace`
- Verify: cd build && ctest → 81/81 pass

### 2. Create v2 vcpkg.json
- Write root `vcpkg.json` for v2 dependencies:
  ```json
  {
    "name": "micecam",
    "version": "2.0.0",
    "dependencies": ["ffmpeg", "qt6", "spdlog", "nlohmann-json", "gtest"]
  }
  ```
- (depthai-core is optional, not in vcpkg — handled via submodule or system install)
- Remove old `old/vcpkg.json.v1` reference from CMake if present

### 3. GitHub Actions CI (`ci.yml`)
Create `.github/workflows/ci.yml`:
```yaml
name: CI
on: [push, pull_request]
jobs:
  build:
    strategy:
      matrix:
        os: [ubuntu-24.04, macos-14, windows-2022]
        build_type: [Release]
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4
      - name: Install dependencies (macOS)
        if: runner.os == 'macOS'
        run: brew install ffmpeg qt spdlog nlohmann-json googletest pkg-config
      - name: Install dependencies (Linux)
        if: runner.os == 'Linux'
        run: sudo apt-get update && sudo apt-get install -y libavcodec-dev libavformat-dev libavutil-dev libavdevice-dev libswscale-dev qt6-base-dev qt6-declarative-dev libspdlog-dev nlohmann-json3-dev libgtest-dev pkg-config
      - name: Install dependencies (Windows)
        if: runner.os == 'Windows'
        run: |
          vcpkg install ffmpeg[core,avcodec,avformat,avdevice,swscale]:x64-windows qt6[core,quick,quickcontrols2]:x64-windows spdlog:x64-windows nlohmann-json:x64-windows gtest:x64-windows
      - name: Configure
        run: cmake -B build -S . -DCMAKE_BUILD_TYPE=${{ matrix.build_type }} -DBUILD_SPIKE=OFF
      - name: Build
        run: cmake --build build -j
      - name: Test
        run: cd build && ctest --output-on-failure
```

### 4. Final CMakeLists.txt Cleanup
- Remove any remaining v1 build system references
- Ensure `BUILD_SPIKE` option preserved
- Ensure `add_subdirectory(cmd/spike)` conditional works
- Ensure test discovery with CTest works correctly
- Set default build type to Release

### 5. Final Build Verification
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
cd build && ctest --output-on-failure
```
Expected: 81/81 pass, 0 warnings, Release binary at `build/cmd/micecam/micecam`

## Acceptance Criteria

- [ ] AC-001: BLOCKER-001 fixed → FullValidationPassesWhenDiskHasSpace passes (81/81 tests)
- [ ] AC-002: `vcpkg.json` at root with correct v2 dependencies
- [ ] AC-003: `.github/workflows/ci.yml` created
- [ ] AC-004: CI yml passes YAML validation (gh actions lint)
- [ ] AC-005: CMakeLists.txt clean (no dead v1 code paths)
- [ ] AC-006: `cmake -B build -S . -DCMAKE_BUILD_TYPE=Release && cmake --build build -j` succeeds with 0 warnings
- [ ] AC-007: `cd build && ctest --output-on-failure` passes 100%

## Forbidden Scope

- Do NOT implement UI/QML changes
- Do NOT add new features
- Do NOT modify encoding/camera/pipeline logic beyond BLOCKER-001 fix
- Do NOT delete old/ directory

## Required Harness Process

- TDD: N/A (bug fix only, no new feature)
- Build + test verification
- Write `.pm/runtime/worker-report.md`
- One git commit

## Verification Commands

```bash
cd /Volumes/DataHub/Projects/MiceCam
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
cd build && ctest --output-on-failure
```

## Output

`.pm/runtime/worker-report.md` with build output, test results, and AC checklist.
