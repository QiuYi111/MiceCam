# MiceCam SDK Packaging Script
$ErrorActionPreference = "Stop"

$SdkDir = "$PSScriptRoot/MiceCam_SDK"
$BuildDir = "$PSScriptRoot/build"
$BinDir = "$SdkDir/bin"

Write-Host "📦 Packaging MiceCam SDK..." -ForegroundColor Cyan

# 1. Clean & Config
if (-not (Test-Path $BuildDir)) { New-Item -ItemType Directory -Path $BuildDir | Out-Null }
Push-Location $BuildDir

# Resolve absolute path for CMake (use / for safety)
$SdkDir = $SdkDir -replace "\\", "/"
Write-Host "Configuring CMake (Release) -> $SdkDir" -ForegroundColor Yellow
cmake .. -DCMAKE_BUILD_TYPE=Release "-DCMAKE_INSTALL_PREFIX=$SdkDir" -DWITH_CAMERA_BACKEND=ON

if ($LASTEXITCODE -ne 0) { throw "CMake Configure Failed" }

# 2. Build & Install
Write-Host "Building and Installing..." -ForegroundColor Yellow
cmake --build . --config Release --target install

if ($LASTEXITCODE -ne 0) { throw "Install Failed" }
Pop-Location

# 3. Bundle Dependencies (DLLs)
Write-Host "Bundling Dependencies..." -ForegroundColor Yellow

# Locations (Adjust based on your environment or use vcpkg logic)
$VcpkgBin = "C:/vcpkg/installed/x64-windows/bin"
$DepthAiBin = "$PSScriptRoot/3rdParty/depthai-core-prebuild/bin"

# Copy OpenCV & FFmpeg DLLs
if (Test-Path $VcpkgBin) {
    Write-Host "  Copying vcpkg DLLs..."
    Copy-Item "$VcpkgBin/*.dll" -Destination $BinDir -Force
} else {
    Write-Warning "vcpkg bin not found at $VcpkgBin. You may need to copy DLLs manually."
}

# Copy DepthAI DLLs
if (Test-Path $DepthAiBin) {
    Write-Host "  Copying DepthAI DLLs..."
    Copy-Item "$DepthAiBin/*.dll" -Destination $BinDir -Force
}

# 4. Copy Docs
if (Test-Path "$PSScriptRoot/docs/API_REFERENCE.md") {
    Copy-Item "$PSScriptRoot/docs/API_REFERENCE.md" -Destination "$SdkDir/docs" -Force
}

Write-Host ""
Write-Host "✅ SDK Created at: $SdkDir" -ForegroundColor Green
Write-Host "Structure:"
Get-ChildItem $SdkDir -Recurse | Select-Object FullName
