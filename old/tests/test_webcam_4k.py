"""
USB Webcam 4K Pipeline Test
Tests the Python bindings with a 4K 30fps USB webcam using the FFmpeg backend.
"""
import sys
import time
import os

# Add the module path
sys.path.insert(0, os.path.abspath('build/bindings/python/Release'))

try:
    import _micecam
except ImportError as e:
    print(f"Error importing _micecam: {e}")
    # Try to find missing DLLs if needed
    sys.exit(1)

print(f"MiceCam Version: {_micecam.__version__}")
print(f"Webcam Support: {_micecam.has_webcam_support()}")

# Frame counter
frame_count = 0
start_time = None
bytes_received = 0

def on_frame(data, seq, timestamp):
    global frame_count, start_time, bytes_received
    if start_time is None:
        start_time = time.time()
    frame_count += 1
    bytes_received += len(data)

    # Print status every 30 frames
    if frame_count % 30 == 0:
        elapsed = time.time() - start_time
        fps = frame_count / elapsed if elapsed > 0 else 0
        mbps = (bytes_received * 8 / 1_000_000) / elapsed if elapsed > 0 else 0
        print(f"Frame {seq}: {len(data)} bytes, {fps:.1f} fps, {mbps:.1f} Mbps")

# Create pipeline
print("\nCreating USB Webcam pipeline (4K @ 30fps)...")
output_dir = os.path.abspath("webcam_test_output")
os.makedirs(output_dir, exist_ok=True)

try:
    # Use 'ffmpeg' or 'usb' backend
    pipeline = _micecam.Pipeline(
        output_dir=output_dir,
        session_name="webcam_4k_test",
        backend_name="ffmpeg",
        width=3840,
        height=2160,
        fps=30.0,
        device_id=0
    )

    # Attach callback
    pipeline.attach_callback(on_frame)

    # Run for 10 seconds
    print("Starting capture for 10 seconds...")
    pipeline.start()

    time.sleep(10)

    print("\nStopping pipeline...")
    pipeline.stop()

    # Print final stats
    if start_time:
        elapsed = time.time() - start_time
        print(f"\n=== Capture Complete ===")
        print(f"Total frames: {frame_count}")
        print(f"Duration: {elapsed:.1f}s")
        print(f"Average FPS: {frame_count/elapsed:.1f}")
        print(f"Total data: {bytes_received/1_000_000:.1f} MB")

        # Check stats from pipeline
        stats = pipeline.get_stats()
        print(f"Pipeline Stats: {stats}")

except Exception as e:
    print(f"Error during test: {e}")
    import traceback
    traceback.print_exc()
