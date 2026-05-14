import os
import time
import threading
import json
import logging
from flask import Flask, request, jsonify, send_from_directory

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler("server.log"),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger(__name__)

app = Flask(__name__, static_folder='ui', static_url_path='')

# Try to import micecam SDK
try:
    import _micecam
    SDK_AVAILABLE = True
    logger.info(f"MiceCam SDK loaded. Version: {_micecam.__version__}")
except ImportError as e:
    SDK_AVAILABLE = False
    logger.error(f"Failed to load micecam SDK: {e}")
    # Mock for UI dev
    class MockPipeline:
        def __init__(self, *args, **kwargs): pass
        def start(self): pass
        def stop(self): pass
        def get_stats(self): return {"captured_frames": 0, "current_throughput_mbps": 0.0}
        def is_running(self): return True

# Global state
import sys
# Add parent dir to path to find micecam_utils
sys.path.append(os.path.join(os.path.dirname(__file__), ".."))
# Add root dir to path to find micecam package
sys.path.append(os.path.join(os.path.dirname(__file__), "..", ".."))

import micecam_utils # Ensure this is in the same dir

# Global state
pipeline = None
pipeline_lock = threading.Lock()
start_time = 0
current_bin_path = None
current_session_config = {} # Store session details for decoding

@app.route('/')
def index():
    return send_from_directory('ui', 'index.html')

@app.route('/api/cameras', methods=['GET'])
def get_cameras():
    cameras = []

    # Check SDK capabilities
    if SDK_AVAILABLE:
        # 1. Check for OAK
        if _micecam.has_oak_support():
             try:
                 cameras.append({
                     "index": "oak",
                     "name": "Luxonis OAK-4P (Hardware Sync)",
                     "type": "oak",
                     "resolutions": ["1920x1080", "1280x720", "1280x800"]
                 })
             except Exception as e:
                 logger.warning(f"Error checking OAK: {e}")

        # 2. Check for USB (FFmpeg)
        if _micecam.has_webcam_support():
            for i in range(2):
                cameras.append({
                    "index": i,
                    "name": f"USB Camera Device {i}",
                    "type": "usb",
                    "resolutions": ["3840x2160", "1920x1080", "1280x720"]
                })

    return jsonify(cameras)

@app.route('/api/start', methods=['POST'])
def start_recording():
    global pipeline, start_time, current_bin_path, current_session_config
    if not SDK_AVAILABLE:
        return jsonify({"success": False, "error": "SDK not available"}), 500

    data = request.json
    session_name = data.get('session_name') or f"session_{int(time.time())}"
    device_id = data.get('device_index', 0)
    backend = "oak" if device_id == "oak" else "ffmpeg"
    auto_decode = data.get('auto_decode', False)

    # ... (resolution parsing omitted)
    width, height = 1920, 1080
    if data.get('resolution'):
        try:
            w, h = data['resolution'].split('x')
            width, height = int(w), int(h)
        except:
             logger.warning(f"Invalid resolution format: {data.get('resolution')}")

    fps = float(data.get('fps', 30))
    output_dir = data.get('output_dir') or 'recordings'

    # Ensure raw output dir
    if not os.path.exists(output_dir):
        try:
            os.makedirs(output_dir)
        except:
            pass

    # Track the binary file path for stats
    current_bin_path = os.path.join(output_dir, f"{session_name}.bin")
    current_session_config = {
        "output_dir": output_dir,
        "session_name": session_name,
        "auto_decode": auto_decode
    }

    # Save pending job to disk (for Supervisor to pick up if we crash)
    try:
        with open("pending_decode.json", "w") as f:
            json.dump(current_session_config, f)
    except Exception as e:
        logger.error(f"Failed to write pending_decode.json: {e}")

    with pipeline_lock:
        if pipeline and pipeline.is_running():
            return jsonify({"success": False, "error": "Already recording"}), 400

        try:
            # Handle pure integer index for USB
            dev_idx_int = 0
            if backend == "ffmpeg":
                try:
                    dev_idx_int = int(device_id)
                except:
                    dev_idx_int = 0

            logger.info(f"Starting pipeline: {backend} {width}x{height}@{fps}")
            pipeline = _micecam.Pipeline(
                output_dir,
                session_name,
                backend,
                width, height, fps,
                dev_idx_int
            )
            pipeline.start()
            start_time = time.time()
            return jsonify({"success": True})
        except Exception as e:
            logger.error(f"Start failed: {e}")
            return jsonify({"success": False, "error": str(e)}), 500

@app.route('/api/stop', methods=['POST'])
def stop_recording():
    global pipeline, current_session_config
    with pipeline_lock:
        if pipeline:
            try:
                logger.info("Stopping pipeline...")
                pipeline.stop()
                logger.info("Pipeline stopped.")
                pipeline = None

                # Try to spawn decoder immediately (if we don't crash)
                if current_session_config.get("auto_decode"):
                    cfg = current_session_config
                    cmd = [sys.executable, "decoder.py", cfg['output_dir'], cfg['session_name']]
                    logger.info(f"Spawning decoder: {cmd}")
                    # Log output to file for debugging
                    with open("decoder_debug.log", "a") as log_file:
                        subprocess.Popen(cmd, stdout=log_file, stderr=subprocess.STDOUT)

                return jsonify({"success": True})
            except Exception as e:
                logger.error(f"Stop failed: {e}")
                return jsonify({"success": False, "error": str(e)}), 500
        return jsonify({"success": True, "message": "Not running"})

@app.route('/api/status', methods=['GET'])
def get_status():
    global pipeline, current_bin_path
    if pipeline:
        try:
            stats = pipeline.get_stats()
            stats['is_recording'] = True
            elapsed = time.time() - start_time
            if elapsed < 0.001: elapsed = 1.0 # avoid div/0

            captured = stats['captured_frames']
            dropped = stats['dropped_frames']
            written = captured - dropped # Approximate

            # FPS
            fps_val = captured / elapsed

            # Real file size
            file_size_mb = 0.0
            try:
                if current_bin_path and os.path.exists(current_bin_path):
                    file_size_mb = os.path.getsize(current_bin_path) / (1024 * 1024)
            except:
                pass

            return jsonify({
                "is_recording": True,
                "captured": captured,
                "dropped": dropped,
                "written": written,
                "bytes": file_size_mb * 1000000, # UI divides by 1M to get MB
                "fps": fps_val,
                "elapsed_seconds": elapsed
            })
        except Exception as e:
             logger.error(f"Status check failed: {e}")
             return jsonify({"is_recording": False})
    return jsonify({"is_recording": False})

@app.route('/api/decode_progress', methods=['GET'])
def get_decode_progress():
    try:
        if os.path.exists("decode_progress.json"):
            with open("decode_progress.json", "r") as f:
                return jsonify(json.load(f))
    except:
        pass
    return jsonify({"status": "idle", "percent": 0})

if __name__ == '__main__':
    print("🐭 MiceCam UI Server running at http://127.0.0.1:18080")
    app.run(host='127.0.0.1', port=18080)
