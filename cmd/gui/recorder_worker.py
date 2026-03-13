import sys
import os
from pathlib import Path


def configure_extension_search_path():
    base_dir = Path(__file__).resolve().parent
    if getattr(sys, "frozen", False):
        exe_dir = Path(sys.executable).resolve().parent
        packaged_dirs = [
            exe_dir / "_internal",
            Path(getattr(sys, "_MEIPASS", "")) / "_internal" if getattr(sys, "_MEIPASS", None) else None,
            Path(getattr(sys, "_MEIPASS", "")) if getattr(sys, "_MEIPASS", None) else None,
            exe_dir,
        ]

        for candidate in packaged_dirs:
            if not candidate or not candidate.exists():
                continue
            candidate_str = str(candidate)
            if candidate_str not in sys.path:
                sys.path.insert(0, candidate_str)
            try:
                os.add_dll_directory(candidate_str)
            except (AttributeError, FileNotFoundError):
                pass
        return

    # Make source-mode execution robust across VS multi-config builds by looking
    # for the compiled extension in the common Release/Debug output directories.
    project_root = base_dir.parent.parent
    candidates = [
        project_root / "build-codex-oak-py314" / "bindings" / "python" / "Release",
        project_root / "build-feat-winpkg" / "bindings" / "python" / "Release",
        project_root / "build" / "bindings" / "python" / "Release",
        project_root / "build" / "bindings" / "python" / "Debug",
        project_root / "build" / "bindings" / "python",
        project_root / "build-native-win230" / "bindings" / "python" / "Release",
        project_root / "build-native-win4" / "bindings" / "python" / "Release",
    ]

    for candidate in candidates:
        candidate_str = str(candidate)
        if candidate.exists() and candidate_str not in sys.path:
            sys.path.append(candidate_str)


configure_extension_search_path()

import time
import json
import threading
import signal
import _micecam
import base64
import queue

# Inject Debug Logger
try:
    import numpy as np
    NUMPY_AVAILABLE = True
except ImportError:
    NUMPY_AVAILABLE = False

try:
    import debug_utils
    debug_utils.hook_exceptions()
    debug_utils.log("WORKER", "Module Loaded")
    debug_utils.log("WORKER", f"_micecam from: {getattr(_micecam, '__file__', 'unknown')}")
except ImportError:
    # Fallback if debug_utils not found (should be there)
    pass

# Global flags
stop_requested = False

def signal_handler(sig, frame):
    global stop_requested
    if 'debug_utils' in globals(): debug_utils.log("WORKER", f"Signal Received: {sig}")
    stop_requested = True

preview_queue = queue.Queue(maxsize=1)
PREVIEW_ENABLED = os.environ.get("MICECAM_ENABLE_PREVIEW", "0") == "1"

def preview_worker_loop():
    while not stop_requested:
        try:
            # Wait for a frame to encode, timeout to check stop_requested
            preview_data = preview_queue.get(timeout=0.5)
            # preview_data is (width, height, bytes)
            w, h, raw_bytes, cam_idx = preview_data

            b64_img = base64.b64encode(raw_bytes).decode('ascii')
            msg = {
                "image": b64_img,
                "width": w,
                "height": h,
                "index": cam_idx,
                "format": "raw" if len(raw_bytes) == w * h else "jpeg"
            }
            print(f"PREVIEW_UPDATE:{json.dumps(msg)}", flush=True)

        except queue.Empty:
            continue
        except Exception as e:
            if 'debug_utils' in globals(): debug_utils.log("WORKER", f"Preview Encode Error: {e}")

def attach_preview_callback(pipeline, cam_idx, target_fps=5.0):
    if not NUMPY_AVAILABLE:
        return
    min_interval = 1.0 / target_fps
    last_time = [0.0]  # mutable state for callback

    def callback(data_view, seq_id, timestamp):
        now = time.time()
        if now - last_time[0] < min_interval:
            return

        try:
            # We assume NV12 or similar format where the first W*H bytes are the Y (luma/grayscale) channel.
            # Convert memoryview to numpy array. The pipeline width/height is needed, but we can infer
            # or just take the length. Actually, without knowing the exact width/height in the callback, it's tricky.
            # But we can get pipeline stats or assume from config.
            # Better trick: we passed W and H to the pipeline, so we know the size.
            pass
        except Exception:
            pass

    # That approach requires W/H inside the callback. We'll implement it inline.

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

    # Ensure output directory exists (Fix for Error 3: Path not found)
    try:
        if not os.path.exists(output_dir):
            print(f"[Worker] Creating output directory: {output_dir}")
            os.makedirs(output_dir, exist_ok=True)
    except Exception as e:
        print(f"[Worker] Error creating output directory: {e}")
        # Continue and let C++ fail if it must, or return?
        # Proceeding might result in the same error but let's hope it works.

    if 'debug_utils' in globals():
        debug_utils.log_environment()
        debug_utils.log("WORKER", f"Args: {sys.argv}")

    print(f"[Worker] Starting recording session: {session_name} ({backend}) {'(Append Mode)' if append_mode else ''}")

    status_file = "recorder_status.json"
    pipelines = []
    oak_master = None

    if 'debug_utils' in globals(): debug_utils.log("WORKER", "Configuring Pipelines...")

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

        if PREVIEW_ENABLED:
            preview_thread = threading.Thread(target=preview_worker_loop, daemon=True)
            preview_thread.start()

        for i, p in enumerate(pipelines):
            if PREVIEW_ENABLED:
                # Attach a callback to extract preview frames
                min_interval = 1.0 / 5.0 # 5 FPS preview
                last_time = {"t": 0.0}

                # We need to capture variables
                def make_cb(cam_index, w, h, t_state):
                    def cb(data_view, seq_id, timestamp):
                        now = time.time()
                        if now - t_state["t"] < min_interval:
                            return
                        t_state["t"] = now

                        try:
                            # The stream may be MJPEG or Raw YUV
                            raw_data = data_view.tobytes()

                            # Detect format
                            is_jpeg = raw_data.startswith(b'\xff\xd8')

                            if is_jpeg:
                                # Use full JPEG for preview (compressed)
                                final_bytes = raw_data
                                final_w, final_h = w, h
                            elif NUMPY_AVAILABLE:
                                # Handle Raw YUV - Extract Y Channel
                                arr = np.frombuffer(raw_data, dtype=np.uint8)
                                expected_len = w * h

                                # UYVY or YUYV (2 bytes per pixel)
                                if len(arr) == expected_len * 2:
                                    # UYVY: Y is at index 1, 3, 5...
                                    # YUYV: Y is at index 0, 2, 4...
                                    # Based on header 7E 99 80 9A -> U Y V Y likely
                                    y_channel = arr[1::2].reshape((h, w))
                                elif len(arr) >= expected_len:
                                    # NV12 or similar (Y is the first W*H bytes)
                                    y_channel = arr[:expected_len].reshape((h, w))
                                else:
                                    return # Unknown or partial frame

                                # Downsample to save CPU/IPC
                                scale = max(1, w // 320)
                                small_y = y_channel[::scale, ::scale]
                                final_h, final_w = small_y.shape
                                if not small_y.flags['C_CONTIGUOUS']:
                                    small_y = np.ascontiguousarray(small_y)
                                final_bytes = small_y.tobytes()
                            else:
                                return # Cannot handle raw without numpy

                            # Put to queue, replace if full
                            try:
                                preview_queue.put_nowait((final_w, final_h, final_bytes, cam_index))
                            except queue.Full:
                                # drain and replace
                                try:
                                    preview_queue.get_nowait()
                                except:
                                    pass
                                try:
                                    preview_queue.put_nowait((final_w, final_h, final_bytes, cam_index))
                                except:
                                    pass

                        except Exception:
                            pass
                    return cb

                p.attach_callback(make_cb(i, width, height, last_time))

            p.start()

        print(f"[Worker] {len(pipelines)} pipeline(s) started.")
        start_time = time.time()

        # 3. Monitor Loop
        last_frames = 0
        last_ts = time.time()

        # Throughput tracking
        last_total_bytes = 0

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

            # Calculate Total Size & Throughput
            current_total_bytes = 0
            file_extensions = ['.bin', '.mp4', '.mkv', '.avi'] # Possible containers
            # We know the specific filenames from pipeline creation
            # Single: session_name + ext? pipeline adds extensions internally usually.
            # OAK: session_name_A.bin ...

            # Robust Strategy: Check the expected filenames
            expected_files = []
            if backend == "oak":
                suffixes = ['_A', '_B', '_C', '_D']
                # The C++ pipeline appends .bin if raw, or .mp4 if encoded.
                # Assuming .bin for now based on code history "Failed to open... .bin"
                for s in suffixes:
                    expected_files.append(os.path.join(output_dir, f"{session_name}{s}.bin"))
            else:
                 # Standard
                 expected_files.append(os.path.join(output_dir, f"{session_name}.bin"))
                 expected_files.append(os.path.join(output_dir, f"{session_name}.mp4"))

            for fpath in expected_files:
                try:
                    if os.path.exists(fpath):
                        current_total_bytes += os.path.getsize(fpath)
                except: pass

            total_mb = current_total_bytes / (1024 * 1024)

            # Enrich for UI
            summary_stats = {
                "captured": captured,
                "dropped": dropped,
                "written": captured,
                "elapsed_seconds": now - start_time,
                "is_recording": True,
                "mb": round(total_mb, 2)
            }

            if dt >= 1.0:
                fps = round((captured - last_frames) / dt, 1) # Total FPS across all streams? Or avg?
                # If 4 cameras running at 30fps, captured increases by 120 per second.
                # User usually expects "per camera" or "system total"?
                # UI label says "FPS". If it shows 120 for 30fps setup, that's fine.
                # But let's average it for clarity if OAK? No, sum is better for "System Load".
                # Actually, main_window passes fps arg as float.
                if len(pipelines) > 1:
                     summary_stats["fps"] = round((captured - last_frames) / (dt * len(pipelines)), 1)
                else:
                     summary_stats["fps"] = fps

                stats_bytes = current_total_bytes - last_total_bytes
                mbps = (stats_bytes * 8) / (1000 * 1000) / dt # Megabits per second
                summary_stats["mbps"] = round(mbps, 2)

                last_frames = captured
                last_ts = now
                last_total_bytes = current_total_bytes

                # Update status file atomically
                with open(status_file + ".tmp", "w") as f:
                    json.dump(summary_stats, f)
                os.replace(status_file + ".tmp", status_file)

                # IPC for Qt (Print to stdout)
                print(f"STATUS_UPDATE:{json.dumps(summary_stats)}", flush=True)

            stop_file = os.environ.get("MICECAM_STOP_FILE", "stop_signal.txt")
            if os.path.exists(stop_file):
                stop_requested = True
                try: os.remove(stop_file)
                except: pass
            # Legacy fallback
            elif os.path.exists("stop_signal.txt"):
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
        if sys.stdout is not None:
             sys.stdout.reconfigure(line_buffering=True)
        if sys.stderr is not None:
             sys.stderr.reconfigure(line_buffering=True)

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    main()
