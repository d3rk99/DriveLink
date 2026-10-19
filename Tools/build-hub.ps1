# Builds the hub (Rust): Hub\DriveLinkHub.exe. Usage: powershell -ExecutionPolicy Bypass -File Tools\build-hub.ps1
# The running hub keeps the exe locked: stop it (console Ctrl+C, or "sc stop DriveLinkHub") before building.
$ErrorActionPreference = "Stop"
$hub = Join-Path (Split-Path -Parent $PSScriptRoot) "Hub"
Push-Location $hub
try {
  cargo build --release
  if ($LASTEXITCODE) { throw "cargo build failed" }
  Copy-Item -Force "target\release\drivelink-hub.exe" "DriveLinkHub.exe"
} finally { Pop-Location }
Write-Host "Built $hub\DriveLinkHub.exe"
