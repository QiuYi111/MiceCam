# Build MiceCam EXE
Write-Host "Installing PyInstaller..."
uv pip install pyinstaller

Write-Host "Cleaning previous builds..."
if (Test-Path "dist") { Remove-Item -Recurse -Force "dist" }
if (Test-Path "build_pyinstaller") { Remove-Item -Recurse -Force "build_pyinstaller" }

# Determine SDK Path (User specific)
$sdk_path = "build/bindings/python/Release"
if (!(Test-Path $sdk_path)) {
    $sdk_path = "..\build\bindings\python\Release" # Try relative
}

# --- 1. Build Worker (The "Engine") ---
Write-Host "Building MiceCamWorker.exe (Engine)..."
# This contains the heavy libraries: _micecam, depthai, opencv
# It does NOT use PyQt6
uv run pyinstaller --noconsole `
    --name "MiceCamWorker" `
    --workpath "build_pyinstaller/worker" `
    --distpath "dist/MiceCam" `
    --add-data "$($sdk_path);." `
    --hidden-import "micecam" `
    --hidden-import "_micecam" `
    --hidden-import "debug_utils" `
    --exclude-module "PyQt6" `
    --icon "app_icon.ico" `
    recorder_worker.py

# --- 2. Build UI (The "Client") ---
Write-Host "Building MiceCam.exe (UI)..."
# This contains ONLY UI logic. 
# EXPLICITLY EXCLUDE heavy libs to prevent DLL conflicts.
uv run pyinstaller --noconsole `
    --name "MiceCam" `
    --workpath "build_pyinstaller/ui" `
    --distpath "dist/MiceCam" `
    --hidden-import "PyQt6" `
    --hidden-import "micecam_utils" `
    --hidden-import "debug_utils" `
    --exclude-module "micecam" `
    --exclude-module "_micecam" `
    --exclude-module "depthai" `
    --exclude-module "cv2" `
    --exclude-module "numpy" `
    --exclude-module "recorder_worker" `
    --icon "app_icon.ico" `
    micecam_app.py

Write-Host "Build Complete!"
Write-Host " - UI:     dist/MiceCam/MiceCam.exe"
Write-Host " - Worker: dist/MiceCam/MiceCamWorker.exe"

# --- 3. Organized Release Folder ---
$release_dir = "dist/MiceCam_Release"
if (Test-Path $release_dir) { Remove-Item -Recurse -Force $release_dir }
New-Item -ItemType Directory -Force -Path $release_dir | Out-Null

Write-Host "Organizing Release to $release_dir ..."

# Copy UI (Root)
# We copy contents of dist/MiceCam/MiceCam/* to dist/MiceCam_Release/*
Copy-Item -Recurse -Force "dist/MiceCam/MiceCam/*" $release_dir

# Copy Worker (Tools)
# We copy contents of dist/MiceCam/MiceCamWorker to dist/MiceCam_Release/tools/MiceCamWorker
$tools_dir = "$release_dir/tools/MiceCamWorker"
New-Item -ItemType Directory -Force -Path $tools_dir | Out-Null
Copy-Item -Recurse -Force "dist/MiceCam/MiceCamWorker/*" $tools_dir

Write-Host "Done! Distribution Ready at: $release_dir"
Write-Host "User should run: $release_dir/MiceCam.exe"
