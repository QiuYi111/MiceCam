import sys
import os

# Add the directory containing this script to sys.path
# This allows running from the project root while maintaining internal module resolution
sys.path.append(os.path.dirname(os.path.abspath(__file__)))


def configure_packaged_runtime():
    # PyInstaller one-folder builds place binary dependencies under _internal.
    # Register that directory early so worker-mode imports can load _micecam and
    # its dependent DLLs before any dynamic imports happen.
    meipass = getattr(sys, "_MEIPASS", None)
    exe_dir = os.path.dirname(sys.executable)
    if not meipass and not getattr(sys, "frozen", False):
        return

    dll_dirs = []
    for candidate in [
        meipass,
        os.path.join(meipass, "_internal") if meipass else None,
        exe_dir,
        os.path.join(exe_dir, "_internal"),
    ]:
        if candidate and candidate not in dll_dirs:
            dll_dirs.append(candidate)

    for dll_dir in dll_dirs:
        if not dll_dir or not os.path.isdir(dll_dir):
            continue
        if dll_dir not in sys.path:
            sys.path.insert(0, dll_dir)
        try:
            os.add_dll_directory(dll_dir)
        except (AttributeError, FileNotFoundError):
            pass

    os.environ["PATH"] = os.pathsep.join(dll_dirs + [os.environ.get("PATH", "")])


configure_packaged_runtime()

# --- DEBUGGING HOOK ---
try:
    import debug_utils
    debug_utils.hook_exceptions()
    debug_utils.log("APP", f"Startup - Argv: {sys.argv}")
except ImportError:
    pass
# ----------------------

# Dispatcher Pattern MUST be at the top to avoid unrelated imports in worker mode
if len(sys.argv) > 1 and sys.argv[1] == "--worker":
    if 'debug_utils' in globals(): debug_utils.log("APP", "Dispatching to Worker Mode")

    # Run as Recorder Worker
    # We need to adjust sys.argv to allow argparse (if used) or just pass args manually
    # recorder_worker expects: script, out, name, backend, w, h, fps, dev
    # Our QProcess sends: exe, --worker, out, name...
    # So we strip the first 2 args: [exe, --worker]
    sys.argv = [sys.argv[0]] + sys.argv[2:]

    try:
         # Dynamically import ONLY what is needed
         import recorder_worker
         if 'debug_utils' in globals(): debug_utils.log("APP", "Imported recorder_worker, calling main()")
         recorder_worker.main()
    except Exception as e:
         if 'debug_utils' in globals(): debug_utils.log("APP", f"Worker Main Error: {e}")
         print(f"Worker Start Error: {e}")
         import traceback
         traceback.print_exc()
         sys.exit(1)

    if 'debug_utils' in globals(): debug_utils.log("APP", "Worker Exit 0")
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
