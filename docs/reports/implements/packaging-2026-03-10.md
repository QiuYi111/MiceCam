# Walkthrough: Windows EXE and MSI Packaging

I have successfully configured the GitHub Actions workflow to automatically package the MiceCam application for Windows.

## Changes Made

### 1. GitHub CI/CD Pipeline
- **File**: [.github/workflows/package.yml](file:///Users/qiujingyi.7/MiceCam/.github/workflows/package.yml)
- **New Features**:
    - Added `PyInstaller` integration to build two standalone executables: `MiceCam.exe` (GUI) and `MiceCamWorker.exe` (Recording Engine).
    - Integrated `WiX Toolset` (Heat, Candle, Light) to bundle these executables into a single `MiceCam-Installer.msi`.
    - Updated artifact upload to include `.msi`, `.exe`, and Python packages.

### 2. Installer Configuration
- **File**: [packaging/windows/micecam.wxs](file:///Users/qiujingyi.7/MiceCam/packaging/windows/micecam.wxs)
- **Details**: Defines the installer metadata, installation folder (`C:\Program Files\MiceCam`), and Start Menu shortcuts.

### 3. Local Helper Script
- **File**: [scripts/build_windows.ps1](file:///Users/qiujingyi.7/MiceCam/scripts/build_windows.ps1)
- **Purpose**: Allows you to run the same packaging logic on a local Windows machine if needed.

## Verification Steps

To verify the changes and download your MSI:

1.  **Push Changes**:
    ```bash
    git add .
    git commit -m "feat: add windows exe and msi packaging"
    git push
    ```
2.  **Monitor GitHub Actions**: Go to the **Actions** tab in your repository and wait for the "Build and Package" workflow to finish.
3.  **Download**: Once complete, look in the **Artifacts** section of the run. You will find `MiceCam-Installer.msi` inside the Windows artifact zip.

## Important Note on C++ Backend
The packaging logic automatically finds the compiled `micecam*.pyd` file and bundles it inside the executables. This ensures that the high-speed acquisition features work out-of-the-box after installation.
