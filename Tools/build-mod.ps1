# Packs core DriveLink, the automatic Zen bridge and travel companion into @DriveLink\Addons.
# Usage: powershell -ExecutionPolicy Bypass -File Tools\build-mod.ps1 [-Dev]
#   (none) = the release build: no test switches
#   -Dev   = adds the DL_DEV define: F9 demo film, debug.json test switches (test car on login, car rebuild zone),
#            clients log every detail
param([switch]$Dev,[string]$Destination="E:\DayZMods\Mods\@DriveLink")
$ErrorActionPreference = "Stop"
$repo    = Split-Path -Parent $PSScriptRoot
$source  = Join-Path $repo "Addons\DriveLink"
if ($Dev) {
  # a copy whose CfgMods declares DL_DEV (the scripts' #ifdef DL_DEV parts); the film's build leftovers stay out
  $stage = Join-Path $env:TEMP "DriveLinkDevBuild\DriveLink"
  if (Test-Path -LiteralPath $stage) { if([IO.Path]::GetFullPath($stage) -ne [IO.Path]::GetFullPath((Join-Path $env:TEMP "DriveLinkDevBuild\DriveLink"))){throw "Unexpected dev build path"}; Remove-Item -LiteralPath $stage -Recurse -Force }
  New-Item -ItemType Directory -Force -Path $stage | Out-Null
  Get-ChildItem -LiteralPath $source -Force | Where-Object { $_.Name -ne "gui" } | Copy-Item -Destination $stage -Recurse -Force
  New-Item -ItemType Directory -Force -Path (Join-Path $stage "gui\anim") | Out-Null
  Get-ChildItem -LiteralPath (Join-Path $source "gui\anim") -Force | Where-Object { $_.Name -ne "_minidayz_src" } | Copy-Item -Destination (Join-Path $stage "gui\anim") -Recurse -Force
  $cfg = Join-Path $stage "config.cpp"
  $text = Get-Content -LiteralPath $cfg -Raw
  if ($text -notmatch 'type = "mod";') { throw "config.cpp: no type = mod line to put the DL_DEV define after" }
  $text = $text -replace 'type = "mod";', ('type = "mod";' + "`r`n`t`tdefines[] = {`"DL_DEV`"};")
  Set-Content -LiteralPath $cfg -Value $text -Encoding ASCII
  $source = $stage
  Write-Host "DEV build (DL_DEV)"
}
$modRoot = $Destination
$out     = Join-Path $modRoot "Addons"
$builder = "E:\Steam\steamapps\common\DayZ Tools\Bin\AddonBuilder\AddonBuilder.exe"
if (!(Test-Path -LiteralPath $builder)) { throw "AddonBuilder not found at $builder" }
New-Item -ItemType Directory -Force -Path $out | Out-Null
$builtPbos = @()
foreach ($addon in @(@{Name='DriveLink';Source=$source},
    @{Name='DriveLink_ZenLogin';Source=(Join-Path $repo 'Addons\DriveLink_ZenLogin')},
    @{Name='DriveLinkRestart';Source=(Join-Path $repo 'Addons\DriveLinkRestart')})) {
$pbo = Join-Path $out ($addon.Name + '.pbo')
$started = Get-Date
# AddonBuilder exits 0 even on "[ERROR]: Build failed" (2026-10-18: the old PBO stayed because a running DayZ
# client/server held it open), so check its log text and that the PBO really is new.
# Its stderr banner ("Setting breakpad minidump ...") would stop the script under "Stop" in PowerShell 5.1.
$ErrorActionPreference = "Continue"
$log = & $builder $addon.Source $out ("-prefix=" + $addon.Name) "-packonly" "-clear" 2>&1 | Out-String
$ErrorActionPreference = "Stop"
if ($LASTEXITCODE -ne 0 -or $log -match "\[ERROR\]") {
  Write-Host $log
  throw "AddonBuilder failed. If it failed while copying the PBO: close the DayZ game and stop the test servers (they keep the PBO open)."
}
if (!(Test-Path $pbo) -or (Get-Item $pbo).LastWriteTime -lt $started.AddSeconds(-2)) { throw "No new PBO at $pbo" }
Write-Host "Built $pbo ($((Get-Item $pbo).Length) bytes)"
$builtPbos += $pbo
}
# The user requires Derk signing only; fail closed if that key is unavailable.
& (Join-Path $PSScriptRoot 'sign-derk.ps1') -ModPath $modRoot
Copy-Item -LiteralPath (Join-Path $repo "Mod\mod.cpp") -Destination $modRoot -Force
$link ="E:\Steam\steamapps\common\DayZServer\@DriveLink"
if ($Destination -eq "E:\DayZMods\Mods\@DriveLink" -and !(Test-Path -LiteralPath $link)) { New-Item -ItemType Junction -Path $link -Target $modRoot | Out-Null; Write-Host "Linked $link" }
