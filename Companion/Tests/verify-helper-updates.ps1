param([string]$HelperFolder=(Join-Path $PSScriptRoot '..\dist\Helper'))
$ErrorActionPreference='Stop'
$work=Join-Path (Join-Path $PSScriptRoot '..\Build') ('AutoUpdate-'+(Get-Date -Format yyyyMMdd-HHmmss))
New-Item -ItemType Directory -Path $work|Out-Null
$source=@'
using System;using System.IO;using System.Linq;using System.Collections.Generic;using System.Threading.Tasks;using System.Web.Script.Serialization;
class UpdateHarness {
 static int passed;static string root;static JavaScriptSerializer json=new JavaScriptSerializer();
 static void Check(bool ok,string name){if(!ok)throw new Exception(name);File.AppendAllText(Path.Combine(root,"result.txt"),"PASS "+name+Environment.NewLine);passed++;}
 static bool Reject(Action action){try{action();return false;}catch(IOException){return true;}}
 static HelperUpdateManifest Fixture(string source){
  Directory.CreateDirectory(source);var entries=new List<HelperUpdateFile>();
  foreach(string path in TravelUpdates.Executables.Concat(new[]{"Assets/test.txt"})){
   string file=TravelUpdates.Child(source,path);Directory.CreateDirectory(Path.GetDirectoryName(file));File.WriteAllText(file,"new-"+path);
   entries.Add(new HelperUpdateFile{path=path,size=new FileInfo(file).Length,sha256=TravelUpdates.Hash(file)});
  }
  var manifest=new HelperUpdateManifest{schema=1,version="0.4.8-rc4",versionCode=40804,files=entries.ToArray()};File.WriteAllText(Path.Combine(source,"helper-update.json"),json.Serialize(manifest));return manifest;
 }
 static string Target(string name){string target=Path.Combine(root,name);Directory.CreateDirectory(target);foreach(string file in TravelUpdates.Executables)File.WriteAllText(Path.Combine(target,file),"old-"+file);File.WriteAllText(Path.Combine(target,"helper-config.json"),"player-settings");File.WriteAllText(Path.Combine(target,"update-source.json"),"trusted-source");Directory.CreateDirectory(Path.Combine(target,"Networks"));File.WriteAllText(Path.Combine(target,"Networks","network.public.json"),"unchanged-network");return target;}
 static bool Originals(string target){return TravelUpdates.Executables.All(x=>File.ReadAllText(Path.Combine(target,x))=="old-"+x)&&!File.Exists(Path.Combine(target,"Assets","test.txt"));}
 static bool Settings(string target){return File.ReadAllText(Path.Combine(target,"helper-config.json"))=="player-settings"&&File.ReadAllText(Path.Combine(target,"update-source.json"))=="trusted-source"&&File.ReadAllText(Path.Combine(target,"Networks","network.public.json"))=="unchanged-network";}
 static int Main(string[] args){root=args[0];try{
  var actual=TravelUpdates.ReadManifest(Path.Combine(args[1],"helper-update.json"));TravelUpdates.Verify(args[1],actual);
  Check(actual.versionCode==TravelUpdates.VersionCode&&actual.version==TravelUpdates.Version,"Actual build manifest matches updater and hashes");
  string source=Path.Combine(root,"Source"),cache=Path.Combine(root,"Cache");var m=Fixture(source);
  Check(TravelUpdates.Stage(source,40804,cache)==null,"Same version is not reinstalled");Check(TravelUpdates.Stage(source,40805,cache)==null,"Older version cannot downgrade running helper");
  Check(TravelUpdates.Stage(Path.Combine(root,"NoSource"),40803,cache)==null,"Missing mod folder leaves current helper alone");
  foreach(string bad in new[]{"../helper-config.json","Assets/../helper-config.json","Assets/../../outside.exe","C:/outside.exe","Assets\\outside.exe","helper-config.json","Networks/network.public.json"}){
   var altered=json.Deserialize<HelperUpdateManifest>(json.Serialize(m));altered.files[5].path=bad;Check(Reject(delegate{TravelUpdates.Validate(altered);}),"Reject unapproved path "+bad);
  }
  var duplicate=json.Deserialize<HelperUpdateManifest>(json.Serialize(m));duplicate.files[5].path=m.files[0].path.ToUpperInvariant();Check(Reject(delegate{TravelUpdates.Validate(duplicate);}),"Reject case-insensitive duplicate filename");
  var incomplete=json.Deserialize<HelperUpdateManifest>(json.Serialize(m));incomplete.files=incomplete.files.Skip(1).ToArray();Check(Reject(delegate{TravelUpdates.Validate(incomplete);}),"Reject missing updater executable");
  string corrupt=Path.Combine(source,TravelUpdates.Executables[0]);File.WriteAllText(corrupt,"partial-download");Check(Reject(delegate{TravelUpdates.Stage(source,40803,cache);}),"Partial or corrupt Workshop download cannot stage");m=Fixture(source);
  var package=TravelUpdates.Stage(source,40803,cache);Check(package!=null&&File.Exists(Path.Combine(package.stage,"ApplyUpdate.exe")),"Stage immutable payload and separate updater runner");
  string target=Target("Success");bool activated=false;TravelUpdates.Apply(package.stage,target,delegate{activated=true;});TravelUpdates.Verify(target,m);Check(activated&&Settings(target),"Successful update preserves player, network and update-source settings");
  package=TravelUpdates.Stage(source,40803,cache);target=Target("Rollback");Check(Reject(delegate{TravelUpdates.Apply(package.stage,target,delegate{throw new IOException("test startup failure");});})&&Originals(target)&&Settings(target),"Failed new-agent activation rolls all files back and preserves settings");
  package=TravelUpdates.Stage(source,40803,cache);target=Target("Locked");using(var held=File.Open(Path.Combine(target,TravelUpdates.Executables[2]),FileMode.Open,FileAccess.Read,FileShare.Read)){
   Check(Reject(delegate{TravelUpdates.Apply(package.stage,target,delegate{});})&&Originals(target)&&Settings(target),"Locked executable rolls back previously replaced files");
  }
  Check(!Directory.GetFiles(target,"*.update-new",System.IO.SearchOption.AllDirectories).Any(),"Failed replacement leaves no incoming-file locks or debris");
  package=TravelUpdates.Stage(source,40803,cache);target=Target("TamperedStage");File.WriteAllText(Path.Combine(package.stage,"Payload",TravelUpdates.Executables[0]),"changed");Check(Reject(delegate{TravelUpdates.Apply(package.stage,target,delegate{});})&&Originals(target),"Changed staged payload is rejected before installation");
  using(var maintenance=TravelUpdates.LockMaintenance())try{
   bool blocked=Task.Run(delegate{return Reject(delegate{using(var second=TravelUpdates.LockMaintenance()){second.ReleaseMutex();}});}).Result;Check(blocked,"Setup/update/uninstall maintenance is serialized across threads");
  }finally{maintenance.ReleaseMutex();}
  File.AppendAllText(Path.Combine(root,"result.txt"),passed+" checks passed; live installation and DayZ untouched."+Environment.NewLine);return 0;
 }catch(Exception e){File.AppendAllText(Path.Combine(root,"result.txt"),"FAIL "+e+Environment.NewLine);return 1;}}
}
'@
$cs=Join-Path $work 'UpdateHarness.cs';[IO.File]::WriteAllText($cs,$source)
Copy-Item -LiteralPath (Join-Path $HelperFolder 'DriveLinkUpdater.exe') -Destination $work
$exe=Join-Path $work 'UpdateHarness.exe'
& C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe /nologo /target:exe /platform:x64 /reference:System.Web.Extensions.dll ('/reference:'+(Join-Path $work 'DriveLinkUpdater.exe')) ('/out:'+$exe) $cs
if($LASTEXITCODE-ne 0){throw 'Updater harness compile failed'}
& $exe $work $HelperFolder
Get-Content -LiteralPath (Join-Path $work 'result.txt')
if($LASTEXITCODE-ne 0){throw 'Updater harness failed'}
