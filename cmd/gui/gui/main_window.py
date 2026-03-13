import sys
import os
import time
from PyQt6.QtWidgets import (QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
                             QLabel, QPushButton, QComboBox, QLineEdit,
                             QGroupBox, QPlainTextEdit, QMessageBox, QFrame,
                             QGridLayout, QScrollArea, QSizePolicy, QCheckBox, QProgressBar)
from PyQt6.QtCore import Qt, QTimer, QSize, pyqtSignal
from PyQt6.QtGui import QFont, QIcon, QColor, QTextCursor, QImage, QPixmap
from PyQt6.QtMultimedia import QMediaDevices, QCameraDevice

from gui.recorder_thread import RecorderThread
from gui.decoder_thread import DecoderThread


SDK_AVAILABLE = True
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
QProgressBar {
    border: 1px solid #ccc;
    border-radius: 4px;
    text-align: center;
    background-color: #eee;
}
QProgressBar::chunk {
    background-color: #1a4b8c;
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
/* Preview */
QLabel#PreviewLabel {
    background-color: #202124;
    border-radius: 6px;
    min-height: 200px;
    alignment: center;
}
"""

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("MiceCam Pro")
        self.resize(500, 800)
        self.setStyleSheet(STYLESHEET)

        # State
        self.recorder = None
        self.decoder = None
        self.is_recording = False
        self.preview_labels = []
        self.preview_titles = ["USB / CAM A", "CAM B", "CAM C", "CAM D"]

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

        # 2. Controls Scroll Area
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
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
        self.refresh_session_name()

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

        w_fps, self.lbl_fps = make_stat("FPS", "fps")
        w_frames, self.lbl_frames = make_stat("Frames", "frames")
        w_drop, self.lbl_drop = make_stat("Dropped", "drop")
        w_time, self.lbl_time = make_stat("Duration", "time")
        w_mb, self.lbl_mb = make_stat("MB/s", "mb")
        w_mbps, self.lbl_mbps = make_stat("Mbps", "mbps")

        stat_layout.addWidget(w_fps, 0, 0)
        stat_layout.addWidget(w_time, 0, 1)
        stat_layout.addWidget(w_mbps, 0, 2)
        stat_layout.addWidget(w_frames, 1, 0)
        stat_layout.addWidget(w_drop, 1, 1)
        stat_layout.addWidget(w_mb, 1, 2)

        content_layout.addWidget(stat_grp)

        # -- Controls Group --
        ctl_grp = QGroupBox("⚡ Controls")
        ctl_layout = QVBoxLayout(ctl_grp)

        h_btns = QHBoxLayout()
        self.btn_start = QPushButton("▶ Start Recording")
        self.btn_start.setObjectName("StartBtn")
        self.btn_start.clicked.connect(self.start_recording)

        self.btn_stop = QPushButton("⏹ Stop")
        self.btn_stop.setObjectName("StopBtn")
        self.btn_stop.setEnabled(False)
        self.btn_stop.clicked.connect(self.stop_recording)

        h_btns.addWidget(self.btn_start)
        h_btns.addWidget(self.btn_stop)

        # Auto Decode Checkbox
        self.chk_decode = QCheckBox("Auto-Decode Session after finished")
        self.chk_decode.setChecked(True)

        # Decode Progress
        self.lbl_decode = QLabel("Decoding Progress:")
        self.lbl_decode.setVisible(False)
        self.prog_decode = QProgressBar()
        self.prog_decode.setVisible(False)

        ctl_layout.addLayout(h_btns)
        ctl_layout.addWidget(self.chk_decode)
        ctl_layout.addWidget(self.lbl_decode)
        ctl_layout.addWidget(self.prog_decode)

        content_layout.addWidget(ctl_grp)

        # -- Preview Group --
        preview_grp = QGroupBox("👁 Preview")
        preview_layout = QVBoxLayout(preview_grp)

        self.chk_preview = QCheckBox("Enable Live Preview (Uses more CPU)")
        self.chk_preview.setChecked(True)

        preview_layout.addWidget(self.chk_preview)

        preview_grid = QGridLayout()
        preview_grid.setSpacing(10)

        for idx, title in enumerate(self.preview_titles):
            cell = QWidget()
            cell_layout = QVBoxLayout(cell)
            cell_layout.setContentsMargins(0, 0, 0, 0)
            cell_layout.setSpacing(6)

            title_label = QLabel(title)
            title_label.setStyleSheet("color: #5f6368; font-size: 11px; font-weight: 600;")

            preview_label = QLabel("Preview offline")
            preview_label.setObjectName("PreviewLabel")
            preview_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
            preview_label.setMinimumSize(220, 160)
            preview_label.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
            preview_label.setStyleSheet("color: #7f8c8d; background-color: #ecf0f1;")

            self.preview_labels.append(preview_label)

            cell_layout.addWidget(title_label)
            cell_layout.addWidget(preview_label)
            preview_grid.addWidget(cell, idx // 2, idx % 2)

        preview_layout.addLayout(preview_grid)

        content_layout.addWidget(preview_grp)

        # -- Log Widget --
        log_grp = QGroupBox("📜 System Log")
        log_layout = QVBoxLayout(log_grp)
        self.log_view = QPlainTextEdit()
        self.log_view.setReadOnly(True)
        self.log_view.setMaximumHeight(150)
        log_layout.addWidget(self.log_view)

        content_layout.addWidget(log_grp)

        scroll.setWidget(scroll_content)
        layout.addWidget(scroll)

        self.reset_preview_tiles(None)

        self.log("System Ready. SDK Available: " + str(SDK_AVAILABLE))
        if not SDK_AVAILABLE:
            self.log("WARNING: Worker script not found. Recording will fail.")

    def log(self, text):
        ts = time.strftime("[%H:%M:%S]")
        line = f"{ts} {text}"
        self.log_view.appendPlainText(line)
        self.log_view.moveCursor(QTextCursor.MoveOperation.End)

    def refresh_session_name(self):
        ts = time.strftime("session_%Y%m%d_%H%M%S")
        self.edt_session.setText(ts)

    def refresh_cameras(self):
        self.cb_camera.clear()
        self.cb_camera.addItem("Luxonis OAK (DepthAI)", "oak")

        # Real Enumeration via QtMultimedia
        cameras = QMediaDevices.videoInputs()
        for i, cam in enumerate(cameras):
            # i is likely the dshow index if order is preserved
            name = cam.description()
            self.cb_camera.addItem(f"{name} (Index {i})", str(i))

    def on_camera_changed(self):
        data = self.cb_camera.currentData()
        if data == "oak":
            self.cb_res.clear()
            self.cb_res.addItems(["1280x800", "1280x720"])
            self.reset_preview_tiles("oak")
        else:
            self.cb_res.clear()
            self.cb_res.addItems(["1920x1080", "1280x720", "640x480"])
            self.reset_preview_tiles("ffmpeg")

    def start_recording(self):
        if self.is_recording: return

        self.lbl_decode.setVisible(False)
        self.prog_decode.setVisible(False)
        self.prog_decode.setValue(0)

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
            "fps": float(self.edt_fps.text()),
            "preview_enabled": self.chk_preview.isChecked(),
        }

        self.recorder = RecorderThread(cfg)
        self.recorder.log_message.connect(self.log)
        self.recorder.error_occurred.connect(self.on_error)
        self.recorder.stats_updated.connect(self.on_stats)
        self.recorder.preview_updated.connect(self.on_preview_updated)
        self.recorder.finished_recording.connect(self.on_finished)

        self.recorder.start()

        self.is_recording = True
        self.btn_start.setEnabled(False)
        self.btn_stop.setEnabled(True)
        self.cb_camera.setEnabled(False)
        self.reset_preview_tiles(backend)

    def stop_recording(self):
        if self.recorder and self.is_recording:
            self.recorder.stop()
            self.btn_stop.setEnabled(False) # Prevent double click

    def on_stats(self, s):
        self.lbl_frames.setText(str(s['captured']))
        self.lbl_drop.setText(str(s['dropped']))
        self.lbl_fps.setText(str(s['fps']))
        self.lbl_time.setText(f"{s['elapsed']:.1f}s")
        self.lbl_mbps.setText(str(s['mbps']))
        self.lbl_mb.setText(str(s.get('mb', '--')))

    def on_error(self, msg):
        self.log(f"ERROR: {msg}")
        QMessageBox.critical(self, "Error", msg)

    def on_finished(self):
        self.is_recording = False
        self.btn_start.setEnabled(True)
        self.btn_stop.setEnabled(False)
        self.cb_camera.setEnabled(True)

        # Auto Decode Trigger
        if self.chk_decode.isChecked():
            self.start_decoding()
        else:
            self.refresh_session_name()
            self.log("Session finished (No decode).")

        self.reset_preview_tiles(None)

    def on_preview_updated(self, msg):
        if not self.chk_preview.isChecked():
            return

        try:
            import base64
            img_b64 = msg.get("image")
            if not img_b64:
                return

            cam_index = int(msg.get("index", 0))
            if cam_index < 0 or cam_index >= len(self.preview_labels):
                cam_index = 0

            target_label = self.preview_labels[cam_index]

            img_data = base64.b64decode(img_b64)
            fmt = msg.get("format", "jpeg")

            if fmt == "raw":
                w = msg.get("width", 0)
                h = msg.get("height", 0)
                # Create QImage from raw Grayscale 8-bit data
                image = QImage(img_data, w, h, w, QImage.Format.Format_Grayscale8)
                pixmap = QPixmap.fromImage(image)
            else:
                pixmap = QPixmap()
                if not pixmap.loadFromData(img_data):
                    return

            # Scale it nicely for the label while keeping aspect ratio
            scaled_pixmap = pixmap.scaled(
                target_label.size(),
                Qt.AspectRatioMode.KeepAspectRatio,
                Qt.TransformationMode.SmoothTransformation
            )

            target_label.setPixmap(scaled_pixmap)
            target_label.setStyleSheet("background-color: black;")
        except Exception as e:
            pass

    def reset_preview_tiles(self, backend):
        active_count = 4 if backend == "oak" else 1
        for idx, label in enumerate(self.preview_labels):
            label.clear()
            if idx < active_count:
                label.setText("Preview will appear here while recording")
            else:
                label.setText("Unused for current source")
            label.setStyleSheet("color: #7f8c8d; background-color: #ecf0f1;")

    def start_decoding(self):
        self.lbl_decode.setVisible(True)
        self.lbl_decode.setText("Decoding Progress:")
        self.lbl_decode.setStyleSheet("color: black;")
        self.prog_decode.setVisible(True)
        self.log("Starting Auto-Decode...")

        out_dir = self.edt_output.text()
        sess = self.edt_session.text()

        self.decoder = DecoderThread(out_dir, sess)
        self.decoder.progress_updated.connect(self.prog_decode.setValue)
        self.decoder.status_message.connect(self.log)
        self.decoder.finished_decoding.connect(self.on_decode_finished)
        self.decoder.status_message.connect(self.on_decoder_status) # Hook into status to find errors
        self.decoder.start()

        # Disable start while decoding?
        self.btn_start.setEnabled(False)

    def on_decoder_status(self, msg):
        if "Error:" in msg or "Exception" in msg:
            self.lbl_decode.setText(f"Decoding Failed: {msg}")
            self.lbl_decode.setStyleSheet("color: #d93025; font-weight: bold;")
            self.prog_decode.setStyleSheet("QProgressBar::chunk { background-color: #d93025; }")

    def on_decode_finished(self):
        self.btn_start.setEnabled(True)
        self.log("Auto-Decode Finished.")
        self.refresh_session_name()

if __name__ == "__main__":
    from PyQt6.QtWidgets import QApplication
    app = QApplication(sys.argv)
    w = MainWindow()
    w.show()
    sys.exit(app.exec())
