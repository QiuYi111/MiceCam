# Worker Report: Task 3/8 — RecordingPipeline Produces Valid H264 MP4 from Raw Frames

## Summary

Fixed `RecordingPipeline::push_frame()` to encode raw frames through `TranscodeStage::process()` before writing, added flush on stop, and extended stats tracking with `bytes_written`, `encoder_used`, and `encoder_fallback` fields.

## Risk Classification

**BRANCH** — multi-file changes across pipeline infrastructure, domain types, and integration tests.

## Files Changed

| File | Change |
|------|--------|
| `tests/integration/test_recording_pipeline_outputs.cpp` | New: integration test verifying H264 MP4 output from raw RGB frames |
| `CMakeLists.txt` | Registered `test_recording_pipeline_outputs` test target |
| `internal/pipeline/StatsCollector.h` | Added `add_bytes()`, `set_encoder()`, `snapshot()` methods; added `encoder_used_`, `encoder_fallback_` fields |
| `internal/pipeline/StatsCollector.cpp` | Implemented new methods; `finalize()` now delegates to `snapshot()` |
| `internal/pipeline/RecordingPipeline.cpp` | Route `push_frame()` through transcoder; flush transcoder on `stop()`; use actual encoder name in `result()` |

## Details

### StatsCollector
- `add_bytes(uint64_t bytes)`: atomic add to `bytes_written_` counter
- `set_encoder(string name, bool fallback)`: records encoder name and fallback status
- `snapshot()`: returns a `StreamStats` with all fields including `bytes_written`, `encoder_used`, `encoder_fallback`
- `finalize()`: now delegates to `snapshot()` to avoid duplication

### RecordingPipeline::push_frame()
- Calls `TranscodeStage::process()` to get H264-encoded packets
- Sets encoder name and records frame stats only when encoded output is produced
- Writes encoded packets to `StreamWriter::write_packet()` (not raw frame data)
- Tracks `bytes_written` per packet via `StatsCollector::add_bytes()`
- SRT entries and watchdog feed maintained for all frames

### RecordingPipeline::stop()
- Flushes transcoder before closing each writer
- Writes any remaining flushed packets and counts their bytes

### RecordingPipeline::result()
- Uses actual transcoder encoder name instead of hardcoded "libx264"

## Test Results

```
test_recording_pipeline_outputs:    1/1 PASSED
test_recording_pipeline:            7/7 PASSED
test_camera_pipeline_integration:   2/2 PASSED
```

## Acceptance Criteria

- [x] `test_recording_pipeline_outputs` compiles and passes
- [x] MP4 file exists at expected path and contains valid H264 video stream (verified via libavformat)
- [x] `frames_actual == 30`, `bytes_written > 0`, `encoder_used` is non-empty
- [x] Existing `test_recording_pipeline` still passes
- [x] Existing `test_camera_pipeline_integration` still passes
