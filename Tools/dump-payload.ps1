# Writes the latest PENDING transfer's payload_text to Hub\last-payload.json (for debugging; no secrets in it).
# Usage: powershell -ExecutionPolicy Bypass -File Tools\dump-payload.ps1
$ErrorActionPreference = "Stop"
$secrets = Get-Content "C:\DayZMods\DriveLink-secrets.txt" -Raw
$psql = Get-ChildItem "C:\Program Files\PostgreSQL\*\bin\psql.exe" | Sort-Object FullName -Descending | Select-Object -First 1
if (!$psql) { throw "psql.exe not found under C:\Program Files\PostgreSQL" }
Write-Host "Enter the drivelink database password when asked (it is in DriveLink-secrets.txt)."
$out = Join-Path (Split-Path -Parent $PSScriptRoot) "Hub\last-payload.json"
& $psql.FullName -U drivelink -d drivelink -h localhost -At -c "select payload_text from transfers where state='pending' order by created_at desc limit 1" | Out-File -Encoding utf8NoBOM $out
Write-Host "Wrote $out ($((Get-Item $out).Length) bytes)"
