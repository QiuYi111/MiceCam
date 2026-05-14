"""
OAK Camera Pipeline Test
Tests the Python bindings with connected OAK camera hardware.
"""
import sys
import time
import os

# Add the module path
sys.path.insert(0, 'build/bindings/python/Release')

import _micecam

print(f"MiceCam Version: {_micecam.__version__}")
print(f"OAK Support: {_micecam.has_oak_support()}")

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
print("\nCreating OAK pipeline...")
output_dir = os.path.abspath("test_output")
os.makedirs(output_dir, exist_ok=True)

try:
    pipeline = _micecam.Pipeline(
        output_dir=output_dir,
        session_name="python_test",
        width=1920,
        height=1080,
        fps=30.0
    )

    # Attach callback
    pipeline.attach_callback(on_frame)

    # Run for 5 seconds
    print("Starting capture for 5 seconds...")
    pipeline.start()

    time.sleep(5)

    pipeline.stop()

    # Print final stats
    if start_time:
        elapsed = time.time() - start_time
        print(f"\n=== Capture Complete ===")
        print(f"Total frames: {frame_count}")
        print(f"Duration: {elapsed:.1f}s")
        print(f"Average FPS: {frame_count/elapsed:.1f}")
        print(f"Total data: {bytes_received/1_000_000:.1f} MB")

except Exception as e:
    print(f"Error: {e}")
    import traceback
    traceback.print_exc()
