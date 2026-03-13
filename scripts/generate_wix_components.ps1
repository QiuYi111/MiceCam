[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$SourceDir,

    [Parameter(Mandatory = $true)]
    [string]$OutputFile,

    [string]$DirectoryRefId = "INSTALLFOLDER",
    [string]$ComponentGroupId = "ProductComponents"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-HashSuffix {
    param([string]$Value)

    $sha1 = [System.Security.Cryptography.SHA1]::Create()
    try {
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($Value)
        $hash = $sha1.ComputeHash($bytes)
        return ([System.BitConverter]::ToString($hash)).Replace("-", "").Substring(0, 10)
    }
    finally {
        $sha1.Dispose()
    }
}

function New-WixId {
    param(
        [string]$Prefix,
        [string]$Value
    )

    $normalized = $Value -replace "[^A-Za-z0-9_\.]", "_"
    if ([string]::IsNullOrWhiteSpace($normalized)) {
        $normalized = "root"
    }
    if ($normalized[0] -match "[0-9]") {
        $normalized = "_$normalized"
    }
    $normalized = $normalized.Trim("_")
    if ([string]::IsNullOrWhiteSpace($normalized)) {
        $normalized = "root"
    }

    $hash = Get-HashSuffix -Value $Value
    $maxStemLength = 48
    if ($normalized.Length -gt $maxStemLength) {
        $normalized = $normalized.Substring(0, $maxStemLength)
    }

    return "{0}_{1}_{2}" -f $Prefix, $normalized, $hash
}

function Escape-Xml {
    param([string]$Value)

    return [System.Security.SecurityElement]::Escape($Value)
}

$resolvedSourceDir = (Resolve-Path $SourceDir).Path
$outputDir = Split-Path -Parent $OutputFile
if ($outputDir -and -not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

$componentIds = New-Object System.Collections.Generic.List[string]

function Build-DirectoryXml {
    param(
        [string]$CurrentDir,
        [string]$RelativePath,
        [int]$IndentLevel
    )

    $indent = "  " * $IndentLevel
    $lines = New-Object System.Collections.Generic.List[string]

    $files = Get-ChildItem -LiteralPath $CurrentDir -File | Sort-Object Name
    foreach ($file in $files) {
        $relativeFile = if ([string]::IsNullOrEmpty($RelativePath)) {
            $file.Name
        }
        else {
            Join-Path $RelativePath $file.Name
        }

        $componentId = New-WixId -Prefix "CMP" -Value $relativeFile
        $fileId = New-WixId -Prefix "FIL" -Value $relativeFile
        $componentIds.Add($componentId) | Out-Null

        $source = Escape-Xml $file.FullName
        $name = Escape-Xml $file.Name

        $lines.Add("$indent<Component Id=`"$componentId`">") | Out-Null
        $lines.Add("$indent  <File Id=`"$fileId`" Source=`"$source`" Name=`"$name`" />") | Out-Null
        $lines.Add("$indent</Component>") | Out-Null
    }

    $directories = Get-ChildItem -LiteralPath $CurrentDir -Directory | Sort-Object Name
    foreach ($directory in $directories) {
        $relativeChild = if ([string]::IsNullOrEmpty($RelativePath)) {
            $directory.Name
        }
        else {
            Join-Path $RelativePath $directory.Name
        }

        $childLines = Build-DirectoryXml -CurrentDir $directory.FullName -RelativePath $relativeChild -IndentLevel ($IndentLevel + 1)
        if ($childLines.Count -eq 0) {
            continue
        }

        $directoryId = New-WixId -Prefix "DIR" -Value $relativeChild
        $name = Escape-Xml $directory.Name

        $lines.Add("$indent<Directory Id=`"$directoryId`" Name=`"$name`">") | Out-Null
        foreach ($childLine in $childLines) {
            $lines.Add($childLine) | Out-Null
        }
        $lines.Add("$indent</Directory>") | Out-Null
    }

    return $lines
}

$directoryLines = Build-DirectoryXml -CurrentDir $resolvedSourceDir -RelativePath "" -IndentLevel 2

$componentRefLines = New-Object System.Collections.Generic.List[string]
foreach ($componentId in $componentIds) {
    $componentRefLines.Add("    <ComponentRef Id=`"$componentId`" />") | Out-Null
}

$content = @(
    "<Wix xmlns=`"http://wixtoolset.org/schemas/v4/wxs`">",
    "  <Fragment>",
    "    <DirectoryRef Id=`"$DirectoryRefId`">"
)

foreach ($line in $directoryLines) {
    $content += $line
}

$content += @(
    "    </DirectoryRef>",
    "  </Fragment>",
    "  <Fragment>",
    "    <ComponentGroup Id=`"$ComponentGroupId`">"
)

foreach ($line in $componentRefLines) {
    $content += $line
}

$content += @(
    "    </ComponentGroup>",
    "  </Fragment>",
    "</Wix>"
)

[System.IO.File]::WriteAllLines($OutputFile, $content, [System.Text.UTF8Encoding]::new($false))
Write-Host "Generated WiX component manifest: $OutputFile"
