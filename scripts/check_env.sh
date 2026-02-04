#!/bin/bash
# MiceCam Environment Check Script
# Verifies system dependencies and disk performance before running

set -e

echo "🔍 MiceCam Environment Check"
echo "=============================="
echo

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

check_passed=true

# Function to check command
check_cmd() {
    if command -v "$1" &> /dev/null; then
        echo -e "${GREEN}✓${NC} $1 found: $(command -v "$1")"
        return 0
    else
        echo -e "${RED}✗${NC} $1 not found"
        check_passed=false
        return 1
    fi
}

# Function to check library
check_lib() {
    if pkg-config --exists "$1" 2>/dev/null; then
        echo -e "${GREEN}✓${NC} $1 found: $(pkg-config --modversion "$1")"
        return 0
    else
        echo -e "${RED}✗${NC} $1 not found"
        check_passed=false
        return 1
    fi
}

echo "1. Checking Dependencies"
echo "-----------------------"

# Compiler
check_cmd cmake || true
check_cmd g++ || check_cmd clang++ || true

# Libraries
check_pkg_config() {
    if command -v pkg-config &> /dev/null; then
        check_lib "$1" || true
    else
        echo -e "${YELLOW}⚠${NC} pkg-config not found, skipping library checks"
    fi
}

check_pkg_config opencv4 || check_pkg_config opencv
check_pkg_config gtest

echo
echo "2. Disk Performance Test"
echo "------------------------"

# Check write speed
temp_file=$(mktemp)
file_size_mb=200

echo "Testing write speed ($file_size_mb MB)..."
start_time=$(date +%s.%N)

dd if=/dev/zero of="$temp_file" bs=1M count=$file_size_mb 2>&1 | grep -v copied

end_time=$(date +%s.%N)
elapsed=$(echo "$end_time - $start_time" | bc)
speed=$(echo "scale=2; $file_size_mb / $elapsed" | bc)

rm -f "$temp_file"

speed_int=$(echo "$speed" | cut -d. -f1)
if [ "$speed_int" -ge 200 ]; then
    echo -e "${GREEN}✓${NC} Disk write speed: ${speed} MB/s (meets 200 MB/s requirement)"
elif [ "$speed_int" -ge 100 ]; then
    echo -e "${YELLOW}⚠${NC} Disk write speed: ${speed} MB/s (below 200 MB/s target)"
else
    echo -e "${RED}✗${NC} Disk write speed: ${speed} MB/s (too slow for high-speed capture)"
    check_passed=false
fi

echo
echo "3. USB Camera Detection"
echo "-----------------------"

# Check for video devices
video_devices=$(ls /dev/video* 2>/dev/null | wc -l)
if [ "$video_devices" -gt 0 ]; then
    echo -e "${GREEN}✓${NC} Found $video_devices video device(s):"
    ls /dev/video* 2>/dev/null | sed 's/^/  /'
else
    echo -e "${YELLOW}⚠${NC} No USB cameras detected (optional for testing with FakeCamera)"
fi

echo
echo "4. Memory Check"
echo "---------------"

total_mem=$(free -m 2>/dev/null | awk '/^Mem:/{print $2}' || echo "N/A")
if [ "$total_mem" != "N/A" ]; then
    echo -e "${GREEN}✓${NC} Total memory: ${total_mem} MB"
    if [ "$total_mem" -lt 2048 ]; then
        echo -e "${YELLOW}⚠${NC} Low memory, may impact performance with large buffers"
    fi
else
    echo -e "${YELLOW}⚠${NC} Could not determine memory size"
fi

echo
echo "=============================="
if [ "$check_passed" = true ]; then
    echo -e "${GREEN}✓ All checks passed! Ready to build.${NC}"
    exit 0
else
    echo -e "${RED}✗ Some checks failed. Please install missing dependencies.${NC}"
    exit 1
fi
