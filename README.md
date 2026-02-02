# MiceCam

> 实验室高速相机数据采集系统 | High-speed camera data acquisition for laboratory use

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Version: 0.1.0](https://img.shields.io/badge/version-0.1.0-brightgreen.svg)](CHANGELOG.md)
[![Tests: 29/29](https://img.shields.io/badge/tests-29%2F29-brightgreen.svg)](README.md#测试)

**版本**: v0.1.0 | **状态**: 完整 MVP ✅ | **测试**: 29/29 passing (100%)

---

## 特性

- ✅ **高速采集**: 302.5 MB/s RingBuffer 吞吐量, **241.0 MB/s** 真实磁盘 I/O (Windows Unbuffered)
- ✅ **硬件加速**: **Native FFmpeg (libavdevice)** 直取 MJPEG 码流，20-30x 带宽节省
- ✅ **非阻塞架构**: RingBuffer 解耦采集与写盘
- ✅ **零拷贝传递**: `std::unique_ptr` 所有权转移
- ✅ **数据完整性**: CRC32 校验和
- ✅ **模块化**: 可插拔相机后端 (OpenCV / FFmpeg / OAK / Fake)
- ✅ **OAK 支持**: 原生支持 Luxonis OAK 系列相机 (depthai-core)
- ✅ **Windows 优化**: 使用 `VirtualAlloc` 与 `FILE_FLAG_NO_BUFFERING` 消除 I/O 抖动

---

## 快速开始

### Linux/macOS 快速开始

```bash
# 1. 检查环境
./check_env.sh

# 2. 一键构建
./build.sh

# 3. 运行测试
./build/micecam_tests

# 4. 运行演示
./build/micecam_demo

# 5. 处理数据（可选）
uv run python tools/read_bin.py test_output/session_001
```

---

## Python 工具

由于 MiceCam 采用高效的自定义 `.bin` 格式，我们提供了 Python 工具进行数据处理。Python 环境通过 [uv](https://github.com/astral-sh/uv) 进行管理。

### 环境准备

1. **安装 uv**: 见 [uv 安装指南](https://docs.astral.sh/uv/getting-started/installation/)。
2. **初始化环境**:
   ```bash
   uv sync
   ```

### 图片提取 (Extract Frames)

将 `.bin` 文件中的每一帧提取并保存为图片。如果是 MJPEG 采集，该脚本会**零编码开销**直接保存为 `.jpg`，速度极快。

```powershell
# 提取到同名文件夹
uv run python tools/bin_to_images.py <session_path>.bin

# 指定输出目录
uv run python tools/bin_to_images.py <session_path>.bin <output_dir>
```

### 数据读取 (Read Metadata)

读取并显示会话的元数据摘要：

```bash
uv run python tools/read_bin.py <session_path>
```

### Windows 快速开始

```powershell
# 1. 检查环境（在 Developer PowerShell 中运行）
.\check_env.ps1

# 2. 一键构建
.\build.ps1

# 3. 运行测试
.\build\Debug\micecam_tests.exe

# 4. 运行演示
.\build\Debug\micecam_demo.exe

# 5. Release 构建
.\build.ps1 -Release
```

### 代码示例

```cpp
#include "micecam/pipeline/ingestion_pipeline.h"
#include "camera/fake_camera.h"

// 创建相机
auto camera = std::make_unique<FakeCamera>(320 * 240 * 3);
CameraConfig config{.width = 320, .height = 240, .fps = 30.0};
camera->initialize(config);

// 配置会话
SessionConfig session_config;
session_config.output_dir = "./output";
session_config.session_name = "test_001";
session_config.ring_buffer_size = 50;
session_config.enable_checksums = true;

// 启动采集
IngestionPipeline pipeline(std::move(camera), session_config);
pipeline.start();
pipeline.join();
pipeline.stop();

// 查看结果
std::cout << "Drop rate: " << pipeline.get_drop_rate() * 100 << "%\n";
```

**输出文件**:
```
output/
├── test_001.bin              # 原始帧数据
└── test_001_metadata.json    # 元数据（时间戳、校验码）
```

---

## 文档

| 文档 | 说明 |
|------|------|
| [USER_GUIDE.md](USER_GUIDE.md) | 用户指南（使用、配置、FAQ） |
| [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md) | 开发者指南（架构、扩展、测试） |
| [SETUP.md](SETUP.md) | 安装指南（依赖、构建） |
| [PROJECT_SUMMARY.md](PROJECT_SUMMARY.md) | 项目总结（设计、性能、下一步） |
| [tools/README.md](tools/README.md) | Python 工具文档 |

---

## 架构

### 数据流

```
CameraBackend → RingBuffer → DiskWriter → [.bin + _metadata.json]
     ↑              ↓              ↓
  Non-blocking  Zero-copy      Async I/O
```

### 核心组件

- **Frame**: 所有权转移的帧容器
- **RingBuffer**: 线程安全环形缓冲区（可配置大小）
- **ICameraBackend**: 相机驱动接口
- **DiskWriter**: 异步磁盘写入器
- **IngestionPipeline**: 流水线编排器

### 三阶段流水线

| 阶段 | 状态 | 说明 |
|------|------|------|
| Stage 1: 高速采集 | ✅ 完成 | .bin + JSON 元数据 |
| Stage 2: HDF5 转换 | 🚧 设计 | 接口定义，按需实现 |
| Stage 3: 会话管理 | 🚧 未开始 | 文件系统即管理 |

---

## 性能

| 指标 | 目标 | 实际 | 状态 |
|------|------|------|------|
| RingBuffer 吞吐量 | 200+ MB/s | 302.5 MB/s | ✅ |
| 真实磁盘 I/O (Windows) | 150+ MB/s | **241.7 MB/s** | ✅ |
| 4K @ 30fps 录制 | 0 丢帧 | **0 丢帧 (11.8 MB/s)** | ✅ |
| 960p @ 120fps 录制 | 0 丢帧 | **0 丢帧 (8.2 MB/s)** | ✅ |
| 内存占用 | 低 | ~128MB (Aligned Buffer) | ✅ |

---

## 支持的相机

| 后端 | 状态 | 说明 |
|------|------|------|
| **FFmpeg (Native)** | ✅ **核心** | **推荐**。直抓 MJPEG 码流，性能无敌，支持 4K/120fps |
| **OAK (DepthAI)** | ✅ 完成 | **新增**。原生支持 OAK 相机，支持高分辨率与高效转换 |
| USB Camera (OpenCV) | ✅ 完成 | 兼容性后端，适合简单测试 |
| FakeCamera | ✅ 完成 | 纯软件模拟，用于 CI/CD 和压力测试 |

**添加新相机**: 见 [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md)

---

## 开发

### 构建选项

```bash
# 标准构建
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -- -j$(nproc)

# 禁用测试
cmake -B build -DBUILD_TESTS=OFF

# 启用 USB Camera (需 OpenCV)
cmake -B build -DWITH_CAMERA_BACKEND=ON
```

### 测试

```bash
# 运行所有测试
./build/micecam_tests

# 运行特定测试
./build/micecam_tests --gtest_filter="*Stress*"

# 详细输出
./build/micecam_tests --gtest_color=yes --verbose
```

### 代码质量

```bash
# 格式化
clang-format -i include/**/*.h src/**/*.cpp

# 静态分析
clang-tidy include/**/*.h src/**/*.cpp

# 内存检查
valgrind --leak-check=full ./build/micecam_tests
```

---

## 依赖

### 必需

- **C++20** 编译器 (Clang 12+, GCC 10+, MSVC 2019+)
- **CMake** 3.20+
- **Threads** (标准库)

### 可选

- **FFmpeg** 6.0+ (`libavdevice`, `libavformat`, `libavcodec`) - **必需 (用于高性能采集)**
- **OpenCV** 4.x - 可选 (辅助后端)
- **vcpkg** - 推荐的 Windows 依赖管理工具
- **GoogleTest** - 自动下载（FetchContent）

### 推荐工具

- **clang-format** - 代码格式化
- **clang-tidy** - 静态分析
- **tmux** - 终端复用（多窗口监控）

---

## 系统要求

- **操作系统**: Windows 10/11, macOS 10.15+, Ubuntu 20.04+
- **内存**: 建议 4GB+（取决于缓冲区大小）
- **磁盘**: 建议 SSD（200+ MB/s 持续写入）

---

## 故障排除

### 问题：编译失败

```bash
# 清理后重新构建
rm -rf build
./build.sh
```

### 问题：找不到相机

```bash
# 检查设备
ls -la /dev/video*

# macOS
system_profiler SPCameraDataType
```

### 问题：丢帧率高

```cpp
// 增大缓冲区
session_config.ring_buffer_size = 100;

// 或使用更快的磁盘（SSD）
```

更多问题见 [USER_GUIDE.md](USER_GUIDE.md)

---

## 项目状态

### ✅ 已完成

- Stage 1 高速采集
- 非阻塞架构
- 零拷贝传递
- 数据完整性（CRC32）
- 性能监控（丢帧率）
- TDD 测试套件
- 完整文档

### 🚧 进行中

- Stage 2/3（设计完成，按需实现）
- USB Camera 真实硬件验证

### 🎯 MVP 目标

- ✅ 模块化后端架构
- ✅ Stage 1 完整实现
- ⚠️  USB Camera（需硬件验证）

---

## 设计哲学

> **"好品味"** - 数据结构优先，消除特殊情况，实用主义

- **数据结构 > 算法**: Frame + RingBuffer 让代码简洁
- **零拷贝**: 所有权转移避免大尺寸图像拷贝
- **非阻塞**: 相机不等待磁盘，丢帧优于延迟
- **TDD**: 测试先行，性能目标驱动

---

## 许可证

学术实验室使用。

---

## 联系方式

- **文档**: 见 [PROJECT_SUMMARY.md](PROJECT_SUMMARY.md)
- **问题**: 创建 GitHub Issue
- **贡献**: 见 [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md)

---

**MiceCam** - 为科研而生 📷🔬
