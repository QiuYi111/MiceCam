"""
MiceCam Example: OAK Camera Recording & Auto-Decoding
Uses DepthAI backend to record MJPEG from an OAK camera.
"""
import sys
import os
import time
import argparse

# Add SDK path
sys.path.insert(0, os.path.abspath('build/bindings/python/Release'))

try:
    import _micecam
    from micecam_utils import decode_micecam_session
except ImportError as e:
    print(f"Error: Could not find MiceCam SDK. Ensure it's built in build/bindings/python/Release. ({e})")
    sys.exit(1)

def main():
    parser = argparse.ArgumentParser(description="MiceCam OAK Recording Example")
    parser.add_argument("--out", type=str, default="recordings", help="Output directory")
    parser.add_argument("--session", type=str, default="oak_recording", help="Session name")
    parser.add_argument("--width", type=int, default=1920, help="Width")
    parser.add_argument("--height", type=int, default=1080, help="Height")
    parser.add_argument("--fps", type=float, default=30.0, help="Frames per second")
    parser.add_argument("--duration", type=int, default=5, help="Recording duration in seconds")
    parser.add_argument("--decode", action="store_true", default=True, help="Decode to images after recording")
    
    args = parser.parse_args()

    output_dir = os.path.abspath(args.out)
    os.makedirs(output_dir, exist_ok=True)

    print(f"=== MiceCam OAK Recording ===")
    print(f"  Session: {args.session}")
    print(f"  Format:  {args.width}x{args.height} @ {args.fps} FPS")
    print(f"  Duration: {args.duration}s")
    print(f"  Output:   {output_dir}")

    try:
        # Create pipeline for OAK
        pipeline = _micecam.Pipeline(
            output_dir=output_dir,
            session_name=args.session,
            backend_name="oak",
            width=args.width,
            height=args.height,
            fps=args.fps
        )

        def on_frame(data, seq, ts):
            if seq % 30 == 0:
                print(f"  Recording frame {seq}...", end="\r")

        pipeline.attach_callback(on_frame)

        print("\nStarting recording...")
        pipeline.start()
        
        time.sleep(args.duration)
        
        print("\nStopping recording...")
        pipeline.stop()
        
        stats = pipeline.get_stats()
        print(f"Done. Captured {stats['captured_frames']} frames, dropped {stats['dropped_frames']}.")

        if args.decode:
            print("\nStarting automatic decoding...")
            decode_micecam_session(output_dir, args.session)
            print("Decoding complete.")

    except Exception as e:
        print(f"\nError: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()
