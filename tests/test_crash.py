import sys
import os
import time

# Add SDK path
sdk_paths = [
    os.path.abspath("build/bindings/python/Release"),
    r"D:\MiceCam\build\bindings\python\Release"
]
for p in sdk_paths:
    if os.path.exists(p) and p not in sys.path:
        sys.path.insert(0, p)

try:
    import _micecam
    print("SDK Loaded.")
except ImportError as e:
    print(f"Failed to load SDK: {e}")
    sys.exit(1)

def main():
    print("Initializing OAK Master...")
    try:
        master = _micecam.OAKMaster()
        print("Created Master Object.")
        if master.initialize(1280, 800, 30.0):
             print("Initialized Master Hardware.")
        else:
             print("Failed Init.")

        master.start()
        print("Started.")
        time.sleep(2)
        master.stop()
        print("Stopped.")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()
