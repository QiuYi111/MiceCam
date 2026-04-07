import json
import os

def decode_micecam_session(output_dir, session_name, target_dir=None, progress_callback=None):
    """
    Decodes a MiceCam session into individual images.
    Supports multi-sensor OAK sessions (session_A.bin, session_B.bin, etc.)
    """
    # 1. Detect if this is a multi-sensor session
    sensors = []
    # Check for X-suffix files (A, B, C, D)
    for suffix in ['_A', '_B', '_C', '_D']:
        b_path = os.path.join(output_dir, f"{session_name}{suffix}.bin")
        j_path = os.path.join(output_dir, f"{session_name}{suffix}_metadata.jsonl")
        if os.path.exists(b_path) and os.path.exists(j_path):
            sensor_label = f"CAM{suffix}"
            sensors.append({"suffix": suffix, "label": sensor_label, "bin": b_path, "jsonl": j_path})

    # If no multi-sensor files, try the single-camera default
    if not sensors:
        b_path = os.path.join(output_dir, f"{session_name}.bin")
        j_path = os.path.join(output_dir, f"{session_name}_metadata.jsonl")
        if os.path.exists(b_path) and os.path.exists(j_path):
            sensors.append({"suffix": "", "label": "", "bin": b_path, "jsonl": j_path})

    if not sensors:
        print(f"Error: No valid session files found for '{session_name}' in {output_dir}")
        return

    if target_dir is None:
        target_dir = os.path.join(output_dir, f"{session_name}_images")

    os.makedirs(target_dir, exist_ok=True)

    total_sensors = len(sensors)
    print(f"Decoding {total_sensors} sensors from session '{session_name}'...")

    for i, s in enumerate(sensors):
        sensor_target = os.path.join(target_dir, s["label"]) if s["label"] else target_dir
        os.makedirs(sensor_target, exist_ok=True)

        # Internal decoder for one file
        decode_single_file(s["bin"], s["jsonl"], sensor_target,
                          lambda p: progress_callback((i + p/100.0) / total_sensors * 100.0) if progress_callback else None)

    if progress_callback:
        progress_callback(100.0)
    print(f"Successfully decoded {total_sensors} sensors into {target_dir}")

def decode_single_file(bin_path, jsonl_path, target_dir, progress_cb):
    total_bytes = os.path.getsize(bin_path)
    frame_count = 0

    # 1. Detect pixel format from session_start metadata
    pixel_format = "mjpeg"
    width, height = 1920, 1080 # Defaults
    try:
        with open(jsonl_path, 'r') as f_meta:
            for line in f_meta:
                msg = json.loads(line)
                if msg.get("type") == "session_start":
                    pixel_format = msg.get("pixel_format", "mjpeg")
                    width = msg.get("width", width)
                    height = msg.get("height", height)
                    break
    except:
        pass

    import cv2
    import numpy as np

    with open(jsonl_path, 'r') as f_jsonl, open(bin_path, 'rb') as f_bin:
        for line in f_jsonl:
            try:
                msg = json.loads(line)
                if msg.get("type") != "frame": continue

                offset = msg["offset"]
                size = msg["size"]
                ts_ns = msg["timestamp_ns"]

                f_bin.seek(offset)
                frame_data = f_bin.read(size)
                if len(frame_data) < size: continue

                img_path = os.path.join(target_dir, f"{ts_ns}.jpg")

                if pixel_format == "uyvy422":
                    # Convert raw UYVY buffer to BGR then save as JPG
                    # Optimization: using frombuffer to avoid copy
                    yuv = np.frombuffer(frame_data, dtype=np.uint8).reshape((height, width, 2))
                    bgr = cv2.cvtColor(yuv, cv2.COLOR_YUV2BGR_UYVY)
                    cv2.imwrite(img_path, bgr, [int(cv2.IMWRITE_JPEG_QUALITY), 95])
                else:
                    # Default: Assume MJPEG/JPEG bitstream
                    with open(img_path, 'wb') as f_img:
                        f_img.write(frame_data)

                frame_count += 1
                if frame_count % 20 == 0 and progress_cb:
                    progress_cb((offset + size) / total_bytes * 100.0)
            except Exception as e:
                # Silently skip corrupt frames but continue
                pass
