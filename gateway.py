import os
import time
import json
import threading
import subprocess
import sys
import logging
from flask import Flask, request, jsonify, send_from_directory

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

app = Flask(__name__, static_folder='ui', static_url_path='')

# Global State
recorder_process = None
current_session = None # { "name": ..., "output_dir": ..., "auto_decode": ... }
last_status = {"is_recording": False}

# Ensure SDK is loadable
possible_sdk_paths = [
    os.path.abspath("build/bindings/python/Release"),
    r"D:\MiceCam\build\bindings\python\Release"
]
for p in possible_sdk_paths:
    if os.path.exists(p) and p not in sys.path:
        sys.path.insert(0, p)

# Try to import for capability checking only
try:
    import _micecam
    SDK_AVAILABLE = True
    logger.info(f"Gateway loaded SDK version: {_micecam.__version__ if hasattr(_micecam, '__version__') else 'unknown'}")
except ImportError as e:
    SDK_AVAILABLE = False
    logger.warning(f"Gateway could not load SDK: {e}")

@app.route('/')
def index():
    return send_from_directory('ui', 'index.html')

@app.route('/api/cameras', methods=['GET'])
def get_cameras():
    cameras = []
    
    # Always offer OAK (Force enable)
    cameras.append({
         "index": "oak", 
         "name": "Luxonis OAK-4P", 
         "type": "oak", 
         "resolutions": ["3840x2160", "1920x1080", "1280x800", "1280x720"]
    })
    
    if SDK_AVAILABLE and _micecam.has_webcam_support():
        for i in range(2):
            cameras.append({
                "index": i, 
                "name": f"USB Camera {i}", 
                "type": "usb", 
                "resolutions": ["3840x2160", "1920x1080", "1280x720"]
            })
    elif not SDK_AVAILABLE:
        # Fallback for when SDK fails to load - still show something
        cameras.append({
            "index": 0, "name": "USB Camera 0 (Fallback)", "type": "usb", "resolutions": ["1920x1080"]
        })
        
    return jsonify(cameras)

def get_sdk_env():
    env = os.environ.copy()
    sdk_path = os.path.abspath("build/bindings/python/Release")
    if not os.path.exists(sdk_path):
         sdk_path = r"D:\MiceCam\build\bindings\python\Release"
    
    if "PYTHONPATH" in env:
        env["PYTHONPATH"] = sdk_path + os.pathsep + env["PYTHONPATH"]
    else:
        env["PYTHONPATH"] = sdk_path
    return env

@app.route('/api/start', methods=['POST'])
def start_recording():
    global recorder_process, current_session, last_status
    
    if recorder_process and recorder_process.poll() is None:
        return jsonify({"success": False, "error": "Already recording"}), 400
        
    data = request.json
    session_name = data.get('session_name') or f"session_{int(time.time())}"
    output_dir = data.get('output_dir') or 'recordings'
    
    if not os.path.exists(output_dir): 
        try: os.makedirs(output_dir)
        except: pass
        
    device_id = data.get('device_index', 0)
    backend = "oak" if device_id == "oak" else "ffmpeg"
    width, height = 1920, 1080
    if data.get('resolution'):
        try: w, h = data['resolution'].split('x'); width, height = int(w), int(h)
        except: pass
    fps = float(data.get('fps', 30))
    dev_idx_int = 0
    try: dev_idx_int = int(device_id)
    except: pass

    cmd = [
        sys.executable, "recorder_worker.py",
        output_dir, session_name, backend,
        str(width), str(height), str(fps), str(dev_idx_int)
    ]
    
    current_session = {
        "session_name": session_name,
        "output_dir": output_dir,
        "auto_decode": data.get('auto_decode', False)
    }
    
    if os.path.exists("stop_signal.txt"): os.remove("stop_signal.txt")
    if os.path.exists("recorder_status.json"): os.remove("recorder_status.json")

    logger.info(f"Launching worker: {cmd}")
    
    # Use explicit environment and log file
    env = get_sdk_env()
    log_file = open("worker.log", "w") # Keep open? subprocess will inherit
    # We assign stdout/stderr to this file. Note: This file handle stays open in Gateway? 
    # Python closes it on GC? Better to let Popen handle it or keep it alive?
    # For safety, we'll let Popen use it, but we need to ensure it flushes.
    # Alternatively, use 'a' append mode.
    
    try:
        recorder_process = subprocess.Popen(cmd, env=env, stdout=log_file, stderr=subprocess.STDOUT)
    except Exception as e:
        logger.error(f"Failed to spawn worker: {e}")
        return jsonify({"success": False, "error": str(e)}), 500
    
    last_status = {"is_recording": True}
    
    return jsonify({"success": True})

@app.route('/api/stop', methods=['POST'])
def stop_recording():
    with open("stop_signal.txt", "w") as f:
        f.write("STOP")
    return jsonify({"success": True})

@app.route('/api/status', methods=['GET'])
def get_status():
    global last_status
    if recorder_process and recorder_process.poll() is None:
        try:
            # Retry loops for Windows file locking
            for _ in range(3):
                try:
                    if os.path.exists("recorder_status.json"):
                        with open("recorder_status.json", "r") as f:
                            stats = json.load(f)
                            stats["is_recording"] = True
                            last_status = stats
                            return jsonify(stats)
                except:
                    time.sleep(0.05)
                    continue
        except: pass
        return jsonify(last_status)
    else:
        return jsonify({"is_recording": False})

@app.route('/api/decode_progress', methods=['GET'])
def get_decode_progress():
    target_session = request.args.get('session_name')
    
    # Retry to handle Windows file locking contention
    for _ in range(5):
        try:
            if os.path.exists("decode_progress.json"):
                with open("decode_progress.json", "r") as f:
                    data = json.load(f)
                    # Filter by session if provided
                    if target_session and data.get("session") != target_session:
                        return jsonify({"status": "idle", "percent": 0, "reason": "session_mismatch"})
                    return jsonify(data)
        except:
            time.sleep(0.05)
            continue
            
    return jsonify({"status": "idle", "percent": 0})

def monitor_loop():
    global recorder_process, current_session
    while True:
        if recorder_process:
            ret = recorder_process.poll()
            if ret is not None:
                logger.info(f"Worker exited with code {ret}")
                recorder_process = None
                
                if current_session and current_session.get("auto_decode"):
                    logger.info("Triggering auto-decode...")
                    cfg = current_session
                    cmd_dec = [sys.executable, "decoder.py", cfg['output_dir'], cfg['session_name']]
                    # Inject env for decoder too, just in case
                    start_decoder(cmd_dec)
                    
                current_session = None
        
        time.sleep(1)

def start_decoder(cmd):
    env = get_sdk_env()
    with open("decoder_service.log", "a") as f:
        subprocess.Popen(cmd, env=env, stdout=f, stderr=subprocess.STDOUT)

if __name__ == "__main__":
    t = threading.Thread(target=monitor_loop, daemon=True)
    t.start()
    print("⛩️  MiceCam Gateway running on port 18080")
    app.run(host='127.0.0.1', port=18080)
