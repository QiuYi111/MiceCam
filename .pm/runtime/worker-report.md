# Worker Report: Stage 2 — Foundation

## Summary

All acceptance criteria met. CMakeLists.txt.v2 created alongside v1. All 15 domain/interface headers compile. All 4 .cpp impls link. Build succeeds with zero warnings. Qt window opens and displays "MiceCam v2 / Foundation Ready".

## Task

- **Task ID**: Stage 2 — CMake v2 + Domain Model + Plugin Interfaces
- **Phase**: Phase 1 (Foundation) of MiceCam v2 rewrite
- **Risk Classification**: branch (defines shared contracts, touches multiple modules, no infra changes)

## Scope Executed

| # | Item | Status |
|---|------|--------|
| 1 | CMakeLists.txt.v2 (C++20, FFmpeg, Qt6, spdlog, nlohmann_json, depthai, GTest) | Done |
| 2 | 9 domain type headers (DeviceInfo, StreamConfig, FrameTimestamp, SessionMetadata, StreamStats, AlertRecord, EncoderConfig, Capabilities, PluginDescriptor) | Done |
| 3 | 3 plugin interfaces (ICameraBackend, IDeviceEnumerator, WatchdogObserver) | Done |
| 4 | 3 pipeline interfaces (IEncoder, IStreamWriter, IStatsCollector) | Done |
| 5 | TimestampEngine (h+cpp) | Done |
| 6 | PluginRegistry (h+cpp) | Done |
| 7 | SessionMetadata::to_json/from_json (cpp) | Done |
| 8 | StreamStats::to_json (cpp) | Done |
| 9 | Skeleton main.cpp + main.qml | Done |
| 10 | Build succeeds: `cmake --build build -j` | Done |
| 11 | Qt window launches | Done |

## Files Created (21 files)

```
CMakeLists.txt.v2
internal/domain/AlertRecord.h
internal/domain/Capabilities.h
internal/domain/DeviceInfo.h
internal/domain/EncoderConfig.h
internal/domain/FrameTimestamp.h
internal/domain/PluginDescriptor.h
internal/domain/PluginRegistry.h
internal/domain/PluginRegistry.cpp
internal/domain/SessionMetadata.h
internal/domain/SessionMetadata.cpp
internal/domain/StreamConfig.h
internal/domain/StreamStats.h
internal/domain/StreamStats.cpp
internal/domain/TimestampEngine.h
internal/domain/TimestampEngine.cpp
api/micecam/ICameraBackend.h
api/micecam/IDeviceEnumerator.h
api/micecam/WatchdogObserver.h
internal/pipeline/IEncoder.h
internal/pipeline/IStreamWriter.h
internal/pipeline/IStatsCollector.h
cmd/micecam_v2/main.cpp
cmd/micecam_v2/qml/main.qml
cmd/micecam_v2/qml/qml.qrc
```

## Build Output

```
cmake -B build -S .
-- Checking for modules 'libavcodec;libavformat;libavutil;libavdevice;libswscale'
--   Found libavcodec, version 62.28.101
--   Found libavformat, version 62.12.101
--   Found libavutil, version 60.26.101
--   Found libavdevice, version 62.3.101
--   Found libswscale, version 9.5.101
-- Checking for module 'spdlog'
--   Found spdlog, version 1.17.0
-- Checking for module 'nlohmann_json'
--   Found nlohmann_json, version 3.12.0
-- Configuring done
-- Generating done

cmake --build build -j
[100%] Linking CXX executable cmd/micecam/micecam
[100%] Built target micecam
```

Binary: `build/cmd/micecam/micecam` (Mach-O 64-bit arm64, 632KB)

## Verification

- [x] AC-001: `CMakeLists.txt.v2` exists alongside v1 `CMakeLists.txt`
- [x] AC-002: All domain type headers compile
- [x] AC-003: All plugin interface headers compile
- [x] AC-004: All pipeline interface headers compile
- [x] AC-005: TimestampEngine compiles and links
- [x] AC-006: PluginRegistry compiles and links
- [x] AC-007: SessionMetadata::to_json() compiles (nlohmann_json included)
- [x] AC-008: StreamStats::to_json() compiles
- [x] AC-009: `cmake -B build -S . && cmake --build build -j` succeeds
- [x] AC-010: `./build/cmd/micecam/micecam` launches Qt window with "MiceCam v2 / Foundation Ready"
- [x] AC-011: No compile warnings (`-Wall -Wextra` clean)

## Constraints Honored

- No v1 files modified (CMakeLists.txt backed up and restored)
- All new files only — no v1 source files touched
- Interfaces only — no camera backends or encoders implemented
- Domain types use `micecam::domain` namespace
- Interfaces use `micecam::api` or `micecam::pipeline` namespace
- Tests not written (not in scope for Stage 2)

## Notes

- CMakeLists.txt.v2 uses PkgConfig for FFmpeg/spdlog/nlohmann_json (Homebrew compatible) and `find_package(Qt6 CONFIG)` with CMAKE_PREFIX_PATH pointing to `/opt/homebrew/opt/qt`
- `depthai-core` is optional (`find_package(depthai QUIET)`), sets `WITH_DEPTHAI` if found
- `BUILD_SPIKE` option preserved for spike subdirectory
- GTest `CONFIG` mode used for Homebrew googletest (target: `GTest::gtest`)
