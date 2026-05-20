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

# Generate WiX component manifest
$components = @()
$componentIndex = 0

Get-ChildItem -LiteralPath $sourcePath -Recurse -File | ForEach-Object {
    $relPath = $_.FullName.Substring($sourcePath.Length)
    $dir = Split-Path -Parent $relPath
    $componentId = "cmp_" + [string]($componentIndex++).ToString("D5")

    $components += @{
        Id          = $componentId
        Guid        = [guid]::NewGuid().ToString("B").ToUpper()
        Directory   = if ($dir) { "dir_$(($dir -replace '[\\/]', '_'))" } else { "INSTALLFOLDER" }
        Source      = $_.FullName
        Name        = $_.Name
    }
}

# Build XML
$xml = '<?xml version="1.0" encoding="utf-8"?>' + "`n"
$xml += '<Wix xmlns="http://wixtoolset.org/schemas/v4/wxs">' + "`n"
$xml += "  <Fragment>`n"
$xml += "    <ComponentGroup Id=`"$ComponentGroupId`" Directory=`"INSTALLFOLDER`">`n"

foreach ($c in $components) {
    $xml += "      <Component Id=`"$($c.Id)`" Guid=`"$($c.Guid)`">`n"
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
Write-Host "WiX components written to $OutputFile ($($components.Count) files)"
