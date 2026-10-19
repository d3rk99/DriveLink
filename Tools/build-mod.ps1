# Packs Addons\DriveLink into @DriveLink\Addons\DriveLink.pbo (scripts + config, -packonly) and links it into
# BOTH test servers. Usage: powershell -ExecutionPolicy Bypass -File Tools\build-mod.ps1
$ErrorActionPreference = "Stop"
$repo    = Split-Path -Parent $PSScriptRoot
$source  = Join-Path $repo "Addons\DriveLink"
$modRoot = "C:\DayZMods\Mods\@DriveLink"
$out     = Join-Path $modRoot "Addons"
$builder = "E:\Steam\steamapps\common\DayZ Tools\Bin\AddonBuilder\AddonBuilder.exe"
if (!(Test-Path -LiteralPath $builder)) { throw "AddonBuilder not found at $builder" }
New-Item -ItemType Directory -Force -Path $out | Out-Null
$pbo = Join-Path $out "DriveLink.pbo"
$started = Get-Date
# AddonBuilder exits 0 even on "[ERROR]: Build failed" (2026-10-18: the old PBO stayed because a running DayZ
# client/server held it open), so check its log text and that the PBO really is new.
$log = & $builder $source $out "-prefix=DriveLink" "-packonly" "-clear" 2>&1 | Out-String
if ($LASTEXITCODE -ne 0 -or $log -match "\[ERROR\]") {
  Write-Host $log
  throw "AddonBuilder failed. If it failed while copying the PBO: close the DayZ game and stop the test servers (they keep the PBO open)."
}
if (!(Test-Path $pbo) -or (Get-Item $pbo).LastWriteTime -lt $started.AddSeconds(-2)) { throw "No new PBO at $pbo" }
Write-Host "Built $pbo ($((Get-Item $pbo).Length) bytes)"
$link = "E:\Steam\steamapps\common\DayZServer\@DriveLink"
if (!(Test-Path -LiteralPath $link)) { New-Item -ItemType Junction -Path $link -Target $modRoot | Out-Null; Write-Host "Linked $link" }
