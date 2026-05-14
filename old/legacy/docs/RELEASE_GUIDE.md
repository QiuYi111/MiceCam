# MiceCam SDK Release Guide

To create a distributable release of MiceCam, you should include the following files. This ensures that users can run your Python examples (or their own scripts) without needing to build the project from source.

## 📦 Suggested Directory Structure

```text
MiceCam_Release/
├── bin/                       # Runtime Binaries
│   ├── _micecam.cp314-win_amd64.pyd
│   ├── depthai-core.dll
│   ├── depthai-opencv.dll
│   ├── libusb-1.0.dll
│   ├── avcodec-62.dll
│   ├── avdevice-62.dll
│   ├── avformat-62.dll
│   ├── avutil-60.dll
│   ├── swresample-6.dll
│   └── swscale-9.dll
├── examples/                  # Python Examples
│   ├── example_usb_record.py
│   ├── example_oak_record.py
│   └── micecam_utils.py       # Helper for auto-decoding
└── README_RELEASE.md          # Quick start for users
```

## 📝 Release Checklist

### 1. Python Binaries
- `_micecam.cp314-win_amd64.pyd`: The core Python module.
  > [!NOTE]
  > This is specific to Python 3.14 on Windows. If your users have a different Python version, you will need to build for that version.

### 2. Runtime Dependencies (DLLs)
These are required for the Python module to load correctly:
- **DepthAI**: `depthai-core.dll`, `depthai-opencv.dll`, `libusb-1.0.dll`
- **FFmpeg**: `avcodec-62.dll`, `avdevice-62.dll`, `avformat-62.dll`, `avutil-60.dll`, `swresample-6.dll`, `swscale-9.dll`

### 3. User Scripts
- `example_usb_record.py`: Configurable USB camera recording.
- `example_oak_record.py`: Configurable OAK camera recording.
- `micecam_utils.py`: Essential for post-processing the binary data into images.

## 🚀 Deployment Steps
1.  Collect all files listed above into a ZIP file.
2.  Instruct users to add the `bin/` directory to their `PYTHONPATH` or place their scripts in the same folder as the DLLs.
3.  Ensure users have the **NVIDIA Driver** installed if they intend to use the GPU decoding features (though for pure recording, this is not strictly required).
