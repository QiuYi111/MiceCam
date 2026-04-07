# MiceCam Windows Environment Check Script
# Verifies system dependencies and disk performance before building
#
# Usage: .\check_env.ps1

$ErrorActionPreference = "SilentlyContinue"

Write-Host ""
Write-Host "======================================" -ForegroundColor Cyan
Write-Host "  MiceCam Environment Check (Windows) " -ForegroundColor Cyan
Write-Host "======================================" -ForegroundColor Cyan
Write-Host ""

$checksPassed = $true

function Test-CommandExists {
    param($Command, $DisplayName)

    $result = Get-Command $Command -ErrorAction SilentlyContinue
    if ($result) {
        Write-Host "  [OK] $DisplayName" -ForegroundColor Green
        if ($result.Source) {
            Write-Host "       Path: $($result.Source)" -ForegroundColor DarkGray
        }
        return $true
    } else {
        Write-Host "  [X]  $DisplayName - NOT FOUND" -ForegroundColor Red
        return $false
    }
}

# ============================================
# 1. Build Tools Check
# ============================================
Write-Host "1. Build Tools" -ForegroundColor Yellow
Write-Host "   -----------"

if (-not (Test-CommandExists "cmake" "CMake")) {
    $script:checksPassed = $false
    Write-Host "       Install: https://cmake.org/download/" -ForegroundColor Yellow
}

# Check for MSVC compiler
$clResult = Get-Command "cl" -ErrorAction SilentlyContinue
if ($clResult) {
    Write-Host "  [OK] MSVC Compiler (cl.exe)" -ForegroundColor Green
} else {
    Write-Host "  [!]  MSVC Compiler (cl.exe) - Not in PATH" -ForegroundColor Yellow
    Write-Host "       Run from 'Developer PowerShell for VS' or add VS Build Tools to PATH" -ForegroundColor Yellow
}

# Check for Git (optional but useful)
Test-CommandExists "git" "Git" | Out-Null

Write-Host ""

# ============================================
# 2. C++ Standard Check
# ============================================
Write-Host "2. C++ Standard Support" -ForegroundColor Yellow
Write-Host "   --------------------"

# Check Visual Studio version
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vsWhere) {
    $vsInfo = & $vsWhere -latest -format json | ConvertFrom-Json
    if ($vsInfo) {
        $vsVersion = $vsInfo.installationVersion
        Write-Host "  [OK] Visual Studio $vsVersion detected" -ForegroundColor Green

        # VS 2019 (16.x) and later support C++20
        $majorVersion = [int]($vsVersion.Split('.')[0])
        if ($majorVersion -ge 16) {
            Write-Host "       C++20 support: Yes" -ForegroundColor Green
        } else {
            Write-Host "       C++20 support: Limited (VS 2019+ recommended)" -ForegroundColor Yellow
        }
    }
} else {
    Write-Host "  [!]  Could not detect Visual Studio version" -ForegroundColor Yellow
}

Write-Host ""

# ============================================
# 3. Disk Performance Test
# ============================================
Write-Host "3. Disk Performance" -ForegroundColor Yellow
Write-Host "   -----------------"

$tempFile = [System.IO.Path]::Combine([System.IO.Path]::GetTempPath(), "micecam_perf_test.tmp")
$fileSizeMB = 100
$bufferSize = 1MB
$buffer = New-Object byte[] $bufferSize

try {
    Write-Host "  Testing write speed ($fileSizeMB MB)..." -ForegroundColor DarkGray

    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $fs = [System.IO.File]::OpenWrite($tempFile)

    for ($i = 0; $i -lt $fileSizeMB; $i++) {
        $fs.Write($buffer, 0, $buffer.Length)
    }

    $fs.Flush()
    $fs.Close()
    $sw.Stop()

    $elapsedSeconds = $sw.ElapsedMilliseconds / 1000
    $speedMBps = [math]::Round($fileSizeMB / $elapsedSeconds, 1)

    if ($speedMBps -ge 200) {
        Write-Host "  [OK] Write speed: $speedMBps MB/s (meets 200 MB/s target)" -ForegroundColor Green
    } elseif ($speedMBps -ge 100) {
        Write-Host "  [!]  Write speed: $speedMBps MB/s (below 200 MB/s target)" -ForegroundColor Yellow
        Write-Host "       Consider using an SSD for high-speed capture" -ForegroundColor Yellow
    } else {
        Write-Host "  [X]  Write speed: $speedMBps MB/s (too slow for high-speed capture)" -ForegroundColor Red
        $script:checksPassed = $false
    }
}
catch {
    Write-Host "  [X]  Disk test failed: $_" -ForegroundColor Red
}
finally {
    if (Test-Path $tempFile) {
        Remove-Item $tempFile -Force -ErrorAction SilentlyContinue
    }
}

Write-Host ""

# ============================================
# 4. Camera Detection
# ============================================
Write-Host "4. Camera Detection" -ForegroundColor Yellow
Write-Host "   -----------------"

try {
    $cameras = Get-PnpDevice -Class Camera -Status OK -ErrorAction SilentlyContinue
    if ($cameras) {
        $cameraCount = @($cameras).Count
        Write-Host "  [OK] Found $cameraCount camera device(s):" -ForegroundColor Green
        foreach ($cam in $cameras) {
            Write-Host "       - $($cam.FriendlyName)" -ForegroundColor DarkGray
        }
    } else {
        Write-Host "  [!]  No cameras detected" -ForegroundColor Yellow
        Write-Host "       (Optional - FakeCamera can be used for testing)" -ForegroundColor Yellow
    }
}
catch {
    Write-Host "  [!]  Could not enumerate cameras" -ForegroundColor Yellow
}

Write-Host ""

# ============================================
# 5. Memory Check
# ============================================
Write-Host "5. System Memory" -ForegroundColor Yellow
Write-Host "   --------------"

try {
    $mem = Get-CimInstance Win32_ComputerSystem -ErrorAction Stop
    $totalMemGB = [math]::Round($mem.TotalPhysicalMemory / 1GB, 1)

    if ($totalMemGB -ge 4) {
        Write-Host "  [OK] Total RAM: $totalMemGB GB" -ForegroundColor Green
    } elseif ($totalMemGB -ge 2) {
        Write-Host "  [!]  Total RAM: $totalMemGB GB" -ForegroundColor Yellow
        Write-Host "       4 GB+ recommended for large buffer sizes" -ForegroundColor Yellow
    } else {
        Write-Host "  [X]  Total RAM: $totalMemGB GB (insufficient)" -ForegroundColor Red
        $script:checksPassed = $false
    }
}
catch {
    Write-Host "  [!]  Could not determine memory size" -ForegroundColor Yellow
}

Write-Host ""

# ============================================
# 6. OpenCV Check (Optional)
# ============================================
Write-Host "6. OpenCV (Optional)" -ForegroundColor Yellow
Write-Host "   ------------------"

$opencvFound = $false

# Check common OpenCV locations
$opencvPaths = @(
    "$env:OPENCV_DIR",
    "C:\opencv",
    "C:\tools\opencv",
    "$env:USERPROFILE\opencv"
)

foreach ($path in $opencvPaths) {
    if ($path -and (Test-Path $path)) {
        Write-Host "  [OK] OpenCV found at: $path" -ForegroundColor Green
        $opencvFound = $true
        break
    }
}

if (-not $opencvFound) {
    Write-Host "  [!]  OpenCV not detected (optional for USB camera)" -ForegroundColor Yellow
    Write-Host "       Install via vcpkg: vcpkg install opencv4:x64-windows" -ForegroundColor Yellow
}

Write-Host ""

# ============================================
# Summary
# ============================================
Write-Host "======================================" -ForegroundColor Cyan

if ($checksPassed) {
    Write-Host "  All checks passed! Ready to build.  " -ForegroundColor Green
    Write-Host "======================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor Yellow
    Write-Host "  1. Run: .\build.ps1" -ForegroundColor White
    Write-Host "  2. Or manually: mkdir build; cd build; cmake ..; cmake --build ." -ForegroundColor White
    Write-Host ""
    exit 0
} else {
    Write-Host "  Some checks failed. See above.      " -ForegroundColor Red
    Write-Host "======================================" -ForegroundColor Cyan
    Write-Host ""
    exit 1
}
