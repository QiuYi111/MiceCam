# MiceCam SDK (Windows Release)

MiceCam is a high-performance camera ingestion system designed for scientific research. It supports OAK-4P quad-camera synchronization and standard USB webcams at 4K resolution with zero-drop disk writing.

## ✨ Key Features
- **Zero-Drop Recording**: Direct MJPEG-to-disk writing with unbuffered I/O.
- **Multi-Camera Sync**: Native support for Luxonis OAK-4P hardware synchronization.
- **4K High Speed**: Optimized FFmpeg backend for 3840x2160 @ 30fps capture.
- **Python Bindings**: Simple, high-level Python API for recording and processing.
- **Automated Post-processing**: Built-in tools for extracting frames with nanosecond timestamps.

## 📁 Release Contents
- `bin/`: Python module (`_micecam`) and runtime DLLs (DepthAI, FFmpeg).
- `examples/`: Ready-to-use recording and decoding scripts.
- `API_REFERENCE.md`: Detailed documentation of the Python objects.

## 🚀 Quick Start (USB Webcam)

### 1. Requirements
- **Windows 10/11 (x64)**
- **Python 3.14 (AMD64)**
- **USB 3.0 Port** (for 4K recording)

### 2. Environment Setup
Add the `bin` directory to your `PYTHONPATH` so Python can find the `_micecam` module:
```powershell
$env:PYTHONPATH = "C:\Path\To\MiceCam\release\bin"
```

### 3. Start Recording (USB Webcam)
Run the included example script to start a 5-second 4K capture from a standard webcam:
```powershell
python examples/example_usb_record.py --session mouse_001 --width 3840 --height 2160 --fps 30 --duration 5
```

### 4. Start Recording (OAK-4P)
For multi-camera hardware-synchronized recording using DepthAI:
```powershell
python examples/example_oak_record.py --session synchronized_trial --width 1920 --height 1080 --fps 30 --duration 10
```
> [!TIP]
> OAK-4P hardware is optimized for synchronized multi-stream capture. Ensure your OAK device is connected via USB 3.0 for best performance.

The script will:
1. Initialize the USB camera via FFmpeg.
2. Record MJPEG data directly to `recordings/mouse_001.bin`.
3. Stream metadata to `recordings/mouse_001_metadata.jsonl`.
4. **Automatically decode** the results into `recordings/mouse_001_images/` as timestamped JPGs.

## 📂 Data Format
MiceCam uses a specialized binary format for maximum performance:
- `.bin`: Raw stream data (e.g., MJPEG chunks).
- `.jsonl`: Frame indexes, including absolute offsets, sizes, and hardware timestamps.

Use `micecam_utils.py` to convert these files into standard image formats after your session.

---
*MiceCam SDK v1.0.0 | Developer: QiuYi111*
