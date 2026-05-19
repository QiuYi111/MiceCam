#!/usr/bin/env bash
# Phase 6 Hardware Gate: Two-Source USB/AVFoundation One-Hour Recording
#
# This script documents and runs the hardware validation gate for
# MacBook Pro Camera + iPhone Continuity Camera through the FFmpeg plugin path.
#
# Prerequisites:
#   - MacBook Pro with built-in camera
#   - iPhone connected via USB (Continuity Camera enabled)
#   - MiceCam built: cmake --build build -j 4
#   - ffprobe available (optional, for MP4 validation)
#
# Usage:
#   bash scripts/hardware_gate_two_source.sh [--validate-only <dir>] [--duration <seconds>]
#
# Options:
#   --validate-only <dir>   Run artifact validation only (no recording)
#   --duration <seconds>    Recording duration (default: 3600 = 1 hour)
#   --dry-run               Print procedure without executing
#
# Expected devices (macOS AVFoundation):
#   - "MacBook Pro Camera" (built-in FaceTime HD)
#   - "iPhone" (Continuity Camera via USB)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

DURATION=3600
VALIDATE_ONLY=""
DRY_RUN=false
SESSION_DIR=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --validate-only)
            VALIDATE_ONLY="$2"
            shift 2
            ;;
        --duration)
            DURATION="$2"
            shift 2
            ;;
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            exit 2
            ;;
    esac
done

echo "=== MiceCam Phase 6 Hardware Gate ==="
echo "Machine: $(hostname)"
echo "Date: $(date)"
echo "Duration: ${DURATION}s"
echo ""

if [ -n "$VALIDATE_ONLY" ]; then
    echo "Mode: validate-only"
    echo "Session dir: $VALIDATE_ONLY"
    python3 "$SCRIPT_DIR/validate_session_artifacts.py" "$VALIDATE_ONLY" --strict
    exit $?
fi

SESSION_DIR="$PROJECT_ROOT/build/hardware_gate_session_$(date +%Y%m%d_%H%M%S)"
echo "Session output: $SESSION_DIR"
echo ""

# ---- Step 1: Verify Build ----
echo "--- Step 1: Verify Build ---"
if [ "$DRY_RUN" = true ]; then
    echo "[DRY-RUN] Would verify build exists"
else
    if [ ! -f "$PROJECT_ROOT/build/cmd/micecam/micecam" ] && [ ! -f "$PROJECT_ROOT/build/micecam" ]; then
        echo "ERROR: MiceCam executable not found. Run: cmake --build build -j 4"
        exit 1
    fi
    echo "Build verified."
fi
echo ""

# ---- Step 2: List Expected Devices ----
echo "--- Step 2: Expected Device Names ---"
echo "  Device 1: MacBook Pro Camera (AVFoundation built-in)"
echo "  Device 2: iPhone (Continuity Camera via USB)"
echo ""
echo "Verify iPhone is connected via USB and Continuity Camera is enabled"
echo "in iPhone Settings > General > AirPlay & Handoff > Continuity Camera."
echo ""

if [ "$DRY_RUN" = true ]; then
    echo "[DRY-RUN] Would enumerate AVFoundation devices"
else
    echo "Enumerating AVFoundation devices..."
    if command -v ffprobe &>/dev/null; then
        ffprobe -f avfoundation -list_devices true -i "" 2>&1 | grep -E "^\[AVFoundation" || true
    else
        echo "  (ffprobe not available for device listing)"
    fi
fi
echo ""

# ---- Step 3: Run Two-Source Recording ----
echo "--- Step 3: Two-Source Recording ---"
echo ""
echo "PROCEDURE:"
echo "  1. Launch MiceCam application"
echo "  2. Verify both cameras appear in the source grid"
echo "     - FFmpeg source group: MacBook Pro Camera"
echo "     - FFmpeg source group: iPhone Continuity Camera"
echo "  3. Select both cameras for recording"
echo "  4. Start recording"
echo "  5. Record for ${DURATION} seconds"
echo "  6. Stop recording"
echo "  7. Wait for finalization to complete"
echo "  8. Verify session output directory contains:"
echo "     - Per-stream: .mp4, .srt, _meta.json, _stats.json"
echo ""

if [ "$DRY_RUN" = true ]; then
    echo "[DRY-RUN] Would create session dir: $SESSION_DIR"
    echo "[DRY-RUN] Would run ${DURATION}s recording"
else
    mkdir -p "$SESSION_DIR"
    echo "Session directory created: $SESSION_DIR"
    echo ""
    echo "INSTRUCTIONS:"
    echo "  Run MiceCam, select both cameras, record for ${DURATION}s."
    echo "  Save session output to: $SESSION_DIR"
    echo ""
    echo "After recording completes, validate artifacts:"
    echo "  python3 $SCRIPT_DIR/validate_session_artifacts.py $SESSION_DIR --strict"
    echo ""
    echo "Or run:"
    echo "  bash $0 --validate-only $SESSION_DIR"
fi
echo ""

# ---- Step 4: Crash/Disconnect Test Procedure ----
echo "--- Step 4: Crash/Disconnect Test Procedure (Documentation) ---"
echo ""
echo "CRASH/DISCONNECT TEST:"
echo ""
echo "  Purpose: Verify plugin crash and device disconnect are handled gracefully."
echo ""
echo "  Test 1: Camera Disconnect During Recording"
echo "    1. Start recording with both cameras"
echo "    2. After 30 seconds, physically disconnect iPhone USB cable"
echo "    3. Verify:"
echo "       - MiceCam UI remains controllable"
echo "       - Affected stream shows disconnect diagnostic"
echo "       - MacBook Pro Camera stream continues recording"
echo "       - _stats.json records the disconnect event"
echo "    4. Reconnect iPhone"
echo "    5. Verify iPhone reappears in device list"
echo ""
echo "  Test 2: Force-Kill Plugin Process"
echo "    1. Start recording with both cameras"
echo "    2. After 30 seconds, identify FFmpeg plugin process:"
echo "       ps aux | grep micecam_ffmpeg"
echo "    3. Force kill: kill -9 <pid>"
echo "    4. Verify:"
echo "       - MiceCam UI remains controllable"
echo "       - Plugin shows crash diagnostic with error code"
echo "       - Recorded artifacts are finalized (not corrupted)"
echo "       - _stats.json reflects early termination"
echo "    5. Restart recording to verify plugin relaunch"
echo ""
echo "  Test 3: USB Bandwidth Saturation"
echo "    1. Start recording with both cameras at maximum resolution"
echo "    2. Monitor _stats.json for backpressure_events"
echo "    3. Verify any drops are explicit, not silent"
echo ""

echo "=== Hardware Gate Procedure Complete ==="
echo ""
echo "Next: Run the recording and validate with:"
echo "  python3 $SCRIPT_DIR/validate_session_artifacts.py <session_dir> --strict"
echo "  bash $0 --validate-only <session_dir>"
