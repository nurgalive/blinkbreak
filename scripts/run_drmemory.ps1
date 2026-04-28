# run_drmemory.ps1
# Portable Dr. Memory runner for BlinkBreak on Windows.

param(
    [string]$Preset = "debug",
    [string]$Configuration = "Debug",
    [string]$ExecutablePath = "",
    [string]$ResultRoot = "",
    [string]$DrMemoryVersion = "2.6.20434",
    [string[]]$AppArgs = @("--version"),
    [switch]$BuildFirst,
    [switch]$Redownload
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path $PSScriptRoot -Parent
$tmpRoot = Join-Path $repoRoot "tmp"
$zipName = "DrMemory-Windows-$DrMemoryVersion.zip"
$zipPath = Join-Path $tmpRoot $zipName
$extractRoot = Join-Path $tmpRoot "drmemory"
$packageRoot = Join-Path $extractRoot "DrMemory-Windows-$DrMemoryVersion"
$drMemoryExe = Join-Path $packageRoot "bin64\drmemory.exe"

if ([string]::IsNullOrWhiteSpace($ExecutablePath)) {
    $ExecutablePath = Join-Path $repoRoot "build\$Preset\src\$Configuration\blinkbreak.exe"
}

if ([string]::IsNullOrWhiteSpace($ResultRoot)) {
    $ResultRoot = Join-Path $tmpRoot "drmemory-results"
}

New-Item -ItemType Directory -Force -Path $tmpRoot | Out-Null
New-Item -ItemType Directory -Force -Path $ResultRoot | Out-Null

if ($BuildFirst) {
    Write-Host "Configuring CMake preset '$Preset'..."
    & cmake --preset=$Preset
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    Write-Host "Building preset '$Preset' ($Configuration)..."
    & cmake --build --preset=$Preset --config $Configuration
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

if ($Redownload) {
    Remove-Item $zipPath -Force -ErrorAction SilentlyContinue
    Remove-Item $extractRoot -Recurse -Force -ErrorAction SilentlyContinue
}

if (-not (Test-Path $zipPath)) {
    $downloadUri = "https://github.com/DynamoRIO/drmemory/releases/download/cronbuild-$DrMemoryVersion/$zipName"
    Write-Host "Downloading Dr. Memory $DrMemoryVersion..."
    Invoke-WebRequest -Uri $downloadUri -OutFile $zipPath
}

if (-not (Test-Path $drMemoryExe)) {
    Write-Host "Extracting Dr. Memory archive..."
    Expand-Archive -Path $zipPath -DestinationPath $extractRoot -Force
}

if (-not (Test-Path $drMemoryExe)) {
    Write-Error "Dr. Memory executable not found at $drMemoryExe"
    exit 1
}

if (-not (Test-Path $ExecutablePath)) {
    Write-Error "BlinkBreak executable not found: $ExecutablePath"
    Write-Error "Build first with 'cmake --build --preset=$Preset --config $Configuration' or pass -BuildFirst."
    exit 1
}

$drMemoryArgs = @(
    "-batch",
    "-brief",
    "-logdir", $ResultRoot,
    "-exit_code_if_errors", "1"
)

Write-Host "Running Dr. Memory against: $ExecutablePath"
if ($AppArgs.Count -gt 0) {
    Write-Host ("Application arguments: " + ($AppArgs -join " "))
} else {
    Write-Host "Application arguments: <none>"
}

& $drMemoryExe @drMemoryArgs -- $ExecutablePath @AppArgs
$drMemoryExitCode = $LASTEXITCODE

$latestResultDir = Get-ChildItem $ResultRoot -Directory -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -ne "symcache" } |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1

if ($null -ne $latestResultDir) {
    $resultFile = Join-Path $latestResultDir.FullName "results.txt"
    Write-Host "Latest result directory: $($latestResultDir.FullName)"
    if (Test-Path $resultFile) {
        Write-Host "Results file: $resultFile"
    }
}

exit $drMemoryExitCode
