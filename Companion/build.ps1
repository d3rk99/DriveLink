# Compile the Windows helper without installing it, launching DayZ or touching any running installation.
param([Parameter(Mandatory=$true)][string]$AssetsPath,[string]$Destination=(Join-Path $PSScriptRoot 'dist\Helper'))
$ErrorActionPreference='Stop'
$compiler='C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe'
if(!(Test-Path -LiteralPath $compiler)){throw '.NET Framework 4 compiler is required'}
if(!(Test-Path -LiteralPath $AssetsPath -PathType Container)){throw 'Supply the Assets directory from the distributed DriveLink Helper folder'}
if(Test-Path -LiteralPath $Destination){throw 'Output exists; choose a new empty destination to avoid replacing an installed helper'}
New-Item -ItemType Directory -Path $Destination -Force|Out-Null
Copy-Item -LiteralPath $AssetsPath -Destination (Join-Path $Destination 'Assets') -Recurse
function Compile([string[]]$CompilerArguments){
    & $compiler @CompilerArguments
    if($LASTEXITCODE-ne 0){throw 'Windows companion compilation failed'}
}
$helper=Join-Path $Destination 'DriveLinkRestartHelper.exe'
$updater=Join-Path $Destination 'DriveLinkUpdater.exe'
$uninstall=Join-Path $Destination 'DriveLinkUninstall.exe'
Compile @('/nologo','/target:winexe','/platform:x64','/optimize+',('/out:'+$helper),'/reference:System.Windows.Forms.dll','/reference:System.Drawing.dll','/reference:System.Web.Extensions.dll','/reference:System.Management.dll',(Join-Path $PSScriptRoot 'Helper\Program.cs'),(Join-Path $PSScriptRoot 'Helper\RoadFilm.cs'),(Join-Path $PSScriptRoot 'Helper\Setup.cs'))
Compile @('/nologo','/target:winexe','/platform:x64','/optimize+','/reference:System.Web.Extensions.dll',('/out:'+$updater),(Join-Path $PSScriptRoot 'TrayTravel\Update.cs'))
Compile @('/nologo','/target:winexe','/platform:x64','/reference:System.Windows.Forms.dll','/reference:System.Web.Extensions.dll',('/out:'+$uninstall),(Join-Path $PSScriptRoot 'TrayTravel\Startup.cs'),(Join-Path $PSScriptRoot 'TrayTravel\Uninstall.cs'))
$references=@('/reference:System.Web.Extensions.dll','/reference:System.Windows.Forms.dll','/reference:System.Drawing.dll',('/reference:'+$helper),('/reference:'+$uninstall),('/reference:'+$updater))
Compile (@('/nologo','/target:winexe','/platform:x64')+$references+@(('/out:'+(Join-Path $Destination 'DriveLinkTrayAgent.exe')),(Join-Path $PSScriptRoot 'TrayTravel\Agent.cs')))
Compile (@('/nologo','/target:winexe','/platform:x64')+$references+@(('/out:'+(Join-Path $Destination 'DriveLink Setup.exe')),(Join-Path $PSScriptRoot 'TrayTravel\ReleaseSetup.cs')))
& (Join-Path $PSScriptRoot 'Tools\write-helper-update-manifest.ps1') -HelperFolder $Destination
Write-Output ('Built Windows companion without installing it: '+$Destination)
