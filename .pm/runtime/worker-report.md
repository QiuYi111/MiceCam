# Worker Report: Phase 0 — Plugin Protocol Contract Freeze

**Worker**: harness-intern (single worker)  
**Task**: `.pm/runtime/next-task.md` — 003-phase-0-contract-freeze  
**Branch**: `codex/camera-plugin-runtime`  
**Date**: 2026-05-15

## Summary

All 14 Phase 0 deliverables implemented. Full build succeeds (100%), all 25 tests pass (100%), proto compiles with protoc 34.1, manifest validation passes, invalid manifests correctly rejected.

## Changed/New Files (1,553 lines total)

### New Files (11)

| File | Lines | Description |
|------|-------|-------------|
| `api/micecam/camera_plugin.proto` | 295 | gRPC service: 11 RPCs, 23 messages, 8 enums |
| `api/micecam/plugin_manifest_schema.json` | 103 | JSON Schema (draft-07) for plugin.json validation |
| `internal/domain/PluginManifest.h` | 36 | Struct + from_json/to_json/validate API |
| `internal/domain/PluginManifest.cpp` | 108 | Implementation: semver regex, process model validation |
| `internal/domain/PluginSource.h` | 24 | Camera source group: BUNDLED/LINKED, diagnostics state |
| `internal/domain/PluginDeviceInfo.h` | 27 | Extended device descriptor with optional exclusive_resource_id |
| `internal/domain/StreamRingDescriptor.h` | 50 | Ring contract: PayloadHeader, ownership, policy, platform handle |
| `internal/domain/PluginErrorRegistry.h` | 48 | 17 error codes + ErrorMeta registry (FR-019) |
| `internal/domain/PluginErrorRegistry.cpp` | 89 | Error registry entries with severity/recovery/messages |
| `internal/domain/ResourceRequest.h` | 25 | Resource allocation model: slots, bandwidth, budgets |
| `3rdParty/bundled_plugins/micecam.ffmpeg/plugin.json` | 27 | Golden manifest for official FFmpeg plugin |
| `scripts/validate_plugin_manifest.py` | 54 | Manifest validator using jsonschema |
| `tests/unit/test_plugin_contract.cpp` | 275 | 22 tests: manifest validation, error registry, domain defaults |

### Updated Files (3)

| File | Lines | Changes |
|------|-------|---------|
| `internal/domain/PluginDescriptor.h` | 21 (+7) | Extended with id, api_version, path, source_type, PluginSourceType enum |
| `internal/domain/PluginRegistry.h` | 35 (+10) | Added register_external(), has_external(), source-grouped queries |
| `internal/domain/PluginRegistry.cpp` | 52 (+21) | Implementations for new external plugin methods |
| `CMakeLists.txt` | 284 (+25) | find_package(Protobuf QUIET), proto codegen target, new test registration |

## Design Decisions

1. **Proto-only, no gRPC codegen**: CMake uses `protobuf_generate_cpp` (generates `.pb.h/.pb.cc`) but not `grpc_generate_cpp`. gRPC server/client implementation is deferred to Phase 2+. The proto file uses `service CameraPluginService` syntax but no server stubs are generated yet — this is intentional per the task's forbidden scope.

2. **PluginSourceType lives in PluginDescriptor.h**: Extracted the enum into PluginDescriptor.h to avoid a circular dependency. PluginSource.h includes PluginDescriptor.h for the type.

3. **Protobuf is optional**: CMake uses `find_package(Protobuf QUIET)` and guards the proto target with `if(Protobuf_FOUND)`. The build works without protobuf (proto-dependent test coverage is in the manifest/registry tests).

4. **StreamRingDescriptor platform_handle**: Uses `std::string platform_handle_type` + `uintptr_t platform_handle_value` rather than `google::protobuf::Any` to keep the C++ domain model separable from the proto wire format.

5. **Error registry completeness**: All 17 error codes cover the FR-019 failure modes (MANIFEST_PARSE_ERROR through SDK_MISSING), plus the additional ones from the task spec (STREAM_WRITE_FAILED, BACKPRESSURE, DISK_FULL, EXCLUSIVE_CONFLICT).

## Verification Commands and Results

```bash
# Proto compilation
$ protoc --cpp_out=/tmp/micecam_proto api/micecam/camera_plugin.proto -I.
Proto compiled OK  ✓ (generates camera_plugin.pb.cc 559.7K, camera_plugin.pb.h 566.2K)

# Manifest validation
$ python3 scripts/validate_plugin_manifest.py \
    3rdParty/bundled_plugins/micecam.ffmpeg/plugin.json \
    api/micecam/plugin_manifest_schema.json
PASS: 3rdParty/bundled_plugins/micecam.ffmpeg/plugin.json  ✓
Validation passed.  ✓

# Invalid manifest correctly rejected
$ python3 scripts/validate_plugin_manifest.py /tmp/bad_manifest.json api/micecam/plugin_manifest_schema.json
7 validation error(s) found.  ✓ (correctly rejected)

# Full build
$ cmake --build build -j4
[100%] Built target micecam_ui  ✓

# Full test suite
$ ctest --test-dir build --output-on-failure
100% tests passed, 0 tests failed out of 25  ✓

# Contract tests specifically
$ ctest --test-dir build --output-on-failure -R test_plugin_contract
100% tests passed, 0 tests failed out of 1  ✓
```

## Acceptance Checklist

| # | Criterion | Status |
|---|-----------|--------|
| 1 | `camera_plugin.proto` defines all 11 RPCs and required enums | ✓ |
| 2 | `plugin_manifest_schema.json` validates against golden manifest | ✓ |
| 3 | `PluginManifest` rounds-trips through JSON and validates semver | ✓ |
| 4 | `PluginSource`/`PluginDeviceInfo`/`StreamRingDescriptor`/`ResourceRequest` headers compile | ✓ |
| 5 | `PluginErrorRegistry` covers all FR-019 failure modes | ✓ |
| 6 | `PluginDescriptor` extended with manifest fields | ✓ |
| 7 | `PluginRegistry` extended with external plugin support | ✓ |
| 8 | Golden FFmpeg `plugin.json` passes schema validation | ✓ |
| 9 | `validate_plugin_manifest.py` rejects invalid manifests | ✓ |
| 10 | Protobuf codegen builds with `find_package(Protobuf)` | ✓ (Protobuf 34.1 found) |
| 11 | No existing tests regressed | ✓ (25/25 pass) |
| 12 | No UI/QML files modified | ✓ |
| 13 | No gRPC server/client code written | ✓ |
| 14 | No new CMake dependencies beyond protobuf | ✓ |

## Remaining Risks

- **Protobuf version sensitivity**: The proto uses `protobuf_generate_cpp` from CMake 3.20+ FindProtobuf. On CI without Homebrew, protobuf may not be found (handled gracefully with `QUIET` + `if(Protobuf_FOUND)`).
- **gRPC codegen**: The proto defines `service CameraPluginService` but gRPC C++ codegen (`grpc_cpp_plugin`) is not invoked. This is deferred to Phase 2. The static proto library is ready to be extended with gRPC when needed.
- **PluginRegistry method stubs**: `get_source_grouped_plugins()` currently delegates to `get_external_plugins()`. Real source grouping by plugin id will be implemented in Phase 1.
