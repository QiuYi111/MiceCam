import sys
import time
import os
import json
from PyQt6.QtCore import QThread, pyqtSignal

# Add SDK path (fallback logic similar to gateway)
sdk_paths = [
    os.path.abspath("build/bindings/python/Release"),
    os.path.abspath(r"../../build/bindings/python/Release"),
    r"D:\MiceCam\build\bindings\python\Release"
]
for p in sdk_paths:
    if os.path.exists(p) and p not in sys.path:
        sys.path.insert(0, p)

try:
    import _micecam
    SDK_AVAILABLE = True
except ImportError as e:
    SDK_AVAILABLE = False
    print(f"Failed to load SDK: {e}")

class RecorderThread(QThread):
    # Signals
    stats_updated = pyqtSignal(dict) # {captured, dropped, fps, mbps}
    error_occurred = pyqtSignal(str)
    log_message = pyqtSignal(str)
    finished_recording = pyqtSignal()
    
    def __init__(self, config):
        """
        config: dict with keys:
          - output_dir
          - session_name
          - width, height, fps
          - device_id (int or 'oak')
          - backend ('oak', 'ffmpeg', etc)
        """
        super().__init__()
        self.config = config
        self.pipeline = None
        self.is_running = False
        self._Stop_requested = False

    def run(self):
        if not SDK_AVAILABLE:
            self.error_occurred.emit("MiceCam SDK not loaded.")
            return

        cfg = self.config
        self.log_message.emit(f"Initializing pipeline: {cfg['session_name']} ({cfg['backend']})")
        
        try:
            # Handle Backend Selection
            dev_idx = 0
            if cfg['backend'] != 'oak':
                try: dev_idx = int(cfg['device_id'])
                except: pass
                
            # Pipelines storage
            self.pipelines = []
            self.oak_master = None
            
            if cfg['backend'] == 'oak':
                 self.log_message.emit("Initializing OAK-4P Master device...")
                 try:
                     # Initialize Master
                     self.oak_master = _micecam.OAKMaster()
                     if not self.oak_master.initialize(cfg['width'], cfg['height'], float(cfg['fps'])):
                         raise RuntimeError("Failed to initialize OAK hardware")
                     
                     # Create 4 Proxies
                     for i, suffix in enumerate(['_A', '_B', '_C', '_D']):
                         # Assuming the Python binding supports (output, name, master, socket_idx, w, h, fps, append)
                         # We need to match the signature from recorder_worker.py / module.cpp
                         p = _micecam.Pipeline(
                             cfg['output_dir'], 
                             f"{cfg['session_name']}{suffix}", 
                             self.oak_master, 
                             i, 
                             cfg['width'], cfg['height'], float(cfg['fps']), 
                             False # append not supported in GUI yet
                         )
                         self.pipelines.append(p)
                         
                     self.oak_master.start() # Start hardware
                 except Exception as e:
                     self.error_occurred.emit(f"OAK Hardware Error: {e}")
                     return
            else:
                # Standard single camera
                self.pipeline = _micecam.Pipeline(
                    cfg['output_dir'], 
                    cfg['session_name'], 
                    cfg['backend'], 
                    cfg['width'], cfg['height'], float(cfg['fps']), 
                    dev_idx
                )
                self.pipelines.append(self.pipeline)
            
            # Start all software pipelines
            for p in self.pipelines:
                p.start()
                
            self.is_running = True
            self.log_message.emit(f"Recording started ({len(self.pipelines)} sensors).")
            
            start_time = time.time()
            last_frames = 0
            last_ts = time.time()
            
            while not self._Stop_requested:
                 # Check all
                 if any(not p.is_running() for p in self.pipelines):
                     self.error_occurred.emit("One or more pipelines stopped unexpectedly!")
                     break
                 
                 # Aggregate Stats
                 total_captured = 0
                 total_dropped = 0
                 total_mbps = 0.0
                 
                 for p in self.pipelines:
                     s = p.get_stats()
                     total_captured += s['captured_frames']
                     total_dropped += s['dropped_frames']
                     total_mbps += s['throughput_mbps']
                 
                 # Calc FPS (Aggregate)
                 now = time.time()
                 dt = now - last_ts
                 current_fps = 0.0
                 if dt >= 1.0:
                     msg_fps = (total_captured - last_frames) / dt
                     # Average FPS per sensor for display? Or total? 
                     # Usually user wants to know "Is it 30fps?". 
                     # If we have 4 sensors at 30fps, total is 120. 
                     # Let's show average per sensor.
                     current_fps = msg_fps / len(self.pipelines)
                     last_frames = total_captured
                     last_ts = now
                 
                 # Enrich stats
                 ui_stats = {
                     "captured": total_captured,
                     "dropped": total_dropped,
                     "fps": round(current_fps, 1),
                     "elapsed": now - start_time,
                     "mbps": round(total_mbps, 1)
                 }
                 self.stats_updated.emit(ui_stats)
                 
                 time.sleep(0.1) 
            
            # Stop sequence
            for p in self.pipelines:
                p.stop()
            
            if self.oak_master:
                self.oak_master.stop()
                
            self.log_message.emit("Recording stopped.")
                
        except Exception as e:
            self.error_occurred.emit(f"Critical Error: {str(e)}")
            self.log_message.emit(f"CRASH: {str(e)}")
        
        self.finished_recording.emit()
        self.is_running = False

    def stop(self):
        self._Stop_requested = True
        self.wait()
