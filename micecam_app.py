import sys
import os

# Dispatcher Pattern MUST be at the top to avoid unrelated imports in worker mode
if len(sys.argv) > 1 and sys.argv[1] == "--worker":
    # Run as Recorder Worker
    # We need to adjust sys.argv to allow argparse (if used) or just pass args manually
    # recorder_worker expects: script, out, name, backend, w, h, fps, dev
    # Our QProcess sends: exe, --worker, out, name...
    # So we strip the first 2 args: [exe, --worker]
    sys.argv = [sys.argv[0]] + sys.argv[2:]
    
    try:
         # Dynamically import ONLY what is needed
         import recorder_worker
         recorder_worker.main()
    except Exception as e:
         print(f"Worker Start Error: {e}")
         import traceback
         traceback.print_exc()
         sys.exit(1)
    sys.exit(0)

from PyQt6.QtWidgets import QApplication
from PyQt6.QtGui import QIcon
import qdarktheme
import darkdetect

from gui.main_window import MainWindow


def main():
    # High DPI scaling
    if hasattr(sys, 'set_int_max_str_digits'):
        sys.set_int_max_str_digits(0) 
        
    app = QApplication(sys.argv)
    
    window = MainWindow()
    window.show()
    
    sys.exit(app.exec())

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        import traceback
        with open("crash.txt", "w") as f:
            f.write(traceback.format_exc())
        print("CRASH LOGGED TO crash.txt")
        sys.exit(1)
