param([Parameter(Mandatory=$true)][string]$HelperFolder)
$ErrorActionPreference='Stop'
$source=Get-Content -LiteralPath (Join-Path $PSScriptRoot '..\TrayTravel\Update.cs') -Raw
$version=[regex]::Match($source,'const string Version="([^"]+)"').Groups[1].Value
$versionCode=[int][regex]::Match($source,'const int VersionCode=([0-9]+)').Groups[1].Value
if(!$version -or $versionCode -le 0){throw 'Updater version is missing'}
$root=[IO.Path]::GetFullPath($HelperFolder).TrimEnd('\')
$files=@()
foreach($name in @('DriveLinkRestartHelper.exe','DriveLinkTrayAgent.exe','DriveLinkUninstall.exe','DriveLink Setup.exe','DriveLinkUpdater.exe')){
 $file=Get-Item -LiteralPath (Join-Path $root $name);$files+=@{path=$name;size=$file.Length;sha256=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLower()}
}
foreach($file in Get-ChildItem -LiteralPath (Join-Path $root 'Assets') -Recurse -File|Sort-Object FullName){
 if($file.Extension-eq '.biprivatekey'){throw 'Private key in assets'}
 $files+=@{path=$file.FullName.Substring($root.Length+1).Replace('\','/');size=$file.Length;sha256=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLower()}
}
$manifest=@{schema=1;version=$version;versionCode=$versionCode;files=$files}|ConvertTo-Json -Depth 5
[IO.File]::WriteAllText((Join-Path $root 'helper-update.json'),$manifest,[Text.UTF8Encoding]::new($false))
Write-Output ('Helper update manifest: '+$version+' ('+$files.Count+' files)')
