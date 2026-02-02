# MiceCam 用户指南

## 快速开始

MiceCam 是一个实验室高速相机数据采集系统。本指南将帮助你快速上手。

### 系统要求

- **操作系统**: macOS 10.15+, Ubuntu 20.04+, 或类似 Linux
- **编译器**: C++20 支持 (Clang 12+, GCC 10+)
- **CMake**: 3.20+
- **磁盘速度**: 建议 200+ MB/s 持续写入

### 5 分钟快速入门

```bash
# 1. 检查环境
./check_env.sh

# 2. 构建项目
./build.sh

# 3. 运行测试
./build/micecam_tests

# 4. 运行演示
./build/micecam_demo
```

---

## 基础使用

### 1. 使用 FakeCamera 测试

适合开发和测试场景（不需要硬件）。

```cpp
#include "micecam/pipeline/ingestion_pipeline.h"
#include "camera/fake_camera.h"
#include <iostream>

int main() {
    // 创建 FakeCamera（320x240 RGB，30 FPS）
    const size_t frame_size = 320 * 240 * 3;
    auto camera = std::make_unique<FakeCamera>(frame_size);

    // 配置相机
    CameraConfig config;
    config.width = 320;
    config.height = 240;
    config.fps = 30.0;

    camera->initialize(config);
    camera->set_max_frames(100);  // 采集 100 帧

    // 配置会话
    SessionConfig session_config;
    session_config.output_dir = "./output";
    session_config.session_name = "test_001";
    session_config.ring_buffer_size = 50;  // 缓冲区大小
    session_config.enable_checksums = true;  // 启用校验和
    session_config.camera_backend_name = "FakeCamera";
    session_config.width = 320;
    session_config.height = 240;
    session_config.fps = 30.0;

    // 创建并启动流水线
    IngestionPipeline pipeline(std::move(camera), session_config);
    pipeline.start();

    // 等待采集完成
    pipeline.join();
    pipeline.stop();

    // 检查结果
    std::cout << "Frames captured: " << pipeline.get_frames_captured() << "\n";
    std::cout << "Frames dropped: " << pipeline.get_frames_dropped() << "\n";
    std::cout << "Drop rate: " << (pipeline.get_drop_rate() * 100) << "%\n";

    return 0;
}
```

**输出文件：**
```
output/
├── test_001.bin              # 原始帧数据
└── test_001_metadata.json    # 元数据（时间戳、校验码）
```

### 2. 使用 USB Camera

需要安装 OpenCV 和连接摄像头。

#### 安装 OpenCV

**macOS:**
```bash
brew install opencv
```

**Ubuntu:**
```bash
sudo apt-get update
sudo apt-get install libopencv-dev
```

#### 使用代码

```cpp
#include "micecam/pipeline/ingestion_pipeline.h"
#include "micecam/camera/usb_camera_backend.h"

int main() {
    // 创建 USB Camera
    auto camera = std::make_unique<USBCameraBackend>();

    // 配置相机
    CameraConfig config;
    config.width = 1920;
    config.height = 1080;
    config.fps = 60.0;
    config.device_id = 0;  // /dev/video0

    if (!camera->initialize(config)) {
        std::cerr << "Failed to initialize camera\n";
        return 1;
    }

    // 配置会话
    SessionConfig session_config;
    session_config.output_dir = "/data/experiments";
    session_config.session_name = "experiment_001";
    session_config.ring_buffer_size = 100;  // 高速相机需要更大缓冲区
    session_config.enable_checksums = true;
    session_config.camera_backend_name = "USBCameraBackend";
    session_config.width = 1920;
    session_config.height = 1080;
    session_config.fps = 60.0;

    // 启动流水线
    IngestionPipeline pipeline(std::move(camera), session_config);

    if (!pipeline.start()) {
        std::cerr << "Failed to start pipeline\n";
        return 1;
    }

    // 采集 10 秒
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // 停止
    pipeline.stop();

    return 0;
}
```

---

## 配置参数说明

### SessionConfig

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `output_dir` | string | "." | 输出目录 |
| `session_name` | string | "session" | 会话名称（文件名前缀） |
| `enable_checksums` | bool | true | 是否计算 CRC32 校验和 |
| `ring_buffer_size` | size_t | 10 | 环形缓冲区大小（帧数） |
| `camera_backend_name` | string | "unknown" | 相机后端名称 |
| `width` | int | 0 | 图像宽度 |
| `height` | int | 0 | 图像高度 |
| `fps` | double | 0.0 | 帧率 |

### CameraConfig

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `width` | int | 640 | 图像宽度 |
| `height` | int | 480 | 图像高度 |
| `fps` | double | 30.0 | 帧率 |
| `device_id` | int | 0 | 相机设备 ID |

---

## 性能调优

### 1. RingBuffer 大小

**问题**: 丢帧率高（>10%）

**原因**: 磁盘写入速度跟不上相机采集速度

**解决方案**: 增大 `ring_buffer_size`

```cpp
// 小缓冲区（适合慢速相机）
session_config.ring_buffer_size = 10;

// 大缓冲区（适合高速相机）
session_config.ring_buffer_size = 100;

// 超大缓冲区（磁盘很慢时）
session_config.ring_buffer_size = 500;
```

**权衡**:
- ✅ 更大缓冲区 = 更少丢帧
- ❌ 更大缓冲区 = 更多内存占用

### 2. 校验和计算

**问题**: CPU 使用率高

**解决方案**: 禁用校验和

```cpp
session_config.enable_checksums = false;
```

**注意**: 仅在数据完整性不是关键因素时禁用。

### 3. 磁盘选择

**推荐**:
- ✅ SSD（推荐）
- ✅ NVMe SSD（最佳）
- ❌ 机械硬盘（太慢）

**检查磁盘速度**:
```bash
./check_env.sh
```

---

## 输出文件格式

### 二进制文件 (.bin)

连续的原始帧数据：

```
[Frame 1][Frame 2][Frame 3]...[Frame N]
```

每帧格式：
```
[原始图像数据 - width × height × channels 字节]
```

### 元数据文件 (_metadata.json)

JSON 格式，包含会话信息和帧级元数据：

```json
{
  "session": {
    "session_name": "test_001",
    "camera_backend": "FakeCamera",
    "width": 640,
    "height": 480,
    "fps": 30.0,
    "start_timestamp_ns": 1234567890123456789,
    "end_timestamp_ns": 1234567891234567890,
    "total_frames": 100,
    "total_bytes": 92160000,
    "session_checksum": 1234567890
  },
  "frames": [
    {
      "sequence_id": 1,
      "timestamp_ns": 1234567890234567890,
      "offset": 0,
      "size": 921600,
      "checksum": 987654321
    },
    ...
  ]
}
```

**字段说明**:
- `sequence_id`: 帧序号（从 1 开始）
- `timestamp_ns`: 时间戳（纳秒，Unix epoch）
- `offset`: 在 .bin 文件中的字节偏移
- `size`: 帧大小（字节）
- `checksum`: CRC32 校验和

---

## 读取采集的数据

### Python 示例

```python
import json
import numpy as np

# 读取元数据
with open('output/test_001_metadata.json', 'r') as f:
    metadata = json.load(f)

width = metadata['session']['width']
height = metadata['session']['height']
total_frames = metadata['session']['total_frames']

# 读取二进制数据
with open('output/test_001.bin', 'rb') as f:
    # 读取第 1 帧
    frame_1_meta = metadata['frames'][0]
    f.seek(frame_1_meta['offset'])
    frame_1_data = f.read(frame_1_meta['size'])

    # 转换为 numpy array (RGB)
    frame_1 = np.frombuffer(frame_1_data, dtype=np.uint8)
    frame_1 = frame_1.reshape(height, width, 3)

    print(f"Frame 1 shape: {frame_1.shape}")
    print(f"Frame 1 timestamp: {frame_1_meta['timestamp_ns']}")
```

---

## 常见问题

### Q1: 采集时丢帧率高

**症状**: `Drop rate: 30%+`

**原因**: 磁盘写入速度慢

**解决方案**:
1. 增大 `ring_buffer_size`
2. 使用更快的磁盘（SSD）
3. 降低相机分辨率或帧率

### Q2: 找不到相机

**症状**: `Failed to initialize camera`

**检查**:
```bash
# 列出可用相机
ls -la /dev/video*

# macOS
system_profiler SPCameraDataType
```

### Q3: 编译错误

**症状**: `OpenCV not found`

**解决**:
```bash
# macOS
brew install opencv

# Ubuntu
sudo apt-get install libopencv-dev
```

### Q4: 内存占用高

**原因**: `ring_buffer_size` 太大

**解决**:
```cpp
session_config.ring_buffer_size = 10;  // 减小缓冲区
```

---

## 下一步

### 分析数据

使用你喜欢的工具：
- Python + NumPy + OpenCV
- MATLAB
- Julia
- C++ 读取器

### 转换格式

当前输出 `.bin + JSON`，可转换为：
- HDF5（需要实现 Stage 2）
- Video (MP4, AVI)
- Image sequence (PNG, TIFF)

### 会话管理

当前输出文件命名：
```
{session_name}.bin
{session_name}_metadata.json
```

建议：
- 使用时间戳作为 session_name
- 按实验组织目录结构
- 使用脚本管理旧数据

---

## 技术支持

- **文档**: `README.md`, `SETUP.md`, `PROJECT_SUMMARY.md`
- **示例**: `demo_simple.cpp`
- **测试**: `./build/micecam_tests`

**问题反馈**: 请创建 Issue 并附上：
1. 系统信息（OS、编译器版本）
2. 错误日志
3. 最小复现示例

---

**祝采集顺利！** 📷
