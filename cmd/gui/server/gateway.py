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
    logger.info("Gateway loaded SDK")
except ImportError as e:
    SDK_AVAILABLE = False
    logger.warning(f"Gateway could not load SDK: {e}")

# Global State
recorder_process = None
current_session = None # { "name": ..., "output_dir": ..., "auto_decode": ..., "user_stop_intent": False }
last_status = {"is_recording": False}

@app.route('/')
def index():
    return send_from_directory('ui', 'index.html')

@app.route('/favicon.ico')
def favicon():
    return send_from_directory('ui', 'favicon.ico') # Assuming favicon.ico exists in ui/ or handle 404 gracefully
    
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

@app.route('/api/status', methods=['GET'])
def get_status():
    global last_status
    
    # 1. Check if we are in "Recovery Mode" (Crash detected, restarting worker)
    if current_session and current_session.get("is_recovering"):
        # We pretend everything is fine to the UI to avoid reset
        return jsonify({
            "is_recording": True, 
            "status": "recovering",
            "fps": 0,
            "session": current_session["session_name"]
        })

    # 2. Check active worker
    if recorder_process and recorder_process.poll() is None:
        try:
            if os.path.exists("recorder_status.json"):
                with open("recorder_status.json", "r") as f:
                    stats = json.load(f)
                    stats["is_recording"] = True
                    last_status = stats
                    return jsonify(stats)
        except: pass
        return jsonify(last_status)
    else:
        return jsonify({"is_recording": False})

@app.route('/api/decode_progress', methods=['GET'])
def get_decode_progress():
    target_session = request.args.get('session_name')
    try:
        if os.path.exists("decode_progress.json"):
            with open("decode_progress.json", "r") as f:
                data = json.load(f)
                if target_session and data.get("session") != target_session:
                    return jsonify({"status": "idle", "percent": 0})
                return jsonify(data)
    except: pass
    return jsonify({"status": "idle", "percent": 0})

@app.route('/api/cameras', methods=['GET'])
def get_cameras():
    cameras = []
    
    # Try to probe capabilities via SDK
    oak_resolutions = ["1280x800", "1280x720"]
    usb_resolutions = ["1920x1080", "1280x720", "640x480"]
    
    if SDK_AVAILABLE:
        try:
            # Create a temp pipeline to probe (optimized)
            temp_p = _micecam.Pipeline("tmp", "probe", "oak", 1280, 800, 30.0, 0)
            caps = temp_p.get_capabilities()
            oak_resolutions = caps.get("resolutions", oak_resolutions)
            temp_p.stop()
        except: pass

    # Always offer OAK
    cameras.append({
         "index": "oak", 
         "name": "Luxonis OAK-4P (Quad Sync)", 
         "type": "oak", 
         "resolutions": oak_resolutions
    })
    
    if SDK_AVAILABLE and _micecam.has_webcam_support():
        for i in range(2):
            cameras.append({
                "index": i, 
                "name": f"USB Camera {i}", 
                "type": "usb", 
                "resolutions": usb_resolutions
            })
            
    return jsonify(cameras)

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
    
    if backend == "oak":
        width, height = 1280, 800
    else:
        width, height = 1920, 1080

    if data.get('resolution'):
        try: w, h = data['resolution'].split('x'); width, height = int(w), int(h)
        except: pass
    fps = float(data.get('fps', 30))
    # Pass 'oak' or integer string to worker
    worker_dev_idx = str(device_id) 

    current_session = {
        "session_name": session_name,
        "output_dir": output_dir,
        "auto_decode": data.get('auto_decode', False),
        "backend": backend,
        "width": width, "height": height, "fps": fps, "dev_idx": worker_dev_idx,
        "width": width, "height": height, "fps": fps, "dev_idx": worker_dev_idx,
        "user_stop_intent": False,
        "restart_count": 0,
        "is_recovering": False
    }
    
    launch_worker()
    return jsonify({"success": True})

def launch_worker():
    global recorder_process, current_session
    cfg = current_session
    cmd = [
        sys.executable, "recorder_worker.py",
        cfg['output_dir'], cfg['session_name'], cfg['backend'],
        str(cfg['width']), str(cfg['height']), str(cfg['fps']), str(cfg['dev_idx'])
    ]
    
    if cfg['restart_count'] > 0:
        cmd.append("--append") # Logic for worker to resume bin file

    if os.path.exists("stop_signal.txt"): os.remove("stop_signal.txt")
    
    env = get_sdk_env()
    log_file = open(f"worker_{cfg['session_name']}.log", "a")
    recorder_process = subprocess.Popen(cmd, env=env, stdout=log_file, stderr=subprocess.STDOUT)
    logger.info(f"Launched worker [Restart: {cfg['restart_count']}]: {cfg['session_name']}")

@app.route('/api/stop', methods=['POST'])
def stop_recording():
    global current_session
    if current_session:
        current_session["user_stop_intent"] = True
    with open("stop_signal.txt", "w") as f:
        f.write("STOP")
    return jsonify({"success": True})

def monitor_loop():
    global recorder_process, current_session
    while True:
        if recorder_process:
            ret = recorder_process.poll()
            if ret is not None:
                logger.info(f"Worker exited with code {ret}")
                
                # Check if it was a crash or intentional
                if current_session and not current_session.get("user_stop_intent") and ret != 0:
                    if current_session["restart_count"] < 5: # Increased limit
                        logger.warning(f"CRASH DETECTED. Restarting session {current_session['session_name']}...")
                        current_session["restart_count"] += 1
                        current_session["is_recovering"] = True # ENTER RECOVERY MODE
                        
                        launch_worker()
                        
                        # Give it a moment to spin up before clearing recovery flag
                        # Ideally, we should check if process is stable, but a sleep helps for now
                        time.sleep(1.0) 
                        current_session["is_recovering"] = False
                        continue # Skip cleanup
                    else:
                        logger.error("Maximum restarts reached. Stopping.")

                # Clean exit or unrecoverable crash
                if current_session and current_session.get("user_stop_intent") and current_session.get("auto_decode"):
                    logger.info("Intentional stop. Triggering auto-decode...")
                    cfg = current_session
                    cmd_dec = [sys.executable, "decoder.py", cfg['output_dir'], cfg['session_name']]
                    start_decoder(cmd_dec)
                
                recorder_process = None
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
