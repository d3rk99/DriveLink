# Run after cargo build --release --locked in Hub. Packages only allowlisted files.
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$hub = Join-Path $repo 'Hub'
$manifest = Get-Content -LiteralPath (Join-Path $hub 'Cargo.toml') -Raw
if ($manifest -notmatch '(?m)^version\s*=\s*"([^"]+)"') { throw 'Hub version missing' }
$version = $Matches[1]
$out = Join-Path $repo "dist\hub-v$version"
if (Test-Path -LiteralPath $out) { throw "Output already exists: $out" }
$stage = Join-Path $out 'DriveLinkHub'
New-Item -ItemType Directory -Path $stage -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $hub 'target\release\drivelink-hub.exe') -Destination (Join-Path $stage 'DriveLinkHub.exe')
foreach ($name in @('INSTALL.txt','Start-Hub.cmd','Install-Service.cmd','Uninstall-Service.cmd')) {
    Copy-Item -LiteralPath (Join-Path $hub "package\$name") -Destination $stage
}
Copy-Item -LiteralPath (Join-Path $repo 'README.md') -Destination (Join-Path $stage 'SERVER-GUIDE.md')
Copy-Item -LiteralPath (Join-Path $hub 'CHANGELOG.md') -Destination $stage
Add-Type -AssemblyName System.IO.Compression.FileSystem
$minimal = Join-Path $out "DriveLinkHub-v$version-windows-x64.zip"
[IO.Compression.ZipFile]::CreateFromDirectory($stage, $minimal, [IO.Compression.CompressionLevel]::Optimal, $true)
$maps = @('chernarusplus','enoch','sakhal','nasdara')
foreach ($map in $maps) {
    $source = Join-Path $hub "maps\$map"
    $metaPath = Join-Path $source 'meta.json'
    $meta = Get-Content -LiteralPath $metaPath -Raw | ConvertFrom-Json
    if ($meta.map -ne $map -or $meta.size -le 0 -or $meta.minZoom -gt 0 -or $meta.maxZoom -ne 0) { throw "Invalid map metadata: $map" }
    $destination = Join-Path $stage "maps\$map"
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    Copy-Item -LiteralPath $metaPath -Destination $destination
    for ($zoom = $meta.minZoom; $zoom -le $meta.maxZoom; $zoom++) {
        $tiles = @(Get-ChildItem -LiteralPath (Join-Path $source "$zoom") -File -Filter '*.jpg' | Where-Object Name -Match '^\d+_\d+\.jpg$')
        if (!$tiles.Count) { throw "Missing map tiles: $map zoom $zoom" }
        $level = Join-Path $destination "$zoom"
        New-Item -ItemType Directory -Path $level -Force | Out-Null
        foreach ($tile in $tiles) { Copy-Item -LiteralPath $tile.FullName -Destination $level }
    }
}
$full = Join-Path $out "DriveLinkHub-v$version-windows-x64-with-official-maps.zip"
[IO.Compression.ZipFile]::CreateFromDirectory($stage, $full, [IO.Compression.CompressionLevel]::Optimal, $true)
$checksums = foreach ($file in @($minimal, $full, (Join-Path $stage 'DriveLinkHub.exe'))) {
    $hash = Get-FileHash -LiteralPath $file -Algorithm SHA256
    '{0}  {1}' -f $hash.Hash.ToLowerInvariant(), (Split-Path -Leaf $file)
}
$checksums | Set-Content -LiteralPath (Join-Path $out 'SHA256SUMS.txt') -Encoding ascii
Write-Output "Packages and checksums written to $out"
