# Starts one isolated Chernarus server with the CE spike addon (-serverMod) and a phase config.
# Usage: powershell -ExecutionPolicy Bypass -File Tests\CESpike\run-spike.ps1 -Phase baseline [-Fresh] [-Stop]
#   -Fresh deletes this test mission's own storage (storage_77) first, so CE spawns everything new.
#   -Stop  only stops the server this script started.
# Owns: mpmissions\dlcetest.chernarusplus (marker file), Tests\CESpike\Build\ (gitignored).
param([string]$Phase='baseline',[switch]$Fresh,[switch]$Stop,[int]$GamePort=2702,[int]$QueryPort=27017)
$ErrorActionPreference='Stop'
$here=$PSScriptRoot
$build=Join-Path $here 'Build'
$serverRoot='E:\Steam\steamapps\common\DayZServer'
$missionName='dlcetest.chernarusplus'
$mission=Join-Path $serverRoot "mpmissions\$missionName"
$marker=Join-Path $mission '.dlce-spike-test'
$pidFile=Join-Path $build 'server-process.json'

function Stop-Owned {
 if(!(Test-Path -LiteralPath $pidFile)){return}
 $r=Get-Content -Raw -LiteralPath $pidFile|ConvertFrom-Json
 $p=Get-Process -Id $r.processId -ErrorAction SilentlyContinue
 if($p -and $p.StartTime.ToUniversalTime().Ticks -eq ([datetime]$r.startTimeUtc).ToUniversalTime().Ticks){
  Stop-Process -Id $p.Id; Wait-Process -Id $p.Id -Timeout 20 -ErrorAction SilentlyContinue
  Write-Host "Stopped spike server $($p.Id)"
 }
 Remove-Item -LiteralPath $pidFile
}
Stop-Owned
if($Stop){return}

# addon: pack + Derk-sign (the server never checks -serverMod signatures, but every PBO here is Derk-signed)
$mod=Join-Path $build '@DL_CESpike'
New-Item -ItemType Directory -Force -Path (Join-Path $mod 'Addons') | Out-Null
$builder='E:\Steam\steamapps\common\DayZ Tools\Bin\AddonBuilder\AddonBuilder.exe'
$ErrorActionPreference='Continue'
$log=& $builder (Join-Path $here 'DL_CESpike') (Join-Path $mod 'Addons') '-prefix=DL_CESpike' '-packonly' '-clear' 2>&1|Out-String
$ErrorActionPreference='Stop'
if($LASTEXITCODE -ne 0 -or $log -match '\[ERROR\]'){Write-Host $log;throw 'AddonBuilder failed'}
& (Join-Path $here '..\..\Tools\sign-derk.ps1') -ModPath $mod | Out-Null

# mission: a copy of the vanilla Chernarus mission with CE vehicle/event logging on
if((Test-Path -LiteralPath $mission) -and !(Test-Path -LiteralPath $marker)){throw "$mission exists without the spike marker"}
if(!(Test-Path -LiteralPath $mission)){
 New-Item -ItemType Directory -Path $mission | Out-Null
 Get-ChildItem -LiteralPath (Join-Path $serverRoot 'mpmissions\dayzOffline.chernarusplus') | Where-Object Name -notlike 'storage_*' | Copy-Item -Destination $mission -Recurse
 Set-Content -LiteralPath $marker -Value 'DriveLink CE spike test mission'
 $core=Join-Path $mission 'cfgeconomycore.xml'
 $t=Get-Content -Raw -LiteralPath $core
 foreach($k in 'log_ce_dynamicevent','log_ce_vehicle','log_ce_statistics','log_ce_loop'){$t=$t.Replace("name=`"$k`" value=`"false`"","name=`"$k`" value=`"true`"")}
 Set-Content -LiteralPath $core -Value $t -Encoding UTF8
 # CE pauses while no player is online (idle mode); this server never has players
 $g=Join-Path $mission 'db\globals.xml'
 $t=Get-Content -Raw -LiteralPath $g
 $t=$t.Replace('name="IdleModeCountdown" type="0" value="60"','name="IdleModeCountdown" type="0" value="0"').Replace('name="IdleModeStartup" type="0" value="1"','name="IdleModeStartup" type="0" value="0"')
 Set-Content -LiteralPath $g -Value $t -Encoding UTF8
}
$storage=Join-Path $mission 'storage_77'
if($Fresh -and (Test-Path -LiteralPath $storage)){Remove-Item -LiteralPath $storage -Recurse -Force;Write-Host 'Fresh storage'}

# profile + phase config
$profile=Join-Path $build 'profile'
New-Item -ItemType Directory -Force -Path (Join-Path $profile 'DLCESpike') | Out-Null
$phaseFile=Join-Path $here "phases\$Phase.json"
if(!(Test-Path -LiteralPath $phaseFile)){throw "No phase config $phaseFile"}
Copy-Item -LiteralPath $phaseFile -Destination (Join-Path $profile 'DLCESpike\config.json') -Force

$cfg=Join-Path $build 'serverDZ.cfg'
@"
hostname="DriveLink CE spike";
password="";
maxPlayers=2;
BattlEye=0;
verifySignatures=2;
forceSameBuild=1;
steamQueryPort=$QueryPort;
instanceId=77;
storageAutoFix=1;
serverTime="2026/9/20/12/00";
serverTimeAcceleration=1;
serverTimePersistent=0;
class Missions { class DayZ { template="$missionName"; }; };
"@ | Set-Content -LiteralPath $cfg -Encoding ascii

if(Get-NetUDPEndpoint -LocalPort $GamePort,$QueryPort -ErrorAction SilentlyContinue){throw "Ports $GamePort/$QueryPort in use"}
$args=@("-port=$GamePort","-config=$cfg","-profiles=$profile","-serverMod=$mod",'-dologs','-adminlog','-freezecheck')
$p=Start-Process -FilePath (Join-Path $serverRoot 'DayZServer_x64.exe') -WorkingDirectory $serverRoot -ArgumentList $args -WindowStyle Hidden -PassThru
@{processId=$p.Id;startTimeUtc=$p.StartTime.ToUniversalTime().ToString('o');phase=$Phase}|ConvertTo-Json|Set-Content -LiteralPath $pidFile
Write-Host "Spike server PID $($p.Id), phase $Phase, profile $profile"
