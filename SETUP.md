# MiceCam Setup Guide

## Quick Start (macOS)

```bash
# Install dependencies
brew install opencv googletest cmake

# Verify installation
./check_env.sh

# Build
./build.sh
```

## Manual Setup

### 1. Install OpenCV

**macOS:**
```bash
brew install opencv
export OpenCV_DIR="$(brew --prefix opencv4)"
```

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install libopencv-dev
```

**From Source (if you need specific version):**
```bash
git clone https://github.com/opencv/opencv.git
cd opencv
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install
```

### 2. Install GoogleTest

**macOS:**
```bash
brew install googletest
```

**Ubuntu/Debian:**
```bash
sudo apt-get install libgtest-dev
cd /usr/src/gtest
sudo cmake CMakeLists.txt
sudo make
sudo cp lib/*.a /usr/lib
```

**Note:** The project uses FetchContent for GoogleTest, so manual installation is optional.

## Build Without OpenCV

To build just the core components (without camera backend):

1. Comment out OpenCV-dependent files in `CMakeLists.txt`
2. Or install OpenCV as shown above

## Troubleshooting

### CMake can't find OpenCV

```bash
export OpenCV_DIR=/path/to/opencv/build
cmake ..
```

### pkg-config not working

```bash
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
```

## Disk Performance

The system requires 200+ MB/s sustained write speed. Check with:

```bash
./check_env.sh
```

Typical performance:
- SSD: 500-3000 MB/s ✓
- HDD: 100-200 MB/s (may be borderline)
- Network storage: Variable (check actual throughput)
