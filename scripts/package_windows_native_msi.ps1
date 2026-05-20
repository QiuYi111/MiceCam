[CmdletBinding()]
param(
    [string]$BuildDir = "build",
    [string]$DistDir = "dist",
    [string]$Version = "0.1.0",
    [string]$WixToolPath = ".wix-tools",
    [switch]$SkipStage
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$BuildRoot = Join-Path $ProjectRoot $BuildDir
$DistRoot = Join-Path $ProjectRoot $DistDir
$Stage = Join-Path $DistRoot "MiceCam"
$ComponentsFile = Join-Path $ProjectRoot "packaging\windows\components.wxs"
$InstallerPath = Join-Path $DistRoot "MiceCam-$Version-windows-x64.msi"
$WixExe = Join-Path $ProjectRoot "$WixToolPath\wix.exe"

function Require-File {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) {
        throw "Required file is missing: $Path"
    }
}

function Resolve-FirstExisting {
    param([string[]]$Candidates)
    foreach ($candidate in $Candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }
    return $null
}

function Resolve-MsysUcrtPrefix {
    $candidates = @()

    foreach ($prefix in @($env:MSYSTEM_PREFIX, $env:MINGW_PREFIX)) {
        if (-not [string]::IsNullOrWhiteSpace($prefix)) {
            $candidates += $prefix
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($env:RUNNER_TEMP)) {
        $candidates += (Join-Path $env:RUNNER_TEMP "msys64\ucrt64")
    }

    foreach ($commandName in @("windeployqt6.exe", "windeployqt.exe")) {
        $command = Get-Command $commandName -ErrorAction SilentlyContinue
        if ($command) {
            $candidates += (Split-Path -Parent (Split-Path -Parent $command.Source))
        }
    }

    $candidates += @(
        "C:\msys64\ucrt64",
        "D:\a\_temp\msys64\ucrt64"
    )

    foreach ($candidate in $candidates) {
        if ([string]::IsNullOrWhiteSpace($candidate)) {
            continue
        }
        $bin = Join-Path $candidate "bin"
        if (Test-Path -LiteralPath $bin) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    throw "MSYS2 UCRT64 prefix was not found."
}

function Copy-DependencyDlls {
    param(
        [string[]]$Executables,
        [string]$Destination
    )

    $pending = New-Object System.Collections.Generic.Queue[string]
    $seen = New-Object System.Collections.Generic.HashSet[string]

    foreach ($exe in $Executables) {
        if (Test-Path -LiteralPath $exe) {
            $pending.Enqueue((Resolve-Path -LiteralPath $exe).Path)
        }
    }

    while ($pending.Count -gt 0) {
        $binary = $pending.Dequeue()
        $lddOutput = & bash -lc "ldd '$(cygpath -u "$binary")' 2>/dev/null || true"
        foreach ($line in $lddOutput) {
            if ($line -notmatch '=>\s+(/[^ ]+\.dll)') {
                continue
            }

            $unixPath = $Matches[1]
            if ($unixPath -notmatch '^/ucrt64/bin/') {
                continue
            }

            $winPath = (& cygpath -w $unixPath).Trim()
            if (-not (Test-Path -LiteralPath $winPath)) {
                continue
            }

            $name = Split-Path -Leaf $winPath
            if ($seen.Add($name)) {
                $destPath = Join-Path $Destination $name
                Copy-Item -LiteralPath $winPath -Destination $destPath -Force
                $pending.Enqueue($destPath)
            }
        }
    }
}

$UcrtPrefix = Resolve-MsysUcrtPrefix
$UcrtBin = Join-Path $UcrtPrefix "bin"
$MsysUsrBin = Join-Path (Split-Path -Parent $UcrtPrefix) "usr\bin"
$env:Path = "$UcrtBin;$MsysUsrBin;$env:Path"

if (-not $SkipStage) {
    New-Item -ItemType Directory -Force -Path $DistRoot | Out-Null
    if (Test-Path -LiteralPath $Stage) {
        Remove-Item -LiteralPath $Stage -Recurse -Force
    }
    New-Item -ItemType Directory -Force -Path $Stage | Out-Null

    $uiExe = Resolve-FirstExisting @(
        (Join-Path $BuildRoot "cmd\micecam_ui\micecam_ui.exe"),
        (Join-Path $BuildRoot "cmd\micecam_ui\MiceCam.exe")
    )
    $ffmpegPlugin = Join-Path $BuildRoot "cmd\plugins\micecam_ffmpeg\micecam_ffmpeg_plugin.exe"
    $oakPlugin = Join-Path $BuildRoot "cmd\plugins\micecam_oak\micecam_oak_plugin.exe"
    $ffmpegManifest = Join-Path $ProjectRoot "3rdParty\bundled_plugins\micecam.ffmpeg\plugin.json"
    $oakManifest = Join-Path $ProjectRoot "3rdParty\bundled_plugins\micecam.oak\plugin.json"

    if (-not $uiExe) {
        throw "Native UI executable was not found in $BuildRoot\cmd\micecam_ui"
    }
    Require-File $ffmpegPlugin
    Require-File $oakPlugin
    Require-File $ffmpegManifest
    Require-File $oakManifest

    Copy-Item -LiteralPath $uiExe -Destination (Join-Path $Stage "MiceCam.exe") -Force
    New-Item -ItemType Directory -Force -Path "$Stage\3rdParty\bundled_plugins\micecam.ffmpeg\bin" | Out-Null
    New-Item -ItemType Directory -Force -Path "$Stage\3rdParty\bundled_plugins\micecam.oak\bin" | Out-Null
    Copy-Item -LiteralPath $ffmpegManifest -Destination "$Stage\3rdParty\bundled_plugins\micecam.ffmpeg\plugin.json" -Force
    Copy-Item -LiteralPath $oakManifest -Destination "$Stage\3rdParty\bundled_plugins\micecam.oak\plugin.json" -Force
    Copy-Item -LiteralPath $ffmpegPlugin -Destination "$Stage\3rdParty\bundled_plugins\micecam.ffmpeg\bin\micecam-ffmpeg.exe" -Force
    Copy-Item -LiteralPath $oakPlugin -Destination "$Stage\3rdParty\bundled_plugins\micecam.oak\bin\micecam-oak.exe" -Force

    $windeployqt = Resolve-FirstExisting @(
        (Join-Path $UcrtBin "windeployqt6.exe"),
        (Join-Path $UcrtBin "windeployqt.exe"),
        (Join-Path $UcrtPrefix "qt6\bin\windeployqt6.exe"),
        (Join-Path $UcrtPrefix "qt6\bin\windeployqt.exe"),
        (Join-Path $UcrtPrefix "lib\qt6\bin\windeployqt6.exe"),
        (Join-Path $UcrtPrefix "lib\qt6\bin\windeployqt.exe")
    )
    if (-not $windeployqt) {
        $found = Get-ChildItem -Path $UcrtPrefix -Filter "windeployqt*.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($found) {
            $windeployqt = $found.FullName
        }
    }
    if (-not $windeployqt) {
        throw "windeployqt was not found in $UcrtPrefix (checked: bin, qt6/bin, lib/qt6/bin, recursive)"
    }

    & $windeployqt --release --qmldir (Join-Path $ProjectRoot "cmd\micecam_ui\qml") (Join-Path $Stage "MiceCam.exe")
    if ($LASTEXITCODE -ne 0) {
        throw "windeployqt failed."
    }

    Copy-DependencyDlls -Executables @(
        (Join-Path $Stage "MiceCam.exe"),
        (Join-Path $Stage "3rdParty\bundled_plugins\micecam.ffmpeg\bin\micecam-ffmpeg.exe"),
        (Join-Path $Stage "3rdParty\bundled_plugins\micecam.oak\bin\micecam-oak.exe")
    ) -Destination $Stage

    @"
MiceCam native beta package.

This MSI installs the native Qt/QML application plus official bundled camera
plugins. Hardware-in-the-loop validation must be performed on target machines.
"@ | Set-Content (Join-Path $Stage "README-beta.txt")
} else {
    New-Item -ItemType Directory -Force -Path $DistRoot | Out-Null
    if (-not (Test-Path -LiteralPath $Stage)) {
        throw "Stage directory not found: $Stage. Run without -SkipStage first."
    }
}

if (-not (Test-Path -LiteralPath $WixExe)) {
    dotnet tool install wix --tool-path (Join-Path $ProjectRoot $WixToolPath)
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to install WiX CLI."
    }
}

& $WixExe extension add WixToolset.UI.wixext
if ($LASTEXITCODE -ne 0) {
    throw "Failed to install WiX UI extension."
}

& powershell -ExecutionPolicy Bypass -File (Join-Path $ProjectRoot "scripts\generate_wix_components.ps1") `
    -SourceDir $Stage `
    -OutputFile $ComponentsFile `
    -ComponentGroupId "ProductComponents"
if ($LASTEXITCODE -ne 0) {
    throw "Failed to generate WiX component manifest."
}

if (Test-Path -LiteralPath $InstallerPath) {
    Remove-Item -LiteralPath $InstallerPath -Force
}

& $WixExe build `
    (Join-Path $ProjectRoot "packaging\windows\micecam.wxs") `
    $ComponentsFile `
    -ext WixToolset.UI.wixext `
    -arch x64 `
    -d ProductVersion=$Version `
    -o $InstallerPath
if ($LASTEXITCODE -ne 0) {
    throw "WiX build failed."
}

Write-Host "MSI created: $InstallerPath"
