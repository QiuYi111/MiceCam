import requests
import time
import os
import signal
import subprocess

API_URL = "http://127.0.0.1:18080/api"

def test_oak_recording():
    session_name = "test_auto_resume"
    print(f"--- Starting OAK Stress Test: {session_name} ---")

    # Cooldown
    time.sleep(2)

    # 1. Start Recording
    payload = {
        "device_index": "oak",
        "session_name": session_name,
        "auto_decode": True,
        "fps": 30.0
    }
    resp = requests.post(f"{API_URL}/start", json=payload)
    if not resp.json().get("success"):
        print(f"FAILED to start: {resp.json()}")
        return

    print("Recording started. Waiting 10s for activity...")
    time.sleep(10)

    # Check status
    stats = requests.get(f"{API_URL}/status").json()
    print(f"Initial Stats: {stats}")

    if not stats.get("is_recording") or stats.get("error"):
        print(f"ERROR: Recording failed or inactive: {stats.get('error')}")
        return

    # 2. Simulate Crash
    print("--- SIMULATING CRASH: Killing recorder worker ---")
    # On Windows, we find the process using 'recorder_worker.py'
    try:
        proc_list = subprocess.check_output('wmic process where "commandline like \'%recorder_worker.py%\'" get processid', shell=True).decode()
        pids = [p.strip() for p in proc_list.split('\n') if p.strip() and p.strip().isdigit()]
        for pid in pids:
            print(f"Killing PID {pid}")
            subprocess.run(f"taskkill /F /PID {pid}", shell=True)
    except Exception as e:
        print(f"Failed to kill worker: {e}")

    print("Worker killed. Waiting for Gateway to auto-resume (approx 3-5s)...")
    time.sleep(10)

    stats = requests.get(f"{API_URL}/status").json()
    print(f"Stats after resume: {stats}")

    captured = stats.get("captured")
    if stats.get("is_recording") and captured is not None and captured >= 0:
        print("SUCCESS: Auto-resume verified.")
    else:
        print("FAILED: Auto-resume not detected.")
        return

    # 3. Stop and Verify Decoding
    print("Stopping recording...")
    requests.post(f"{API_URL}/stop")

    print("Waiting for decoding to complete...")
    for _ in range(30):
        time.sleep(2)
        prog = requests.get(f"{API_URL}/decode_progress", params={"session_name": session_name}).json()
        print(f"Decoding: {prog.get('percent', 0)}% ({prog.get('status')})")
        if prog.get("status") == "completed":
            print("Decoding finished.")
            break

    # 4. Final Directory Check
    target_dir = os.path.join("recordings", f"{session_name}_images")
    subfolders = ["CAM_A", "CAM_B", "CAM_C", "CAM_D"]
    all_ok = True
    for sf in subfolders:
        path = os.path.join(target_dir, sf)
        if os.path.exists(path) and any(os.scandir(path)):
            print(f"Folder {sf}: OK (Contains files)")
        else:
            print(f"Folder {sf}: FAILED (Missing or empty)")
            all_ok = False

    if all_ok:
        print("--- ALL OAK FEATURES VERIFIED SUCCESSFULLY ---")
    else:
        print("--- FEATURE VERIFICATION FAILED ---")

if __name__ == "__main__":
    test_oak_recording()
