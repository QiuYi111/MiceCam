# Task: Stage 3 — Encoding Infrastructure

## Objective

Implement the complete H264 encoding chain: hardware encoder detection, FFmpeg encoder wrapper, transcode stage, MP4 writer, SRT writer, and metadata writer. Must produce valid `.mp4` + `.srt` + `_meta.json` + `_stats.json` from a mock frame source.

## Context

Phase 2 from plan.md. The spike (cmd/spike/) proved VideoToolbox works on macOS and libx264 fallback is viable. Now implement the production encoding infrastructure.

## Bounded Scope

### 1. HardwareEncoderSelector (`internal/infrastructure/`)

`HardwareEncoderSelector.h` + `.cpp`:
- `static std::string detect_platform_encoder()` — returns best hardware encoder name for current platform
- Detection order: macOS→`h264_videotoolbox`, NVIDIA→`h264_nvenc`, Intel→`h264_qsv`, Linux→`h264_vaapi`, AMD→`h264_amf`
- Falls back to `libx264` if no hardware encoder detected
- `static bool is_hardware_encoder(const std::string& name)`
- `static std::string get_fallback_encoder() → "libx264"`

### 2. FFmpegEncoder (`internal/infrastructure/`)

`FFmpegEncoder.h` + `.cpp` — implements `IEncoder`:
- `initialize(EncoderConfig)` → creates AVCodecContext with h264 codec, configures bitrate/keyframe/CRF/b_frames
- `encode(const uint8_t* rgb_data, int width, int height, int64_t pts)` → converts RGB to AVFrame, encodes, returns H264 NAL unit bytes
- `flush(vector<uint8_t>& out)` → drains remaining encoder packets
- `encoder_name()` → returns active encoder name
- Two-pass fallback: try config encoder name → if fail, try `libx264` → if fail, try any `AV_CODEC_ID_H264`
- VideoToolbox: set `max_b_frames=0`, `color_range=AVCOL_RANGE_MPEG`
- Uses swscale for RGB→YUV420P conversion

### 3. TranscodeStage (`internal/pipeline/`)

`TranscodeStage.h` + `.cpp`:
- `initialize(const EncoderConfig&)` → creates FFmpegEncoder
- `process(const uint8_t* data, size_t size, int width, int height, int64_t pts, const std::string& source_format)` → if source is already H264, passthrough; otherwise decode+encode
- `flush(vector<uint8_t>& out)` → drain encoder
- Passthrough: check if `source_format == "h264"`, return input unchanged

### 4. StreamWriter (`internal/infrastructure/`)

`StreamWriter.h` + `.cpp` — implements `IStreamWriter`:
- `open(path, width, height, fps)` → creates AVFormatContext with MP4 muxer, writes header
- `write_packet(data, size, pts, dts, keyframe)` → creates AVPacket, writes via av_interleaved_write_frame
- `close()` → writes trailer, closes file
- Thread-safe (mutex) for concurrent stream writes

### 5. SRTWriter (`internal/infrastructure/`)

`SRTWriter.h` + `.cpp`:
- `open(const std::string& path)` → opens `.srt` file for appending
- `write_entry(uint64_t seq, const domain::FrameTimestamp& ts, bool skipped)` → writes SRT subtitle entry:
  ```
  1
  00:00:00,000 --> 00:00:00,033
  seq=1 offset_us=0 skipped=false
  ```
- `close()` → flushes and closes

### 6. MetadataWriter (`internal/infrastructure/`)

`MetadataWriter.h` + `.cpp`:
- `write_session_header(const SessionMetadata&)` → writes `_meta.json`
- `write_session_footer(total_frames, total_bytes, session_checksum)` → updates `_meta.json` with end stats
- `write_stats(const std::string& path, const std::vector<StreamStats>&)` → writes `_stats.json`
- All JSON via nlohmann_json, pretty-print with indent=2

### 7. Unit Tests

`tests/unit/`:
- `test_encoder_selector.cpp` — verify correct encoder for platform, fallback works
- `test_ffmpeg_encoder.cpp` — encode 30 synthetic frames → verify H264 output
- `test_stream_writer.cpp` — write frames → verify ffprobe reports valid MP4
- `test_srt_writer.cpp` — write timestamps → verify SRT format
- `test_metadata_writer.cpp` — write meta+stats → verify JSON schema

### 8. Integration Test

`tests/integration/test_encoding_pipeline.cpp`:
- Create mock frame source (synthetic color bars, 30 frames, 1280×720)
- FFmpegEncoder → TranscodeStage → StreamWriter → output.mp4
- Run ffprobe, verify codec=h264, container valid
- StreamWriter → verify file size > 0, playable

## Acceptance Criteria

- [ ] AC-001: HardwareEncoderSelector returns `h264_videotoolbox` on macOS
- [ ] AC-002: FFmpegEncoder encodes 30 frames → valid H264 (ffprobe confirms)
- [ ] AC-003: FFmpegEncoder fallback to libx264 when hardware encoder init fails
- [ ] AC-004: TranscodeStage passthrough (H264→H264) works
- [ ] AC-005: StreamWriter produces valid .mp4 (ffprobe: mov,mp4 container, h264 codec)
- [ ] AC-006: SRTWriter produces valid .srt with correct entry format
- [ ] AC-007: MetadataWriter produces valid JSON (_meta.json, _stats.json parse without error)
- [ ] AC-008: Integration test: mock source → encode → .mp4 → ffprobe confirms valid
- [ ] AC-009: All 5 unit tests pass (ctest)
- [ ] AC-010: cmake --build build -j succeeds with no warnings

## Forbidden Scope

- Do NOT implement camera backends (OAK, FFmpeg camera) — that's Stage 4
- Do NOT implement RecordingPipeline — that's Stage 4
- Do NOT implement UI changes — this is headless encoding
- Do NOT modify domain types unless a gap is found
- Do NOT implement Watchdog, AlertManager, FeishuWebhook

## Required Harness Process

Branch risk: full chain (harness-risk→harness-context→harness-tdd→harness-eval→harness-report)
But for efficiency: TDD (write tests first, then implement), build, verify with ffprobe/ctest.

## Verification Commands

```bash
cd /Volumes/DataHub/Projects/MiceCam
cmake -B build -S .
cmake --build build -j
cd build && ctest --output-on-failure
```

## Output

`.pm/runtime/worker-report.md` with build output, test results, and all AC verification.
