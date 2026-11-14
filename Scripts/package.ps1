# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
# Packages a Windows Shipping build into Builds/<Name>. Close the Unreal Editor first (Live Coding blocks packaging).
# Usage: powershell -ExecutionPolicy Bypass -File Scripts/package.ps1 -Name Package0 [-EngineDir "C:\Program Files\Epic Games\UE_5.8"]

param(
    [Parameter(Mandatory = $true)][string]$Name,
    [string]$EngineDir = "C:\Program Files\Epic Games\UE_5.8",
    [string]$Configuration = "Shipping"
)

$ErrorActionPreference = "Stop"
$ProjectDir = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $ProjectDir "TouchdownMars.uproject"
$OutDir = Join-Path $ProjectDir "Builds\$Name"
$LogDir = Join-Path $ProjectDir "Saved\Logs"
$Log = Join-Path $LogDir "package_$Name.log"

if (Get-Process UnrealEditor -ErrorAction SilentlyContinue) {
    throw "Close the Unreal Editor before packaging (Live Coding blocks the build)."
}
New-Item -ItemType Directory -Force $LogDir | Out-Null

$start = Get-Date
& "$EngineDir\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun `
    "-project=$Project" -platform=Win64 "-clientconfig=$Configuration" `
    -build -cook -stage -pak -iostore -package -archive "-archivedirectory=$OutDir" `
    -nop4 -utf8output -unattended -NoLiveCoding *> $Log
$code = $LASTEXITCODE
$secs = [int]((Get-Date) - $start).TotalSeconds

if ($code -ne 0) {
    Get-Content $Log -Tail 30
    throw "Packaging failed (exit $code) after $secs s. Full log: $Log"
}

$exe = Get-ChildItem $OutDir -Recurse -Filter "TouchdownMars*.exe" | Select-Object -First 1
$sizeMB = [int]((Get-ChildItem $OutDir -Recurse -File | Measure-Object Length -Sum).Sum / 1MB)
Write-Output "PACKAGE OK: $Name in $secs s, $sizeMB MB, exe: $($exe.FullName)"
