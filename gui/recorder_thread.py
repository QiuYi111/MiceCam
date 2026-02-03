import sys
import os
import time
import json
from PyQt6.QtCore import QObject, pyqtSignal, QProcess, QByteArray, QStandardPaths

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
        # We use the "Dispatcher Pattern" where we call the main exe with --worker
        # This works for both Python source and PyInstaller Frozen EXE.
        
        exe_path = sys.executable
        
        # 2. Build Arguments
        # call: <exe> --worker <output_dir> <session_name> <backend> <width> <height> <fps> <dev_idx>
        
        script_arg = []
        if not getattr(sys, 'frozen', False):
             # In source mode, sys.executable is python.exe
             # We need to pass the script name 'micecam_app.py'
             # Assuming micecam_app.py is in cwd or we find it
             app_script = os.path.join(os.getcwd(), "micecam_app.py")
             if not os.path.exists(app_script):
                 app_script = "micecam_app.py" # Hope
             script_arg = [app_script]
             
        args = script_arg + [
            "--worker",
            cfg['output_dir'],
            cfg['session_name'],
            cfg['backend'],
            str(cfg['width']),
            str(cfg['height']),
            str(cfg['fps']),
            str(cfg['device_id'])
        ]
        
        self.log_message.emit(f"Spawning Worker: {exe_path} {args}")
        self.process = QProcess()
        
        # --- FIX: Clean Environment for Child Process ---
        # PyInstaller bundled apps sometimes inherit env vars that confuse the child process
        # specifically if it's the same executable spawning itself.
        env = QProcess.systemEnvironment()
        clean_env = []
        for e in env:
            # Filtering out PYTHON vars is crucial if running from a messy environment
            # but usually PyInstaller handles this. However, to be safe:
            if e.startswith("PYTHONHOME=") or e.startswith("PYTHONPATH="):
                continue
            clean_env.append(e)
            
        self.process.setEnvironment(clean_env)
        # ---------------------------------------------
        
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
        data = self.process.readAllStandardOutput().data().decode('utf-8', errors='ignore')
        for line in data.splitlines():
            line = line.strip()
            if not line: continue
            
            if line.startswith("STATUS_UPDATE:"):
                try:
                    json_str = line[len("STATUS_UPDATE:"):]
                    stats = json.loads(json_str)
                    
                    # Transform to UI Format if needed
                    # worker sends: captured, dropped, written, elapsed_seconds, is_recording, fps
                    ui_stats = {
                        "captured": stats.get("captured", 0),
                        "dropped": stats.get("dropped", 0),
                        "fps": stats.get("fps", 0.0),
                        "elapsed": stats.get("elapsed_seconds", 0.0),
                        "mbps": 0.0 # Worker doesn't calculate this yet?
                    }
                    self.stats_updated.emit(ui_stats)
                except Exception as e:
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
