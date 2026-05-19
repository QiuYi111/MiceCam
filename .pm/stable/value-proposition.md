# Value Proposition

## For laboratory researchers

**Before**: Camera recording requires Python scripts, custom `.bin` format, fragile process isolation, silent frame drops on macOS.

**After**: Double-click app, one-click record. Standard `.mp4` files that play in any video player. Frame drops are measured and alerted, not hidden. Feishu notifications when something goes wrong.

## For data scientists

**Before**: Proprietary `.bin` format requires custom decoder. Timestamps from `system_clock` are unreliable (non-monotonic across daylight savings, NTP adjustments). Must write glue code to extract frames.

**After**: Standard `.mp4` — open in any tool (FFmpeg, OpenCV, scikit-video). `.srt` timestamps with steady_clock precision. `_stats.json` with per-frame encoding metadata. Zero friction from capture to analysis.

## Key differentiator

**Uncompromising data integrity**: Every frame accounted for, every failure surfaced, every timestamp precise. Built for science, not surveillance.
