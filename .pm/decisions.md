# Decision Log

## D-001: Complete rewrite strategy
**Date**: 2026-05-13
**Decision**: Full greenfield rewrite on `feat/v2-rewrite` branch. v1 preserved on `main`.
**Rationale**: Python/C++ hybrid architecture is unmaintainable. Incremental migration would preserve the worst of both worlds.
**Trade-offs**: High upfront cost, zero backward compatibility, but clean architecture from day one.

## D-002: H264 hardware encoding priority
**Date**: 2026-05-13
**Decision**: OAK uses DepthAI internal H264 encoder. USB cameras use FFmpeg hardware encoder with libx264 fallback.
**Rationale**: OAK hardware encoding is near-zero host CPU. FFmpeg abstracts platform differences (VideoToolbox, NVENC, QSV, VAAPI).
**Trade-offs**: Hardware encoder quality slightly lower than software at same bitrate. Compensated by high bitrate (5-10 Mbps).

## D-003: Mixed encoding strategy (not pure plugin-side or pure centralized)
**Date**: 2026-05-13
**Decision**: Each backend provides "best effort" encoding. TranscodeStage normalizes to unified H264 output.
**Rationale**: OAK can provide pre-encoded H264 (zero host overhead). USB needs host-side encoding. Unifying post-backend simplifies pipeline.

## D-004: steady_clock + system_clock timestamp
**Date**: 2026-05-13
**Decision**: Single wall clock anchor at session start. steady_clock for all per-frame timestamps. Hardware PTS (OAK) used for interval correction.
**Rationale**: system_clock is not monotonic (NTP, DST). steady_clock guarantees monotonic increase. Hardware PTS provides sensor-level precision.
**Trade-offs**: Absolute time depends on clock sync at session start. Acceptable for experiment-level analysis.

## D-005: Apple HIG design system
**Date**: 2026-05-13
**Decision**: SF fonts, SF Symbols, navy blue accent (#1B2A4A), light mode, HIG-compliant layout and interactions.
**Rationale**: User preference for Apple aesthetic. SF system fonts available on macOS. Cross-platform fallback to Segoe UI / Noto Sans.
