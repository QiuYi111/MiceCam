import subprocess
import time
import sys
import os
import json

def main():
    print("🛡️ MiceCam Server Supervisor Started")
    print("   This watchdog will automatically restart the backend if the camera driver crashes.")
    print("   Press Ctrl+C to stop the supervisor.")

    restart_count = 0
    
    while True:
        try:
            print(f"\n[Supervisor] 🚀 Starting server instance (Session {restart_count+1})...")
            
            # Run server.py using the same Python interpreter
            cmd = [sys.executable, "server.py"]
            
            # Inject PYTHONPATH for SDK
            env = os.environ.copy()
            sdk_path = os.path.abspath("build/bindings/python/Release")
            # fallback for dev layout
            if not os.path.exists(sdk_path):
                 sdk_path = r"D:\MiceCam\build\bindings\python\Release"
            
            if "PYTHONPATH" in env:
                env["PYTHONPATH"] = sdk_path + os.pathsep + env["PYTHONPATH"]
            else:
                env["PYTHONPATH"] = sdk_path
                
            print(f"[Supervisor] 🔧 PYTHONPATH set to: {sdk_path}")
            
            # Start the process
            start_ts = time.time()
            process = subprocess.run(cmd, cwd=os.getcwd(), env=env)
            
            # Process finished
            duration = time.time() - start_ts
            exit_code = process.returncode
            
            # Check for pending decode job (Crash recovery)
            p_job = "pending_decode.json"
            if os.path.exists(p_job):
                print(f"[Supervisor] 🔎 Found job file: {p_job}")
                try:
                    with open(p_job, "r") as f:
                        job = json.load(f)
                    
                    if job.get("auto_decode"):
                        print(f"[Supervisor] 🧹 Processing decode job for: {job.get('session_name')}")
                        # Spawn background decoder
                        cmd_decode = [sys.executable, "decoder.py", job['output_dir'], job['session_name']]
                        # Run detached/background
                        subprocess.Popen(cmd_decode, cwd=os.getcwd(), env=env)
                        print("[Supervisor] 🎞️ Decoder started in background.")
                    else:
                        print("[Supervisor] Job does not require auto-decode.")
                    
                    # Clean up job file
                    os.remove(p_job)
                    print("[Supervisor] Job file removed.")
                except Exception as e:
                    print(f"[Supervisor] ❌ Failed to process pending job: {e}")
            else:
                # Debug only if needed, but keeps logs clean usually
                pass
            
            if exit_code == 0:
                print("[Supervisor] ✅ Server stopped normally.")
                break
            
            # If server crashed quickly (boot loop protection)
            if duration < 2:
                print("[Supervisor] ⚠️ Server crashed too quickly. Waiting 2s before retry...")
                time.sleep(2)
            else:
                print(f"[Supervisor] ⚠️ Server exited with code {exit_code}. Restarting immediately...")
            
            restart_count += 1
            
        except KeyboardInterrupt:
            print("\n[Supervisor] 🛑 Stopped by user.")
            break
        except Exception as e:
            print(f"[Supervisor] ❌ Error: {e}")
            time.sleep(1)

if __name__ == "__main__":
    main()
