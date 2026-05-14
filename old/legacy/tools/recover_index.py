#!/usr/bin/env python3
import json
import os
import sys
import mmap
from pathlib import Path

def recover_mjpeg_index(bin_path, metadata_path_out):
    bin_path = Path(bin_path)
    if not bin_path.exists():
        print(f"Error: {bin_path} does not exist.")
        return

    print(f"Fast scanning {bin_path} using mmap...")
    file_size = bin_path.stat().st_size
    frames = []

    with open(bin_path, "rb") as f:
        # Use mmap for high-performance scanning on large files
        with mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as mm:
            pos = 0
            while True:
                soi = mm.find(b"\xff\xd8", pos)
                if soi == -1:
                    break

                if frames:
                    frames[-1]["size"] = soi - frames[-1]["offset"]

                frames.append({
                    "sequence_id": len(frames),
                    "timestamp_ns": 0,
                    "offset": soi,
                    "size": 0,
                    "checksum": 0
                })

                if len(frames) % 1000 == 0:
                    print(f"Found {len(frames)} frames... ({(soi/file_size)*100:.1f}%)", end="\r")

                pos = soi + 2

    if frames:
        frames[-1]["size"] = file_size - frames[-1]["offset"]

    print(f"\nScan complete. Total frames found: {len(frames)}")

    metadata = {
        "session": {
            "session_name": bin_path.stem + "_recovered",
            "camera_backend": "Recovered from scan",
            "width": 3840,
            "height": 2160,
            "fps": 30.0,
            "total_frames": len(frames),
            "total_bytes": file_size,
            "start_timestamp_ns": 0,
            "end_timestamp_ns": 0,
            "session_checksum": 0
        },
        "frames": frames
    }

    print(f"Writing reconstructed metadata to {metadata_path_out}...")
    with open(metadata_path_out, "w") as f:
        json.dump(metadata, f, indent=2)
    print("Done.")

if __name__ == "__main__":
    bin_file = "test_output/ffmpeg_bench_4K_30fps.bin"
    out_json = "test_output/ffmpeg_bench_4K_30fps_recovered_metadata.json"
    recover_mjpeg_index(bin_file, out_json)
