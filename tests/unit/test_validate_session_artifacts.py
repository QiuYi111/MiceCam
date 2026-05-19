#!/usr/bin/env python3
"""No-hardware tests for validate_session_artifacts.py.

Uses synthetic temp artifacts to cover all validator logic.
Does not require real camera hardware or ffprobe.

Run: python3 -m pytest tests/unit/test_validate_session_artifacts.py -v
"""

import json
import os
import tempfile
import time
import unittest
from pathlib import Path

import sys

SCRIPTS_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), "scripts")
sys.path.insert(0, SCRIPTS_DIR)

from validate_session_artifacts import (
    CheckResult,
    ValidationResult,
    validate_mp4,
    validate_srt,
    validate_meta_json,
    validate_stats_json,
    validate_session,
    discover_streams,
)


class TestValidateMP4(unittest.TestCase):
    @staticmethod
    def _unlink(path, retries=5, delay=0.1):
        for _ in range(retries):
            try:
                os.unlink(path)
                return
            except PermissionError:
                time.sleep(delay)

    def test_missing_file(self):
        r = validate_mp4("/nonexistent/video.mp4")
        self.assertEqual(r.status, "FAIL")
        self.assertIn("not found", r.message)

    def test_empty_file(self):
        with tempfile.NamedTemporaryFile(suffix=".mp4", delete=False) as f:
            f.write(b"")
            f.flush()
            f.close()
            try:
                r = validate_mp4(f.name)
                self.assertEqual(r.status, "FAIL")
                self.assertIn("empty", r.message)
            finally:
                self._unlink(f.name)

    def test_non_mp4_file_ffprobe_skip_or_fail(self):
        with tempfile.NamedTemporaryFile(suffix=".mp4", delete=False) as f:
            f.write(b"This is not an MP4 file at all, just garbage data")
            f.flush()
            f.close()
            try:
                r = validate_mp4(f.name)
                self.assertIn(r.status, ("FAIL", "SKIP"))
            finally:
                self._unlink(f.name)


class TestValidateSRT(unittest.TestCase):
    def _write_srt(self, content: str) -> str:
        f = tempfile.NamedTemporaryFile(mode="w", suffix=".srt", delete=False, encoding="utf-8")
        f.write(content)
        f.flush()
        f.close()
        self.addCleanup(os.unlink, f.name)
        return f.name

    def test_missing_file(self):
        r = validate_srt("/nonexistent/video.srt")
        self.assertEqual(r.status, "FAIL")
        self.assertIn("not found", r.message)

    def test_empty_file(self):
        path = self._write_srt("")
        r = validate_srt(path)
        self.assertEqual(r.status, "FAIL")
        self.assertIn("empty", r.message)

    def test_valid_monotonic_srt(self):
        srt = (
            "1\n"
            "00:00:00,000 --> 00:00:00,033\n"
            "frame 0\n\n"
            "2\n"
            "00:00:00,033 --> 00:00:00,067\n"
            "frame 1\n\n"
            "3\n"
            "00:00:00,067 --> 00:00:00,100\n"
            "frame 2\n\n"
        )
        path = self._write_srt(srt)
        r = validate_srt(path)
        self.assertEqual(r.status, "PASS")
        self.assertIn("3 monotonic entries", r.message)

    def test_non_monotonic_srt(self):
        srt = (
            "1\n"
            "00:00:00,000 --> 00:00:00,033\n"
            "frame 0\n\n"
            "2\n"
            "00:00:00,010 --> 00:00:00,050\n"
            "frame 1 (non-monotonic start)\n\n"
        )
        path = self._write_srt(srt)
        r = validate_srt(path)
        self.assertEqual(r.status, "FAIL")
        self.assertIn("not monotonic", r.message)

    def test_single_entry_srt(self):
        srt = (
            "1\n"
            "00:00:00,000 --> 00:00:01,000\n"
            "frame 0\n\n"
        )
        path = self._write_srt(srt)
        r = validate_srt(path)
        self.assertEqual(r.status, "PASS")

    def test_gap_in_timestamps(self):
        srt = (
            "1\n"
            "00:00:00,000 --> 00:00:00,033\n"
            "frame 0\n\n"
            "2\n"
            "00:00:05,000 --> 00:00:05,033\n"
            "frame 1 (large gap but still monotonic)\n\n"
        )
        path = self._write_srt(srt)
        r = validate_srt(path)
        self.assertEqual(r.status, "PASS")
        self.assertIn("2 monotonic entries", r.message)


class TestValidateMetaJson(unittest.TestCase):
    def _write_json(self, data: dict) -> str:
        f = tempfile.NamedTemporaryFile(mode="w", suffix="_meta.json", delete=False, encoding="utf-8")
        json.dump(data, f)
        f.flush()
        f.close()
        self.addCleanup(os.unlink, f.name)
        return f.name

    def test_missing_file(self):
        r = validate_meta_json("/nonexistent/_meta.json")
        self.assertEqual(r.status, "FAIL")
        self.assertIn("not found", r.message)

    def test_invalid_json(self):
        f = tempfile.NamedTemporaryFile(mode="w", suffix="_meta.json", delete=False, encoding="utf-8")
        f.write("{invalid json")
        f.flush()
        f.close()
        self.addCleanup(os.unlink, f.name)
        r = validate_meta_json(f.name)
        self.assertEqual(r.status, "FAIL")
        self.assertIn("Invalid JSON", r.message)

    def test_valid_meta_with_required_fields(self):
        path = self._write_json({
            "plugin_id": "com.micecam.ffmpeg",
            "plugin_version": "1.0.0",
            "source_name": "FFmpeg Camera",
            "device_persistent_id": "macbook-pro-camera-001",
            "api_version": "1.0",
            "resolved_output": "h264",
            "capability_snapshot": {},
            "resolved_config": {},
        })
        r = validate_meta_json(path)
        self.assertEqual(r.status, "PASS")
        self.assertIn("Valid metadata", r.message)

    def test_missing_required_field(self):
        path = self._write_json({
            "plugin_id": "com.micecam.ffmpeg",
            "source_name": "FFmpeg Camera",
        })
        r = validate_meta_json(path)
        self.assertEqual(r.status, "FAIL")
        self.assertIn("Missing required fields", r.message)
        self.assertIn("plugin_version", r.message)

    def test_strict_mode_warns_optional_fields(self):
        path = self._write_json({
            "plugin_id": "com.micecam.ffmpeg",
            "plugin_version": "1.0.0",
            "source_name": "FFmpeg Camera",
            "device_persistent_id": "cam-001",
            "session_start_wall_time": "2025-01-15T10:30:00.123456",
            "keyframe_interval": 30,
            "i_frame_latency_ms": 5.2,
            "p_frame_latency_ms": 1.1,
            "crash_window_sec": 10.0,
            "actual_encoder_name": "h264_videotoolbox",
            "calibration_duration_ms": 250,
            "parallel_test_passed": True,
            "requested_streams": ["stream_0"],
        })
        r = validate_meta_json(path, strict=True)
        self.assertEqual(r.status, "WARN")
        self.assertIn("Missing optional fields", r.message)


class TestValidateStatsJson(unittest.TestCase):
    def _write_json(self, data: dict) -> str:
        f = tempfile.NamedTemporaryFile(mode="w", suffix="_stats.json", delete=False, encoding="utf-8")
        json.dump(data, f)
        f.flush()
        f.close()
        self.addCleanup(os.unlink, f.name)
        return f.name

    def test_missing_file(self):
        r = validate_stats_json("/nonexistent/_stats.json")
        self.assertEqual(r.status, "FAIL")
        self.assertIn("not found", r.message)

    def test_invalid_json(self):
        f = tempfile.NamedTemporaryFile(mode="w", suffix="_stats.json", delete=False, encoding="utf-8")
        f.write("not json at all")
        f.flush()
        f.close()
        self.addCleanup(os.unlink, f.name)
        r = validate_stats_json(f.name)
        self.assertEqual(r.status, "FAIL")
        self.assertIn("Invalid JSON", r.message)

    def test_valid_stats_no_drops(self):
        path = self._write_json({
            "transport_frames_total": 108000,
            "transport_drops_total": 0,
            "transport_backpressure_events": 0,
            "transport_bytes_total": 5432100000,
            "session_duration_ms": 3600000,
        })
        r = validate_stats_json(path)
        self.assertEqual(r.status, "PASS")
        self.assertIn("No drops", r.message)

    def test_valid_stats_with_drops(self):
        path = self._write_json({
            "transport_frames_total": 107500,
            "transport_drops_total": 12,
            "transport_backpressure_events": 3,
        })
        r = validate_stats_json(path)
        self.assertEqual(r.status, "WARN")
        self.assertIn("Drops: 12", r.message)
        self.assertIn("backpressure events: 3", r.message)

    def test_missing_required_field(self):
        path = self._write_json({
            "transport_frames_total": 1000,
        })
        r = validate_stats_json(path)
        self.assertEqual(r.status, "FAIL")
        self.assertIn("Missing required fields", r.message)
        self.assertIn("transport_drops_total", r.message)

    def test_wrong_type_for_field(self):
        path = self._write_json({
            "transport_frames_total": "not_a_number",
            "transport_drops_total": 0,
            "transport_backpressure_events": 0,
        })
        r = validate_stats_json(path)
        self.assertEqual(r.status, "FAIL")
        self.assertIn("Wrong type", r.message)


class TestDiscoverStreams(unittest.TestCase):
    def test_empty_directory(self):
        with tempfile.TemporaryDirectory() as d:
            streams = discover_streams(d)
            self.assertEqual(streams, [])

    def test_discovers_streams_from_mp4(self):
        with tempfile.TemporaryDirectory() as d:
            for name in ("stream_0.mp4", "stream_1.mp4"):
                Path(d, name).write_bytes(b"\x00" * 100)
            streams = discover_streams(d)
            self.assertEqual(len(streams), 2)
            ids = [s[0] for s in streams]
            self.assertIn("stream_0", ids)
            self.assertIn("stream_1", ids)

    def test_stream_paths(self):
        with tempfile.TemporaryDirectory() as d:
            Path(d, "stream_cam0.mp4").write_bytes(b"\x00" * 100)
            streams = discover_streams(d)
            self.assertEqual(len(streams), 1)
            sid, mp4, srt, meta, stats = streams[0]
            self.assertEqual(sid, "stream_cam0")
            self.assertTrue(mp4.endswith("stream_cam0.mp4"))
            self.assertTrue(srt.endswith("stream_cam0.srt"))
            self.assertTrue(meta.endswith("stream_cam0_meta.json"))
            self.assertTrue(stats.endswith("stream_cam0_stats.json"))


class TestValidateSession(unittest.TestCase):
    def test_nonexistent_directory(self):
        v = validate_session("/nonexistent/dir")
        self.assertTrue(v.has_failures)

    def test_empty_directory(self):
        with tempfile.TemporaryDirectory() as d:
            v = validate_session(d)
            self.assertTrue(v.has_failures)

    def test_full_session_success(self):
        with tempfile.TemporaryDirectory() as d:
            Path(d, "stream_0.mp4").write_bytes(b"\x00" * 200)

            srt_content = (
                "1\n"
                "00:00:00,000 --> 00:00:00,033\n"
                "frame 0\n\n"
                "2\n"
                "00:00:00,033 --> 00:00:00,067\n"
                "frame 1\n\n"
            )
            Path(d, "stream_0.srt").write_text(srt_content, encoding="utf-8")

            meta = {
                "plugin_id": "com.micecam.ffmpeg",
                "plugin_version": "1.0.0",
                "source_name": "FFmpeg Camera",
                "device_persistent_id": "cam-001",
            }
            Path(d, "stream_0_meta.json").write_text(json.dumps(meta), encoding="utf-8")

            stats = {
                "transport_frames_total": 108000,
                "transport_drops_total": 0,
                "transport_backpressure_events": 0,
            }
            Path(d, "stream_0_stats.json").write_text(json.dumps(stats), encoding="utf-8")

            v = validate_session(d)
            non_mp4_failures = [r for r in v.results if r.status == "FAIL" and "MP4" not in r.name]
            self.assertEqual(len(non_mp4_failures), 0,
                             f"Unexpected non-MP4 failures: {[r.name + ': ' + r.message for r in non_mp4_failures]}")
            mp4_results = [r for r in v.results if "MP4" in r.name]
            self.assertTrue(len(mp4_results) > 0)
            self.assertIn(mp4_results[0].status, ("PASS", "SKIP", "FAIL"))

    def test_session_with_missing_srt(self):
        with tempfile.TemporaryDirectory() as d:
            Path(d, "stream_0.mp4").write_bytes(b"\x00" * 100)
            meta = {
                "plugin_id": "com.micecam.ffmpeg",
                "plugin_version": "1.0.0",
                "source_name": "FFmpeg Camera",
                "device_persistent_id": "cam-001",
            }
            Path(d, "stream_0_meta.json").write_text(json.dumps(meta), encoding="utf-8")
            stats = {
                "transport_frames_total": 100,
                "transport_drops_total": 0,
                "transport_backpressure_events": 0,
            }
            Path(d, "stream_0_stats.json").write_text(json.dumps(stats), encoding="utf-8")

            v = validate_session(d)
            self.assertTrue(v.has_failures)
            srt_results = [r for r in v.results if "SRT" in r.name]
            self.assertTrue(any(r.status == "FAIL" for r in srt_results))


class TestValidateMetaSpec004Strict(unittest.TestCase):
    def _write_json(self, data: dict) -> str:
        f = tempfile.NamedTemporaryFile(mode="w", suffix="_meta.json", delete=False, encoding="utf-8")
        json.dump(data, f)
        f.flush()
        f.close()
        self.addCleanup(os.unlink, f.name)
        return f.name

    def _base_meta(self) -> dict:
        return {
            "plugin_id": "com.micecam.ffmpeg",
            "plugin_version": "1.0.0",
            "source_name": "FFmpeg Camera",
            "device_persistent_id": "cam-001",
            "api_version": "1.0",
            "resolved_output": "h264",
            "capability_snapshot": {},
            "resolved_config": {},
            "session_start_wall_time": "2025-01-15T10:30:00.123456",
            "keyframe_interval": 30,
            "i_frame_latency_ms": 5.2,
            "p_frame_latency_ms": 1.1,
            "crash_window_sec": 10.0,
            "actual_encoder_name": "h264_videotoolbox",
            "calibration_duration_ms": 250,
            "parallel_test_passed": True,
            "requested_streams": ["stream_0"],
        }

    def test_valid_meta_all_spec004_fields_strict(self):
        path = self._write_json(self._base_meta())
        r = validate_meta_json(path, strict=True)
        self.assertEqual(r.status, "PASS")
        self.assertIn("Valid metadata", r.message)

    def test_missing_spec004_field_strict(self):
        for field in ("session_start_wall_time", "keyframe_interval", "i_frame_latency_ms",
                       "p_frame_latency_ms", "crash_window_sec", "actual_encoder_name",
                       "calibration_duration_ms", "parallel_test_passed", "requested_streams"):
            data = self._base_meta()
            del data[field]
            path = self._write_json(data)
            r = validate_meta_json(path, strict=True)
            self.assertEqual(r.status, "FAIL", f"Expected FAIL for missing {field}")
            self.assertIn(field, r.message)

    def test_wrong_type_spec004_field_strict(self):
        type_cases = [
            ("keyframe_interval", "not_int"),
            ("i_frame_latency_ms", "not_number"),
            ("p_frame_latency_ms", "not_number"),
            ("crash_window_sec", "not_number"),
            ("calibration_duration_ms", "not_number"),
            ("parallel_test_passed", "not_bool"),
            ("requested_streams", "not_list"),
        ]
        for field, bad_val in type_cases:
            data = self._base_meta()
            data[field] = bad_val
            path = self._write_json(data)
            r = validate_meta_json(path, strict=True)
            self.assertEqual(r.status, "FAIL", f"Expected FAIL for wrong type in {field}")
            self.assertIn("wrong type", r.message)

    def test_non_positive_number_spec004_strict(self):
        zero_cases = [
            ("keyframe_interval", 0),
            ("i_frame_latency_ms", -1),
            ("p_frame_latency_ms", 0),
            ("crash_window_sec", -5),
            ("calibration_duration_ms", 0),
        ]
        for field, bad_val in zero_cases:
            data = self._base_meta()
            data[field] = bad_val
            path = self._write_json(data)
            r = validate_meta_json(path, strict=True)
            self.assertEqual(r.status, "FAIL", f"Expected FAIL for {field}={bad_val}")
            self.assertIn("must be > 0", r.message)

    def test_empty_string_spec004_strict(self):
        for field in ("session_start_wall_time", "actual_encoder_name"):
            data = self._base_meta()
            data[field] = ""
            path = self._write_json(data)
            r = validate_meta_json(path, strict=True)
            self.assertEqual(r.status, "FAIL", f"Expected FAIL for empty {field}")

    def test_empty_requested_streams_strict(self):
        data = self._base_meta()
        data["requested_streams"] = []
        path = self._write_json(data)
        r = validate_meta_json(path, strict=True)
        self.assertEqual(r.status, "FAIL")
        self.assertIn("empty list", r.message)

    def test_invalid_iso8601_wall_time_strict(self):
        data = self._base_meta()
        data["session_start_wall_time"] = "not-a-date"
        path = self._write_json(data)
        r = validate_meta_json(path, strict=True)
        self.assertEqual(r.status, "FAIL")
        self.assertIn("not ISO 8601", r.message)


class TestValidateSRTWallTime(unittest.TestCase):
    def _write_srt(self, content: str) -> str:
        f = tempfile.NamedTemporaryFile(mode="w", suffix=".srt", delete=False, encoding="utf-8")
        f.write(content)
        f.flush()
        f.close()
        self.addCleanup(os.unlink, f.name)
        return f.name

    def test_valid_srt_with_wall_time_strict(self):
        srt = (
            "1\n"
            "00:00:00,000 --> 00:00:00,033\n"
            "wall_time=2025-01-15T10:30:00.123456\n\n"
            "2\n"
            "00:00:00,033 --> 00:00:00,067\n"
            "wall_time=2025-01-15T10:30:00.156456\n\n"
        )
        path = self._write_srt(srt)
        r = validate_srt(path, strict=True)
        self.assertEqual(r.status, "PASS")
        self.assertIn("monotonic", r.message)

    def test_valid_srt_without_wall_time_strict_warns(self):
        srt = (
            "1\n"
            "00:00:00,000 --> 00:00:00,033\n"
            "frame 0\n\n"
            "2\n"
            "00:00:00,033 --> 00:00:00,067\n"
            "frame 1\n\n"
        )
        path = self._write_srt(srt)
        r = validate_srt(path, strict=True)
        self.assertEqual(r.status, "WARN")
        self.assertIn("no wall_time", r.message)

    def test_srt_with_invalid_wall_time_format(self):
        srt = (
            "1\n"
            "00:00:00,000 --> 00:00:00,033\n"
            "wall_time=bad-format\n\n"
            "2\n"
            "00:00:00,033 --> 00:00:00,067\n"
            "wall_time=2025-01-15T10:30:00.123456\n\n"
        )
        path = self._write_srt(srt)
        r = validate_srt(path, strict=True)
        self.assertEqual(r.status, "WARN")
        self.assertIn("invalid wall_time", r.message)

    def test_non_strict_no_wall_time_still_passes(self):
        srt = (
            "1\n"
            "00:00:00,000 --> 00:00:00,033\n"
            "frame 0\n\n"
        )
        path = self._write_srt(srt)
        r = validate_srt(path, strict=False)
        self.assertEqual(r.status, "PASS")


class TestValidateStatsStructure(unittest.TestCase):
    def _write_json(self, data, suffix="_stats.json") -> str:
        f = tempfile.NamedTemporaryFile(mode="w", suffix=suffix, delete=False, encoding="utf-8")
        json.dump(data, f)
        f.flush()
        f.close()
        self.addCleanup(os.unlink, f.name)
        return f.name

    def test_stats_json_is_array_fails(self):
        path = self._write_json([1, 2, 3])
        r = validate_stats_json(path)
        self.assertEqual(r.status, "FAIL")
        self.assertIn("must be a dict", r.message)

    def test_stats_json_is_list_fails(self):
        path = self._write_json([])
        r = validate_stats_json(path)
        self.assertEqual(r.status, "FAIL")
        self.assertIn("must be a dict", r.message)

    def test_valid_stats_dict_passes(self):
        path = self._write_json({
            "transport_frames_total": 1000,
            "transport_drops_total": 0,
            "transport_backpressure_events": 0,
        })
        r = validate_stats_json(path)
        self.assertEqual(r.status, "PASS")


class TestValidationResult(unittest.TestCase):
    def test_all_pass(self):
        v = ValidationResult(session_dir="/tmp")
        v.results.append(CheckResult(name="t", status="PASS", message="ok"))
        self.assertTrue(v.passed)
        self.assertFalse(v.has_failures)

    def test_with_skip(self):
        v = ValidationResult(session_dir="/tmp")
        v.results.append(CheckResult(name="t", status="PASS", message="ok"))
        v.results.append(CheckResult(name="t2", status="SKIP", message="ffprobe"))
        self.assertTrue(v.passed)
        self.assertFalse(v.has_failures)

    def test_with_failure(self):
        v = ValidationResult(session_dir="/tmp")
        v.results.append(CheckResult(name="t", status="PASS", message="ok"))
        v.results.append(CheckResult(name="t2", status="FAIL", message="broken"))
        self.assertFalse(v.passed)
        self.assertTrue(v.has_failures)


if __name__ == "__main__":
    unittest.main()
