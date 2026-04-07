# Project Systematic Review: MiceCam
**Date:** 2026-03-10
**Scope:** Python UI (`cmd/gui/gui`), C++ Core Architecture (`internal/`), pybind11 Bindings (`bindings/`).

## 1. Python UI & Application Logic (`cmd/gui/gui`)

### 🔴 Bugs & Logic Constraints
- **Multi-byte UTF-8 Corruption (`recorder_thread.py`)**: The `handle_stdout` method decodes raw byte chunks via `data_bytes.decode('utf-8', errors='replace')`. If a multi-byte character (e.g., Chinese logs or emojis) is split across two `QProcess` chunks, it will be irreparably corrupted into replacement characters (``). This can break JSON parsing down the line.
  - *Fix Suggestion*: Maintain a `bytearray` buffer and only decode when complete lines are formed, or use an incremental UTF-8 decoder.
- **Global State Collision (`recorder_thread.py`)**: The `stop()` method writes a `stop_signal.txt` file into the current working directory to signal the C++ worker. This effectively breaks if multiple instances of the app run concurrently or if the CWD is not writable.
  - *Fix Suggestion*: Pass a unique, session-specific file path via CLI arguments to the worker, or use standardized IPC (Named Pipes / ZeroMQ).
- **Fragile Path Resolution**: `SDK_AVAILABLE` detection and `exe_path` resolution rely heavily on relative `os.path.join("..")` checks which can easily break depending on where the user executes the entry point.

### 🟡 UX/UI Improvements (`main_window.py`)
- The UI is clean, functional, and uses modern PyQt6 styling.
- **Hardcoded Selections**: `refresh_cameras()` strictly hardcodes `"Luxonis OAK-4P (Quad Sync)"`. The device menu should ideally query actual Luxonis devices dynamically.
- **Error Obfuscation**: If the `DecoderThread` encounters an exception, it emits a status message but stays silent in the UI except for a log line. A visual error state (e.g., turning the progress bar red) would vastly improve UX.

## 2. C++ Backend & Core Engine (`internal/`)

### 🔴 Logic & Portability Bugs
- **Windows Unbuffered I/O Padding (`disk_writer.cpp`)**: To satisfy Windows `FILE_FLAG_NO_BUFFERING` sector-alignment, `flush_aggregation_buffer()` injects zero-padding bytes (`padding = 4096 - remainder`) into the data stream during intermediate flushes. While the metadata index correctly skips the padding by relying on `total_bytes_on_disk_`, this fundamentally wastes disk space and couples the binary format to the Windows platform's hardware sector size.
  - *Fix Suggestion*: Instead of padding the data midway, aggregate frames continuously and only pad at the very end of the file when finalizing, or use `WriteFile` with aligned memory but truncated valid file sizes via `SetEndOfFile()`.

### 🟡 Performance Profiling
- **Memory Copy Penalty (`disk_writer.cpp`)**: The codebase advertises "zero-copy semantics", and successfully achieves this via `std::move(*frame)` in the `RingBuffer`. However, `DiskWriter::write_loop()` copies the frame memory into `aggregation_buffer_` via `std::memcpy`. This imposes a 1-copy penalty (e.g., copying 2-5MB frames continuously).
  - *Optimization*: Utilize Scatter/Gather I/O (e.g., `writev` on POSIX or `WriteFileGather` on Windows) to write directly from the individual Frame buffers to disk in a single system call, bypassing the aggregation copy entirely.

## 3. Python Bindings (`bindings/python/module.cpp`)

### 🟡 Performance
- **Callback Overhead**: In `PyPipeline::attach_callback`, the C++ core wraps `frame.data` into `py::bytes` before passing it to Python. Constructing `py::bytes` forces a deep memory allocation and copy inside Python's heap.
  - *Optimization*: Use the Python Buffer Protocol (`py::memoryview` or `py::array_t<uint8_t>`) to pass a zero-copy reference to Python. The Python callback can then operate directly on C++ memory without the GC allocation overhead.

## Conclusion & Next Steps
The architecture adheres strictly to DDD and isolates the C++ heavy lifting from the Python UI efficiently via `QProcess` and bindings. The immediate next steps should be prioritizing the UTF-8 stdout bug and the Windows I/O padding logic to prevent production data corruption and format drift.
