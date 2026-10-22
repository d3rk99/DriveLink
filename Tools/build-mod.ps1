# Packs core DriveLink and its automatic Zen bridge into @DriveLink\Addons (scripts + config, -packonly) and links it into
# BOTH test servers. Usage: powershell -ExecutionPolicy Bypass -File Tools\build-mod.ps1 [-Dev]
#   (none) = the release build: no test switches
#   -Dev   = adds the DL_DEV define: F9 demo film, debug.json test switches (test car on login, car rebuild zone),
#            clients log every detail
param([switch]$Dev)
$ErrorActionPreference = "Stop"
$repo    = Split-Path -Parent $PSScriptRoot
$source  = Join-Path $repo "Addons\DriveLink"
if ($Dev) {
  # a copy whose CfgMods declares DL_DEV (the scripts' #ifdef DL_DEV parts); the film's build leftovers stay out
  $stage = Join-Path $env:TEMP "DriveLinkDevBuild\DriveLink"
  if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
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
$modRoot = "C:\DayZMods\Mods\@DriveLink"
$out     = Join-Path $modRoot "Addons"
$builder = "E:\Steam\steamapps\common\DayZ Tools\Bin\AddonBuilder\AddonBuilder.exe"
if (!(Test-Path -LiteralPath $builder)) { throw "AddonBuilder not found at $builder" }
New-Item -ItemType Directory -Force -Path $out | Out-Null
$builtPbos = @()
foreach ($addon in @(@{Name='DriveLink';Source=$source},
    @{Name='DriveLink_ZenLogin';Source=(Join-Path $repo 'Addons\DriveLink_ZenLogin')})) {
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
# signed with the release key (kept OUTSIDE the repo; a copy in C:\DayZMods\KeyBackups), its public key in keys\,
# mod.cpp (name, logo, description) at the mod's root
$signDir = "C:\DayZMods\DriveLink-signing"
$privKey = Join-Path $signDir "DriveLink.biprivatekey"
$pubKey  = Join-Path $signDir "DriveLink.bikey"
$signer  = "E:\Steam\steamapps\common\DayZ Tools\Bin\DsUtils\DSSignFile.exe"
Get-ChildItem -LiteralPath $out -Filter "*.bisign" | Remove-Item -Force
if (Test-Path -LiteralPath $privKey) {
  foreach ($builtPbo in $builtPbos) {
    & $signer $privKey $builtPbo
    if ($LASTEXITCODE -ne 0) { throw "DSSignFile failed: $builtPbo" }
  }
  $keys = Join-Path $modRoot "keys"
  New-Item -ItemType Directory -Force -Path $keys | Out-Null
  Get-ChildItem -LiteralPath $keys -Filter "*.bikey" | Remove-Item -Force
  Copy-Item -LiteralPath $pubKey -Destination $keys -Force
  Write-Host "Signed with DriveLink.biprivatekey, keys\DriveLink.bikey"
} else { Write-Host "WARNING: no $privKey - the PBO is NOT signed" }
Copy-Item -LiteralPath (Join-Path $repo "Mod\mod.cpp") -Destination $modRoot -Force
$link ="E:\Steam\steamapps\common\DayZServer\@DriveLink"
if (!(Test-Path -LiteralPath $link)) { New-Item -ItemType Junction -Path $link -Target $modRoot | Out-Null; Write-Host "Linked $link" }
