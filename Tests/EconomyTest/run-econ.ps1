# Vehicle economy test (Research/vehicle-economy.md): a test hub + two isolated Chernarus servers (A, B), no players.
# Cars "travel" as snapshot files (DL_MissionServerEconTest.c); departure/arrival/veto/ledger are the real code.
# Usage: powershell -ExecutionPolicy Bypass -File Tests\EconomyTest\run-econ.ps1 -Step <name>
#   build  dev mod -> Build\@DriveLink, hub -> Hub\target\release
#   reset  stop everything, wipe both servers' storage + DriveLink state and the test hub database
#   run    -Server A|B -Steps <name>  start one server with steps\<name>.json as its debug.json (-Wait: until it exits)
#   carry  copy A's outbox to B's inbox (-From A -To B)
#   hub / stop / cars
# Owns: Tests\EconomyTest\Build\ (gitignored), mpmissions\dlcetest.chernarusplus (A, from Tests\CESpike) and
# mpmissions\dlcetest2.chernarusplus (B).
param([Parameter(Mandatory=$true)][string]$Step,[string]$Server='A',[string]$Steps='',[string]$From='A',[string]$To='B',[switch]$Wait,[int]$TimeoutSec=900)
$ErrorActionPreference='Stop'
$here=$PSScriptRoot
$repo=(Resolve-Path (Join-Path $here '..\..')).Path
$build=Join-Path $here 'Build'
$serverRoot='E:\Steam\steamapps\common\DayZServer'
$hubExe=Join-Path $repo 'Hub\target\release\drivelink-hub.exe'
$hubDir=Join-Path $build 'hub'
$key='econtest-local-key'
$servers=@{
 A=@{Id='cher_a';Mission='dlcetest.chernarusplus';Instance=77;Port=2702;Query=27017};
 B=@{Id='cher_b';Mission='dlcetest2.chernarusplus';Instance=78;Port=2712;Query=27027}
}
New-Item -ItemType Directory -Force -Path $build | Out-Null

function Owned-Stop([string]$pidFile){
 if(!(Test-Path -LiteralPath $pidFile)){return}
 $r=Get-Content -Raw -LiteralPath $pidFile|ConvertFrom-Json
 $p=Get-Process -Id $r.processId -ErrorAction SilentlyContinue
 if($p -and $p.StartTime.ToUniversalTime().Ticks -eq ([datetime]$r.startTimeUtc).ToUniversalTime().Ticks){Stop-Process -Id $p.Id;Wait-Process -Id $p.Id -Timeout 20 -ErrorAction SilentlyContinue}
 Remove-Item -LiteralPath $pidFile
}
function Save-Pid($p,[string]$pidFile){@{processId=$p.Id;startTimeUtc=$p.StartTime.ToUniversalTime().ToString('o')}|ConvertTo-Json|Set-Content -LiteralPath $pidFile}
function Mission-Path($s){Join-Path $serverRoot ('mpmissions\'+$s.Mission)}
function Profile-Path([string]$name){Join-Path $build "$name\profile"}

function Ensure-Mission($s){
 $m=Mission-Path $s
 $marker=Join-Path $m '.dlce-spike-test'
 if(Test-Path -LiteralPath $m){ if(!(Test-Path -LiteralPath $marker)){throw "$m exists without the test marker"}; return }
 $a=Mission-Path $servers.A
 if(!(Test-Path -LiteralPath $a)){throw 'Run Tests\CESpike\run-spike.ps1 once first (it makes the A mission with CE logging and idle mode off)'}
 New-Item -ItemType Directory -Path $m | Out-Null
 Get-ChildItem -LiteralPath $a | Where-Object Name -notlike 'storage_*' | Copy-Item -Destination $m -Recurse
}

function Start-Hub {
 if(Test-Path -LiteralPath (Join-Path $build 'hub-process.json')){
  $r=Get-Content -Raw -LiteralPath (Join-Path $build 'hub-process.json')|ConvertFrom-Json
  if(Get-Process -Id $r.processId -ErrorAction SilentlyContinue){return}
 }
 New-Item -ItemType Directory -Force -Path $hubDir | Out-Null
 $cfg=Join-Path $hubDir 'hub.json'
 if(!(Test-Path -LiteralPath $cfg)){
  $list=@()
  foreach($n in 'A','B'){$s=$servers[$n];$list+=@{id=$s.Id;name="Econ test $n";lanHost='127.0.0.1';publicHost='127.0.0.1';port=$s.Port;queryPort=$s.Query}}
  @{listen='127.0.0.1:7390';database='drivelink.db';serverKey=$key;adminPassword='econtest-local-admin';servers=$list}|ConvertTo-Json -Depth 5|Set-Content -LiteralPath $cfg -Encoding ascii
 }
 $p=Start-Process -FilePath $hubExe -ArgumentList @('-config',$cfg) -WorkingDirectory $hubDir -WindowStyle Hidden -PassThru
 Save-Pid $p (Join-Path $build 'hub-process.json')
 Start-Sleep -Seconds 2
 $null=Invoke-RestMethod 'http://127.0.0.1:7390/health'
 Write-Host "Test hub PID $($p.Id) on 127.0.0.1:7390"
}

switch($Step){
 'build' {
  & (Join-Path $repo 'Tools\build-mod.ps1') -Dev -Destination (Join-Path $build '@DriveLink')
  Push-Location (Join-Path $repo 'Hub'); try { cargo build --release; if($LASTEXITCODE -ne 0){throw 'hub build failed'} } finally { Pop-Location }
 }
 'hub' { Start-Hub }
 'stop' {
  foreach($n in 'A','B'){Owned-Stop (Join-Path $build "server-$n.json")}
  Owned-Stop (Join-Path $build 'hub-process.json')
 }
 'reset' {
  foreach($n in 'A','B'){Owned-Stop (Join-Path $build "server-$n.json")}
  Owned-Stop (Join-Path $build 'hub-process.json')
  foreach($n in 'A','B'){
   $s=$servers[$n]; Ensure-Mission $s
   $st=Join-Path (Mission-Path $s) ('storage_'+$s.Instance)
   if(Test-Path -LiteralPath $st){Remove-Item -LiteralPath $st -Recurse -Force}
   $dl=Join-Path (Profile-Path $n) 'DriveLink'
   if(Test-Path -LiteralPath $dl){Remove-Item -LiteralPath $dl -Recurse -Force}
  }
  if(Test-Path -LiteralPath $hubDir){Get-ChildItem -LiteralPath $hubDir -Filter 'drivelink.db*'|Remove-Item}
  Write-Host 'Reset: both storages, DriveLink state and the test hub database are empty'
 }
 'carry' {
  $out=Join-Path (Profile-Path $From) 'DriveLink\econ-outbox.json'
  if(!(Test-Path -LiteralPath $out)){throw "No outbox on $From"}
  Move-Item -LiteralPath $out -Destination (Join-Path (Profile-Path $To) 'DriveLink\econ-inbox.json') -Force
  Write-Host "Carried $From -> $To"
 }
 'cars' { & $hubExe -config (Join-Path $hubDir 'hub.json') cars }
 'run' {
  Start-Hub
  $s=$servers[$Server]; Ensure-Mission $s
  $pidFile=Join-Path $build "server-$Server.json"
  Owned-Stop $pidFile
  $profile=Profile-Path $Server
  $dl=Join-Path $profile 'DriveLink'
  New-Item -ItemType Directory -Force -Path $dl | Out-Null
  $cfgFile=Join-Path $dl 'config.json'
  if(!(Test-Path -LiteralPath $cfgFile)){
   @{hubUrl='http://127.0.0.1:7390';hubKey=$key;serverId=$s.Id;log='verbose';carEconomy='veto'}|ConvertTo-Json|Set-Content -LiteralPath $cfgFile -Encoding ascii
  }
  if($Steps){Copy-Item -LiteralPath (Join-Path $here "steps\$Steps.json") -Destination (Join-Path $dl 'debug.json') -Force}
  $cfg=Join-Path $build "serverDZ-$Server.cfg"
  @"
hostname="DriveLink economy test $Server";
password="";
maxPlayers=2;
BattlEye=0;
verifySignatures=2;
forceSameBuild=1;
steamQueryPort=$($s.Query);
instanceId=$($s.Instance);
storageAutoFix=1;
serverTime="2026/9/20/12/00";
serverTimePersistent=0;
class Missions { class DayZ { template="$($s.Mission)"; }; };
"@ | Set-Content -LiteralPath $cfg -Encoding ascii
  $mod=Join-Path $build '@DriveLink'
  $args=@("-port=$($s.Port)","-config=$cfg","-profiles=$profile","-mod=$mod",'-dologs','-adminlog','-freezecheck')
  $p=Start-Process -FilePath (Join-Path $serverRoot 'DayZServer_x64.exe') -WorkingDirectory $serverRoot -ArgumentList $args -WindowStyle Hidden -PassThru
  Save-Pid $p $pidFile
  Write-Host "Server $Server ($($s.Id)) PID $($p.Id)"
  if($Wait){
   if(!$p.WaitForExit($TimeoutSec*1000)){Owned-Stop $pidFile;throw "Server $Server did not exit within $TimeoutSec s"}
   Remove-Item -LiteralPath $pidFile -ErrorAction SilentlyContinue
   Write-Host "Server $Server exited"
  }
 }
 default { throw "Unknown step $Step" }
}
