[CmdletBinding()]
param(
    [string]$SourceDir = "dist\MiceCam",
    [string]$OutputFile = "packaging\windows\components.wxs",
    [string]$ComponentGroupId = "ProductComponents"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $SourceDir)) {
    throw "Source directory not found: $SourceDir"
}

$sourcePath = (Resolve-Path -LiteralPath $SourceDir).Path
$sourcePath = $sourcePath.TrimEnd('\') + '\'

$components = @()
$directories = @{}
$componentIndex = 0
$dirIndex = 0

Get-ChildItem -LiteralPath $sourcePath -Recurse -File | ForEach-Object {
    $relPath = $_.FullName.Substring($sourcePath.Length)
    $dir = Split-Path -Parent $relPath
    $componentId = "cmp_" + [string]($componentIndex++).ToString("D5")

    # Map directory path to a WiX directory ID
    if ($dir) {
        if (-not $directories.ContainsKey($dir)) {
            $dirId = "dir_" + [string]($dirIndex++).ToString("D3")
            $directories[$dir] = @{
                Id = $dirId
                Path = $dir
            }
        }
        $dirRef = $directories[$dir].Id
    } else {
        $dirRef = "INSTALLFOLDER"
    }

    $components += @{
        Id        = $componentId
        Guid      = [guid]::NewGuid().ToString("B").ToUpper()
        Directory = $dirRef
        Source    = $_.FullName
        Name      = $_.Name
    }
}

# Build directory hierarchy XML
$dirXml = ""
$sortedDirs = $directories.Values | Sort-Object { ($_.Path -split '[\\/]').Count }, Path
foreach ($d in $sortedDirs) {
    $parts = $d.Path -split '[\\/]'
    $parent = if ($parts.Count -gt 1) {
        $parentPath = ($parts[0..($parts.Count-2)] -join '\')
        if ($directories.ContainsKey($parentPath)) {
            $directories[$parentPath].Id
        } else {
            "INSTALLFOLDER"
        }
    } else {
        "INSTALLFOLDER"
    }
    $name = $parts[-1]
    $dirXml += "      <Directory Id=`"$($d.Id)`" Name=`"$name`">`n"
}

# Build XML
$xml = '<?xml version="1.0" encoding="utf-8"?>' + "`n"
$xml += '<Wix xmlns="http://wixtoolset.org/schemas/v4/wxs">' + "`n"

if ($dirXml) {
    $xml += "  <Fragment>`n"
    $xml += "    <DirectoryRef Id=`"INSTALLFOLDER`">`n"
    $xml += $dirXml
    for ($i = 0; $i -lt $sortedDirs.Count; $i++) {
        $xml += "      </Directory>`n"
    }
    $xml += "    </DirectoryRef>`n"
    $xml += "  </Fragment>`n"
}

$xml += "  <Fragment>`n"
$xml += "    <ComponentGroup Id=`"$ComponentGroupId`" Directory=`"INSTALLFOLDER`">`n"

foreach ($c in $components) {
    $xml += "      <Component Id=`"$($c.Id)`" Guid=`"$($c.Guid)`" Directory=`"$($c.Directory)`">`n"
    $xml += "        <File Id=`"file_$($c.Id)`" Source=`"$($c.Source)`" />`n"
    $xml += "      </Component>`n"
}

$xml += "    </ComponentGroup>`n"
$xml += "  </Fragment>`n"
$xml += "</Wix>`n"

$outDir = Split-Path -Parent $OutputFile
if ($outDir -and -not (Test-Path -LiteralPath $outDir)) {
    New-Item -ItemType Directory -Force -Path $outDir | Out-Null
}

Set-Content -LiteralPath $OutputFile -Value $xml -Encoding UTF8
Write-Host "WiX components written to $OutputFile ($($components.Count) files, $($directories.Count) subdirectories)"
