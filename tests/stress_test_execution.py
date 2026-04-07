import micecam
import time
import os
import sys

def run_stress_test(backend, duration_sec=600):
    print(f"\n" + "="*50)
    print(f"Starting {duration_sec/60} minute stress test for {backend}...")
    print("="*50)

    output_dir = "stress_test_results"
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    try:
        # Create pipeline
        pipeline = micecam.Pipeline(
            output_dir=output_dir,
            session_name=f"stress_{backend}",
            backend_name=backend,
            width=1920, height=1080, fps=30.0
        )

        print(f"Pipeline created for {backend}. Starting...")
        # Note: pipeline.start() is void and throws if fails
        pipeline.start()
        print(f"Pipeline {backend} started successfully.")

        start_time = time.time()

        while time.time() - start_time < duration_sec:
            stats = pipeline.get_stats()
            elapsed = int(time.time() - start_time)

            captured = stats.get('captured_frames', 0)
            dropped = stats.get('dropped_frames', 0)
            rate = stats.get('drop_rate', 0.0)
            throughput = stats.get('throughput_mbps', 0.0)

            print(f"[{backend}] {elapsed:3}s | Total: {captured:6} | Drops: {dropped:3} | DropRate: {rate:6.4%} | {throughput:6.1f} Mbps")

            time.sleep(10)

        print(f"\nStopping {backend} pipeline...")
        pipeline.stop()

        final_stats = pipeline.get_stats()
        print(f"FINAL RESULT for {backend}:")
        print(f"  Total Frames: {final_stats['captured_frames']}")
        print(f"  Total Drops: {final_stats['dropped_frames']}")
        print(f"  Final Drop Rate: {final_stats['drop_rate']:.4%}")

        if final_stats['drop_rate'] < 0.001:  # < 0.1% drops
            print(f"VERDICT: {backend} is STABLE")
            return True
        else:
            print(f"VERDICT: {backend} stability WARNING (Drop rate > 0.1%)")
            return False

    except Exception as e:
        print(f"CRITICAL ERROR during {backend} test: {e}")
        return False

if __name__ == "__main__":
    print("MiceCam Professional Stability Verification")

    # Enable OAK and USB tests
    results = {}

    # 1. USB Stress Test (10 Min)
    results["USB"] = run_stress_test("usb", duration_sec=600)

    # 2. OAK Stress Test (10 Min)
    results["OAK"] = run_stress_test("oak", duration_sec=600)

    print("\n" + "#"*50)
    print("OVERALL STRESS TEST SUMMARY")
    for cam, res in results.items():
        status = "PASSED" if res else "FAILED"
        print(f"{cam:4}: {status}")
    print("#"*50)

    if all(results.values()):
        sys.exit(0)
    else:
        sys.exit(1)
