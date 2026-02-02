# Changelog

All notable changes to MiceCam will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-02-02

### Added

#### Core Features
- **High-speed data acquisition** - 302.5 MB/s RingBuffer throughput, 169.8 MB/s real disk I/O
- **Non-blocking architecture** - RingBuffer decouples camera capture from disk I/O
- **Zero-copy data transfer** - std::unique_ptr ownership transfer
- **Data integrity** - CRC32 checksums per-frame and session-level
- **Configurable buffer size** - User-tunable RingBuffer capacity
- **Performance monitoring** - Real-time drop rate tracking

#### Camera Backend
- **Modular architecture** - ICameraBackend interface for extensibility
- **USBCameraBackend** - OpenCV-based webcam driver (code complete, requires OpenCV + hardware)
- **FakeCamera** - Test double for TDD development

#### Data Processing Tools (Python)
- **read_bin.py** - Read .bin + JSON metadata files
- **convert.py** - Convert to NumPy, HDF5, or Video formats
- **visualize.py** - Data visualization and statistics
- **test_tools.py** - Tool test suite (5/5 passing)

#### Testing
- **Unit tests** - 24 C++ tests covering Frame, RingBuffer, Pipeline
- **Integration tests** - End-to-end pipeline validation
- **Stress tests** - 200+ MB/s sustained write validation
- **Tool tests** - Python tool validation
- **100% pass rate** - 29/29 tests passing

#### Documentation
- **README.md** - Project overview and quick start
- **USER_GUIDE.md** - Comprehensive user manual
- **DEVELOPER_GUIDE.md** - Architecture and extension guide
- **SETUP.md** - Installation and build instructions
- **PROJECT_SUMMARY.md** - Complete project summary
- **tools/README.md** - Python tools documentation

#### Build System
- **Modern CMake** - Modular build with FetchContent
- **GoogleTest** - Auto-downloaded testing framework
- **nlohmann/json** - Header-only JSON library
- **check_env.sh** - Environment validation script
- **build.sh** - One-click build script

### Performance Metrics

| Metric | Target | Achieved |
|--------|--------|----------|
| RingBuffer throughput | 200+ MB/s | 302.5 MB/s |
| Real disk I/O | 150+ MB/s | 169.8 MB/s |
| Test coverage | High | 100% (29/29) |
| Zero-copy | Yes | Yes |
| Non-blocking | Yes | Yes |

### Architecture

```
CameraBackend → RingBuffer → DiskWriter → [.bin + JSON]
                                                ↓
                                          Python Tools
                                                ↓
                                    NumPy / HDF5 / Video / Analysis
```

### Technical Highlights

- **C++20** - Modern C++ with concepts, ranges
- **TDD** - Test-driven development throughout
- **Zero-copy** - Ownership transfer avoids data copying
- **Non-blocking** - Camera thread never waits on I/O
- **Extensible** - Plugin-style camera backends
- **Pragmatic** - Python tools instead of complex C++ HDF5

### Dependencies

#### Required
- C++20 compiler (Clang 12+, GCC 10+, MSVC 2019+)
- CMake 3.20+
- Threads (standard library)

#### Optional
- OpenCV 4.x - USB Camera support
- Python 3.7+ - Data processing tools
- NumPy - Data analysis
- h5py - HDF5 conversion
- opencv-python - Video export
- matplotlib - Visualization

### Known Limitations

- USB Camera backend requires OpenCV installation and camera hardware
- HDF5 conversion is lightweight (Python-based), not C++ implementation
- Session management (Stage 3) is delegated to filesystem and Python scripts
- No real hardware validation yet (tested with FakeCamera only)

### Next Steps

1. **Hardware validation** - Test with real camera
2. **User feedback** - Collect usage data and requirements
3. **Stage 2/3** - Implement if needed based on feedback
4. **Performance tuning** - Optimize based on real-world usage

### Design Philosophy

> **"Good Taste"** - Data structures over algorithms, eliminate special cases, pragmatism over theory.

- Data structures first (Frame + RingBuffer)
- Zero-copy ownership transfer
- No premature optimization
- Observable behavior (drop rate monitoring)
- Simple, clean, maintainable code

---

## [Unreleased]

### Planned

- Real camera hardware validation
- Stage 2 C++ implementation (if needed)
- Stage 3 session management (if needed)
- Performance profiling and optimization
- Additional camera backends (industrial cameras)

---

For release notes and migration guides, see [USER_GUIDE.md](USER_GUIDE.md) and [SETUP.md](SETUP.md).
