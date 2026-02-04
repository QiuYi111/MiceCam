## 1. Problem Definition

 **Symptom** : The MiceCam Desktop Application terminates immediately ("Flashback") when stating a recording, or seemingly at random during initialization.  **Impact** : The application is unusable for production recording.  **Context** :

* Original WebUI (Gateway):  **Stable** . Uses native Python `multiprocessing`.
* New Qt App (Single EXE):  **Unstable** . Uses PyInstaller-bundled executable spawning itself (`MiceCam.exe --worker`).

## 2. Root Cause Analysis

The crash is almost certainly a **C++ Access Violation (Segmentation Fault)** occurring at the native layer, bypassing Python's exception handling.

### Why "Flashback"?

A "Flashback" (Instant process disappearance without error dialog) indicates the OS has terminated the process due to a severe memory violation.

### Points of Failure in Current Architecture

We attempted a **"Self-Spawning Supervisor"** pattern: `MiceCam.exe (UI)` --> spawns --> `MiceCam.exe --worker (Worker)`

1. **Risk 1: Shared Memory Space / DLL Conflicts (High Probability)**
   * Even though they are separate processes, PyInstaller's `_internal` folder is shared.
   * If the UI process (PyQt6) holds locks on low-level Windows APIs (e.g. COM, USB, Graphics Drivers), and the Worker process tries to initialize the generic Camera SDK (which uses OpenCV/DepthAI), a **Driver-Level Conflict** can occur.
   * *Evidence* : The fact it crashes "instantaneously" suggests a low-level collision.
2. **Risk 2: PyInstaller Bootloader Recursion (Medium Probability)**
   * When `MiceCam.exe` runs, it unpacks/initializes a Python runtime.
   * When it spawns  *itself* , the child process tries to re-initialize against the same temporary directory or environment.
   * Failure here often results in a "Silent Death" or the "Failed to load encodings" error we saw earlier.
3. **Risk 3: Implicit Import (Low Probability, checked)**
   * If the UI process *accidentally* imports

     _micecam (even via a chain like `gui -> utils -> micecam`), then the UI process Itself becomes vulnerable to the Driver crash.
   * *Mitigation* : We actively cleaned this, but Python imports are sticky.

## 3. The "True Fix": Physical Separation

To achieve the stability of the WebUI (Gateway), we must replicate its architecture exactly. The Gateway **did not** package the UI and Worker in one binary. They were separate logic units.

**Proposed Solution: Dual Binary Architecture** Instead of one generic `MiceCam.exe` that tries to do everything, we build two distinct executables:

1. **`MiceCam.exe` (The Output)** :

* **Contains** : ONLY PyQt6, UI Logic, IPC Logic.
* **Excludes** :

    _micecam,`depthai`, `opencv` (physically impossible to load them).

* **Role** : Displays Window. Launches `MiceCamWorker.exe`.

1. **`MiceCamWorker.exe` (The Engine)** :

* **Contains** :

    _micecam,

    recorder_worker.py.

* **Excludes** : PyQt6 (Save size/startup time).
* **Role** : Recording only. Headless.

### Why this fixes it?

* **Operating System Guarantee** : The UI process *cannot* crash from a Driver Segfault because it  *does not have the driver code loaded* .
* **Zero Interference** : The Worker process runs in a completely 100% clean environment, just like running `python recorder_worker.py` from CLI (which we know works).

## 4. Diagnostics Plan (Immediate + Long Term)

To confirm the analysis before refactoring:

1. **Unbuffered File Logging** :

* We have added

  debug_utils.py.
* It writes `[PID] Message` to `MiceCam_Debug.log` instantly (fsync).
* *Expected Result* : We will see the UI log "Spawning Worker", and then the Worker log "Initializing SDK"... followed by silence (Crash). This proves the location.

1. **Crash Dump (Optional)** :

* Enabling Windows Error Reporting (WER) to catch `.dmp` files. (Usually overkill if we just separate binaries).

## 5. Execution Roadmap

1. **Verify Logs** : Run current build once to see `MiceCam_Debug.log`.
2. **Refactor Build Script** : Update

   build_exe.ps1 to generate TWO EXEs (`MiceCam` and `Worker`).

1. **Update Supervisor** : Point

   recorder_thread.py to `Worker.exe` instead of `self.executable --worker`.
