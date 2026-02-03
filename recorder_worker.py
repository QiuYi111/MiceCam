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
    dev_str = sys.argv[7]
    append_mode = "--append" in sys.argv
    
    # dev_idx is an int for USB, but irrelevant for OAK quad-sync master
    dev_idx = 0
    try: dev_idx = int(dev_str)
    except: pass

    print(f"[Worker] Starting recording session: {session_name} ({backend}) {'(Append Mode)' if append_mode else ''}")
    
    status_file = "recorder_status.json"
    pipelines = []
    oak_master = None
    
    try:
        # 1. Pipeline Initialization
        if backend == "oak":
            # Start 4 synchronized pipelines for OAK-4P using a single Master
            print("[Worker] Initializing OAK-4P Master device...")
            try:
                oak_master = _micecam.OAKMaster()
                if not oak_master.initialize(width, height, fps):
                    raise RuntimeError("Failed to initialize OAK hardware")
                
                print("[Worker] Spawning synchronized proxy pipelines...")
                for i, suffix in enumerate(['_A', '_B', '_C', '_D']):
                    p = _micecam.Pipeline(output_dir, f"{session_name}{suffix}", 
                                         oak_master, i, width, height, fps, append_mode)
                    pipelines.append(p)
                
                # Start the actual hardware capture
                oak_master.start()
            except Exception as e:
                print(f"[Worker] CRITICAL ERROR: Hardware sync failed: {e}")
                raise
        else:
            # Standard single camera (USB/FFmpeg)
            p = _micecam.Pipeline(output_dir, session_name, backend, width, height, fps, dev_idx, append_mode)
            pipelines.append(p)
        for p in pipelines:
            p.start()
        
        print(f"[Worker] {len(pipelines)} pipeline(s) started.")
        start_time = time.time()
        
        # 3. Monitor Loop
        last_frames = 0
        last_ts = time.time()
        
        while not stop_requested:
            # Check if any pipeline stopped unexpectedly
            if any(not p.is_running() for p in pipelines):
                print("[Worker] Warning: One or more pipelines stopped unexpectedly")
                break
                
            now = time.time()
            dt = now - last_ts
            
            # Aggregate stats
            all_stats = [p.get_stats() for p in pipelines]
            captured = sum(s.get("captured_frames", 0) for s in all_stats)
            dropped = sum(s.get("dropped_frames", 0) for s in all_stats)
            
            # Enrich for UI
            summary_stats = {
                "captured": captured,
                "dropped": dropped,
                "written": captured, # Approx
                "elapsed_seconds": now - start_time,
                "is_recording": True
            }

            if dt >= 1.0:
                summary_stats["fps"] = round((captured - last_frames) / (dt * len(pipelines)), 1)
                last_frames = captured
                last_ts = now
                
                # Update status file atomically
                with open(status_file + ".tmp", "w") as f:
                    json.dump(summary_stats, f)
                os.replace(status_file + ".tmp", status_file)
                
                # IPC for Qt (Print to stdout)
                print(f"STATUS_UPDATE:{json.dumps(summary_stats)}", flush=True)

            if os.path.exists("stop_signal.txt"):
                stop_requested = True
                try: os.remove("stop_signal.txt")
                except: pass
            
            time.sleep(0.2)

        print("[Worker] Stopping pipelines...")
        for p in pipelines:
            try: p.stop()
            except: pass
        print("[Worker] Exit success.")

    except Exception as e:
        print(f"[Worker] CRITICAL ERROR: {e}", flush=True)
        # Do NOT write "is_recording": False to the status file here.
        # The Gateway (supervisor) is responsible for deciding if the session ends 
        # or if we should restart. Writing False here causes the UI to flicker/reset.
        # We only log the error.
        sys.exit(1)

if __name__ == "__main__":
    # Force unbuffered output for better logging
    if sys.version_info >= (3, 7):
        sys.stdout.reconfigure(line_buffering=True)
        sys.stderr.reconfigure(line_buffering=True)
        
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    main()
