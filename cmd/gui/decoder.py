import sys
import os
import micecam_utils
import time

import json

def main():
    if len(sys.argv) < 3:
        print("Usage: python decoder.py <output_dir> <session_name> [target_dir]")
        sys.exit(1)
        
    output_dir = sys.argv[1]
    session_name = sys.argv[2]
    target_dir = sys.argv[3] if len(sys.argv) > 3 else None
    
    print(f"Starting background decode job for: {session_name}")
    
    def on_progress(pct):
        try:
            status = {
                "session": session_name,
                "percent": pct,
                "status": "decoding" if pct < 100 else "completed"
            }
            # Write safely
            with open("decode_progress.json.tmp", "w") as f:
                json.dump(status, f)
            os.replace("decode_progress.json.tmp", "decode_progress.json")
        except:
            pass

    on_progress(0.0)
    
    try:
        micecam_utils.decode_micecam_session(output_dir, session_name, target_dir, on_progress)
        print("Decode job finished successfully.")
        on_progress(100.0)
    except Exception as e:
        print(f"Decode job failed: {e}")
        try:
            with open("decode_progress.json", "w") as f:
                json.dump({"session": session_name, "percent": 0, "status": "failed", "error": str(e)}, f)
        except:
            pass

if __name__ == "__main__":
    main()
