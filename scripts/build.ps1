# MiceCam Windows Build Script
# Requires: CMake, Visual Studio 2019+ or Build Tools
# 
# Usage:
#   .\build.ps1              # Debug build
#   .\build.ps1 -Release     # Release build
#   .\build.ps1 -Clean       # Clean rebuild

param(
    [switch]$Release,
    [switch]$Clean,
    [switch]$NoTests
)

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "    MiceCam Windows Build Script     " -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

$BuildType = if ($Release) { "Release" } else { "Debug" }
Write-Host "Build Type: $BuildType" -ForegroundColor Yellow

# Clean build if requested
if ($Clean -and (Test-Path "build")) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "build"
}

# Create build directory
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

Push-Location build

try {
    # Configure with CMake
    Write-Host ""
    Write-Host "[1/3] Configuring CMake..." -ForegroundColor Yellow
    
    $cmakeArgs = @("..")
    
    # Add generator if not using default
    # Uncomment the following line to use Ninja if available
    # $cmakeArgs += "-G", "Ninja"
    
    cmake @cmakeArgs
    
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed"
    }
    
    # Build
    Write-Host ""
    Write-Host "[2/3] Building..." -ForegroundColor Yellow
    
    cmake --build . --config $BuildType --parallel
    
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed"
    }
    
    Write-Host ""
    Write-Host "Build successful!" -ForegroundColor Green
    
    # Run tests unless skipped
    if (-not $NoTests) {
        Write-Host ""
        Write-Host "[3/3] Running tests..." -ForegroundColor Yellow
        
        $testExe = ".\$BuildType\micecam_tests.exe"
        if (Test-Path $testExe) {
            & $testExe --gtest_color=yes
            if ($LASTEXITCODE -ne 0) {
                Write-Host "Some tests failed!" -ForegroundColor Red
            } else {
                Write-Host "All tests passed!" -ForegroundColor Green
            }
        } else {
            Write-Host "Test executable not found at $testExe" -ForegroundColor Yellow
            Write-Host "Tests may not have been built." -ForegroundColor Yellow
        }
    } else {
        Write-Host "[3/3] Skipping tests (--NoTests specified)" -ForegroundColor Yellow
    }
    
    Write-Host ""
    Write-Host "=====================================" -ForegroundColor Green
    Write-Host "         Build Complete!             " -ForegroundColor Green
    Write-Host "=====================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "Executables:" -ForegroundColor Cyan
    Write-Host "  Main:  .\build\$BuildType\micecam.exe"
    Write-Host "  Demo:  .\build\$BuildType\micecam_demo.exe"
    Write-Host "  Tests: .\build\$BuildType\micecam_tests.exe"
    Write-Host ""
}
catch {
    Write-Host ""
    Write-Host "Build FAILED: $_" -ForegroundColor Red
    exit 1
}
finally {
    Pop-Location
}
