import sys
import os
import time
import json
from PyQt6.QtCore import QObject, pyqtSignal, QProcess, QByteArray, QStandardPaths, QProcessEnvironment

# Check if worker script exists to define "Availability"
# In a process-isolated architecture, "SDK Available" means "Can launch worker"
SDK_AVAILABLE = True 
if not os.path.exists("recorder_worker.py") and not getattr(sys, 'frozen', False):
    # Try one level up?
    if not os.path.exists(os.path.join("..", "recorder_worker.py")):
         pass # Assume True for now, start() will validate path


class RecorderThread(QObject):
    """
    QProcess-based 'Thread' that acts as a Supervisor for the actual worker process.
    This provides Process Isolation: If C++ crashes, the UI stays alive.
    """
    # Signals (Same Interface as before)
    stats_updated = pyqtSignal(dict) 
    error_occurred = pyqtSignal(str)
    log_message = pyqtSignal(str)
    finished_recording = pyqtSignal()
    
    def __init__(self, config):
        """
        config dict:
          - output_dir
          - session_name
          - width, height, fps
          - device_id (int or 'oak')
          - backend ('oak' or 'ffmpeg')
        """
        super().__init__()
        self.config = config
        self.process = None
        self._is_running = False

    def start(self):
        if self._is_running: return
        self._is_running = True
        
        cfg = self.config
        
        # 1. Resolve Execution Strategy
        # DUAL BINARY MODE:
        # If frozen, we look for 'MiceCamWorker.exe' in the same directory.
        # If not frozen (source), we run 'python recorder_worker.py'.
        
        exe_path = sys.executable
        script_arg = []
        is_frozen = getattr(sys, 'frozen', False)

        if is_frozen:
            # We are in frozen mode.
            # Strategy: Look for worker in 'tools' subdirectory (Distribution Mode)
            # or parallel directories (Dev/Build Mode)
            
            base_dir = os.path.dirname(exe_path) 
            
            # 1. Distribution Mode: base/tools/MiceCamWorker/MiceCamWorker.exe
            # (Assuming we put the whole worker dir inside 'tools')
            p1 = os.path.join(base_dir, "tools", "MiceCamWorker", "MiceCamWorker.exe")
            
            # 2. Parallel Build Mode: base/../MiceCamWorker/MiceCamWorker.exe
            p2 = os.path.join(base_dir, "..", "MiceCamWorker", "MiceCamWorker.exe")
            p2 = os.path.abspath(p2)

            if os.path.exists(p1):
                exe_path = p1
            elif os.path.exists(p2):
                exe_path = p2
            else:
                self.log_message.emit(f"Critial: Worker not found at {p1} or {p2}")
                script_arg = ["--worker"] 
        else:
             # In source mode, sys.executable is python.exe
             # We need to pass the script name 'recorder_worker.py' direclty
             # In source mode, sys.executable is python.exe
             # Use the same directory as this script to find recorder_worker.py
             base_dir = os.path.dirname(os.path.abspath(__file__))
             worker_script = os.path.join(base_dir, "..", "recorder_worker.py")
             if not os.path.exists(worker_script):
                 worker_script = os.path.join(base_dir, "recorder_worker.py")
             
             exe_path = sys.executable
             script_arg = [worker_script]
             
        # 2. Build Arguments
        # If using MiceCamWorker.exe, we just pass the args it expects (no --worker flag if it was built from recorder_worker.py directly)
        
        # Check if we are running the Worker EXE directly or dispatching
        # If exe_path ends with MiceCamWorker.exe, it is the direct entry point.
        
        # recorder_worker.py args: <output_dir> <session_name> <backend> <width> <height> <fps> <dev_idx>
        
        real_args = [
            cfg['output_dir'],
            cfg['session_name'],
            cfg['backend'],
            str(cfg['width']),
            str(cfg['height']),
            str(cfg['fps']),
            str(cfg['device_id'])
        ]
        
        # If we are dispatching via micecam_app.py (fallback), we need --worker
        if "MiceCamWorker" not in exe_path and not is_frozen:
             # Source mode using recorder_worker.py directly -> No --worker needed
             pass
        elif "MiceCamWorker" not in exe_path and is_frozen:
             # Fallback self-spawn -> Needs --worker
             real_args = ["--worker"] + real_args

        args = script_arg + real_args
        
        self.log_message.emit(f"Spawning Worker: {exe_path} {args}")
        self.process = QProcess()
        
        # Environmental Cleaning (Still good practice)
        # Environmental Cleaning (Still good practice)
        # Fix: Use QProcessEnvironment for PyQt6 compliance
        env = QProcessEnvironment.systemEnvironment()
        
        # We want to filter out python vars. 
        # QProcessEnvironment doesn't support easy filtering (it's a map), 
        # but we can remove specific keys if they exist.
        env.remove("PYTHONHOME")
        env.remove("PYTHONPATH")
        
        # If we wanted to copy from a clean dict, we'd iterate and insert, 
        # but systemEnvironment() gives us everything. 
        # Strict isolation is achieved by the separate executable anyway.
            
        self.process.setProcessEnvironment(env)
        
        self.process.readyReadStandardOutput.connect(self.handle_stdout)
        self.process.readyReadStandardError.connect(self.handle_stderr)
        self.process.finished.connect(self.handle_finished)
        
        self.process.start(exe_path, args)
        
    def stop(self):
        if self.process and self.process.state() == QProcess.ProcessState.Running:
            self.log_message.emit("Sending Stop Signal...")
            
            # Method A: Create stop_signal.txt (Universal)
            try:
                with open("stop_signal.txt", "w") as f:
                    f.write("STOP")
            except Exception as e:
                self.log_message.emit(f"Failed to write stop signal: {e}")
                # Fallback: Terminate
                self.process.terminate()
        else:
            self.finished_recording.emit()

    def handle_stdout(self):
        data_bytes = self.process.readAllStandardOutput().data()
        if not data_bytes: return
        
        try:
             # Use errors='replace' to avoid crashing on split multi-byte characters
             chunk = data_bytes.decode('utf-8', errors='replace')
        except:
             return 

        # Initialize buffer if missing (legacy safety)
        if not hasattr(self, '_buffer'): self._buffer = ""
        self._buffer += chunk
        
        while "\n" in self._buffer:
            line, self._buffer = self._buffer.split("\n", 1)
            line = line.strip()
            if not line: continue
            
            if line.startswith("STATUS_UPDATE:"):
                try:
                    json_str = line[len("STATUS_UPDATE:"):]
                    stats = json.loads(json_str)
                    
                    # Transform to UI Format if needed
                    # worker sends: captured, dropped, written, elapsed_seconds, is_recording, fps, mbps, mb
                    ui_stats = {
                        "captured": stats.get("captured", 0),
                        "dropped": stats.get("dropped", 0),
                        "fps": stats.get("fps", 0.0),
                        "elapsed": stats.get("elapsed_seconds", 0.0),
                        "mbps": stats.get("mbps", 0.0),
                        "mb": stats.get("mb", 0.0)
                    }
                    self.stats_updated.emit(ui_stats)
                except Exception as e:
                     # Log parsing errors to console
                    print(f"Failed to parse status: {e}")
            else:
                # Forward generic logs to UI
                self.log_message.emit(f"[Worker] {line}")

    def handle_stderr(self):
        data = self.process.readAllStandardError().data().decode('utf-8', errors='ignore')
        for line in data.splitlines():
             self.log_message.emit(f"[Worker/ERR] {line}")

    def handle_finished(self, exit_code, exit_status):
        self._is_running = False
        if exit_code != 0:
            self.error_occurred.emit(f"Worker crashed with code {exit_code}")
            self.log_message.emit(f"CRASH: Worker process died (Exit {exit_code})")
        else:
            self.log_message.emit("Worker finished normally.")
            
        self.finished_recording.emit()
