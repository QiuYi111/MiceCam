# Context Bundle: Stage 4+5 — Camera Backends + Pipeline + Watchdog

## Risk Classification: BRANCH

Multi-file changes across internal/infrastructure/ and internal/pipeline/. No core domain type modifications. All new code, no existing functionality touched. Task explicitly authorizes branch risk.

## Relevant Interfaces (Read-Only)

| File | Key Types |
|------|-----------|
| `api/micecam/ICameraBackend.h` | `ICameraBackend`: enumerate_devices(), open_stream(), get_capabilities(), backend_name() |
| `api/micecam/IDeviceEnumerator.h` | `IDeviceEnumerator`: enumerate() |
| `api/micecam/WatchdogObserver.h` | `WatchdogObserver`: on_alert(AlertRecord) |

## Domain Types (Read-Only)

| File | Key Types |
|------|-----------|
| `internal/domain/DeviceInfo.h` | DeviceInfo, StreamInfo |
| `internal/domain/StreamConfig.h` | StreamConfig (device_id, stream_index, width, height, framerate, pixel_format) |
| `internal/domain/Capabilities.h` | Capabilities (supports_hardware_encode, encoder_name, streams) |
| `internal/domain/AlertRecord.h` | AlertRecord, AlertSeverity (YELLOW,RED), AlertType (7 types) |
| `internal/domain/SessionMetadata.h` | SessionMetadata with to_json/from_json |
| `internal/domain/StreamStats.h` | StreamStats with to_json |
| `internal/domain/EncoderConfig.h` | EncoderConfig |
| `internal/domain/TimestampEngine.h` | TimestampEngine: capture_wall_anchor, to_session_offset |
| `internal/domain/PluginRegistry.h` | PluginRegistry: register_backend, register_enumerator, discover_all, get_backend |

## Existing Infrastructure (Read-Only)

| File | Key Types |
|------|-----------|
| `internal/infrastructure/FFmpegEncoder.h` | FFmpegEncoder : IEncoder |
| `internal/infrastructure/StreamWriter.h` | StreamWriter : IStreamWriter |
| `internal/infrastructure/SRTWriter.h` | SRTWriter |
| `internal/infrastructure/MetadataWriter.h` | MetadataWriter (static methods) |
| `internal/pipeline/TranscodeStage.h` | TranscodeStage |

## Pipeline Interfaces (Read-Only)

| File | Key Types |
|------|-----------|
| `internal/pipeline/IEncoder.h` | IEncoder |
| `internal/pipeline/IStreamWriter.h` | IStreamWriter |

## New Interfaces Needed

- `internal/pipeline/IStatsCollector.h` — record_frame(), finalize(), add_alert()
- `internal/domain/CameraStream.h` — frame source abstraction (forward-declared in ICameraBackend.h)

## Build System

CMakeLists.txt uses `add_micecam_test(name sources)` function. Need to add new sources to micecam_encoding STATIC library and register new test executables.

## Test Patterns

Existing tests use: `micecam` namespace, anonymous namespace for helpers, GTest ASSERT/EXPECT, test fixtures. File paths use relative includes like `"domain/EncoderConfig.h"`.

## Forbidden Scope

- No UI/QML changes
- No domain type modifications (unless proven gap)
- No existing encoding infra changes (unless bug fix)
- No remote streaming, multi-machine sync, audio
- No deletion of old/ directory
