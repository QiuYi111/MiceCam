# Build script for Windows packaging
# This script is intended to be run on a Windows machine with PowerShell.

$ProjectRoot = Get-Location
$BuildDir = "$ProjectRoot\build"
$DistDir = "$ProjectRoot\dist"
$PythonEnv = "$ProjectRoot\.venv"

echo "🔨 Initializing Windows Environment..."
if (!(Test-Path "vcpkg")) {
    echo "📦 vcpkg not found. Cloning vcpkg..."
    git clone https://github.com/microsoft/vcpkg.git vcpkg
    .\vcpkg\bootstrap-vcpkg.bat
}

# 1. Build C++ Backend
echo "🔨 Building C++ SDK and Bindings..."
cmake -B $BuildDir -S . `
    -DCMAKE_TOOLCHAIN_FILE="$ProjectRoot\vcpkg\scripts\buildsystems\vcpkg.cmake" `
    -DCMAKE_BUILD_TYPE=Release `
    -DWITH_PYTHON_BINDINGS=ON
cmake --build $BuildDir --config Release -j$env:NUMBER_OF_PROCESSORS

# 2. Setup Python environment
echo "🐍 Setting up Python environment..."
uv sync --all-extras
uv pip install pyinstaller

# 3. Package to EXE
echo "📦 Packaging GUI to EXE..."
# Note: We need to include the build artifacts (.pyd and required DLLs)
# Assuming micecam module is in 'internal/' as per pyproject.toml
# and compiled .pyd is in 'build/Release' or similar.

$PydFile = Get-ChildItem -Path "$BuildDir" -Filter "micecam*.pyd" -Recurse | Select-Object -First 1
if ($null -eq $PydFile) {
    Write-Error "❌ Could not find compiled .pyd file. Build might have failed."
    exit 1
}

# PyInstaller command
# --noconsole: Prevents terminal window from opening
# --add-data: Adds the compiled backend
& uv run pyinstaller --noconsole --clean `
    --add-data "$($PydFile.FullName);internal/micecam" `
    --name "MiceCam" `
    --workpath "$ProjectRoot\build\pyinstaller_work" `
    --distpath "$DistDir" `
    cmd/gui/gui/main_window.py

echo "✅ EXE created in $DistDir\MiceCam"

# 4. Packaging to MSI (Requires WiX Toolset)
# This is a placeholder for WiX integration.
# To generate a real MSI, one would typically use a .wxs file.
echo "ℹ️  MSI generation requires WiX Toolset installed on Windows."
echo "   See: https://wixtoolset.org/"
