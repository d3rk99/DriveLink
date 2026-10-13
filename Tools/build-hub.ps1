# Builds the hub: Hub\DriveLinkHub.exe. Usage: powershell -ExecutionPolicy Bypass -File Tools\build-hub.ps1
$ErrorActionPreference = "Stop"
$hub = Join-Path (Split-Path -Parent $PSScriptRoot) "Hub"
Push-Location $hub
try { go build -o DriveLinkHub.exe . ; if ($LASTEXITCODE) { throw "go build failed" } } finally { Pop-Location }
Write-Host "Built $hub\DriveLinkHub.exe"
