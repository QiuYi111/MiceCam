# Phase Report: Timestamp Source Switch to `system_clock`

**Date**: 2026-04-07 01:00
**Scope**: Recording frame/session timestamps
**Branch**: `codex/fix-system-clock-timestamps`

## Summary

This change switches persisted recording timestamps from `high_resolution_clock` semantics to explicit `system_clock` semantics.

The previous implementation stored `Frame.timestamp` and session start/end metadata using `time_since_epoch()` from `high_resolution_clock`. On platforms where `high_resolution_clock` aliases a monotonic clock, those persisted values are not real wall-clock timestamps and cannot be safely interpreted across processes or after restart.

## Implemented Changes

- Changed `micecam::Frame::timestamp` to `std::chrono::system_clock::time_point`.
- Changed default frame construction to stamp frames with `std::chrono::system_clock::now()`.
- Changed `DiskWriter` session start and end metadata to use `system_clock`, with the session start captured in `start()` rather than object construction.
- Kept `steady_clock`-style duration/timeout logic untouched for performance timing and polling code.
- Updated the disk-writer streaming test to assert deterministic frame timestamps after `system_clock` precision casting and to bound session start/end timestamps around the real `start()`/`stop()` window.

## Why This Was Needed

Persisted timestamps in metadata files are expected to represent real-world time.

Using a monotonic or implementation-defined clock for persisted epoch values can cause:

- invalid absolute timestamps in JSONL/HDF5 metadata
- mismatches between Python callback timestamps and recorded metadata
- incorrect downstream analysis when users correlate recordings with external logs or events

## Verification

- Rebuilt `micecam_tests.exe` in `build-codex-usb-enum` after the timestamp change.
- The code compiles successfully with the updated `system_clock` timestamp path.
- Automated GoogleTest execution is still blocked on this machine by the existing Windows runtime loading issue during test discovery (`0xc0000135`), so runtime execution of the updated test could not be completed here.

## Residual Risks

- Any external consumer that accidentally relied on the old monotonic pseudo-epoch behavior will now observe real wall-clock timestamps instead.
- If any backend emits hardware-native timestamps in the future, they should remain a separate field rather than replacing the wall-clock capture timestamp written to disk.
