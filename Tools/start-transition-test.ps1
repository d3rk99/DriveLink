[CmdletBinding()]
param([switch]$Restart, [switch]$Client, [switch]$MenuOnly, [switch]$WithZen, [ValidateSet(2802,2902)][int]$Port=2802,
    [string]$ModRoot = 'E:\DayZMods\FilmPrototypeRuntime\@DriveLinkFilmPrototype')
$ErrorActionPreference = 'Stop'
$runtime = 'E:\DayZMods\FilmPrototypeRuntime'
$mod = $ModRoot
$serverRoot = 'E:\Steam\steamapps\common\DayZServer'
$workshop = 'E:\Steam\steamapps\common\DayZ\!Workshop'
$mods = @((Join-Path $serverRoot '@CF'), (Join-Path $serverRoot '@VPPAdminTools'))
if ($WithZen) {
    $mods += @((Join-Path $workshop '@Zens Core Mod'), (Join-Path $workshop '@Zens Immersive Login'))
}
$mods += $mod
foreach ($path in $mods) {
    if (!(Test-Path -LiteralPath $path)) { throw "Missing compatibility test mod: $path" }
}
if ($Client) {
    if (Get-Process -Name DayZ_x64,DayZ_BE -ErrorAction SilentlyContinue) { throw 'Exit the running DayZ client before launching this test.' }
    $clientRoot = 'E:\Steam\steamapps\common\DayZ'
    $args = @('0','1','1','-exe','DayZ_x64.exe','-noPause','-noSplash','-window',
        ('"-mod=' + ($mods -join ';') + '"'), '-dlTransitionDemo',
        "-profiles=$runtime\ClientProfile", '-name=DriveLinkFilmTest')
    if (!$MenuOnly) { $args += @('-connect=127.0.0.1', "-port=$Port") }
    Start-Process -FilePath (Join-Path $clientRoot 'DayZ_BE.exe') -ArgumentList $args -WorkingDirectory $clientRoot -WindowStyle Hidden
    return
}
foreach ($path in $mods) {
    Get-ChildItem -LiteralPath (Join-Path $path 'keys') -Filter '*.bikey' | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $serverRoot 'keys') -Force
    }
}
if (!(Get-Process -Name DriveLinkHub -ErrorAction SilentlyContinue)) {
    Start-Process -FilePath 'E:\DayZMods\DriveLink\dist\hub-v0.4.1\DriveLinkHub\DriveLinkHub.exe' -ArgumentList @('-config','E:\DayZMods\DriveLink\Hub\hub.json') -WorkingDirectory 'E:\DayZMods\DriveLink\Hub' -WindowStyle Hidden
    Start-Sleep -Seconds 2
}
$null = Invoke-RestMethod 'http://127.0.0.1:7290/admin/api/state'
foreach ($server in @(@{Root='E:\DayZMods\TestServer';Port=2802},@{Root='E:\DayZMods\TestServer2';Port=2902})) {
    $config = Join-Path $server.Root 'serverDZ.cfg'
    $running = @(Get-CimInstance Win32_Process -Filter "Name='DayZServer_x64.exe'" | Where-Object { $_.CommandLine -like "*-config=$config *" })
    if ($running -and !$Restart) { Write-Output "$config already running"; continue }
    foreach ($process in $running) { Stop-Process -Id $process.ProcessId -Force }
    foreach ($process in $running) { Wait-Process -Id $process.ProcessId -Timeout 20 -ErrorAction SilentlyContinue }
    $args = @("-config=$config", "-profiles=$($server.Root)\Profiles", "-port=$($server.Port)",
        '-dologs','-adminlog','-netlog','-freezecheck', ('"-mod=' + ($mods -join ';') + '"'))
    $started = Start-Process -FilePath (Join-Path $serverRoot 'DayZServer_x64.exe') -ArgumentList $args -WorkingDirectory $serverRoot -WindowStyle Hidden -PassThru
    Write-Output "Prototype test server on $($server.Port): PID $($started.Id)"
}
