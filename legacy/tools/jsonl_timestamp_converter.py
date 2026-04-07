#!/usr/bin/env python3
"""Convert MiceCam JSONL timestamps from steady_clock (ns since boot) to human-readable ISO format.

Usage:
    python jsonl_timestamp_converter.py input.jsonl "2026-02-26 15:46:21" [-o output.jsonl]
    python jsonl_timestamp_converter.py input.jsonl "2026-02-26 15:46:21" --in-place
"""

import argparse
import json
import sys
from datetime import datetime, timezone, timedelta


def parse_boot_time(s: str) -> datetime:
    """Parse boot time string, assume local timezone (Asia/Shanghai by default)."""
    for fmt in ("%Y-%m-%d %H:%M:%S", "%Y-%m-%d %H:%M", "%Y/%m/%d %H:%M:%S"):
        try:
            dt = datetime.strptime(s.strip(), fmt)
            # Assume local timezone — use system tz or default to Asia/Shanghai
            try:
                local_tz = datetime.now().astimezone().tzinfo
            except OSError:
                local_tz = timezone(timedelta(hours=8))  # CST fallback
            return dt.replace(tzinfo=local_tz)
        except ValueError:
            continue
    raise ValueError(f"Cannot parse boot time: {s!r}")


def convert_value(val: int, boot_epoch_ns: int) -> str:
    """Convert ns-since-boot to ISO 8601 string."""
    target_epoch_ns = boot_epoch_ns + val
    target_s = target_epoch_ns / 1e9
    dt = datetime.fromtimestamp(target_s, tz=timezone.utc)
    # Format with microsecond precision (ns doesn't fit in ISO)
    iso = dt.strftime("%Y-%m-%dT%H:%M:%S")
    us = int(target_epoch_ns % 1_000_000_000 / 1000)
    return f"{iso}.{us:06d}Z"


def process_file(input_path: str, boot_time: datetime, output_path: str | None):
    ts_fields = {"timestamp_ns", "start_timestamp_ns", "end_timestamp_ns"}

    with open(input_path, "r", encoding="utf-8") as fin:
        lines = fin.readlines()

    boot_epoch_ns = int(boot_time.timestamp() * 1e9)
    converted = 0
    total = len(lines)

    out_lines = []
    for line in lines:
        line = line.strip()
        if not line:
            out_lines.append(line)
            continue
        try:
            record = json.loads(line)
        except json.JSONDecodeError:
            out_lines.append(line)
            continue

        changed = False
        for field in ts_fields:
            if field in record and isinstance(record[field], int) and record[field] > 0:
                original_ns = record.pop(field)
                record[f"{field}_iso"] = convert_value(original_ns, boot_epoch_ns)
                record[f"{field}_raw"] = original_ns
                changed = True

        if changed:
            converted += 1

        out_lines.append(json.dumps(record, ensure_ascii=False))

    dest = output_path or input_path.replace(".jsonl", "_readable.jsonl")
    with open(dest, "w", encoding="utf-8") as fout:
        fout.write("\n".join(out_lines))
        if out_lines and out_lines[-1]:
            fout.write("\n")

    print(f"Converted {converted}/{total} records -> {dest}")


def main():
    parser = argparse.ArgumentParser(description="Convert MiceCam JSONL timestamps to human-readable format")
    parser.add_argument("input", help="Input JSONL file path")
    parser.add_argument("boot_time", help="Boot time, e.g. '2026-02-26 15:46:21'")
    parser.add_argument("-o", "--output", help="Output file path (default: *_readable.jsonl)")
    parser.add_argument("--in-place", action="store_true", help="Overwrite input file")
    args = parser.parse_args()

    if args.in_place:
        args.output = args.input

    boot_time = parse_boot_time(args.boot_time)
    print(f"Boot time: {boot_time.isoformat()}")

    process_file(args.input, boot_time, args.output)


if __name__ == "__main__":
    main()
