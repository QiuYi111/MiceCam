# Active Stage: 003 Camera Plugin Runtime — Phase 6 Hardware Gate

## Stage ID

`003-phase-6-hardware-gate`

## Stage goal

Prepare and run the USB/AVFoundation plugin runtime hardware validation gate for two real video sources.

## Why this stage matters

The architecture now has protocol contracts, plugin registry, FFmpeg plugin path, recording consumer, resource manager, and OAK skeleton. Phase 6 proves the available real USB/AVFoundation plugin path is production-worthy on target hardware, with explicit artifact validation and no silent drops.

## Inputs

- Official FFmpeg plugin
- Plugin recording consumer
- Resource manager
- Hardware matrix: MacBook Pro Camera and iPhone Continuity Camera
- Existing recording output artifacts: `.mp4`, `.srt`, `_meta.json`, `_stats.json`

## Allowed work

- Add hardware gate scripts/procedures that can be skipped or dry-run without hardware
- Add artifact validation script for MP4/SRT/meta/stats
- Add plugin smoke test harness for two source recording
- Add crash/disconnect test procedure documentation
- Add ffprobe/SRT monotonicity/stat validation helpers
- Add no-hardware-safe tests for validators/scripts

## Forbidden work

- UI/QML changes
- Changing proto definitions
- Security/sandboxing/signing policy
- Requiring hardware in normal unit/CI tests
- OAK hardware validation beyond documented pending status

## Exit criteria

- [ ] Hardware gate procedure exists.
- [ ] Artifact validation script exists and is test-covered without hardware.
- [ ] MP4 validation uses `ffprobe` when available and reports structured skip/error when unavailable.
- [ ] SRT timestamps are checked for monotonicity.
- [ ] `_meta.json` and `_stats.json` required plugin fields are checked.
- [ ] Two-source one-hour run command/procedure is documented.
- [ ] Normal build and unit tests pass without hardware.
- [ ] If real hardware is unavailable in this environment, PM stops with a precise user decision/request for hardware execution.
