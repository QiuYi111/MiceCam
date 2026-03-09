#!/usr/bin/env python3
"""
MiceCam Python Demo

This script demonstrates how to use the MiceCam SDK from Python.
It starts a recording session and prints frame statistics in real-time.

Usage:
    python python_demo.py --duration 10 --output ./recordings
"""

import argparse
import time
import sys

# Add the build directory to path for development
sys.path.insert(0, './build/Release')

try:
    import micecam
except ImportError:
    print("Error: Could not import micecam module.")
    print("Make sure you built with -DBUILD_PYTHON_BINDINGS=ON")
    sys.exit(1)


def main():
    parser = argparse.ArgumentParser(description='MiceCam Python Demo')
    parser.add_argument('--backend', default='usb', choices=['usb', 'oak'],
                        help='Camera backend to use')
    parser.add_argument('--device', type=int, default=0,
                        help='Camera device index')
    parser.add_argument('--width', type=int, default=1920,
                        help='Frame width')
    parser.add_argument('--height', type=int, default=1080,
                        help='Frame height')
    parser.add_argument('--fps', type=float, default=30.0,
                        help='Target frame rate')
    parser.add_argument('--duration', type=float, default=5.0,
                        help='Recording duration in seconds')
    parser.add_argument('--output', default='./recordings',
                        help='Output directory')
    parser.add_argument('--session', default='python_demo',
                        help='Session name')
    args = parser.parse_args()
    
    print(f"MiceCam Python SDK v{micecam.__version__}")
    print(f"Starting {args.backend} camera @ {args.width}x{args.height} {args.fps}fps")
    print(f"Recording to: {args.output}/{args.session}")
    print("-" * 50)
    
    # Frame counter for callback
    frame_count = [0]
    total_bytes = [0]
    start_time = [None]
    
    def on_frame(data, seq_id, timestamp):
        """Callback invoked for each captured frame."""
        if start_time[0] is None:
            start_time[0] = timestamp
        
        frame_count[0] += 1
        total_bytes[0] += len(data)
        
        # Print stats every 30 frames
        if frame_count[0] % 30 == 0:
            elapsed = timestamp - start_time[0]
            fps = frame_count[0] / elapsed if elapsed > 0 else 0
            mbps = (total_bytes[0] / 1e6) / elapsed if elapsed > 0 else 0
            print(f"\rFrames: {frame_count[0]:5d} | "
                  f"FPS: {fps:6.1f} | "
                  f"Throughput: {mbps:6.1f} MB/s | "
                  f"Frame size: {len(data)/1024:.1f} KB", end='')
    
    try:
        # Create and start pipeline using context manager
        with micecam.Pipeline(
            backend=args.backend,
            device_id=args.device,
            width=args.width,
            height=args.height,
            fps=args.fps,
            output_dir=args.output,
            session_name=args.session
        ) as pipeline:
            
            # Attach our callback
            pipeline.attach_callback(on_frame)
            
            # Record for specified duration
            print(f"Recording for {args.duration} seconds...")
            time.sleep(args.duration)
            
            # Get final stats
            stats = pipeline.get_stats()
        
        print("\n" + "-" * 50)
        print("Recording complete!")
        print(f"  Captured frames: {stats['captured_frames']}")
        print(f"  Dropped frames:  {stats['dropped_frames']}")
        print(f"  Drop rate:       {stats['drop_rate']*100:.2f}%")
        print(f"  Total data:      {total_bytes[0]/1e6:.1f} MB")
        
    except Exception as e:
        print(f"\nError: {e}")
        return 1
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
