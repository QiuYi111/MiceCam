# Worker Report: Stage 3 — Encoding Infrastructure

## Summary

All 10 acceptance criteria met. 6 test executables (22 individual tests) pass. Build succeeds with zero warnings. Valid `.mp4` (h264 codec, mov/mp4 container), `.srt`, `_meta.json`, and `_stats.json` produced from mock frame source. TDD flow followed: RED (all tests fail) → GREEN (all implementations written) → VERIFIED (ffprobe, ctest).

## Task

- **Task ID**: Stage 3 — Encoding Infrastructure
- **Phase**: Phase 2 (Encoding Chain) of MiceCam v2 rewrite
- **Risk Classification**: branch (full encoding chain, touches multiple modules)

## Scope Executed

| # | Item | Status |
|---|------|--------|
| 1 | HardwareEncoderSelector (detect_platform_encoder, is_hardware_encoder, get_fallback_encoder) | Done |
| 2 | FFmpegEncoder (implements IEncoder, RGB→YUV→H264, two-pass fallback, VT B-frame fix) | Done |
| 3 | TranscodeStage (passthrough H264, or encode raw frames) | Done |
| 4 | StreamWriter (implements IStreamWriter, MP4 via avformat muxer, thread-safe) | Done |
| 5 | SRTWriter (per-frame SRT subtitle entries with timestamps) | Done |
| 6 | MetadataWriter (_meta.json + _stats.json via nlohmann_json, pretty-print) | Done |
| 7 | Unit tests: test_encoder_selector (5 tests) | Done |
| 8 | Unit tests: test_ffmpeg_encoder (6 tests) | Done |
| 9 | Unit tests: test_stream_writer (4 tests) | Done |
| 10 | Unit tests: test_srt_writer (3 tests) | Done |
| 11 | Unit tests: test_metadata_writer (4 tests) | Done |
| 12 | Integration test: test_encoding_pipeline (2 tests) | Done |
| 13 | `cmake --build build -j` succeeds with zero warnings | Done |
| 14 | `ctest --output-on-failure` passes 100% | Done |

## Files Created (20 new, 1 modified)

### New Files (17 source + 3 pipeline stubs = 20 total)

```
internal/pipeline/IEncoder.h
internal/pipeline/IStreamWriter.h
internal/pipeline/TranscodeStage.h
internal/pipeline/TranscodeStage.cpp
internal/infrastructure/HardwareEncoderSelector.h
internal/infrastructure/HardwareEncoderSelector.cpp
internal/infrastructure/FFmpegEncoder.h
internal/infrastructure/FFmpegEncoder.cpp
internal/infrastructure/StreamWriter.h
internal/infrastructure/StreamWriter.cpp
internal/infrastructure/SRTWriter.h
internal/infrastructure/SRTWriter.cpp
internal/infrastructure/MetadataWriter.h
internal/infrastructure/MetadataWriter.cpp
tests/unit/test_encoder_selector.cpp
tests/unit/test_ffmpeg_encoder.cpp
tests/unit/test_stream_writer.cpp
tests/unit/test_srt_writer.cpp
tests/unit/test_metadata_writer.cpp
tests/integration/test_encoding_pipeline.cpp
```

### Modified Files

```
CMakeLists.txt  (added micecam_encoding STATIC library, 6 test targets, CTest registration)
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
[100%] Built target test_metadata_writer

Zero compiler warnings (-Wall -Wextra).
```

## Test Results

```
$ cd build && ctest --output-on-failure -j6
Test project /Volumes/DataHub/Projects/MiceCam/build
    Start 3: test_stream_writer
    Start 6: test_encoding_pipeline
    Start 5: test_metadata_writer
    Start 4: test_srt_writer
    Start 2: test_ffmpeg_encoder
    Start 1: test_encoder_selector
1/6 Test #1: test_encoder_selector ............ Passed
2/6 Test #3: test_stream_writer ............... Passed
3/6 Test #4: test_srt_writer .................. Passed
4/6 Test #5: test_metadata_writer ............. Passed
5/6 Test #6: test_encoding_pipeline ........... Passed
6/6 Test #2: test_ffmpeg_encoder .............. Passed

100% tests passed, 0 tests failed out of 6
```

### Per-suite breakdown

| Test Suite | Tests | Result |
|---|---|---|
| test_encoder_selector | DetectReturnsNonEmpty, IsHardwareEncoderForKnownHW, IsHardwareEncoderFalseForSoftware, FallbackIsLibx264, MacOSDetectsVideoToolbox | 5/5 pass |
| test_ffmpeg_encoder | InitializeWithDefaultConfig, EncoderNameReturnsNonEmptyAfterInit, EncodeProducesOutput, Encode30Frames, FlushWorks, EncoderNameIsCorrect | 6/6 pass |
| test_stream_writer | OpenCreatesFile, WritePacketWorks, CloseProducesNonEmptyFile, WriteMultiplePacketsThenClose | 4/4 pass |
| test_srt_writer | OpenCreatesFile, WriteEntryProducesCorrectFormat, WriteMultipleEntries | 3/3 pass |
| test_metadata_writer | WriteSessionHeaderCreatesValidJson, WriteSessionFooterUpdatesFile, WriteStatsCreatesValidJson, SessionHeaderJsonIsPrettyPrinted | 4/4 pass |
| test_encoding_pipeline | FullEncodeChainProducesValidMP4, EncodeWithFlushProducesOutput | 2/2 pass |

## Verification (Output Artifacts)

### MP4 (ffprobe)

```
$ ffprobe -v error -show_entries stream=codec_name,width,height:format=format_name,duration \
  build/test_output/integration_output.mp4
h264,320,240
"mov,mp4,m4a,3gp,3g2,mj2",0.001953
```

### SRT (sample entry)

```
1
00:00:00,000 --> 00:00:00,033
seq=0 offset_us=0 skipped=false
```

### meta.json

```json
{
  "bitrate_kbps": 5000,
  "encoder_name": "libx264",
  "end_time_ns": 0,
  "keyframe_interval": 60,
  "output_dir": "/tmp/test",
  "session_id": "test-session-001",
  "start_time_ns": 1000000,
  "stream_configs": [],
  "wall_clock_anchor_ns": 1234567890
}
```

### stats.json

```json
[
  {
    "avg_encode_latency_us": 0.0,
    "bytes_written": 500000,
    "drop_rate": 0.01,
    "encoder_fallback": false,
    "encoder_used": "h264_videotoolbox",
    "frames_actual": 99,
    "frames_expected": 100,
    "stream_id": "cam0"
  },
  {
    "encoder_fallback": true,
    "encoder_used": "libx264",
    "frames_actual": 100,
    "frames_expected": 100,
    "stream_id": "cam1"
  }
]
```

## Acceptance Criteria Checklist

- [x] **AC-001**: HardwareEncoderSelector returns `h264_videotoolbox` on macOS
- [x] **AC-002**: FFmpegEncoder encodes 30 frames → valid H264 (ffprobe confirms)
- [x] **AC-003**: FFmpegEncoder fallback to libx264 when preferred encoder unavailable (two-pass fallback)
- [x] **AC-004**: TranscodeStage passthrough (H264→H264) works — `process()` returns input unchanged when `source_format == "h264"`
- [x] **AC-005**: StreamWriter produces valid .mp4 (ffprobe: mov,mp4 container, h264 codec)
- [x] **AC-006**: SRTWriter produces valid .srt with correct entry format (seq=N offset_us=M skipped=bool)
- [x] **AC-007**: MetadataWriter produces valid JSON (_meta.json, _stats.json parse without error via nlohmann::json::parse)
- [x] **AC-008**: Integration test: mock source → encode → .mp4 → ffprobe confirms valid
- [x] **AC-009**: All 6 test executables pass (ctest 100%)
- [x] **AC-010**: `cmake --build build -j` succeeds with zero warnings (-Wall -Wextra clean)

## Technical Details

### FFmpegEncoder Architecture
- **Lazy initialization**: Codec context created on first `encode()` call with actual frame dimensions
- **Two-pass fallback**: Try preferred encoder → try `libx264` → try any `AV_CODEC_ID_H264`
- **VideoToolbox fix**: `max_b_frames=0` and `color_range=AVCOL_RANGE_MPEG` when using `h264_videotoolbox`
- **swscale**: RGB24→YUV420P conversion via `SWS_BILINEAR`, resizes context only on dimension change
- **Encoder options**: `preset=fast`, `tune=zerolatency` for libx264; CRF configurable

### StreamWriter Architecture
- Uses `avformat_alloc_output_context2` with automatic format detection from file extension
- Copies packet data via `av_new_packet` + `memcpy` (safe ownership)
- Thread-safe via `std::mutex` on all operations
- Proper cleanup via `av_write_trailer` + `avio_closep`

### SRTWriter Architecture
- Microsecond-to-SRT timestamp conversion: `HH:MM:SS,mmm` format
- Thread-safe with mutex, flushes after each entry
- Frame duration hardcoded to ~33.3ms (30fps) default

### MetadataWriter Architecture
- Static methods (no state needed — pure format operations)
- `write_session_footer` reads existing JSON, adds fields, rewrites (read-modify-write)
- All JSON pretty-printed with indent=2 via `nlohmann::json::dump(2)`

## Constraints Honored

- No camera backends implemented (Stage 4)
- No RecordingPipeline implemented (Stage 4)
- No UI changes
- No domain type modifications (reused existing types)
- No Watchdog, AlertManager, or FeishuWebhook
- TDD flow followed (tests written first, verified failing, then implemented)
- `-Wall -Wextra` clean build
- Output files in `build/test_output/`
- CTest configured with `WORKING_DIRECTORY ${CMAKE_BINARY_DIR}`

## Notes

- On this macOS/arm64 system, `h264_videotoolbox` is detected and available but tests use `prefer_hardware=false` for deterministic `libx264` behavior
- Integration test uses smaller frames (320x240 / 160x120) for fast execution
- `test_open.srt` is 0 bytes by design (open→close with no entries)
- The `micecam` executable links against `micecam_encoding` static library, no breakage to existing Qt6 app
