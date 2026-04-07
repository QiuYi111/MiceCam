[CmdletBinding()]
param(
    [switch]$SkipNativeBuild,
    [switch]$SkipPyInstaller,
    [switch]$SkipMsi
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
Set-Location $ProjectRoot

$NativeBuildDir = Join-Path $ProjectRoot "build-python-package"
$DistRoot = Join-Path $ProjectRoot "dist"
$AppDistDir = Join-Path $DistRoot "MiceCam"
$WorkerStageRoot = Join-Path $ProjectRoot "dist_worker_stage"
$WorkerStageDir = Join-Path $WorkerStageRoot "MiceCamWorker"
$WorkerInstallDir = Join-Path $AppDistDir "tools\\MiceCamWorker"
$PyInstallerWorkDir = Join-Path $ProjectRoot "build_pyinstaller"
$WorkerPyInstallerWorkDir = Join-Path $ProjectRoot "build_pyinstaller_worker"
$PackagingDir = Join-Path $ProjectRoot "packaging\\windows"
$ComponentsFile = Join-Path $PackagingDir "components.wxs"
$InstallerPath = Join-Path $DistRoot "MiceCam-Installer.msi"
$VenvPython = Join-Path $ProjectRoot ".venv\\Scripts\\python.exe"
$WixToolPath = Join-Path $ProjectRoot ".wix-tools"
$WixExe = Join-Path $WixToolPath "wix.exe"

function Require-Command {
    param([string]$Name)

    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Required command '$Name' was not found in PATH."
    }
}

function Find-CMakeExe {
    $candidates = @(
        "cmake",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    )

    foreach ($candidate in $candidates) {
        $command = Get-Command $candidate -ErrorAction SilentlyContinue
        if ($command) {
            return $command.Source
        }
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    throw "Required command 'cmake' was not found in PATH or common Visual Studio locations."
}

function Find-BindingsReleaseDir {
    $candidates = @(
        "build-python-package\\bindings\\python\\Release",
        "build-codex-oak-py314\\bindings\\python\\Release",
        "build\\bindings\\python\\Release",
        "build-feat-winpkg\\bindings\\python\\Release",
        "build-native-win230\\bindings\\python\\Release",
        "build-native-win4\\bindings\\python\\Release"
    ) | ForEach-Object { Join-Path $ProjectRoot $_ }

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            $pyd = Get-ChildItem -LiteralPath $candidate -Filter "_micecam*.pyd" -File -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($pyd) {
                return $candidate
            }
        }
    }

    return $null
}

Write-Host "Using project root: $ProjectRoot"

Require-Command -Name "uv"
Require-Command -Name "dotnet"

if (-not (Test-Path $VenvPython)) {
    throw "Expected virtualenv Python at $VenvPython"
}

$bindingsReleaseDir = Find-BindingsReleaseDir
if (-not $SkipNativeBuild) {
    $cmakeExe = Find-CMakeExe
    $toolchain = Join-Path $ProjectRoot "vcpkg\\scripts\\buildsystems\\vcpkg.cmake"
    if (-not (Test-Path $toolchain)) {
        throw "vcpkg toolchain not found at $toolchain"
    }

    Write-Host "Building fresh native bindings in $NativeBuildDir ..."
    & $cmakeExe -B $NativeBuildDir -S . -DCMAKE_TOOLCHAIN_FILE="$toolchain" -DCMAKE_BUILD_TYPE=Release -DBUILD_PYTHON_BINDINGS=ON -DBUILD_QT_UI=OFF -DMICECAM_BUILD_TESTS=OFF -DBUILD_TESTS=OFF -DPython3_EXECUTABLE="$VenvPython" -DPython3_ROOT_DIR="$($VenvPython | Split-Path -Parent | Split-Path -Parent)"
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }

    & $cmakeExe --build $NativeBuildDir --config Release
    if ($LASTEXITCODE -ne 0) { throw "CMake build failed." }

    $bindingsReleaseDir = Find-BindingsReleaseDir
}

if (-not $bindingsReleaseDir) {
    throw "Could not find a compiled _micecam binding. Build it first or rerun without -SkipNativeBuild."
}

Write-Host "Using bindings from: $bindingsReleaseDir"

if (-not $SkipPyInstaller) {
    Write-Host "Syncing Python environment..."
    uv sync --all-extras
    if ($LASTEXITCODE -ne 0) { throw "uv sync failed." }

    Write-Host "Ensuring PyInstaller is available..."
    uv pip install pyinstaller
    if ($LASTEXITCODE -ne 0) { throw "Failed to install PyInstaller." }

    Write-Host "Building one-folder application via PyInstaller..."
    if (Test-Path $AppDistDir) {
        Remove-Item -LiteralPath $AppDistDir -Recurse -Force
    }
    if (Test-Path $WorkerStageRoot) {
        Remove-Item -LiteralPath $WorkerStageRoot -Recurse -Force
    }

    uv run pyinstaller .\MiceCam.spec --clean --distpath "$DistRoot" --workpath "$PyInstallerWorkDir"
    if ($LASTEXITCODE -ne 0) { throw "PyInstaller build failed." }

    Write-Host "Building dedicated worker executable..."
    uv run pyinstaller .\MiceCamWorker.spec --clean --distpath "$WorkerStageRoot" --workpath "$WorkerPyInstallerWorkDir"
    if ($LASTEXITCODE -ne 0) { throw "Worker PyInstaller build failed." }

    New-Item -ItemType Directory -Force -Path $WorkerInstallDir | Out-Null
    Copy-Item -Path (Join-Path $WorkerStageDir "*") -Destination $WorkerInstallDir -Recurse -Force
}

if (-not (Test-Path (Join-Path $AppDistDir "MiceCam.exe"))) {
    throw "Packaged application not found at $AppDistDir\\MiceCam.exe"
}
if (-not (Test-Path (Join-Path $WorkerInstallDir "MiceCamWorker.exe"))) {
    throw "Packaged worker not found at $WorkerInstallDir\\MiceCamWorker.exe"
}

if ($SkipMsi) {
    Write-Host "Skipping MSI generation as requested."
    exit 0
}

if (-not (Test-Path $WixExe)) {
    Write-Host "Installing WiX CLI locally..."
    dotnet tool install wix --tool-path $WixToolPath
    if ($LASTEXITCODE -ne 0) { throw "Failed to install WiX CLI." }
}

Write-Host "Generating WiX component manifest..."
& powershell -ExecutionPolicy Bypass -File .\scripts\generate_wix_components.ps1 -SourceDir "$AppDistDir" -OutputFile "$ComponentsFile"
if ($LASTEXITCODE -ne 0) { throw "Failed to generate WiX component manifest." }

Write-Host "Building MSI..."
if (Test-Path $InstallerPath) {
    Remove-Item -LiteralPath $InstallerPath -Force
}

& $WixExe build .\packaging\windows\micecam.wxs .\packaging\windows\components.wxs -ext WixToolset.UI.wixext -arch x64 -o "$InstallerPath"
if ($LASTEXITCODE -ne 0) { throw "WiX build failed." }

Write-Host "MSI created: $InstallerPath"
