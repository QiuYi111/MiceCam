# [RFC-001] MiceCam 核心库化架构与 GPU 零拷贝预览方案

| 项目               | 内容                                                  |
| ------------------ | ----------------------------------------------------- |
| **状态**     | **正式提案 (Proposed)**                         |
| **版本**     | **v1.1**                                        |
| **作者**     | QiuYi111 & AI Assistant                               |
| **目标硬件** | NVIDIA GTX 1080 Ti (Pascal), NVMe SSD                 |
| **关键词**   | C++20 SDK, NVDEC, OpenGL Interop, Pybind11, Zero-Copy |

## 1. 摘要 (Executive Summary)

目前的 MiceCam 作为一个独立的可执行程序，已经证明了其在高速数据采集（240MB/s+ 磁盘 I/O）上的卓越性能。为了支撑实验室未来的多样化需求（如实时闭环反馈、多相机同步、自定义 GUI），我们需要将核心逻辑重构为通用库  **`libmicecam`** 。

本方案旨在：

1. **库化 (Library-fication)** ：将采集核心与应用层解耦，提供 C++ 和 Python 双语言接口。
2. **插件化 (Pluggability)** ：建立统一的硬件抽象层 (HAL)，支持 Webcam, OAK, FLIR 等异构设备。
3. **极速预览 (High-Performance Preview)** ：利用 1080 Ti 的 **NVDEC** 引擎，实现 4K MJPEG 流的硬件解码与渲染，确保预览路径**零 CPU 拷贝**且 **不阻塞录制** 。

## 2. 总体架构设计

### 2.1 系统分层图

系统将从单一的“垂直烟囱”结构转变为“分层平台”结构：

```
graph TD
    subgraph "Application Layer (用户层)"
        GUI[Qt/ImGui 监控端]
        PyScript[Python 实验脚本]
        ScanImage[ScanImage 插件]
    end

    subgraph "Interface Layer (接口层)"
        CPP_API[C++ Public API]
        PyBind[Pybind11 Bindings]
    end

    subgraph "Core Layer (核心库 libmicecam)"
        Pipeline[Pipeline 控制器]
        Observer[Frame Observer 分发器]
        RingBuffer[RingBuffer (内存池)]
        DiskWriter[DiskWriter (异步落盘)]
    end

    subgraph "HAL (硬件抽象层)"
        Factory[Device Factory]
        Backend_FFmpeg[FFmpeg Backend]
        Backend_OAK[OAK Backend]
        Backend_FLIR[FLIR Backend (Future)]
    end

    GUI --> CPP_API
    PyScript --> PyBind
    PyBind --> CPP_API
    CPP_API --> Pipeline
    Pipeline --> HAL
    Pipeline --> RingBuffer
    Pipeline --> Observer
```

### 2.2 数据流设计 (T-Junction Model)

我们在 RingBuffer 的消费端设计了一个“T型分流”，确保录制的绝对优先级。

* **路径 A (Critical)** : RingBuffer -> DiskWriter -> NVMe SSD (Unbuffered I/O)。**[绝不允许阻塞，优先级最高]**
* **路径 B (Best Effort)** : RingBuffer -> Observer -> GPU/Python。**[允许丢帧，优先级普通]**

## 3. 详细 API 设计 (Interface Definition)

本节定义核心库对外的头文件接口，工程团队需严格遵守此契约开发。

### 3.1 基础类型 (`include/micecam/types.h`)

```
#pragma once
#include <cstdint>
#include <string>
#include <variant>
#include <map>

namespace micecam {

// 像素格式枚举
enum class PixelFormat {
    MJPEG,      // 压缩流 (Webcam Default)
    RGB24,      // 原始 RGB
    MONO8,      // 8位灰度
    MONO16,     // 16位灰度 (FLIR/Scientific)
    NV12        // YUV 4:2:0 (GPU Decode Output)
};

// 只读帧视图 (核心数据结构)
// 设计意图：轻量级传递，不涉及所有权转移，适合回调使用
struct FrameView {
    const uint8_t* data;       // 数据指针 (如 MJPEG 码流头)
    size_t size;               // 数据长度
    uint64_t sequence_id;      // 全局唯一帧号
    double timestamp;          // 硬件时间戳 (秒)
    PixelFormat format;        // 像素格式
    uint32_t width;            // 宽
    uint32_t height;           // 高
  
    // 扩展元数据 (JSON String)
    // 内容示例: {"gain": 100, "temp": 37.5, "trigger_idx": 5}
    const char* metadata_json; 
};

// 系统配置
struct SystemConfig {
    // 硬件选择
    std::string backend_name;  // "ffmpeg", "oak", "flir"
    int device_id = 0;
  
    // 采集参数
    int width = 3840;
    int height = 2160;
    double fps = 30.0;
  
    // 存储参数
    std::string session_name;
    std::string output_dir;
    size_t ring_buffer_size = 200; // 增大缓冲以应对抖动
    bool enable_disk_write = true; // 设为 false 可仅预览
  
    // 后端特有参数 (灵活扩展)
    std::map<std::string, std::string> backend_options;
};

// 统计信息
struct PipelineStats {
    uint64_t captured_frames;
    uint64_t dropped_frames;
    double drop_rate;
    double current_throughput_mbps;
    uint64_t pending_buffer_size;
};

} // namespace micecam
```

### 3.2 观察者接口 (`include/micecam/observer.h`)

```
#pragma once
#include "types.h"

namespace micecam {

class IFrameObserver {
public:
    virtual ~IFrameObserver() = default;
  
    // 核心回调：当 RingBuffer 中有新帧可用时触发
    // [Performance Warning]: 
    // 实现必须在 <1ms 内返回。耗时操作（如深度学习推理）必须
    // 在实现内部自行拷贝数据并分发到独立线程。
    virtual void on_frame(const FrameView& frame) = 0;
};

}
```

### 3.3 核心控制器 (`include/micecam/pipeline.h`)

```
#pragma once
#include <memory>
#include "types.h"
#include "observer.h"

namespace micecam {

class Pipeline {
public:
    explicit Pipeline(const SystemConfig& config);
    ~Pipeline();

    // 禁止拷贝，允许移动
    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline(Pipeline&&) noexcept;
    Pipeline& operator=(Pipeline&&) noexcept;

    // 生命周期控制
    void start();
    void stop(); // 阻塞直到所有数据落盘
  
    // 动态挂载观察者 (线程安全)
    // 允许在运行时添加/移除预览窗口或分析脚本
    void attach_observer(std::shared_ptr<IFrameObserver> observer);
    void detach_observer(std::shared_ptr<IFrameObserver> observer);

    // 状态查询
    PipelineStats get_stats() const;
    bool is_healthy() const;

private:
    class Impl; // PImpl 模式
    std::unique_ptr<Impl> impl_;
};

}
```

## 4. GPU 零拷贝预览实施细节 (GPU Zero-Copy Implementation)

 **目标** ：在 **GTX 1080 Ti** 上实现 MJPEG -> OpenGL Texture 的全 GPU 路径。

### 4.1 技术栈选型

* **CUDA Toolkit 11.x+** : 基础运行时。
* **NVIDIA Video Codec SDK 11.x** : `nvcuvid` 用于解码。
* **OpenGL** : 用于渲染纹理。

### 4.2 核心类 `GpuJpegDecoder` 伪代码

```
class GpuJpegDecoder : public IFrameObserver {
public:
    void on_frame(const FrameView& frame) override {
        // 1. 快速检查：如果 GPU 队列已满，直接丢弃，不阻塞 Pipeline
        if (gpu_queue_.is_full()) return;
      
        // 2. 异步上传 (H2D)
        // 使用 pinned memory 作为中转，或者直接 memcpy 到预分配的 GPU buffer
        void* gpu_ptr = gpu_queue_.next_free_slot();
        cudaMemcpyAsync(gpu_ptr, frame.data, frame.size, cudaMemcpyHostToDevice, stream_);
      
        // 3. 提交解码任务
        // cuvidDecodePicture 会读取 GPU 上的 MJPEG 数据并解码到显存 Surface
        CUVIDPICPARAMS params = {0};
        params.pBitstreamData = gpu_ptr;
        params.nBitstreamDataLen = frame.size;
        cuvidDecodePicture(decoder_handle_, &params);
      
        // 4. 映射到 OpenGL (Interop)
        // 将解码后的 NV12 Surface 映射为 OpenGL Texture
        map_surface_to_texture();
    }
};
```

### 4.3 资源映射流程 (Interop Flow)

1. **初始化** :

* 创建 OpenGL Context。
* `cuGLCtxCreate()` 绑定 CUDA 到该 GL 上下文。
* `glGenTextures` 创建目标纹理。
* `cudaGraphicsGLRegisterImage` 注册该纹理资源。

1. **每帧循环** :

* `cudaGraphicsMapResources()`: 锁定纹理，允许 CUDA 写入。
* `cudaGraphicsSubResourceGetMappedArray()`: 获取映射的 CUDA 数组。
* **Kernel Launch** : 启动 CUDA Kernel (`NV12_to_RGBA`)，将解码后的 YUV 数据写入映射的数组。
* `cudaGraphicsUnmapResources()`: 解锁纹理，交还给 OpenGL。
* **Render** : ImGui / Qt 绘制该 Texture ID。

## 5. Python 绑定实施细节 (Python Bindings)

 **目标** ：支持 `numpy` 直接读取内存，无额外开销。

### 5.1 Pybind11 模块定义

```
// bindings/python/module.cpp
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "micecam/pipeline.h"

namespace py = pybind11;

// Python 适配层观察者
class PyObserverTrampoline : public micecam::IFrameObserver {
public:
    using Callback = std::function<void(py::array_t<uint8_t>, uint64_t)>;
  
    explicit PyObserverTrampoline(Callback cb) : cb_(cb) {}

    void on_frame(const micecam::FrameView& frame) override {
        // 获取 GIL
        py::gil_scoped_acquire acquire;
      
        // 创建 numpy array view (零拷贝)
        // 注意：这个 array 在回调结束后失效，如果 Python 端需要保留，必须 copy()
        auto array = py::array_t<uint8_t>(
            { (py::ssize_t)frame.size },  // shape
            { sizeof(uint8_t) },          // strides
            frame.data,                   // data pointer
            py::capsule(frame.data, [](void*){}) // dummy deleter (we don't own data)
        );
      
        cb_(array, frame.sequence_id);
    }
private:
    Callback cb_;
};
```

## 6. 风险评估与应对 (Risk Management)

| 风险点                   | 影响         | 应对策略                                                                   |
| ------------------------ | ------------ | -------------------------------------------------------------------------- |
| **GPU 上下文冲突** | 进程崩溃     | 严格限制 GPU 操作在单一渲染线程；使用 `cuCtxPushCurrent`管理栈。         |
| **PCIe 带宽竞争**  | SSD 写入降速 | 4K MJPEG 仅 ~15MB/s，远低于 PCIe x16 带宽；使用 `cudaStream`异步传输。   |
| **Python GC 停顿** | 预览卡顿     | 回调中仅做数据拷贝，复杂逻辑放到 Python 侧的独立 `multiprocessing`进程。 |
| **驱动版本不兼容** | 初始化失败   | 运行时动态加载 `nvcuvid.dll`，版本不匹配时自动回退到 OpenCV 软解。       |

## 7. 实施路线图 (Detailed Roadmap)

### Phase 1: 核心库解耦 (Week 1)

* [ ] 建立 CMake `micecam_core` 目标。
* [ ] 迁移 `DiskWriter` 和 `RingBuffer` 到 `src/internal`。
* [ ] 实现 `SystemConfig` 解析器。
* [ ] **验收标准** : `tests/pipeline_test` 编译通过并运行成功。

### Phase 2: Python 绑定与观察者 (Week 2)

* [ ] 实现 `IFrameObserver` 分发逻辑。
* [ ] 完成 Pybind11 模块编译。
* [ ] **验收标准** : Python 脚本能启动录制，并打印出实时帧率。

### Phase 3: GPU 预览集成 (Week 3-4)

* [ ] 编写 CUDA `NV12_to_RGBA` kernel。
* [ ] 实现 `GpuJpegDecoder`。
* [ ] 集成 ImGui 编写 Demo。
* [ ] **验收标准** : 在 1080 Ti 上实现 4K @ 30fps 预览，CPU 占用 < 10%。
