# User Journeys

## Journey 1: Set up and record an experiment

1. Researcher enters lab, launches MiceCam v2
2. OAK-D and USB cameras auto-discovered, grid populates with previews
3. Sets output directory, verifies codec settings (default 5Mbps)
4. Clicks Record — all streams start simultaneously
5. During 30-min experiment, monitors grid for animal activity, checks status bar for frame count and disk
6. Experiment ends — clicks Stop, confirms save
7. Opens output folder, finds `.mp4` + `.srt` + `_meta.json` + `_stats.json` per stream
8. Transfers files to analysis machine

## Journey 2: Recover from camera disconnect

1. Recording in progress, USB camera cable comes loose
2. Red alert banner: "USB Camera disconnected"
3. Feishu notification arrives on researcher's phone
4. Remaining cameras continue recording uninterrupted
5. Researcher reconnects USB camera
6. New `.mp4` file starts for reconnected stream, `_meta.json` records reconnect event
7. Alert clears; green status returns

## Journey 3: Data analysis

1. Data scientist receives session files (5 `.mp4` + 5 `.srt` + `_meta.json`)
2. Opens any `.mp4` in VLC or Python/OpenCV — plays natively
3. Parses `.srt` for per-frame timestamps, verifies monotonicity
4. Reads `_stats.json` for frame counts, drop rates, encoding info
5. Uses `_meta.json` for session parameters (resolution, bitrate, wall clock anchor)
6. Runs behavioral analysis pipeline on timestamped video

## Journey 4: Investigate failed recording

1. Researcher returns to find recording stopped early
2. Opens `_stats.json` — sees `alerts: [{type: "pipeline_stall", timestamp: ...}]`
3. Checks Feishu — notification arrived at stall time
4. Opens spdlog file — sees encoder buffer overflow leading to stall
5. Adjusts bitrate from 10→5Mbps for next session
6. Next session runs without issues

Source: spec.md US-001 through US-008.
