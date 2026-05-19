#!/usr/bin/env bash
# Hardware-in-the-Loop test script for jingyi-lab
# Usage: bash scripts/hil-test.sh [--usb-only] [--full]

set -euo pipefail
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "=== MiceCam v2 HIL Test ==="
echo "Machine: $(hostname)"
echo "Date: $(date)"

# ---- Docker Build ----
docker build -t micecam-hil -f docker/Dockerfile.hil "$REPO_ROOT"

# ---- Container Run with USB passthrough ----
echo ""
echo "Starting container with USB device access..."

USB_DEVICES=""
for dev in /dev/video*; do
    [ -e "$dev" ] && USB_DEVICES="$USB_DEVICES --device=$dev"
done
[ -z "$USB_DEVICES" ] && echo "WARNING: No /dev/video* devices found. USB camera test will be skipped."

docker run --rm --gpus all \
    -v /dev/bus/usb:/dev/bus/usb \
    $USB_DEVICES \
    --privileged \
    micecam-hil

echo ""
echo "=== HIL Test Complete ==="
