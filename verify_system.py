import urllib.request
import json
import time
import sys
import psutil
import os

API_URL = "http://127.0.0.1:18080/api"

def api_call(endpoint, method="GET", data=None):
    url = f"{API_URL}{endpoint}"
    req = urllib.request.Request(url, method=method)
    req.add_header('Content-Type', 'application/json')
    
    if data:
        body = json.dumps(data).encode('utf-8')
        req.data = body
        
    try:
        with urllib.request.urlopen(req) as response:
            return json.loads(response.read().decode('utf-8'))
    except Exception as e:
        print(f"API Error ({endpoint}): {e}")
        return None

def find_worker_pid():
    for proc in psutil.process_iter(['pid', 'name', 'cmdline']):
        try:
            if "recorder_worker.py" in " ".join(proc.info['cmdline'] or []):
                return proc.info['pid']
        except: pass
    return None

def test_happy_path():
    print("\n--- Test 1: Happy Path (Start/Stop) ---")
    
    # 1. Stop if running
    api_call("/stop", "POST")
    time.sleep(1)
    
    # 2. Start
    print("Starting recording...")
    res = api_call("/start", "POST", {
        "device_index": 0,
        "session_name": "test_happy_path",
        "output_dir": "test_output",
        "fps": 30
    })
    if not res or not res.get("success"):
        print("FAIL: Could not start recording")
        return False
        
    # 3. Wait and check status
    time.sleep(3)
    status = api_call("/status")
    if not status or not status.get("is_recording"):
        print(f"FAIL: Status is not recording: {status}")
        return False
    print(f"Status OK: Captured {status.get('captured')} frames")
    
    # 4. Stop
    print("Stopping...")
    api_call("/stop", "POST")
    time.sleep(1)
    status = api_call("/status")
    if status.get("is_recording"):
        print("FAIL: Failed to stop")
        return False
        
    print("PASS: Happy Path")
    return True

def test_chaos_recovery():
    print("\n--- Test 2: Chaos Recovery (Worker Kill) ---")
    
    # 1. Start
    api_call("/start", "POST", {
        "device_index": 0,
        "session_name": "test_chaos", 
        "output_dir": "test_output",
        "fps": 30
    })
    time.sleep(3)
    
    # 2. Kill Worker
    worker_pid = find_worker_pid()
    if not worker_pid:
        print("FAIL: Could not find worker process to kill")
        return False
        
    print(f"killing worker process {worker_pid}...")
    psutil.Process(worker_pid).kill()
    
    # 3. Check Status immediately (Should be recovering or True, NOT False)
    print("Checking status immediately after kill...")
    for i in range(5):
        status = api_call("/status")
        print(f"Status T+{i*0.5}s: {status}")
        if not status or not status.get("is_recording"):
             print("FAIL: API reported is_recording=False during recovery!")
             # Ensure cleanup
             api_call("/stop", "POST")
             return False
        if status.get("status") == "recovering":
            print("SUCCESS: API reported 'recovering' status!")
            break
        time.sleep(0.5)

    # 4. Wait for recovery
    print("Waiting for worker to respawn...")
    time.sleep(3)
    
    new_pid = find_worker_pid()
    if not new_pid or new_pid == worker_pid:
        print("FAIL: Worker did not respawn")
        return False
    print(f"Worker respawned with PID {new_pid}")
    
    # 5. Stop
    api_call("/stop", "POST")
    print("PASS: Chaos Recovery")
    return True

def main():
    if not test_happy_path(): sys.exit(1)
    if not test_chaos_recovery(): sys.exit(1)
    print("\nALL TESTS PASSED")

if __name__ == "__main__":
    main()
