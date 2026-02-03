import json
import os

def decode_micecam_session(output_dir, session_name, target_dir=None):
    """
    Decodes a MiceCam .bin session into individual images using information from .jsonl.
    
    Args:
        output_dir: Directory where the session files are located.
        session_name: Name of the session (without extension).
        target_dir: Directory where images will be saved. Defaults to {output_dir}/{session_name}_images.
    """
    bin_path = os.path.join(output_dir, f"{session_name}.bin")
    jsonl_path = os.path.join(output_dir, f"{session_name}_metadata.jsonl")
    
    if not os.path.exists(bin_path) or not os.path.exists(jsonl_path):
        print(f"Error: Session files not found in {output_dir}")
        return

    if target_dir is None:
        target_dir = os.path.join(output_dir, f"{session_name}_images")
    
    os.makedirs(target_dir, exist_ok=True)
    
    print(f"Decoding session '{session_name}'...")
    print(f"  Binary: {bin_path}")
    print(f"  Metadata: {jsonl_path}")
    print(f"  Output directory: {target_dir}")
    
    frame_count = 0
    with open(jsonl_path, 'r') as f_jsonl, open(bin_path, 'rb') as f_bin:
        for line in f_jsonl:
            try:
                msg = json.loads(line)
                if msg.get("type") != "frame":
                    continue
                
                offset = msg["offset"]
                size = msg["size"]
                # Hardware timestamp or system timestamp in ns
                ts_ns = msg["timestamp_ns"]
                
                # Seek to the frame position and read
                f_bin.seek(offset)
                frame_data = f_bin.read(size)
                
                if len(frame_data) < size:
                    print(f"Warning: Expected {size} bytes at offset {offset}, but got {len(frame_data)}")
                    continue
                
                # Save as .jpg (since it's raw MJPEG from the camera)
                img_path = os.path.join(target_dir, f"{ts_ns}.jpg")
                with open(img_path, 'wb') as f_img:
                    f_img.write(frame_data)
                
                frame_count += 1
                if frame_count % 100 == 0:
                    print(f"  Processed {frame_count} frames...")
                    
            except Exception as e:
                print(f"Error processing line: {e}")
                
    print(f"Successfully decoded {frame_count} frames into {target_dir}")
