import sys
import os
from PyQt6.QtCore import QThread, pyqtSignal

# Ensure root path is in sys.path to import micecam_utils
current_dir = os.path.dirname(os.path.abspath(__file__)) # gui/
root_dir = os.path.dirname(current_dir) # MiceCam/
if root_dir not in sys.path:
    sys.path.insert(0, root_dir)

try:
    import micecam_utils
except ImportError:
    # If running in frozen EXE, root might be different or bundled
    pass 

class DecoderThread(QThread):
    progress_updated = pyqtSignal(int)
    status_message = pyqtSignal(str)
    finished_decoding = pyqtSignal()
    
    def __init__(self, output_dir, session_name):
        super().__init__()
        self.output_dir = output_dir
        self.session_name = session_name
        
    def run(self):
        self.status_message.emit("Starting decoding...")
        try:
            # Re-import here to be safe in frozen envs? 
            import micecam_utils
            
            def cb(pct):
                self.progress_updated.emit(int(pct))
                
            micecam_utils.decode_micecam_session(
                self.output_dir, 
                self.session_name, 
                progress_callback=cb
            )
            self.status_message.emit("Decoding complete.")
        except Exception as e:
            self.status_message.emit(f"Decoding Error: {str(e)}")
            
        self.finished_decoding.emit()
