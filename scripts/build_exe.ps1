# Build MiceCam EXE
# Run from root as: .\scripts\build_exe.ps1

$ScriptRoot = $PSScriptRoot
$ProjectRoot = "$PSScriptRoot\.."

Write-Host "Installing PyInstaller..."
uv pip install pyinstaller

Write-Host "Cleaning previous builds..."
if (Test-Path "$ProjectRoot\dist") { Remove-Item -Recurse -Force "$ProjectRoot\dist" }
if (Test-Path "$ProjectRoot\build_pyinstaller") { Remove-Item -Recurse -Force "$ProjectRoot\build_pyinstaller" }

# Determine SDK Path (User specific)
$sdk_path = "$ProjectRoot\build\bindings\python\Release"
if (!(Test-Path $sdk_path)) {
    $sdk_path = "$ProjectRoot\build\bindings\python\Release" # Try relative
}

# --- 1. Build Worker (The "Engine") ---
Write-Host "Building MiceCamWorker.exe (Engine)..."
Set-Location $ProjectRoot

uv run pyinstaller --noconsole `
    --name "MiceCamWorker" `
    --workpath "build_pyinstaller/worker" `
    --distpath "dist/MiceCam" `
    --add-data "$($sdk_path);." `
    --hidden-import "micecam" `
    --hidden-import "_micecam" `
    --hidden-import "debug_utils" `
    --exclude-module "PyQt6" `
    --icon "assets/app_icon.ico" `
    --paths "app" `
    app/recorder_worker.py

# --- 2. Build UI (The "Client") ---
Write-Host "Building MiceCam.exe (UI)..."

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
    --icon "assets/app_icon.ico" `
    --paths "app" `
    app/micecam_app.py

Write-Host "Build Complete!"
Write-Host " - UI:     dist/MiceCam/MiceCam.exe"
Write-Host " - Worker: dist/MiceCam/MiceCamWorker.exe"

# --- 3. Organized Release Folder ---
$release_dir = "dist/MiceCam_Release"
if (Test-Path $release_dir) { Remove-Item -Recurse -Force $release_dir }
New-Item -ItemType Directory -Force -Path $release_dir | Out-Null

Write-Host "Organizing Release to $release_dir ..."

# Copy UI (Root)
Copy-Item -Recurse -Force "dist/MiceCam/MiceCam/*" $release_dir

# Copy Worker (Tools)
$tools_dir = "$release_dir/tools/MiceCamWorker"
New-Item -ItemType Directory -Force -Path $tools_dir | Out-Null
Copy-Item -Recurse -Force "dist/MiceCam/MiceCamWorker/*" $tools_dir

Write-Host "Done! Distribution Ready at: $release_dir"

# --- 4. Build MSI (Optional) ---
# --- 4. Build MSI (Optional) ---
if (Test-Path "scripts/setup.wxs") {
   Write-Host "Building MSI..."
   
   # Ensure Release Directory
   if (!(Test-Path "release")) { New-Item -ItemType Directory -Force -Path "release" | Out-Null }

   # Clean recordings from release before packing
   if (Test-Path "$release_dir\recordings") { Remove-Item -Recurse -Force "$release_dir\recordings" }
   
   & "C:\Program Files (x86)\WiX Toolset v3.14\bin\heat.exe" dir "$release_dir" -cg MiceCamGroup -dr INSTALLFOLDER -scom -sreg -sfrag -srd -gg -out files.wxs
   & "C:\Program Files (x86)\WiX Toolset v3.14\bin\candle.exe" scripts/setup.wxs files.wxs -ext WixUIExtension
   & "C:\Program Files (x86)\WiX Toolset v3.14\bin\light.exe" -out "release/MiceCam.msi" setup.wixobj files.wixobj -b "$release_dir" -ext WixUIExtension
   
   Write-Host "MSI Created: release/MiceCam.msi"
}

