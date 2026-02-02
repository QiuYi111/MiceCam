# MiceCam 开发者指南

## 架构概览

MiceCam 采用模块化、可扩展的架构设计。

### 核心设计原则

1. **数据结构优先** - 好的数据结构让算法简单
2. **零拷贝** - 使用 `std::unique_ptr` 转移所有权
3. **非阻塞** - 采集线程不等待 I/O
4. **模块化** - 相机后端可插拔
5. **TDD** - 测试先行

### 项目结构

```
include/micecam/
├── core/           # 核心数据结构
│   ├── frame.h
│   └── ring_buffer.h
├── camera/         # 相机后端接口
│   ├── camera_backend.h
│   └── usb_camera_backend.h
└── pipeline/       # 流水线组件
    ├── disk_writer.h
    ├── ingestion_pipeline.h
    └── hdf5_converter.h
```

---

## 添加新的相机后端

### 1. 实现接口

```cpp
// include/micecam/camera/my_camera_backend.h
#pragma once

#include "micecam/camera/camera_backend.h"

namespace micecam {

class MyCameraBackend : public ICameraBackend {
public:
    MyCameraBackend() = default;
    ~MyCameraBackend() override = default;

    bool initialize(const CameraConfig& config) override {
        // 初始化相机
        // config.width, config.height, config.fps
        return true;
    }

    bool start() override {
        // 开始采集
        running_.store(true);
        return true;
    }

    void stop() override {
        // 停止采集
        running_.store(false);
    }

    std::unique_ptr<Frame> get_frame() override {
        if (!running_.load()) {
            return nullptr;
        }

        // 从相机获取帧
        auto data = std::make_unique<std::vector<uint8_t>>(frame_size);

        // TODO: 实际的相机 API 调用
        // fill data with frame bytes...

        // 创建 Frame（时间戳自动打标）
        auto frame = std::make_unique<Frame>(
            frame_count_.fetch_add(1) + 1,
            std::move(data)
        );

        return frame;
    }

    uint64_t get_frame_count() const override {
        return frame_count_.load();
    }

    bool is_running() const override {
        return running_.load();
    }

    std::string get_backend_name() const override {
        return "MyCameraBackend";
    }

private:
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> frame_count_{0};
    size_t frame_size_;
};

}  // namespace micecam
```

### 2. 集成到 CMake

```cmake
# CMakeLists.txt
if(WITH_MY_CAMERA)
    add_library(micecam_my_camera
        src/camera/my_camera_backend.cpp
    )

    target_link_libraries(micecam_my_camera PUBLIC
        micecam_core
        ${MY_CAMERA_LIBS}
    )
endif()
```

### 3. 使用

```cpp
#include "micecam/camera/my_camera_backend.h"

auto camera = std::make_unique<MyCameraBackend>();
CameraConfig config{.width = 1920, .height = 1080, .fps = 60.0};
camera->initialize(config);

SessionConfig session_config;
session_config.camera_backend_name = "MyCameraBackend";
// ...

IngestionPipeline pipeline(std::move(camera), session_config);
pipeline.start();
```

### 4. 测试

```cpp
// tests/camera/my_camera_test.cpp
#include "micecam/camera/my_camera_backend.h"
#include <gtest/gtest.h>

TEST(MyCameraTest, Initialization) {
    MyCameraBackend camera;
    CameraConfig config;
    ASSERT_TRUE(camera.initialize(config));
}

TEST(MyCameraTest, FrameGeneration) {
    MyCameraBackend camera;
    CameraConfig config;
    camera.initialize(config);
    camera.start();

    auto frame = camera.get_frame();
    ASSERT_NE(frame, nullptr);
    EXPECT_GT(frame->size(), 0);

    camera.stop();
}
```

---

## 核心组件详解

### Frame（帧）

**所有权转移**：
```cpp
// 创建
auto frame = std::make_unique<Frame>(seq_id, std::move(data));

// 转移到 RingBuffer
buffer.push(std::move(frame));  // frame 现在为 nullptr

// 从 RingBuffer 取出
auto frame2 = buffer.pop();  // 获得所有权
```

**为什么用 unique_ptr？**
- ✅ 零拷贝 - 不复制图像数据
- ✅ 所有权清晰 - 谁拥有，谁负责
- ✅ 线程安全 - 转移后原指针失效

### RingBuffer（环形缓冲区）

**生产者-消费者模式**：
```cpp
// 生产者（相机线程）
while (running) {
    auto frame = camera->get_frame();
    if (!buffer.try_push(std::move(frame))) {
        // 缓冲区满，丢弃帧
        frames_dropped++;
    }
}

// 消费者（磁盘写入线程）
while (running) {
    auto frame = buffer.pop();  // 阻塞等待
    write_to_disk(std::move(frame));
}
```

**为什么用环形缓冲区？**
- ✅ 固定内存占用
- ✅ 无需动态分配
- ✅ 缓存友好

### DiskWriter（磁盘写入器）

**异步写入**：
```cpp
// 启动写入线程
writer_.consume_from(buffer_);  // 在独立线程运行

// 主线程继续
// 写入在后台进行
```

**原子写入保证**：
- 帧数据先写入
- 元数据在 finalize() 时写入
- OS 保证文件系统的原子性

---

## 测试策略

### 1. 单元测试

测试单个组件：

```cpp
TEST(FrameTest, MoveSemantics) {
    auto data = std::make_unique<std::vector<uint8_t>>(100);
    Frame f1(1, std::move(data));

    Frame f2 = std::move(f1);  // 转移
    EXPECT_EQ(f2.sequence_id, 1);
    EXPECT_FALSE(f1.data);  // f1 已失效
}
```

### 2. 集成测试

测试组件交互：

```cpp
TEST(PipelineTest, EndToEnd) {
    auto camera = std::make_unique<FakeCamera>(frame_size);
    // ... setup ...

    IngestionPipeline pipeline(std::move(camera), config);
    pipeline.start();
    pipeline.join();
    pipeline.stop();

    // 验证输出文件
    EXPECT_TRUE(file_exists("output.bin"));
    EXPECT_TRUE(file_exists("output_metadata.json"));
}
```

### 3. 压力测试

测试性能边界：

```cpp
TEST(StressTest, HighThroughput) {
    // 模拟 200 MB/s
    const size_t frame_size = 200 * 1024 * 1024 / 60;  // 200 MB/s @ 60fps

    auto camera = std::make_unique<FakeCamera>(frame_size);
    // ... run test ...

    EXPECT_GT(bandwidth, 200.0);  // MB/s
}
```

### 4. Mock 注入

使用 FakeCamera：

```cpp
// 生产代码使用真实相机
auto camera = std::make_unique<USBCameraBackend>();

// 测试代码使用 FakeCamera
auto camera = std::make_unique<FakeCamera>(frame_size);
camera->set_max_frames(100);  // 可预测的行为
```

---

## 性能分析

### 1. 识别瓶颈

**使用 profiler**：
```bash
# macOS
Instruments - Time Profiler ./build/micecam_tests

# Linux
perf record ./build/micecam_tests
perf report
```

**常见瓶颈**：
- CRC32 计算 → 考虑查表法优化
- 内存分配 → 使用内存池
- 磁盘 I/O → 更快的磁盘

### 2. 内存分析

**检查泄漏**：
```bash
# macOS
leaks -atExit -- ./build/micecam_tests

# Linux
valgrind --leak-check=full ./build/micecam_tests
```

**当前状态**：
- ✅ 无内存泄漏（RAII）
- ✅ 无数据竞争（mutex + atomic）

### 3. 性能指标

**当前性能**：
- RingBuffer 吞吐量: 300+ MB/s
- 真实磁盘 I/O: 170 MB/s
- 丢帧率: <1%（调优后）

**如何测试**：
```bash
./build/micecam_tests --gtest_filter="*Stress*"
```

---

## 调试技巧

### 1. 日志输出

当前使用 `std::cout` 和 `std::cerr`：

```cpp
std::cout << "Pipeline started\n";
std::cerr << "Warning: Buffer full, dropping frame\n";
```

### 2. GDB 调试

```bash
gdb ./build/micecam_tests
(gdb) break IngestionPipeline::camera_thread_func
(gdb) run
(gdb) print frames_captured_
(gdb) continue
```

### 3. 常见问题

**死锁**：
- 检查 mutex 锁顺序
- 使用 `std::lock_guard` 而非手动 lock/unlock

**数据竞争**：
- 使用 ThreadSanitizer:
  ```bash
  cmake -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" \
        -B build
  ```

**性能下降**：
- 检查是否意外拷贝
- 检查锁竞争
- 使用 profiler

---

## 代码风格

### 格式化

使用 `.clang-format`：
```bash
clang-format -i include/**/*.h src/**/*.cpp
```

### 命名规范

- **类名**: PascalCase (Frame, RingBuffer)
- **函数名**: PascalCase (GetFrame, Start)
- **变量名**: snake_case (frame_count, buffer_size)
- **常量**: kPascalCase (kMaxFrames)

### 注释

```cpp
// 好的注释：解释"为什么"
// 使用环形缓冲区而非队列，避免动态内存分配
RingBuffer buffer_(10);

// 不好的注释：重复代码
// 创建环形缓冲区，大小为 10
RingBuffer buffer_(10);
```

---

## 贡献流程

### 1. Fork 项目

```bash
git clone https://github.com/your-username/MiceCam.git
cd MiceCam
```

### 2. 创建分支

```bash
git checkout -b feature/my-new-camera
```

### 3. 编写代码

- 遵循代码风格
- 添加测试
- 更新文档

### 4. 测试

```bash
./build.sh  # 构建并测试
```

### 5. 提交

```bash
git add .
git commit -m "Add MyCameraBackend support"
git push origin feature/my-new-camera
```

### 6. Pull Request

描述：
- 改动了什么
- 为什么需要
- 测试结果

---

## 参考资料

### 设计文档
- `PROJECT_SUMMARY.md` - 项目总览
- `ITERATION_1.md` - 核心数据结构
- `ITERATION_2.md` - Stage 1 实现
- `ITERATION_3.md` - 性能优化

### 技术文档
- CMake: https://cmake.org/documentation/
- GoogleTest: https://google.github.io/googletest/
- nlohmann/json: https://json.nlohmann.me/

### 相关项目
- ~/camera - 参考项目（过于复杂）
- OpenCV - 相机驱动

---

**欢迎贡献！** 🚀
