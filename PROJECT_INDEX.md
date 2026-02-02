# Project Index: MiceCam

**Generated**: 2026-02-02
**Version**: v0.1.0
**Status**: MVP Complete (Stage 1)

---

## 📁 Project Structure

```
MiceCam/
├── include/micecam/          # Public headers
│   ├── core/                 # Core data structures
│   │   ├── frame.h          # Frame container (ownership-based)
│   │   └── ring_buffer.h    # Thread-safe circular buffer
│   ├── camera/              # Camera backend interface
│   │   ├── camera_backend.h # Abstract interface
│   │   └── usb_camera_backend.h
│   └── pipeline/            # Processing pipeline
│       ├── ingestion_pipeline.h
│       ├── disk_writer.h
│       └── hdf5_converter.h
│
├── src/                     # Implementation
│   ├── core/
│   ├── camera/
│   ├── pipeline/
│   └── main.cpp             # CLI entry point
│
├── tests/                   # Test suite (GoogleTest)
│   ├── core/
│   ├── camera/
│   │   └── fake_camera.{h,cpp}  # Test double
│   ├── pipeline/
│   └── benchmark/
│
├── tools/                   # Python utilities
├── build.sh                 # Build script
├── check_env.sh             # Environment validator
└── CMakeLists.txt           # CMake configuration
```

---

## 🚀 Entry Points

| Entry Point | Path | Purpose |
|-------------|------|---------|
| **CLI** | `src/main.cpp` | Main application entry point |
| **Demo** | `demo_simple.cpp` | Simple usage example |
| **Tests** | `build/micecam_tests` | GoogleTest executable (29 tests) |
| **Demo Exec** | `build/micecam_demo` | Demo program |

---

## 📦 Core Modules

### Module: `micecam_core`
**Purpose**: Core library (always built)

| Component | File | Exports | Description |
|-----------|------|---------|-------------|
| **Frame** | `core/frame.{h,cpp}` | `Frame` class | Ownership-based frame container with timestamp, zero-copy move semantics |
| **RingBuffer** | `core/ring_buffer.{h,cpp}` | `RingBuffer<T>` | Thread-safe circular buffer, producer-consumer sync |
| **DiskWriter** | `pipeline/disk_writer.{h,cpp}` | `DiskWriter` | Async binary + JSON metadata writer |
| **IngestionPipeline** | `pipeline/ingestion_pipeline.{h,cpp}` | `IngestionPipeline` | Orchestrates camera → buffer → disk flow |
| **HDF5Converter** | `pipeline/hdf5_converter.{h,cpp}` | `HDF5Converter` | Bin → HDF5 conversion (Stage 2, interface defined) |

### Module: `micecam_camera` (Optional)
**Purpose**: Camera backend drivers
**Build Flag**: `WITH_CAMERA_BACKEND=ON` (requires OpenCV)

| Component | File | Exports | Description |
|-----------|------|---------|-------------|
| **ICameraBackend** | `camera/camera_backend.h` | `ICameraBackend` | Abstract camera interface |
| **USBCameraBackend** | `camera/usb_camera_backend.{h,cpp}` | `USBCameraBackend` | OpenCV-based webcam driver |
| **FakeCamera** | `tests/camera/fake_camera.{h,cpp}` | `FakeCamera` | Test double for TDD development |

---

## 🧪 Test Coverage

**Total**: 29 tests (100% passing)

### Unit Tests
- `FrameTest`: 4 tests (construction, ownership semantics)
- `RingBufferTest`: 9 tests (capacity, FIFO, blocking, concurrent)
- `PipelineTest`: Integration tests
- `HDF5Test`: HDF5 converter tests
- `ConfigurableBufferTest`: Configuration tests

### Stress/Benchmark Tests
- `RingBuffer200MBps`: 303.8 MB/s achieved ✅
- `FakeCameraFrameGeneration`: Frame generation speed
- `DropRateUnderLoad`: Buffer behavior under stress

### Integration Tests
- `IntegrationTest`: End-to-end pipeline validation

**Test Command**: `./build/micecam_tests`

---

## 🔧 Configuration

| File | Purpose |
|------|---------|
| `CMakeLists.txt` | Main build config (C++20, CMake 3.20+) |
| `build.sh` | One-build script |
| `check_env.sh` | Environment validation (disk speed, deps) |

### CMake Options
```bash
-DBUILD_TESTS=ON           # Build test suite (default)
-DBUILD_BENCHMARKS=ON      # Build performance tests
-DWITH_CAMERA_BACKEND=ON   # Enable USB camera (requires OpenCV)
```

---

## 📚 Documentation

| File | Topic |
|------|-------|
| `README.md` | Project overview, quick start, architecture |
| `prd.md` | Product requirements document |
| `SETUP.md` | Installation guide (dependencies, build) |
| `STATUS.md` | Iteration 1 summary, completion status |
| `ITERATION_2.md` | Next iteration planning |
| `tools/README.md` | Python tools documentation |

---

## 🔗 Key Dependencies

| Dependency | Version | Purpose | Source |
|------------|---------|---------|--------|
| **nlohmann_json** | v3.11.3 | JSON metadata I/O | FetchContent |
| **GoogleTest** | v1.14.0 | Testing framework | FetchContent |
| **OpenCV** | 4.x | USB camera support (optional) | System/pkg |
| **Threads** | - | Concurrency support | Std library |

---

## 🛠️ Tools & Scripts

| Tool | Language | Purpose |
|------|----------|---------|
| `convert.py` | Python | Bin → HDF5 conversion |
| `visualize.py` | Python | Data visualization |
| `read_bin.py` | Python | Read raw binary output |
| `build.sh` | Bash | Build automation |
| `check_env.sh` | Bash | Environment validation (disk speed check) |

---

## 📝 Quick Start

```bash
# 1. Check environment
./check_env.sh

# 2. Build
./build.sh

# 3. Run tests
./build/micecam_tests

# 4. Run demo
./build/micecam_demo

# 5. Process data
python tools/read_bin.py test_output/session_001
```

---

## 🎯 Design Principles

**"Good Taste" Philosophy**:
1. **Data structures > algorithms**: Frame + RingBuffer enable clean code
2. **Zero-copy**: Move semantics (std::unique_ptr) for frame ownership
3. **Non-blocking**: Camera never waits for disk
4. **TDD first**: Tests drive development, performance targets validated

**Three-Stage Architecture**:
- **Stage 1** (✅ Complete): High-speed acquisition → .bin + JSON metadata
- **Stage 2** (🚧 Design): Bin → HDF5 conversion
- **Stage 3** (📋 Planned): Session management, NAS transfer

---

## 📊 Performance Metrics

| Metric | Target | Achieved | Status |
|--------|--------|----------|--------|
| RingBuffer throughput | 200+ MB/s | 302.5 MB/s | ✅ |
| Real disk I/O | 150+ MB/s | 169.8 MB/s | ✅ |
| Zero-copy | Yes | Yes | ✅ |
| Non-blocking | Yes | Yes | ✅ |
| Test coverage | High | 100% (29/29) | ✅ |

---

## 🚨 Known Issues

1. **OpenCV dependency**: Optional, required for USB camera backend
   - Install: `brew install opencv` (macOS) or `apt install libopencv-dev` (Ubuntu)

2. **Stage 2/3**: Interface defined, implementation on-demand
   - HDF5 converter: Interface ready, implement when needed

3. **USB camera verification**: Code complete, hardware validation pending

---

## 📂 File Statistics

- **Source files** (src/ + include/): 15
- **Test files** (tests/): 9
- **Python tools**: 5
- **Shell scripts**: 2
- **Documentation files**: 7

---

**Token Efficiency**: This index (~3KB) replaces reading ~58KB of full codebase.
