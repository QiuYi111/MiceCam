# MiceCam Setup Guide

## Quick Start (Windows)

### 方法一：使用 Visual Studio（推荐）

1. **安装 Visual Studio 2019 或更高版本**
   - 勾选 "Desktop development with C++" 工作负载
   - CMake 已包含

2. **安装 vcpkg（用于 OpenCV）**
```powershell
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install opencv4:x64-windows
.\vcpkg integrate install
```

3. **构建项目**
```powershell
# 在 Developer PowerShell for VS 中运行
.\check_env.ps1      # 检查环境
.\build.ps1          # 一键构建
```

### 方法二：使用 MSYS2/MinGW

```powershell
# 在 MSYS2 MinGW64 终端中
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc mingw-w64-x86_64-opencv

mkdir build && cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

---

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
