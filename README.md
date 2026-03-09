# MiceCam: High-Performance Behavioral Data Acquisition

**MiceCam** is a high-performance, low-latency multi-camera data acquisition system designed for animal behavior research. It utilizes a C++20 core with a RingBuffer non-blocking architecture and zero-copy semantics to ensure reliable data capture on consumer hardware.

---

## 🚀 The Neural-Grid Standard

This project follows the **Neural-Grid Standard**. The only valid interface to the project is the `Makefile`.

### 1. Initialize

Set up your development environment, including `vcpkg` for C++ dependencies and `uv` for Python.

```bash
make init
```

### 2. Build

Configure and build the C++ SDK and Python bindings.

```bash
make build
```

### 3. Verify

Run the full verification suite (linting and tests).

```bash
make verify
```

### 4. Test

Run the test suite directly.

```bash
make test
```

---

## 📂 Project structure

```text
MiceCam/
├── api/                # Public contract (headers)
├── cmd/                # Entry points (main, examples, gui)
├── internal/           # Private implementation
│   ├── domain/         # Core business logic
│   ├── infrastructure/ # Hardware & IO adapters
│   └── micecam/        # Python SDK source
├── docs/               # Technical documentation
├── tests/              # Unit and integration tests
├── vcpkg.json          # C++ dependency manifest
└── pyproject.toml      # Python package definition
```

---

## 🛠️ Key Features

- **Pluggable Backends**: FFmpeg, OpenCV, and DepthAI (OAK) support.
- **High Throughput**: 300+ MB/s memory throughput, 240+ MB/s disk I/O.
- **Hardware Sync**: Sub-millisecond synchronization for OAK cameras.
- **Data Integrity**: Robust binary format with index recovery tools.

---

## 🐍 Python Usage

MiceCam includes a high-level Python SDK wrapping the native C++ core.

```python
import micecam

# Initialize and start acquisition
pipeline = micecam.IngestionPipeline(config="config.json")
pipeline.start()
```

For data processing tools, see `cmd/gui/` and `internal/micecam/tools/`.

---

## 🛡️ License

Reliable Acquisition for Behavioral Science.
