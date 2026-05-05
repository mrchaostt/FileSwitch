param(
    [Parameter(Mandatory = $true)]
    [string]$AppDir,

    [string]$ExeName = "FileSwitch.exe"
)

$ErrorActionPreference = "Stop"

function Get-PeMachine {
    param([Parameter(Mandatory = $true)][string]$Path)

    $fs = [System.IO.File]::Open($Path, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
    $br = New-Object System.IO.BinaryReader($fs)

    try {
        if ($br.ReadUInt16() -ne 0x5A4D) {
            return $null
        }

        [void]$fs.Seek(0x3C, [System.IO.SeekOrigin]::Begin)
        $peOffset = $br.ReadInt32()

        [void]$fs.Seek($peOffset, [System.IO.SeekOrigin]::Begin)
        if ($br.ReadUInt32() -ne 0x00004550) {
            return $null
        }

        $machine = $br.ReadUInt16()
        return New-Object PSObject -Property @{
            Path = $Path
            Machine = $machine
        }
    } finally {
        $br.Close()
        $fs.Close()
    }
}

function Format-PeMachine {
    param([int]$Machine)

    switch ($Machine) {
        0x014c { "x86" }
        0x8664 { "x64" }
        0xaa64 { "arm64" }
        default { ("unknown 0x{0:x4}" -f $Machine) }
    }
}

$exePath = Join-Path $AppDir $ExeName
if (!(Test-Path $exePath)) {
    throw "Executable not found: $exePath"
}

$exeInfo = Get-PeMachine $exePath
if ($null -eq $exeInfo) {
    throw "Not a valid PE executable: $exePath"
}

$exeMachineName = Format-PeMachine $exeInfo.Machine
Write-Host "EXE: $exePath"
Write-Host "EXE architecture: $exeMachineName"
Write-Host ""

$dlls = Get-ChildItem -Path $AppDir -Filter "*.dll" -Recurse | Where-Object { -not $_.PSIsContainer }
$mismatches = @()

foreach ($dll in $dlls) {
    $info = Get-PeMachine $dll.FullName
    if ($null -eq $info) {
        continue
    }

    if ($info.Machine -ne $exeInfo.Machine) {
        $mismatches += New-Object PSObject -Property @{
            Path = $dll.FullName
            Architecture = Format-PeMachine $info.Machine
        }
    }
}

if ($mismatches.Count -gt 0) {
    Write-Host "Mismatched DLL architecture found:"
    foreach ($item in $mismatches) {
        Write-Host ("  {0}  ->  {1}" -f $item.Architecture, $item.Path)
    }
    Write-Host ""
    Write-Host "This can cause Windows startup error 0xc000007b."
    exit 2
}

Write-Host "No DLL architecture mismatch found under: $AppDir"

$commonRuntimeDlls = @(
    "Qt5Core.dll",
    "Qt5Gui.dll",
    "Qt5Widgets.dll",
    "Qt5Network.dll",
    "Qt6Core.dll",
    "Qt6Gui.dll",
    "Qt6Widgets.dll",
    "Qt6Network.dll",
    "libgcc_s_seh-1.dll",
    "libgcc_s_dw2-1.dll",
    "libstdc++-6.dll",
    "libwinpthread-1.dll",
    "vcruntime140.dll",
    "vcruntime140_1.dll",
    "msvcp140.dll"
)

$pathMismatches = @()
foreach ($dllName in $commonRuntimeDlls) {
    $foundPaths = @(& where.exe $dllName 2>$null)
    foreach ($foundPath in $foundPaths) {
        if (!(Test-Path $foundPath)) {
            continue
        }

        $info = Get-PeMachine $foundPath
        if ($null -ne $info -and $info.Machine -ne $exeInfo.Machine) {
            $pathMismatches += New-Object PSObject -Property @{
                Name = $dllName
                Path = $foundPath
                Architecture = Format-PeMachine $info.Machine
            }
        }
    }
}

if ($pathMismatches.Count -gt 0) {
    Write-Host ""
    Write-Host "Potential mismatched DLLs visible on PATH:"
    foreach ($item in $pathMismatches) {
        Write-Host ("  {0}  {1}  ->  {2}" -f $item.Name, $item.Architecture, $item.Path)
    }
    Write-Host "Copy the correct runtime DLLs next to FileSwitch.exe or fix PATH ordering."
    exit 3
}

$platformPlugin = Join-Path $AppDir "platforms\qwindows.dll"
if (!(Test-Path $platformPlugin)) {
    Write-Host ""
    Write-Host "Warning: Qt platform plugin was not found at: $platformPlugin"
    Write-Host "Run windeployqt for the same Qt kit used to build FileSwitch."
}

Write-Host ""
Write-Host ("Startup log path, if main() is reached: {0}" -f (Join-Path $AppDir "FileSwitch-startup.log"))
