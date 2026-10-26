[CmdletBinding()]
param([string]$ModRoot = 'E:\DayZMods\FilmPrototypeRuntime\@DriveLinkFilmPrototype', [switch]$ZenCompat)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$runtime = 'E:\DayZMods\FilmPrototypeRuntime'
$out = Join-Path $modRoot 'Addons'
$keys = Join-Path $runtime 'Signing'
New-Item -ItemType Directory -Path $out,$keys,(Join-Path $modRoot 'keys') -Force | Out-Null
$ds = 'E:\Steam\steamapps\common\DayZ Tools\Bin\DsUtils'
$private = Join-Path $keys 'DriveLinkFilmPrototype.biprivatekey'
if (!(Test-Path -LiteralPath $private)) {
    Push-Location $keys
    try { & (Join-Path $ds 'DSCreateKey.exe') 'DriveLinkFilmPrototype' } finally { Pop-Location }
    if (!(Test-Path -LiteralPath $private)) { throw 'Prototype signing key was not created' }
}
$builder = 'E:\Steam\steamapps\common\DayZ Tools\Bin\AddonBuilder\AddonBuilder.exe'
$source = Join-Path $repo 'Addons\DriveLink'
$started = Get-Date
$ErrorActionPreference = 'Continue'
$log = & $builder $source $out '-prefix=DriveLink' '-packonly' '-clear' 2>&1 | Out-String
$ErrorActionPreference = 'Stop'
$pbo = Join-Path $out 'DriveLink.pbo'
if ($LASTEXITCODE -ne 0 -or $log -match '\[ERROR\]' -or !(Test-Path -LiteralPath $pbo)) { throw $log }
if ((Get-Item -LiteralPath $pbo).LastWriteTime -lt $started.AddSeconds(-2)) { throw 'PBO was not rebuilt' }
& (Join-Path $ds 'DSSignFile.exe') $private $pbo
if ($LASTEXITCODE -ne 0) { throw 'Prototype signing failed' }
# Keep -ZenCompat accepted for older commands; every package now includes the
# automatically guarded bridge, whether or not Zen is installed on the server.
& {
    $compatSource = Join-Path $repo 'Addons\DriveLink_ZenLogin'
    $ErrorActionPreference = 'Continue'
    $compatLog = & $builder $compatSource $out '-prefix=DriveLink_ZenLogin' '-packonly' '-clear' 2>&1 | Out-String
    $ErrorActionPreference = 'Stop'
    $compatPbo = Join-Path $out 'DriveLink_ZenLogin.pbo'
    if ($LASTEXITCODE -ne 0 -or $compatLog -match '\[ERROR\]' -or !(Test-Path -LiteralPath $compatPbo)) { throw $compatLog }
    & (Join-Path $ds 'DSSignFile.exe') $private $compatPbo
    if ($LASTEXITCODE -ne 0) { throw 'Compatibility addon signing failed' }
}
Copy-Item -LiteralPath (Join-Path $keys 'DriveLinkFilmPrototype.bikey') -Destination (Join-Path $modRoot 'keys') -Force
Copy-Item -LiteralPath (Join-Path $repo 'Mod\mod.cpp') -Destination $modRoot -Force
Write-Output "Built signed experimental mod: $modRoot"
