# Build MiceCam EXE
Write-Host "Installing PyInstaller..."
uv pip install pyinstaller

Write-Host "Building EXE..."
# Clean previous
if (Test-Path "dist") { Remove-Item -Recurse -Force "dist" }
if (Test-Path "build") { Remove-Item -Recurse -Force "build" }

# Determine SDK Path (User specific)
$sdk_path = "build/bindings/python/Release"
if (!(Test-Path $sdk_path)) {
    $sdk_path = "..\build\bindings\python\Release" # Try relative
}

# Run PyInstaller
# --noconsole: Hide terminal
# --onefile: Single EXE (Optional, removed for speed/debugging first)
# --add-data: Include bindings
# --name: MiceCam

# Using 'uv run' to ensure context
uv run pyinstaller --noconsole `
    --name "MiceCam" `
    --add-data "$($sdk_path);." `
    --hidden-import "PyQt6" `
    --hidden-import "micecam" `
    --icon "ui/favicon.ico" `
    micecam_app.py

Write-Host "Build Complete. Executable is in dist/MiceCam/MiceCam.exe"
