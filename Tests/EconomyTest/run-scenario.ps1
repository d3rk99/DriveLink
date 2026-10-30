# The whole vehicle economy scenario after a baseline (README.md): every step waits for its server to exit cleanly.
# Usage: powershell -ExecutionPolicy Bypass -File Tests\EconomyTest\run-scenario.ps1   (about 50 minutes)
$ErrorActionPreference='Stop'
$r=Join-Path $PSScriptRoot 'run-econ.ps1'
function Run([string]$server,[string]$steps){
 Write-Host "=== $(Get-Date -Format HH:mm:ss) $server $steps"
 & powershell -ExecutionPolicy Bypass -File $r -Step run -Server $server -Steps $steps -Wait
 if($LASTEXITCODE -ne 0){throw "$server $steps failed"}
}
function Cars { Write-Host "--- hub cars"; & powershell -ExecutionPolicy Bypass -File $r -Step cars }

& powershell -ExecutionPolicy Bypass -File $r -Step reset
Run A baseline;   Run B baseline              # both spawn their 8 hatchbacks and save
Run A send4;      & powershell -ExecutionPolicy Bypass -File $r -Step carry -From A -To B
Run B receive;    Cars                       # T6 setup: 4 of A's cars on B
Run A idle7min                               # T6: A's CE refills 4 -> all 4 vetoed
Run B ruin2;      Cars                       # T7: 2 of them wrecked on B -> gone after 3 reports
Run A idle7min                               # T7: refill of 4 -> 2 allowed, 2 vetoed
Run B sendback2;  & powershell -ExecutionPolicy Bypass -File $r -Step carry -From B -To A
Run A receive;    Cars                       # T9: back home, nothing away
Run A idle7min                               # T9: A has 8, CE's refills vetoed by the came-home guard
Write-Host "=== $(Get-Date -Format HH:mm:ss) done"
