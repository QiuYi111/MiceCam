# MiceCam 深度性能优化与架构重构建议

 **版本** : v2.0 (Professional)
 **日期** : 2026-02-02
 **受众** : 核心开发团队
 **状态** : 关键路径阻碍 (Blocker)

## 0. 核心诊断 (Executive Summary)

 **直言不讳的结论** ：
当前系统在高负载下崩溃的根本原因，是**I/O 访问模式极其低效**叠加了 **数据源（OpenCV）的不可控性** 。

1. **I/O 层面** ：`DiskWriter` 像“机关枪”一样向硬盘发送 60Hz-120Hz 的小数据块（<2MB）写入请求。而在 Windows (NTFS) 上，这是文件系统的噩梦。无论你的 SSD 标称速度多快，这种写法都会把吞吐量拉低到 20MB/s 以下。
2. **采集层面** ：使用 OpenCV (`cv::VideoCapture`) 做高帧率科学采集是业界公认的“反模式”。它专为计算机视觉算法设计（解码每一帧），而非为数据流设计（搬运每一字节）。

## 1. 第一优先级：重构磁盘写入 (I/O Aggregation)

 **目标** ：将磁盘写入吞吐量从 ~20 MB/s 提升至物理极限 (~500 MB/s+)。

### 理论基础

现代 SSD 和文件系统喜欢**大块、对齐**的顺序写入。

* **当前做法** ：1 帧 (2MB) -> `write()` -> 1 帧 (2MB) -> `write()` ... (系统调用开销大，碎片化严重)
* **推荐做法** ：1 帧 -> 内存拷贝 -> ... -> 16 帧 -> **32MB Super Block** -> `write()` (一次系统调用，顺序落盘)

### 代码实现建议 (`src/pipeline/disk_writer.cpp`)

请废弃旧的逐帧写入逻辑，采用双缓冲或聚合缓冲策略：

```
// 伪代码参考

// 配置参数
const size_t AGGREGATION_SIZE = 32 * 1024 * 1024; // 32MB
// 预分配大块内存，避免运行时 new/malloc
std::vector<uint8_t> super_block; 
super_block.reserve(AGGREGATION_SIZE);

void DiskWriter::write_loop() {
    super_block.clear();
  
    while (running_) {
        // 从 RingBuffer 获取一帧
        auto frame = buffer_->pop(); 
      
        // 1. 边界检查：如果加上这一帧会溢出 Super Block，先 Flush
        if (super_block.size() + frame.size() > AGGREGATION_SIZE) {
            flush_to_disk(super_block);
            super_block.clear();
        }
      
        // 2. 内存聚合 (Memory Copy 极其廉价，纳秒级)
        // 注意：这里是把 frame 数据追加到 vector 尾部
        super_block.insert(super_block.end(), frame.data->begin(), frame.data->end());
      
        // 3. 更新元数据索引
        // 关键：现在的 file_offset 应该是 "已落盘总字节 + 当前 super_block 偏移"
        size_t current_offset = total_bytes_written_ + (super_block.size() - frame.size());
        record_metadata(frame, current_offset);
    }
  
    // 退出前把残余数据写入
    if (!super_block.empty()) {
        flush_to_disk(super_block);
    }
}

void DiskWriter::flush_to_disk(const std::vector<uint8_t>& data) {
    // 这里一次性写入 32MB，效率极高
    outfile_.write(reinterpret_cast<const char*>(data.data()), data.size());
    total_bytes_written_ += data.size();
}
```

## 2. 第二优先级：相机后端专业化 (Backend Specialization)

 **痛点** ：由于不想“重复造轮子”，你们试图用 OpenCV 统一所有相机。但对于科学实验， **通用往往意味着平庸** 。为了多模态同步，必须针对硬件特性编程。

### A. FLIR 工业相机 (Spinnaker SDK)

 **现状** : OpenCV 无法获取精确的拍摄时间，导致时间戳抖动 (Jitter)。
 **方案** :

1. **引入 Spinnaker SDK (C++)** 。
2. **图像回调模式** : 不要用 `polling` (while循环去读)，使用 Spinnaker 的 `ImageEvent` 回调。
3. **硬件时间戳** : 读取 Chunk Data 中的 `ChunkTimestamp`。这是相机曝光的那一纳秒，完全不受 Windows 系统负载影响。
4. **Raw Data** : 直接获取 Bayer 格式的原始指针，不进行 Debayer（转彩色），数据量减少 3 倍（8-bit Bayer vs 24-bit BGR），I/O 压力瞬间减轻。

### B. OAK (Luxonis) 相机

 **现状** : 作为 Webcam 使用，浪费了强大的 VPU。
 **方案** :

1. **引入 depthai-core (C++)** 。
2. **片上编码** : 在 OAK 相机内部运行 Pipeline: `Color -> VideoEncoder (MJPEG) -> XLinkOut`。
3. **传输** : USB 传输的是已经压缩的 JPEG 流。
4. **优势** : 主机 CPU 占用率几乎为 0，且彻底解决了 USB 带宽瓶颈。

### C. 普通 USB Webcam

 **现状** : Windows 默认解码为 NV12，带宽爆炸。
 **方案** :
在 `initialize` 中必须强制设置：

```
// 强制 DirectShow 后端
cap.open(id, cv::CAP_DSHOW);
// 强制 MJPG (部分罗技相机可能需要先设分辨率再设格式，或者反过来，需要实验)
cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
// 关闭自动转 RGB
cap.set(cv::CAP_PROP_CONVERT_RGB, 0.0); 
```

## 3. 进阶优化：内存对象池 (Object Pooling)

 **问题** ：
目前的 `RingBuffer` 存储的是 `std::unique_ptr<Frame>`。
每秒钟 `new Frame` 和 `delete Frame` 发生 60-120 次。虽然 C++ 堆分配很快，但在高压测试下，这会导致：

1. **内存碎片** 。
2. **GC 抖动** （虽然 C++ 没有 GC，但频繁 malloc/free 会增加 OS 内存管理开销）。

 **优化方案** :
实现一个简单的 `FramePool`。

1. 启动时预分配 500 个 `Frame` 对象（及其内部的 data vector）。
2. 采集时从 Pool `acquire()` 一个空闲 Frame。
3. 写入磁盘后，不要 delete，而是 `release()` 回 Pool。
4. **收益** : 运行时零内存分配 (Zero Allocation at Runtime)，极度稳定。

## 4. 系统级优化：线程优先级

 **问题** ：
Windows 是非实时系统。如果用户打开了 Chrome 或其他软件，采集线程可能被抢占，导致丢帧。

 **方案** :
在 `IngestionPipeline` 启动线程时，设置线程亲和性与优先级。

```
#include <windows.h> // 如果是 Windows

void set_high_priority() {
    HANDLE thread = GetCurrentThread();
    // 设置为 TIME_CRITICAL (最高) 或 HIGHEST
    SetThreadPriority(thread, THREAD_PRIORITY_TIME_CRITICAL);
}
```

*注意：仅对“采集线程”和“写入线程”设置高优先级，不要对主 UI 线程设置。*

## 5. 路线图建议 (Roadmap)

### 第一阶段：止血 (1-2 天)

* [ ] 实现 `DiskWriter` 的  **Super Block 聚合写入** 。(这是解决崩溃的唯一关键)
* [ ] 验证 `simple_disk_bench` 的 112MB/s 速度能否在实际 App 中复现。

### 第二阶段：降压 (1 周)

* [ ] 修改 Webcam Backend，强制  **DirectShow + MJPEG** 。
* [ ] 针对 OAK 相机，剥离 OpenCV，对接 **depthai-core** 获取压缩流。

### 第三阶段：专业化 (2 周+)

* [ ] 集成  **Spinnaker SDK** ，实现 FLIR 相机的硬件触发与硬件时间戳。
* [ ] 引入  **Object Pool** ，消除运行时内存分配。

## 6. 写给团队的话

不要为了“代码整洁”或“通用性”而牺牲性能。由于你们处理的是高带宽数据流（High Bandwidth Data Streaming），每一毫秒的延迟、每一次多余的内存拷贝都是致命的。

你们的 `RingBuffer` 设计是正确的，现在的瓶颈完全在于**对操作系统 I/O 机制的“天真”使用**和 **对 OpenCV 的过度依赖** 。解决这两点，MiceCam 将会是一个非常强悍的工具。
