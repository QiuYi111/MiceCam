import sys
import os
import time
import json
import threading
import signal
import _micecam

# Global flags
stop_requested = False

def signal_handler(sig, frame):
    global stop_requested
    stop_requested = True

def main():
    global stop_requested
    if len(sys.argv) < 7:
        print("Usage: python recorder_worker.py <output_dir> <session_name> <backend> <width> <height> <fps> <dev_idx>")
        return

    output_dir = sys.argv[1]
    session_name = sys.argv[2]
    backend = sys.argv[3]
    width = int(sys.argv[4])
    height = int(sys.argv[5])
    fps = float(sys.argv[6])
    dev_idx = int(sys.argv[7])

    print(f"[Worker] Starting recording session: {session_name}")
    
    # Setup stats file
    status_file = "recorder_status.json"
    
    try:
        pipeline = _micecam.Pipeline(output_dir, session_name, backend, width, height, fps, dev_idx)
        pipeline.start()
        print("[Worker] Pipeline started.")
        
        start_time = time.time()
        
        last_frames = 0
        last_ts = time.time()
        
        # Main loop
        while not stop_requested and pipeline.is_running():
            now = time.time()
            dt = now - last_ts
            
            # Update status file
            stats = pipeline.get_stats()
            
            # Enrich stats
            elapsed = now - start_time
            stats["elapsed_seconds"] = elapsed
            
            curr_frames = stats.get("captured_frames", 0)
            
            # Key Mapping for UI
            stats["captured"] = curr_frames
            stats["dropped"] = stats.get("dropped_frames", 0)
            # 'written' might not be returned by SDK, estimate it or map it
            # If written_frames exists, use it. Else use captured.
            stats["written"] = stats.get("written_frames", curr_frames)
            
            if dt >= 1.0:
                # Calculate FPS manually over the last ~0.5-1s interval
                manual_fps = (curr_frames - last_frames) / dt
                stats["fps"] = round(manual_fps, 1)
                last_frames = curr_frames
                last_ts = now
            else:
                pass 

            manual_fps = (curr_frames - last_frames) / max(dt, 0.001)
            stats["fps"] = round(manual_fps, 1)
            
            # Try to get file size
            bin_path = os.path.join(output_dir, f"{session_name}.bin")
            if os.path.exists(bin_path):
                 stats["bytes"] = os.path.getsize(bin_path)
            
            # Write atomic
            tmp_file = status_file + ".tmp"
            with open(tmp_file, "w") as f:
                json.dump(stats, f)
            os.replace(tmp_file, status_file)
            
            last_frames = curr_frames
            last_ts = now
            
            # Check for stop signal file (IPC)
            if os.path.exists("stop_signal.txt"):
                print("[Worker] Stop signal detected.")
                stop_requested = True
                try:
                    os.remove("stop_signal.txt")
                except: pass
            
            time.sleep(0.5)

        print("[Worker] Stopping pipeline...")
        pipeline.stop()
        print("[Worker] Pipeline stopped cleanly.")

    except Exception as e:
        print(f"[Worker] Error: {e}")
        # Write error to status
        with open(status_file, "w") as f:
            json.dump({"error": str(e)}, f)
        sys.exit(1)

if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    main()
