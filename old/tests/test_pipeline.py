import urllib.request
import urllib.error
import json
import time

BASE_URL = "http://127.0.0.1:18080"

def log(msg):
    print(f"[TEST] {msg}")

def http_post(url_path, data=None):
    url = f"{BASE_URL}{url_path}"
    req = urllib.request.Request(url, method='POST')
    req.add_header('Content-Type', 'application/json')
    body = json.dumps(data).encode('utf-8') if data else b'{}'
    try:
        with urllib.request.urlopen(req, data=body) as r:
            return r.status, json.loads(r.read().decode())
    except urllib.error.HTTPError as e:
        return e.code, json.loads(e.read().decode())
    except Exception as e:
        return 500, {"error": str(e)}

def http_get(url_path, params=None):
    url = f"{BASE_URL}{url_path}"
    if params:
        from urllib.parse import urlencode
        url += "?" + urlencode(params)
    try:
        with urllib.request.urlopen(url) as r:
            try:
                content = r.read().decode()
                return r.status, json.loads(content)
            except:
                return r.status, {}
    except urllib.error.HTTPError as e:
        try:
            return e.code, json.loads(e.read().decode())
        except:
            return e.code, {}
    except Exception as e:
        return 500, {"error": str(e)}

def run_test():
    session_name = f"test_auto_{int(time.time())}"
    log(f"Starting test for session: {session_name}")

    # 1. Start
    payload = {
        "session_name": session_name,
        "output_dir": "recordings",
        "auto_decode": True,
        "device_index": 0,
        "resolution": "1920x1080",
        "fps": 30
    }

    status, res = http_post("/api/start", payload)
    if status != 200:
        log(f"Start failed: {res}")
        return
    log("Start request sent. Waiting 5s for stability check...")

    # 2. Wait and Check
    for i in range(5):
        time.sleep(1)
        # Check connection first
        try:
            status, msg = http_get("/api/status")
        except:
            log("❌ Gateway unreachable!")
            return

        log(f"Checking status: {msg}")
        if not msg.get("is_recording"):
            log("❌ Recording stopped unexpectedly! Crash detected.")
            return

    log("✅ Stability Check Passed (5s).")

    # 3. Stop
    log("Stopping recording...")
    http_post("/api/stop")

    # 4. Poll
    log("Polling decode progress (Auto-decode check)...")
    timeout = 30
    start = time.time()
    success = False
    while time.time() - start < timeout:
        status, prog = http_get("/api/decode_progress", {"session_name": session_name})
        log(f"Decode progress: {prog}")

        stat = prog.get("status")
        if stat == "completed":
            log("✅ Decode completed!")
            success = True
            break
        elif stat == "failed":
            log(f"❌ Decode failed: {prog.get('error')}")
            break

        time.sleep(1)

    if not success:
        log("❌ Decode timed out.")

    # 5. Isolation Check
    log("Checking isolation (querying wrong session)...")
    _, prog = http_get("/api/decode_progress", {"session_name": "wrong_session"})
    log(f"Wrong session result: {prog}")

    if prog.get("status") == "idle" or prog.get("reason") == "session_mismatch":
         log("✅ Session isolation confirmed.")
    else:
         log("⚠️ Session isolation check failed (Likely Gateway not restarted yet).")

if __name__ == "__main__":
    run_test()
