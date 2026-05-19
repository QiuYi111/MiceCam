# Architecture Guardrails

## Tech stack (approved by user)

| Layer | Choice | Constraint |
|---|---|---|
| Language | C++20 | No Python in core pipeline |
| Build | CMake 3.20+ / vcpkg | Cross-platform macOS/Windows/Linux |
| UI | Qt 6.6+ / QML | Apple HIG design system |
| Encoding | FFmpeg 6.0+ | Hardware encoder preferred, libx264 fallback |
| Camera | DepthAI SDK / libavdevice | Plugin architecture for future backends |
| Logging | spdlog | Async, multi-sink, crash-safe |
| Testing | GTest / CTest | TDD discipline |

## Design constraints

- Plugin camera backend interface: `ICameraBackend` + `IDeviceEnumerator` — all camera access through these
- Encoding: always output H264; source format is backend's responsibility
- Timestamps: wall clock anchor at session start + steady_clock per-frame
- Storage: `.mp4` + `.srt` + `.json` per stream; no custom binary format
- Observer pattern for alerts (watchdog, webhook, UI all observe independently)
- Single-process architecture (no worker subprocess; pipeline runs in-process)

## Removed from v2

- Python bindings (pybind11)
- Python UI (PyQt)
- Custom `.bin` format
- JSONL metadata stream
- HDF5 converter
- GPU MJPEG decoder
- OpenCV USB backend
- Worker process isolation (stdin/stdout IPC)
