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
& $builder $source $out "-prefix=DriveLink" "-packonly" "-clear"
if ($LASTEXITCODE -ne 0) { throw "AddonBuilder failed ($LASTEXITCODE)" }
$pbo = Join-Path $out "DriveLink.pbo"
if (!(Test-Path $pbo)) { throw "No PBO at $pbo" }
Write-Host "Built $pbo ($((Get-Item $pbo).Length) bytes)"
$link = "E:\Steam\steamapps\common\DayZServer\@DriveLink"
if (!(Test-Path -LiteralPath $link)) { New-Item -ItemType Junction -Path $link -Target $modRoot | Out-Null; Write-Host "Linked $link" }
