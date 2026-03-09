# MiceCam Python API Reference

The MiceCam Python bindings are provided via the `_micecam` module. 

## 🏗 Pipeline Class

The `Pipeline` class is the main entry point for camera control.

### `Pipeline(output_dir, session_name, backend_name="oak", width=1920, height=1080, fps=30.0, device_id=0)`
Constructs a new ingestion pipeline.

**Arguments:**
- `output_dir` (str): Directory where `.bin` and `.jsonl` files will be saved.
- `session_name` (str): Base name for the session files.
- `backend_name` (str): Choose `oak` for DepthAI hardware or `ffmpeg` for USB webcams.
- `width` / `height` (int): Requested capture resolution.
- `fps` (float): Requested frame rate.
- `device_id` (int): System index for USB cameras.

### Methods

#### `start()`
Starts the camera hardware and begins writing to disk. Unblocks the GIL.

#### `stop()`
Stops the hardware and finalizes the recording session.

#### `attach_callback(callback)`
Attaches a Python function to be called on every new frame.
- **Callback Signature**: `callback(data: bytes, sequence_id: int, timestamp: float)`
- **Note**: The callback executes in the camera thread. Keep processing light or move to a separate queue.

#### `get_stats() -> dict`
Returns a dictionary of runtime performance metrics.
- `captured_frames`: Total successfully written frames.
- `dropped_frames`: Frames dropped by the internal ring buffer.
- `drop_rate`: Ratio of drops.
- `pending_buffer`: Current occupancy of the jitter buffer.

#### `is_running() -> bool`
Returns `True` if the pipeline is active.

### Context Manager Support
The `Pipeline` supports the `with` statement for automatic cleanup:
```python
with Pipeline(out, name, backend="ffmpeg") as p:
    time.sleep(10)
```

## 🛠 Utilities (`micecam_utils.py`)

### `decode_micecam_session(output_dir, session_name, target_dir=None)`
A pure-python utility to convert MiceCam binary sessions into standard JPEG files.

- Reads the `.jsonl` metadata for frame boundaries.
- Extracts chunks from the `.bin` file.
- Names images using their nanosecond-level timestamps (e.g., `1706523912.jpg`).

## 🔍 System Checks

### `has_oak_support() -> bool`
Returns `True` if the SDK was built with DepthAI support.

### `has_webcam_support() -> bool`
Returns `True` if the SDK was built with FFmpeg/USB support.

---
*MiceCam SDK v1.0.0*
