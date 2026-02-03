# MiceCam 编译与构建坑点记录 (Troubleshooting Guide)

在本项目（尤其是 Windows 环境下）的构建过程中，我们遇到了多个关于依赖、链接和运行时库的典型问题。以下是这些问题的详细记录及其最终解决方案，旨在为后续维护提供参考。

## 1. 依赖库缺失：OpenCV DLL 危机
**现象**：程序编译通过，但运行时报错“找不到 `opencv_world.dll`”，且即便通过 `vcpkg` 安装了 OpenCV，由于系统 PATH 未配置或版本冲突，导致后端加载失败。
**解决方法**：
- **解耦核心**：修改 `CMakeLists.txt`，将 `micecam_core` 与 OpenCV 完全剥离。
- **改用库文件**：将 `DEPTHAI_TARGET_CORE` 定义为 ON，强制 `depthai-core` 仅构建基础功能，不引用 OpenCV。
- **最终方案**：完全移除 OpenCV 依赖，改用原生的 FFmpeg (libavdevice) 进行 USB 图像抓取，从根本上解决了 DLL 依赖问题。

## 2. 链接错误：后端符号未定义 (LNK2019)
**现象**：在添加 FFmpeg 或 OAK 后端时，测试程序报错 `unresolved external symbol... FFmpegCameraBackend`。
**解决方法**：
- **公共宏传播**：确保 `WITH_FFMPEG` 和 `WITH_OAK_CAMERA` 宏在 `CMakeLists.txt` 中使用 `PUBLIC` 作用域定义：
  ```cmake
  target_compile_definitions(micecam_camera PUBLIC WITH_FFMPEG)
  ```
- **显式链接**：在 `target_link_libraries` 中明确指出依赖库的关系，确保静态库及其后端实现被正确加载到可执行文件中。

## 3. 跨语言调用：Python 绑定加载失败
**现象**：`uv run python` 调用 `import micecam` 时提示 `ImportError: DLL load failed`。
**解决方法**：
- **依赖对齐**：Windows 下 `.pyd` 文件本质是 DLL，它依赖的所有动态库（FFmpeg 的 `avcodec-60.dll` 等、DepthAI 的 `depthai-core.dll`）必须与 `.pyd` 处于同一目录下。
- **构建采集脚本**：编写自动化部署脚本，将 `build/Release` 和 `3rdParty` 中的所有依赖 DLL 拷贝至 Python 包目录 (`micecam/`) 下。

## 4. 硬件同步：OAK 四摄同步失效
**现象**：初期录制的四路图像存在时间偏移，抖动超过 100ms。
**解决方法**：
- **硬件锁配置**：在 `oak_camera_backend.cpp` 中启用 `setFrameSyncMode(SyncMode::HARDWARE)`，并非简单的软件触发。
- **时间戳校准**：弃用系统时钟，改用 `depthai` 驱动返回的硬件原始时间戳 (`getTimestampDevice()`)，将帧间差降低至 **< 12us**。

## 5. 编译性能：Ninja 与 Parallel Build
**现象**：MSVC 默认串行编译速度极慢。
**解决方法**：
- **一键脚本**：提供 `build.ps1`，默认调用 `cmake --build build --parallel 8`，大幅缩短迭代时间。

## 6. FFmpeg 版本冲突
**现象**：自编译的 FFmpeg 与系统自带版本冲突，导致 `avformat_open_input` 无法识别设备名。
**解决方法**：
- **PnP ID 识别**：弃用通俗名称（如 "Integrated Camera"），改用 FFmpeg 提供的唯一 PnP 标识符 `video=@device_pnp_...`，确保在任何 Windows 电脑上都能精准锁定相机。

---

**核心总结**：Windows C++ 开发的头号敌人是 **路径与依赖管理**。通过**全路径指定、依赖本地化（Release 目录自包含）以及宏定义的公共化传播**，我们最终构建出了一个稳定且自给自足的录制网关。
