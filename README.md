# MiceCam: 实验室行为学数据采集系统

**MiceCam** 是一款专为动物行为学研究设计的高性能、低延迟多相机数据采集系统。它旨在解决传统采集方案中常见的数据丢帧、同步误差大和长时间录制不稳定等问题，为神经科学与行为学实验提供可靠的数据基础。

本系统基于 C++20 核心开发，采用 RingBuffer 非阻塞架构与零拷贝技术，确保在消费级硬件上也能实现高带宽、高帧率的无损采集。支持 DepthAI (OAK) 硬件同步与标准 USB 相机，配备完整的 Python 数据处理工具链。

---

## 核心特性 (Key Features)

### 1. 模块化与可插拔后端 (Pluggable Camera Backend)
- **灵活架构**: 核心采集引擎通过 `ICameraBackend` 接口与具体硬件解耦，支持无需修改核心代码即可接入新型相机。
- **多后端支持**: 内置原生 FFmpeg（极致性能）、OpenCV（广泛兼容）、DepthAI（OAK 硬件同步）及模拟后端。
- **科研定制**: 允许研究人员针对特定硬件快速开发驱动插件，适应多变的实验需求。

### 2. 高吞吐与稳定性
- **非阻塞 I/O 架构**: 采用生产者-消费者模型，采集线程与写盘线程完全解耦。
- **高性能 RingBuffer**: 实测内存吞吐量超过 **300 MB/s**，支持 4K@30fps 或高帧率 960p@120fps 持续录制。
- **零拷贝设计**: 采用指针传递与内存复用策略，最小化 CPU 占用与内存拷贝开销。
- **Windows 磁盘优化**: 利用 `VirtualAlloc` 与 `FILE_FLAG_NO_BUFFERING` 技术，实现 **240+ MB/s** 的稳定磁盘写入速度。

### 3. 精确硬件同步
- **多摄同步**: 支持 OAK-4P 等多摄模组，实现四路相机硬件级同步采集，帧间抖动 **< 12us**。
- **时间戳对齐**: 每一帧数据均包含精确的硬件时间戳，确保多视角数据严格对齐。

### 4. 数据完整性与安全性
- **数据恢复机制**: 针对实验中可能出现的断电或意外中断，提供 `recover_index.py` 工具，可从原始二进制流中完整重建索引与元数据。
- **CRC32 校验**: (可选) 写入数据时进行实时校验，保障数据存储的比特级正确性。
- **崩溃保护**: 独立进程架构设计（Supervisor 模式），主进程崩溃不影响数据落盘。

---

## 快速开始 (Quick Start)

### Windows 用户 (推荐)

对于大多数 Windows 用户，建议直接使用预编译的安装包：

1. 进入 `release/` 目录或项目 Release 页面下载 `MiceCam.msi`。
2. 运行安装程序完成安装。
3. 从桌面快捷方式启动 MiceCam。

### 源码编译 (Build from Source)

构建需要 C++20 编译器 (MSVC/Clang/GCC) 与 CMake 3.20+。

```powershell
# 1. 环境检查
.\scripts\check_env.ps1

# 2. 编译项目 (自动生成 .exe)
.\scripts\build_exe.ps1

# 3. 运行程序
.\dist\MiceCam_Release\MiceCam.exe
```

---

## 数据处理 (Data Processing)

MiceCam 采用自定义的高效二进制格式 (`.bin`) 存储原始数据，以最大化写入性能。我们提供了完整的 Python 工具链用于数据提取和转换。

### 环境配置

推荐使用 [uv](https://github.com/astral-sh/uv) 管理 Python 环境：

```bash
# 初始化依赖
uv sync
```

### 常用工具

所有工具位于 `tools/` 目录下：

1.  **导出图片 (Bin to Images)**
    将录制数据无损导出为图片序列。对于 MJPEG 格式录像，此过程为零编码损耗直接提取。
    ```bash
    uv run python tools/bin_to_images.py <session_file>.bin [output_dir]
    ```

2.  **查看元数据 (Read Metadata)**
    快速查看录制会话的参数、帧数与时长。
    ```bash
    uv run python tools/read_bin.py <session_file>.bin
    ```

3.  **数据修复 (Recover Index)**
    如果您意外终止了录制（如强制关闭窗口），导致 `.json` 索引文件缺失，使用此工具扫描 `.bin` 文件重建索引。
    ```bash
    uv run python tools/recover_index.py <corrupted_session>.bin
    ```

---

## 系统目录结构

```text
MiceCam/
├── app/                # Python 应用程序逻辑 (GUI, Server)
├── src/                # C++ 核心采集引擎
├── include/            # C++ 头文件
├── tools/              # 数据处理与分析脚本
├── scripts/            # 自动化构建与部署脚本
├── release/            # 安装包产物 (.msi)
├── docs/               # 详细技术文档
└── tests/              # 单元测试与压力测试代码
```

## 技术支持

- **用户指南**: 详见 [docs/USER_GUIDE.md](docs/USER_GUIDE.md)
- **开发文档**: 详见 [docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md)
- **问题反馈**: 如遇 Bug 或有功能建议，请提交 GitHub Issue。

---

**MiceCam** - Reliable Acquisition for Behavioral Science.
