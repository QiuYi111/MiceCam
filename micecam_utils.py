import json
import os

def decode_micecam_session(output_dir, session_name, target_dir=None, progress_callback=None):
    """
    Decodes a MiceCam .bin session into individual images.
    progress_callback: function(percent_complete: float)
    """
    bin_path = os.path.join(output_dir, f"{session_name}.bin")
    jsonl_path = os.path.join(output_dir, f"{session_name}_metadata.jsonl")
    
    if not os.path.exists(bin_path) or not os.path.exists(jsonl_path):
        print(f"Error: Session files not found in {output_dir}")
        return

    total_bytes = os.path.getsize(bin_path)

    if target_dir is None:
        target_dir = os.path.join(output_dir, f"{session_name}_images")
    
    os.makedirs(target_dir, exist_ok=True)
    
    print(f"Decoding session '{session_name}'...")
    
    frame_count = 0
    with open(jsonl_path, 'r') as f_jsonl, open(bin_path, 'rb') as f_bin:
        for line in f_jsonl:
            try:
                msg = json.loads(line)
                if msg.get("type") != "frame":
                    continue
                
                offset = msg["offset"]
                size = msg["size"]
                ts_ns = msg["timestamp_ns"]
                
                f_bin.seek(offset)
                frame_data = f_bin.read(size)
                
                if len(frame_data) < size:
                    print(f"Warning: incomplete frame at {offset}")
                    continue
                
                img_path = os.path.join(target_dir, f"{ts_ns}.jpg")
                with open(img_path, 'wb') as f_img:
                    f_img.write(frame_data)
                
                frame_count += 1
                if frame_count % 10 == 0:
                    if progress_callback and total_bytes > 0:
                        pct = (offset + size) / total_bytes * 100.0
                        progress_callback(pct)
                    
            except Exception as e:
                print(f"Error processing line: {e}")
                
    if progress_callback:
        progress_callback(100.0)
    print(f"Successfully decoded {frame_count} frames into {target_dir}")
