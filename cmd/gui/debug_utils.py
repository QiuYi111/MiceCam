import os
import sys
import time
import datetime

# Global log file path (absolute to ensure it overrides CWD changes)
# We use the user's home or a fixed temp location to ensure write permissions
LOG_FILE = os.path.abspath("MiceCam_Debug.log")

def log(tag, message):
    """
    Writes a log message immediately to disk.
    Format: [TIME] [PID] [TAG] Message
    """
    ts = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
    pid = os.getpid()

    entry = f"[{ts}] [{pid}] [{tag}] {message}\n"

    try:
        with open(LOG_FILE, "a", encoding="utf-8") as f:
            f.write(entry)
            f.flush()
            os.fsync(f.fileno()) # Force write to disk
    except Exception as e:
        # If we can't write to file, print to stderr as backup
        sys.stderr.write(f"LOG FAIL: {entry} -- {e}\n")

def log_environment():
    """Logs critical environment variables and paths."""
    log("ENV", f"Executable: {sys.executable}")
    log("ENV", f"CWD: {os.getcwd()}")
    log("ENV", f"Frozen: {getattr(sys, 'frozen', False)}")
    log("ENV", f"Path: {sys.path}")

    # Log specific variables that might affect loading
    for key in ['PYTHONPATH', 'PYTHONHOME', 'PATH', 'QT_PLUGIN_PATH', 'QML2_IMPORT_PATH']:
        if key in os.environ:
            log("ENV", f"{key}={os.environ[key]}")

def hook_exceptions():
    """Captures unhandled exceptions."""
    def handle_exception(exc_type, exc_value, exc_traceback):
        if issubclass(exc_type, KeyboardInterrupt):
            sys.__excepthook__(exc_type, exc_value, exc_traceback)
            return

        import traceback
        lines = traceback.format_exception(exc_type, exc_value, exc_traceback)
        log("CRASH", "".join(lines))
        sys.__excepthook__(exc_type, exc_value, exc_traceback)

    sys.excepthook = handle_exception
