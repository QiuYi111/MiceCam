# Bug Report: Sustained Disk I/O Bottleneck at High FPS

## 1. Problem Description
During long-duration (10 min+) recordings at high resolutions (2K 30fps) or high framerates (1080p 60fps, 720p 120fps), the MiceCam system experiences catastrophic frame drops (up to 80%) after a short period of stability (~20-100 seconds).

### Symptoms:
- **Short tests (5s):** 0% drops.
- **Long tests (10 min):** Stability for the first 20-100s, followed by sudden large bursts of frame drops.
- **Throughput:** The system fails to maintain the required write speed to the `D:` drive for raw camera data.

---

## 2. Investigation Paths & Results

### Path A: Memory Level Bottleneck
- **Goal:** Verify if `RingBuffer` or in-memory processing is the bottleneck.
- **Trial:** Created `buffer_bench.cpp` to measure purely in-memory throughput.
- **Result:** **3.46 GB/s** throughput. Memory is NOT the bottleneck.

### Path B: Raw Data Capture (MJPEG)
- **Goal:** Reduce CPU and I/O load by capturing raw MJPEG bytes instead of decoding to BGR.
- **Trial:** Setting `cv::CAP_PROP_CONVERT_RGB = false` in `USBCameraBackend` and adjusting the pipeline.
- **Result:** 
  - On Windows, `MSMF` backend often returns uncompressed **NV12** (~1.5 bytes per pixel) even when BGR conversion is disabled.
  - Data rate for 1080p@60 is still ~180 MB/s, which exceeds sustained disk speed.
  - Diagnostic (`diagnose_size.cpp`) confirmed frame sizes were consistent with NV12, not compressed MJPEG.

### Path C: Disk I/O Optimization (Unbuffered I/O)
- **Goal:** Bypass OS file cache overhead using Windows `CreateFileA` with `FILE_FLAG_NO_BUFFERING`.
- **Trial:** Modified `DiskWriter` to use Win32 API.
- **Result:** **Performance dropped to ~5 MB/s**. This likely failed because the `DiskWriter` writes each frame as a single smaller write, whereas unbuffered I/O requires large, sector-aligned blocks to be efficient.

### Path D: Disk I/O Baselines
- **Goal:** Determine the true limits of the `D:` drive.
- **Trial 1 (`disk_bench`):** Standard `std::ofstream` writing ~600KB frames. Result: **~21 MB/s**.
- **Trial 2 (`simple_disk_bench`):** Sequential writing with large 8MB blocks. Result: **~112 MB/s**.
- **Conclusion:** There is a massive performance gap (~5.3x) between writing small frames individually and writing large buffered blocks.

---

## 3. Findings
1. **Sustained vs. Peak:** The drive can peak at high speeds but its sustained write speed for small asynchronous writes is extremely low (~21 MB/s) on this system.
2. **Backends:** `MSMF` ignores the "raw" request for MJPEG. `DirectShow` was tried but had initialization issues at high FPS.
3. **Buffer Exhaustion:** Even with a 1200-frame (20s) buffer, the transient disk slowdowns eventually exhaust the buffer, leading to drops.

## 4. Recommendations for Next Steps
- **Block-Based Writing:** Implement a layer in `DiskWriter` that aggregates frames into large (8MB+) blocks before writing to disk to hit the ~112 MB/s sustained limit.
- **True MJPEG Capture:** Investigate lower-level Windows APIs (Media Foundation or DirectShow filters) to force the camera to provide compressed MJPEG bytes, which would drop the data rate from ~180 MB/s to ~15 MB/s.
