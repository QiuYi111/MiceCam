import sys
import os
import time
from PyQt6.QtWidgets import (QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, 
                             QLabel, QPushButton, QComboBox, QLineEdit, 
                             QGroupBox, QPlainTextEdit, QMessageBox, QFrame,
                             QGridLayout, QScrollArea, QSizePolicy)
from PyQt6.QtCore import Qt, QTimer, QSize
from PyQt6.QtGui import QFont, QIcon, QColor, QTextCursor

from gui.recorder_thread import RecorderThread, SDK_AVAILABLE

# --- STYLESHEET (WebUI Replica) ---
STYLESHEET = """
QMainWindow {
    background-color: #f0f2f5;
}
QGroupBox {
    background-color: white;
    border: 1px solid #e8eaed;
    border-radius: 8px;
    margin-top: 10px;
    font-family: 'Roboto', 'Segoe UI', sans-serif;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    padding: 0 10px;
    color: #5f6368;
    font-weight: bold;
    font-size: 11px;
    text-transform: uppercase;
}
QLabel {
    color: #202124;
    font-family: 'Roboto', 'Segoe UI', sans-serif;
}
QPushButton {
    padding: 10px;
    border-radius: 6px;
    font-weight: 600;
    font-size: 14px;
    border: none;
}
QPushButton#StartBtn {
    background-color: #1a4b8c;
    color: white;
}
QPushButton#StartBtn:hover {
    background-color: #143d72;
}
QPushButton#StartBtn:disabled {
    background-color: #dadce0;
    color: #bdc1c6;
}
QPushButton#StopBtn {
    background-color: #d93025;
    color: white;
}
QPushButton#StopBtn:hover {
    background-color: #b31412;
}
QPushButton#StopBtn:disabled {
    background-color: #fce8e6;
    color: #ea4335;
}
QLineEdit, QComboBox {
    padding: 8px;
    border: 1px solid #dadce0;
    border-radius: 6px;
    background: white;
    selection-background-color: #1a4b8c;
}
QPlainTextEdit {
    background-color: #2d2d2d;
    color: #f0f0f0;
    border-radius: 6px;
    border: 1px solid #ccc;
    font-family: 'Consolas', 'Monaco', monospace;
    font-size: 12px;
}
/* Header */
QFrame#Header {
    background-color: #1a4b8c;
    border-bottom: 2px solid #0f2d52;
}
QLabel#HeaderTitle {
    color: white;
    font-size: 20px;
    font-weight: bold;
}
QLabel#HeaderSub {
    color: rgba(255,255,255, 0.8);
    font-size: 12px;
}
/* Stat Cards */
QLabel#StatValue {
    font-size: 24px;
    font-weight: bold;
    color: #1a4b8c;
}
QLabel#StatLabel {
    font-size: 11px;
    color: #5f6368;
}
"""

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("MiceCam Pro")
        self.resize(500, 750)
        self.setStyleSheet(STYLESHEET)
        
        # State
        self.recorder = None
        self.is_recording = False
        
        # Setup UI
        self.init_ui()
        
        # Populate Cameras (Logic check)
        self.refresh_cameras()

    def init_ui(self):
        central = QWidget()
        self.setCentralWidget(central)
        layout = QVBoxLayout(central)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        
        # 1. Header
        header = QFrame()
        header.setObjectName("Header")
        header.setFixedHeight(80)
        h_layout = QHBoxLayout(header)
        h_layout.setContentsMargins(20, 10, 20, 10)
        
        # Icon placeholder (Emoji works for now)
        icon_lbl = QLabel("🐭")
        icon_lbl.setStyleSheet("font-size: 32px; background: rgba(255,255,255,0.1); border-radius: 20px; padding: 5px;")
        
        title_box = QWidget()
        tb_layout = QVBoxLayout(title_box)
        tb_layout.setSpacing(2)
        tb_layout.setContentsMargins(0,5,0,5)
        t_lbl = QLabel("MiceCam")
        t_lbl.setObjectName("HeaderTitle")
        s_lbl = QLabel("Native Behavior Recording System")
        s_lbl.setObjectName("HeaderSub")
        tb_layout.addWidget(t_lbl)
        tb_layout.addWidget(s_lbl)
        
        h_layout.addWidget(icon_lbl)
        h_layout.addWidget(title_box)
        h_layout.addStretch()
        
        layout.addWidget(header)
        
        # 2. Controls Scroll Area (for smaller screens)
        scroll_content = QWidget()
        content_layout = QVBoxLayout(scroll_content)
        content_layout.setContentsMargins(20, 20, 20, 20)
        content_layout.setSpacing(15)
        
        # -- Source & Output Group --
        src_grp = QGroupBox("📹 Source & Output")
        src_layout = QVBoxLayout(src_grp)
        
        self.cb_camera = QComboBox()
        self.cb_camera.addItems(["Scanning cameras..."])
        self.cb_camera.currentIndexChanged.connect(self.on_camera_changed)
        
        self.edt_session = QLineEdit()
        self.edt_session.setPlaceholderText("Session Name")
        self.refresh_session_name() # Init timestamp
        
        self.cb_res = QComboBox()
        self.cb_res.addItems(["1920x1080", "1280x720", "640x480"])
        
        self.edt_fps = QLineEdit("30")
        
        self.edt_output = QLineEdit("recordings")
        self.edt_output.setPlaceholderText("Output Directory")
        
        src_layout.addWidget(QLabel("Camera Device"))
        src_layout.addWidget(self.cb_camera)
        src_layout.addWidget(QLabel("Session Name"))
        src_layout.addWidget(self.edt_session)
        
        h_res = QHBoxLayout()
        v_res = QVBoxLayout(); v_res.addWidget(QLabel("Resolution")); v_res.addWidget(self.cb_res)
        v_fps = QVBoxLayout(); v_fps.addWidget(QLabel("FPS")); v_fps.addWidget(self.edt_fps)
        h_res.addLayout(v_res, 3)
        h_res.addLayout(v_fps, 1)
        src_layout.addLayout(h_res)
        
        src_layout.addWidget(QLabel("Output Directory"))
        src_layout.addWidget(self.edt_output)
        
        content_layout.addWidget(src_grp)
        
        # -- Stats Group --
        stat_grp = QGroupBox("📊 Statistics")
        stat_layout = QGridLayout(stat_grp)
        
        def make_stat(label, val_id):
            w = QWidget()
            l = QVBoxLayout(w); l.setContentsMargins(5,5,5,5); l.setAlignment(Qt.AlignmentFlag.AlignCenter)
            val = QLabel("0")
            val.setObjectName("StatValue")
            val.setProperty("statId", val_id)
            lbl = QLabel(label)
            lbl.setObjectName("StatLabel")
            l.addWidget(val)
            l.addWidget(lbl)
            return w, val
            
        _, self.lbl_fps = make_stat("FPS", "fps")
        _, self.lbl_frames = make_stat("Frames", "frames")
        _, self.lbl_drop = make_stat("Dropped", "drop")
        _, self.lbl_time = make_stat("Duration", "time")
        _, self.lbl_mb = make_stat("Size (MB)", "mb")
        _, self.lbl_mbps = make_stat("Mbps", "mbps")
        
        stat_layout.addWidget(make_stat("FPS", "fps")[0], 0, 0)
        stat_layout.addWidget(make_stat("Duration", "time")[0], 0, 1)
        stat_layout.addWidget(make_stat("Mbps", "mbps")[0], 0, 2)
        stat_layout.addWidget(make_stat("Frames", "frames")[0], 1, 0)
        stat_layout.addWidget(make_stat("Dropped", "drop")[0], 1, 1)
        stat_layout.addWidget(make_stat("Size (MB)", "mb")[0], 1, 2)
        
        content_layout.addWidget(stat_grp)
        
        # -- Controls Group --
        ctl_grp = QGroupBox("⚡ Controls")
        ctl_layout = QHBoxLayout(ctl_grp)
        
        self.btn_start = QPushButton("▶ Start Recording")
        self.btn_start.setObjectName("StartBtn")
        self.btn_start.clicked.connect(self.start_recording)
        
        self.btn_stop = QPushButton("⏹ Stop")
        self.btn_stop.setObjectName("StopBtn")
        self.btn_stop.setEnabled(False)
        self.btn_stop.clicked.connect(self.stop_recording)
        
        ctl_layout.addWidget(self.btn_start)
        ctl_layout.addWidget(self.btn_stop)
        
        content_layout.addWidget(ctl_grp)
        
        # -- Log Widget --
        log_grp = QGroupBox("📜 System Log")
        log_layout = QVBoxLayout(log_grp)
        self.log_view = QPlainTextEdit()
        self.log_view.setReadOnly(True)
        self.log_view.setMaximumHeight(150)
        log_layout.addWidget(self.log_view)
        
        content_layout.addWidget(log_grp)
        
        # Add scroll content to layout
        layout.addWidget(scroll_content)
        
        self.log("System Ready. SDK Loaded: " + str(SDK_AVAILABLE))

    def log(self, text):
        ts = time.strftime("[%H:%M:%S]")
        line = f"{ts} {text}"
        self.log_view.appendPlainText(line)
        # Auto scroll
        self.log_view.moveCursor(QTextCursor.MoveOperation.End)

    def refresh_session_name(self):
        ts = time.strftime("session_%Y%m%d_%H%M%S")
        self.edt_session.setText(ts)

    def refresh_cameras(self):
        self.cb_camera.clear()
        
        # Probe OAK
        has_oak = False
        try:
             import _micecam
             has_oak = _micecam.has_oak_support()
        except: pass
        
        if has_oak:
            self.cb_camera.addItem("Luxonis OAK-4P (Quad Sync)", "oak")
            
        # Probe USB (Generic)
        for i in range(2):
            self.cb_camera.addItem(f"USB Camera Device {i}", str(i))

    def on_camera_changed(self):
        data = self.cb_camera.currentData()
        if data == "oak":
            self.cb_res.clear()
            self.cb_res.addItems(["1280x800", "1280x720"])
        else:
            self.cb_res.clear()
            self.cb_res.addItems(["1920x1080", "1280x720", "640x480"])

    def start_recording(self):
        if self.is_recording: return
        
        # Config
        dev_id = self.cb_camera.currentData()
        backend = "oak" if dev_id == "oak" else "ffmpeg"
        
        w, h = self.cb_res.currentText().split('x')
        
        cfg = {
            "output_dir": self.edt_output.text(),
            "session_name": self.edt_session.text(),
            "backend": backend,
            "device_id": dev_id,
            "width": int(w),
            "height": int(h),
            "fps": float(self.edt_fps.text())
        }
        
        # Setup Thread
        self.recorder = RecorderThread(cfg)
        self.recorder.log_message.connect(self.log)
        self.recorder.error_occurred.connect(self.on_error)
        self.recorder.stats_updated.connect(self.on_stats)
        self.recorder.finished_recording.connect(self.on_finished)
        
        self.recorder.start()
        
        # Update UI
        self.is_recording = True
        self.btn_start.setEnabled(False)
        self.btn_stop.setEnabled(True)
        self.cb_camera.setEnabled(False)
        
    def stop_recording(self):
        if self.recorder and self.is_recording:
            self.log("Stopping request sent...")
            self.btn_stop.setEnabled(False)
            self.recorder.stop()
            
    def on_stats(self, s):
        self.lbl_frames.setText(str(s['captured']))
        self.lbl_drop.setText(str(s['dropped']))
        self.lbl_fps.setText(str(s['fps']))
        self.lbl_time.setText(f"{s['elapsed']:.1f}s")
        self.lbl_mbps.setText(str(s['mbps']))
        # MB Size approx
        self.lbl_mb.setText("--") # Calculating size from thread needs file path, skipping for now
        
    def on_error(self, msg):
        self.log(f"ERROR: {msg}")
        QMessageBox.critical(self, "Error", msg)
        # Thread will finish automatically
        
    def on_finished(self):
        self.is_recording = False
        self.btn_start.setEnabled(True)
        self.btn_stop.setEnabled(False)
        self.cb_camera.setEnabled(True)
        self.refresh_session_name() # Prep for next
        self.log("Session finished.")

if __name__ == "__main__":
    from PyQt6.QtWidgets import QApplication
    app = QApplication(sys.argv)
    w = MainWindow()
    w.show()
    sys.exit(app.exec())
