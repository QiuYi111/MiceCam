#!/usr/bin/env python3
"""Validate MiceCam session output artifacts for Phase 6 Hardware Gate.

Checks per-stream artifacts: .mp4, .srt, _meta.json, _stats.json.
Uses ffprobe for MP4 validation when available; returns structured skip/warning otherwise.
Checks SRT timestamp monotonicity and required metadata/stats fields.

Usage:
    python3 scripts/validate_session_artifacts.py <session_dir> [--strict]
"""

import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional


@dataclass
class CheckResult:
    name: str
    status: str  # PASS, FAIL, SKIP, WARN
    message: str = ""
    details: list = field(default_factory=list)


@dataclass
class ValidationResult:
    session_dir: str
    results: list = field(default_factory=list)

    @property
    def passed(self) -> bool:
        return all(r.status in ("PASS", "SKIP", "WARN") for r in self.results)

    @property
    def has_failures(self) -> bool:
        return any(r.status == "FAIL" for r in self.results)


def find_ffprobe() -> Optional[str]:
    for candidate in ("ffprobe", "/opt/homebrew/bin/ffprobe", "/usr/local/bin/ffprobe"):
        try:
            result = subprocess.run(
                [candidate, "-version"],
                capture_output=True,
                text=True,
                timeout=5,
            )
            if result.returncode == 0:
                return candidate
        except (FileNotFoundError, subprocess.TimeoutExpired):
            continue
    return None


def validate_mp4(mp4_path: str, strict: bool = False) -> CheckResult:
    result = CheckResult(name=f"MP4: {os.path.basename(mp4_path)}", status="PASS")

    if not os.path.isfile(mp4_path):
        result.status = "FAIL"
        result.message = "File not found"
        return result

    if os.path.getsize(mp4_path) == 0:
        result.status = "FAIL"
        result.message = "File is empty (0 bytes)"
        return result

    ffprobe = find_ffprobe()
    if ffprobe is None:
        result.status = "SKIP"
        result.message = "ffprobe not available; MP4 structure not validated"
        result.details.append("Install ffmpeg for full MP4 validation")
        return result

    try:
        proc = subprocess.run(
            [
                ffprobe,
                "-v", "error",
                "-select_streams", "v:0",
                "-show_entries",
                "stream=codec_name,width,height,nb_frames,duration",
                "-of", "json",
                mp4_path,
            ],
            capture_output=True,
            text=True,
            timeout=30,
        )
        if proc.returncode != 0:
            result.status = "FAIL"
            result.message = f"ffprobe error: {proc.stderr.strip()}"
            return result

        probe_data = json.loads(proc.stdout)
        streams = probe_data.get("streams", [])
        if not streams:
            result.status = "FAIL"
            result.message = "No video streams found"
            return result

        stream = streams[0]
        codec = stream.get("codec_name", "")
        if codec not in ("h264", "hevc"):
            result.status = "FAIL"
            result.message = f"Unexpected codec: {codec} (expected h264 or hevc)"
            return result

        width = int(stream.get("width", 0))
        height = int(stream.get("height", 0))
        if width == 0 or height == 0:
            result.status = "FAIL"
            result.message = f"Invalid dimensions: {width}x{height}"
            return result

        result.details.append(f"codec={codec} {width}x{height}")
        result.message = "Valid MP4 with video stream"

    except (subprocess.TimeoutExpired, json.JSONDecodeError) as e:
        result.status = "FAIL"
        result.message = f"ffprobe check failed: {e}"

    return result


def validate_srt(srt_path: str, strict: bool = False) -> CheckResult:
    result = CheckResult(name=f"SRT: {os.path.basename(srt_path)}", status="PASS")

    if not os.path.isfile(srt_path):
        result.status = "FAIL"
        result.message = "File not found"
        return result

    if os.path.getsize(srt_path) == 0:
        result.status = "FAIL"
        result.message = "File is empty (0 bytes)"
        return result

    timestamps = []
    entry_count = 0
    non_mono_pairs = []

    srt_time_re = re.compile(
        r"(\d{2}):(\d{2}):(\d{2}),(\d{3})\s*-->\s*"
        r"(\d{2}):(\d{2}):(\d{2}),(\d{3})"
    )

    with open(srt_path, "r", encoding="utf-8", errors="replace") as f:
        prev_end_ms = -1
        for line in f:
            m = srt_time_re.search(line)
            if m:
                entry_count += 1
                sh, sm, ss, sms = int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4))
                eh, em, es, ems = int(m.group(5)), int(m.group(6)), int(m.group(7)), int(m.group(8))
                start_ms = (sh * 3600 + sm * 60 + ss) * 1000 + sms
                end_ms = (eh * 3600 + em * 60 + es) * 1000 + ems
                timestamps.append((start_ms, end_ms))

                if start_ms < prev_end_ms:
                    non_mono_pairs.append((entry_count, prev_end_ms, start_ms))
                prev_end_ms = end_ms

    if entry_count == 0:
        result.status = "FAIL"
        result.message = "No valid SRT entries found"
        return result

    if non_mono_pairs:
        result.status = "FAIL"
        result.message = f"SRT timestamps not monotonic ({len(non_mono_pairs)} violation(s))"
        for idx, prev, cur in non_mono_pairs[:5]:
            result.details.append(f"Entry {idx}: prev_end={prev}ms > start={cur}ms")
        return result

    result.message = f"{entry_count} monotonic entries"
    result.details.append(f"duration_ms={timestamps[-1][1] - timestamps[0][0]}")
    return result


META_REQUIRED_FIELDS = [
    ("plugin_id", (str,)),
    ("plugin_version", (str,)),
    ("source_name", (str,)),
    ("device_persistent_id", (str,)),
]

META_OPTIONAL_FIELDS = [
    ("api_version", (str,)),
    ("resolved_output", (str,)),
    ("capability_snapshot", (dict,)),
    ("resolved_config", (dict,)),
]


def validate_meta_json(meta_path: str, strict: bool = False) -> CheckResult:
    result = CheckResult(name=f"META: {os.path.basename(meta_path)}", status="PASS")

    if not os.path.isfile(meta_path):
        result.status = "FAIL"
        result.message = "File not found"
        return result

    try:
        with open(meta_path, "r", encoding="utf-8") as f:
            data = json.load(f)
    except (json.JSONDecodeError, UnicodeDecodeError) as e:
        result.status = "FAIL"
        result.message = f"Invalid JSON: {e}"
        return result

    missing = []
    for field_name, expected_types in META_REQUIRED_FIELDS:
        val = data.get(field_name)
        if val is None:
            missing.append(field_name)
        elif not isinstance(val, expected_types):
            result.details.append(f"{field_name}: wrong type {type(val).__name__}")

    if missing:
        result.status = "FAIL"
        result.message = f"Missing required fields: {', '.join(missing)}"
        return result

    found_fields = [f for f, _ in META_REQUIRED_FIELDS]
    result.message = f"Valid metadata with required fields: {', '.join(found_fields)}"

    if strict:
        missing_opt = []
        for field_name, expected_types in META_OPTIONAL_FIELDS:
            val = data.get(field_name)
            if val is None:
                missing_opt.append(field_name)
        if missing_opt:
            result.status = "WARN"
            result.message = f"Missing optional fields: {', '.join(missing_opt)}"

    return result


STATS_REQUIRED_FIELDS = [
    ("transport_frames_total", (int,)),
    ("transport_drops_total", (int,)),
    ("transport_backpressure_events", (int,)),
]

STATS_OPTIONAL_FIELDS = [
    ("transport_bytes_total", (int, float)),
    ("session_duration_ms", (int, float)),
    ("encoder_name", (str,)),
]


def validate_stats_json(stats_path: str, strict: bool = False) -> CheckResult:
    result = CheckResult(name=f"STATS: {os.path.basename(stats_path)}", status="PASS")

    if not os.path.isfile(stats_path):
        result.status = "FAIL"
        result.message = "File not found"
        return result

    try:
        with open(stats_path, "r", encoding="utf-8") as f:
            data = json.load(f)
    except (json.JSONDecodeError, UnicodeDecodeError) as e:
        result.status = "FAIL"
        result.message = f"Invalid JSON: {e}"
        return result

    missing = []
    wrong_type = []
    for field_name, expected_types in STATS_REQUIRED_FIELDS:
        val = data.get(field_name)
        if val is None:
            missing.append(field_name)
        elif not isinstance(val, expected_types):
            wrong_type.append(f"{field_name}: wrong type {type(val).__name__}")

    if missing:
        result.status = "FAIL"
        result.message = f"Missing required fields: {', '.join(missing)}"
        return result

    if wrong_type:
        result.status = "FAIL"
        result.message = f"Wrong type for fields: {'; '.join(wrong_type)}"
        return result

    drops = data.get("transport_drops_total", 0)
    backpressure = data.get("transport_backpressure_events", 0)
    if drops > 0 or backpressure > 0:
        result.status = "WARN"
        result.message = f"Drops: {drops}, backpressure events: {backpressure} (not silent — logged)"
    else:
        result.message = "No drops or backpressure events"

    return result


def discover_streams(session_dir: str) -> list:
    """Discover stream identifiers from session directory artifacts.

    Looks for patterns like: stream_<id>.mp4, <prefix>.mp4, etc.
    Returns list of (stream_id, mp4_path, srt_path, meta_path, stats_path).
    """
    session_path = Path(session_dir)
    streams = []

    mp4_files = sorted(session_path.glob("*.mp4"))
    if not mp4_files:
        return streams

    for mp4 in mp4_files:
        stem = mp4.stem
        if stem.startswith("stream_"):
            stream_id = stem
        else:
            stream_id = stem

        srt_path = session_path / f"{stem}.srt"
        meta_path = session_path / f"{stem}_meta.json"
        stats_path = session_path / f"{stem}_stats.json"

        streams.append((stream_id, str(mp4), str(srt_path), str(meta_path), str(stats_path)))

    return streams


def validate_session(session_dir: str, strict: bool = False) -> ValidationResult:
    validation = ValidationResult(session_dir=session_dir)

    if not os.path.isdir(session_dir):
        r = CheckResult(name="session_dir", status="FAIL", message="Directory not found")
        validation.results.append(r)
        return validation

    streams = discover_streams(session_dir)
    if not streams:
        r = CheckResult(name="session_dir", status="FAIL",
                        message="No .mp4 files found in session directory")
        validation.results.append(r)
        return validation

    for stream_id, mp4, srt, meta, stats in streams:
        validation.results.append(validate_mp4(mp4, strict))
        validation.results.append(validate_srt(srt, strict))
        validation.results.append(validate_meta_json(meta, strict))
        validation.results.append(validate_stats_json(stats, strict))

    return validation


def print_result(v: ValidationResult) -> None:
    print(f"\nSession: {v.session_dir}")
    print("-" * 60)
    for r in v.results:
        icon = {"PASS": "+", "FAIL": "X", "SKIP": "~", "WARN": "!"}[r.status]
        print(f"  [{icon}] {r.status:4s} {r.name}: {r.message}")
        for d in r.details:
            print(f"         {d}")
    print("-" * 60)
    if v.has_failures:
        print("RESULT: FAIL")
    else:
        print("RESULT: PASS (with warnings/skips)")
    print()


def main():
    if len(sys.argv) < 2:
        print("Usage: validate_session_artifacts.py <session_dir> [--strict]")
        sys.exit(2)

    session_dir = sys.argv[1]
    strict = "--strict" in sys.argv

    validation = validate_session(session_dir, strict)
    print_result(validation)

    sys.exit(1 if validation.has_failures else 0)


if __name__ == "__main__":
    main()
