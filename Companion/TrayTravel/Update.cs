using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading;
using System.Security.Cryptography;
using System.Web.Script.Serialization;
using Microsoft.Win32;

public class HelperUpdateFile {public string path {get;set;} public string sha256 {get;set;} public long size {get;set;}}
public class HelperUpdateManifest {public int schema {get;set;} public string version {get;set;} public int versionCode {get;set;} public HelperUpdateFile[] files {get;set;}}
public class HelperUpdateSource {public string helperFolder {get;set;}}
public class HelperUpdatePackage {public string stage;public HelperUpdateManifest manifest;}
public static class TravelUpdates {
 public const string Version="0.4.8-rc3";
 public const int VersionCode=40803;
 public const string Maintenance="Local\\DriveLinkTravelMaintenance";
 public static readonly string[] Executables={"DriveLinkRestartHelper.exe","DriveLinkTrayAgent.exe","DriveLinkUninstall.exe","DriveLink Setup.exe","DriveLinkUpdater.exe"};
 public static string Installed {get{return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"DriveLinkTravel","LocalTest");}}
 public static string Cache {get{return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"DriveLinkTravel","UpdateCache");}}
 public static Mutex LockMaintenance(){var mutex=new Mutex(false,Maintenance);bool owned=false;try{try{owned=mutex.WaitOne(0);}catch(AbandonedMutexException){owned=true;}if(!owned)throw new IOException("DriveLink setup, update or uninstall is already running. Please try again shortly.");return mutex;}catch{mutex.Dispose();throw;}}
 public static void Log(string message){try{File.AppendAllText(Path.Combine(Installed,"updates.log"),DateTime.UtcNow.ToString("o")+" "+message+Environment.NewLine);}catch(IOException){}catch(UnauthorizedAccessException){}}
 public static string Hash(string file){using(var stream=File.OpenRead(file))using(var sha=SHA256.Create())return BitConverter.ToString(sha.ComputeHash(stream)).Replace("-","").ToLowerInvariant();}
 public static bool SamePath(string a,string b){return String.Equals(Path.GetFullPath(a).TrimEnd('\\'),Path.GetFullPath(b).TrimEnd('\\'),StringComparison.OrdinalIgnoreCase);}
 public static void PlainPath(string file){string current=Path.GetFullPath(file);while(!String.IsNullOrEmpty(current)){if((File.Exists(current)||Directory.Exists(current))&&(File.GetAttributes(current)&FileAttributes.ReparsePoint)!=0)throw new IOException("Linked update paths are not supported: "+current);current=Path.GetDirectoryName(current);}}
 public static string Child(string root,string relative){
  if(String.IsNullOrWhiteSpace(relative)||relative.Contains("\\")||relative.Contains(":")||relative.Split('/').Any(x=>x==""||x=="."||x=="..")||Path.IsPathRooted(relative))throw new IOException("Invalid update file path");
  string result=Path.GetFullPath(Path.Combine(root,relative.Replace('/',Path.DirectorySeparatorChar)));
  if(!result.StartsWith(Path.GetFullPath(root).TrimEnd('\\')+"\\",StringComparison.OrdinalIgnoreCase))throw new IOException("Update path escapes package");
  PlainPath(result);return result;
 }
 public static HelperUpdateManifest ReadManifest(string file){
  if(!File.Exists(file)||new FileInfo(file).Length>1048576)throw new IOException("Helper update manifest is absent or too large");
  var m=new JavaScriptSerializer().Deserialize<HelperUpdateManifest>(File.ReadAllText(file));Validate(m);return m;
 }
 public static void Validate(HelperUpdateManifest m){
  if(m==null||m.schema!=1||m.versionCode<=0||String.IsNullOrEmpty(m.version)||!System.Text.RegularExpressions.Regex.IsMatch(m.version,"^[0-9]+\\.[0-9]+\\.[0-9]+(?:-rc[0-9]+)?$")||m.files==null||m.files.Length<5||m.files.Length>4096)throw new IOException("Invalid helper update manifest");
  var names=new HashSet<string>(StringComparer.OrdinalIgnoreCase);long total=0;
  foreach(var f in m.files){
   if(f==null||String.IsNullOrEmpty(f.path))throw new IOException("Invalid helper update entry");
   Child(Path.GetTempPath(),f.path);
   if(!(Executables.Contains(f.path,StringComparer.OrdinalIgnoreCase)||f.path.StartsWith("Assets/",StringComparison.Ordinal))||!names.Add(f.path)||f.size<0||f.size>268435456||f.sha256==null||!System.Text.RegularExpressions.Regex.IsMatch(f.sha256,"^[0-9a-fA-F]{64}$"))throw new IOException("Unapproved or invalid update file: "+f.path);
   total+=f.size;if(total>268435456)throw new IOException("Helper update package is too large");
  }
  if(Executables.Any(x=>!names.Contains(x)))throw new IOException("Helper update package is incomplete");
 }
 public static void Verify(string root,HelperUpdateManifest m){Validate(m);foreach(var f in m.files){string path=Child(root,f.path);if(!File.Exists(path)||new FileInfo(path).Length!=f.size||!String.Equals(Hash(path),f.sha256,StringComparison.OrdinalIgnoreCase))throw new IOException("Helper update file is missing, changed or still downloading: "+f.path);}}
 public static void RememberSource(string installed,string helper){
  PlainPath(helper);string mod=Path.GetDirectoryName(Path.GetFullPath(helper).TrimEnd('\\'));
  if(!File.Exists(Path.Combine(mod,"Addons","DriveLinkRestart.pbo")))throw new IOException("Update source must be the DriveLink mod's Helper folder");
  ReadManifest(Path.Combine(helper,"helper-update.json"));
  File.WriteAllText(Path.Combine(installed,"update-source.json"),new JavaScriptSerializer().Serialize(new HelperUpdateSource{helperFolder=Path.GetFullPath(helper)}));
 }
 public static string Source(string installed){
  string file=Path.Combine(installed,"update-source.json");if(!File.Exists(file))return null;
  if(new FileInfo(file).Length>16384)throw new IOException("Invalid installed update source");
  var source=new JavaScriptSerializer().Deserialize<HelperUpdateSource>(File.ReadAllText(file));
  if(source==null||String.IsNullOrWhiteSpace(source.helperFolder))throw new IOException("Invalid installed update source");
  PlainPath(source.helperFolder);return Path.GetFullPath(source.helperFolder);
 }
 public static HelperUpdatePackage Stage(string source,int current,string cache){
  if(String.IsNullOrEmpty(source))return null;
  PlainPath(source);string manifest=Path.Combine(source,"helper-update.json");if(!File.Exists(manifest))return null;
  string before=Hash(manifest);var m=ReadManifest(manifest);if(m.versionCode<=current)return null;
  Verify(source,m);PlainPath(cache);Directory.CreateDirectory(cache);
  string stage=Path.Combine(cache,Guid.NewGuid().ToString("N"));Directory.CreateDirectory(stage);
  try{
   string payload=Path.Combine(stage,"Payload");Directory.CreateDirectory(payload);
   foreach(var f in m.files){string target=Child(payload,f.path);Directory.CreateDirectory(Path.GetDirectoryName(target));File.Copy(Child(source,f.path),target);}
   Verify(payload,m);if(before!=Hash(manifest))throw new IOException("Workshop helper update changed while being copied; retry later");
   File.WriteAllText(Path.Combine(stage,"manifest.json"),new JavaScriptSerializer().Serialize(m));
   File.Copy(Path.Combine(payload,"DriveLinkUpdater.exe"),Path.Combine(stage,"ApplyUpdate.exe"));
   return new HelperUpdatePackage{stage=stage,manifest=m};
  }catch{if(Path.GetFullPath(stage).StartsWith(Path.GetFullPath(cache).TrimEnd('\\')+"\\",StringComparison.OrdinalIgnoreCase)){PlainPath(stage);Directory.Delete(stage,true);}throw;}
 }
 public static void CheckStagePath(string stage){
  if(!SamePath(Path.GetDirectoryName(Path.GetFullPath(stage)),Cache)||!System.Text.RegularExpressions.Regex.IsMatch(Path.GetFileName(stage),"^[0-9a-f]{32}$"))throw new IOException("Invalid updater staging directory");PlainPath(stage);
 }
 public static bool GameBusy(){return HasProcess("DayZ_x64")||HasProcess("DayZLauncher")||HasProcess("DayZ_BE")||HasProcess("DriveLinkRestartHelper");}
 static bool HasProcess(string name){var processes=Process.GetProcessesByName(name);try{return processes.Length>0;}finally{foreach(var p in processes)p.Dispose();}}
 // Per-file atomic replacement, with a rollback copy and a durable journal before every mutation.
 public static void Apply(string stage,string installed,Action activate){
  var m=ReadManifest(Path.Combine(stage,"manifest.json"));string payload=Path.Combine(stage,"Payload");Verify(payload,m);PlainPath(installed);
  string backup=Path.Combine(stage,"Backup");Directory.CreateDirectory(backup);var changed=new List<string>();var existed=new HashSet<string>(StringComparer.OrdinalIgnoreCase);var temporary=new List<string>();
  try{
   foreach(var f in m.files){
    string target=Child(installed,f.path),old=Child(backup,f.path);Directory.CreateDirectory(Path.GetDirectoryName(target));Directory.CreateDirectory(Path.GetDirectoryName(old));
    string incoming=target+".update-new";if(File.Exists(incoming))throw new IOException("Previous update temporary file is present");
    temporary.Add(incoming);File.Copy(Child(payload,f.path),incoming);bool had=File.Exists(target);if(had)existed.Add(f.path);
    changed.Add(f.path);File.WriteAllText(Path.Combine(stage,"journal.json"),new JavaScriptSerializer().Serialize(new{files=changed,existed=existed.ToArray()}));
    if(had)File.Replace(incoming,target,old);else File.Move(incoming,target);
   }
   Verify(installed,m);activate();
   try{File.WriteAllText(Path.Combine(stage,"complete.txt"),m.version);}catch(IOException){}catch(UnauthorizedAccessException){}
  }catch{
   foreach(string relative in changed.AsEnumerable().Reverse()){
    string target=Child(installed,relative),old=Child(backup,relative);
    if(File.Exists(old)){File.Copy(old,target,true);}else if(!existed.Contains(relative)&&File.Exists(target))File.Delete(target);
    if(File.Exists(target+".update-new"))File.Delete(target+".update-new");
   }
   File.WriteAllText(Path.Combine(stage,"rolled-back.txt"),DateTime.UtcNow.ToString("o"));throw;
  }finally{
   foreach(string file in temporary)if(File.Exists(file))File.Delete(file);
  }
 }
 public static void Acknowledge(string stage){CheckStagePath(stage);File.WriteAllText(Path.Combine(stage,"agent-ready.txt"),VersionCode.ToString());}
}
class TravelUpdater {
 static Process StartAgent(string stage){return Process.Start(new ProcessStartInfo(Path.Combine(TravelUpdates.Installed,"DriveLinkTrayAgent.exe"),"--startup"+(stage==null?"":" --update-stage \""+stage+"\"")){UseShellExecute=false,CreateNoWindow=true,WorkingDirectory=TravelUpdates.Installed});}
 static int Main(string[] args){
  if(args.Length!=2)return 2;bool parentExited=false;Process next=null;Mutex maintenance=null;
  try{
   string stage=Path.GetFullPath(args[0]);TravelUpdates.CheckStagePath(stage);
   if(!TravelUpdates.SamePath(Process.GetCurrentProcess().MainModule.FileName,Path.Combine(stage,"ApplyUpdate.exe")))throw new IOException("Updater must run from its staged copy");
   int parentId;if(!Int32.TryParse(args[1],out parentId))throw new IOException("Invalid updater parent");
   using(var parent=Process.GetProcessById(parentId)){
    if(!TravelUpdates.SamePath(parent.MainModule.FileName,Path.Combine(TravelUpdates.Installed,"DriveLinkTrayAgent.exe")))throw new IOException("Updater parent is not the installed agent");
    File.WriteAllText(Path.Combine(stage,"updater-ready.txt"),"ready");
    if(!parent.WaitForExit(15000))throw new IOException("Agent has not finished closing");parentExited=true;
   }
   maintenance=TravelUpdates.LockMaintenance();
   {
    if(TravelUpdates.GameBusy())throw new IOException("DayZ or travel started; update deferred");
    TravelUpdates.Apply(stage,TravelUpdates.Installed,delegate{
     try{
      next=StartAgent(stage);var wait=Stopwatch.StartNew();string ready=Path.Combine(stage,"agent-ready.txt");
      while(wait.Elapsed.TotalSeconds<10){if(next.HasExited)throw new IOException("Updated agent could not start");if(File.Exists(ready)&&File.ReadAllText(ready)==TravelUpdates.ReadManifest(Path.Combine(stage,"manifest.json")).versionCode.ToString())return;Thread.Sleep(100);}
      throw new IOException("Updated agent did not acknowledge startup");
     }catch{
      // Release executable locks before Apply rolls back; stop only our newly started agent.
      if(next!=null&&!next.HasExited){next.Kill();if(!next.WaitForExit(3000))throw new IOException("Updated agent has not closed for rollback");}throw;
     }
    });
    try{File.Copy(Path.Combine(stage,"manifest.json"),Path.Combine(TravelUpdates.Installed,"helper-update.json"),true);using(var key=Registry.CurrentUser.OpenSubKey(@"Software\Microsoft\Windows\CurrentVersion\Uninstall\DriveLinkTravel",true)){if(key!=null&&TravelUpdates.SamePath(Convert.ToString(key.GetValue("InstallLocation")),TravelUpdates.Installed))key.SetValue("DisplayVersion",TravelUpdates.ReadManifest(Path.Combine(stage,"manifest.json")).version);}}catch(Exception metadata){TravelUpdates.Log("UPDATE_REGISTRATION_WARNING "+metadata.Message);}
    TravelUpdates.Log("UPDATE_COMPLETE "+TravelUpdates.ReadManifest(Path.Combine(stage,"manifest.json")).version);return 0;
   }
  }catch(Exception error){TravelUpdates.Log("UPDATE_FAILED "+error.Message);if(parentExited&&maintenance!=null){try{if(next!=null&&!next.HasExited){next.Kill();next.WaitForExit(3000);}StartAgent(null);}catch(Exception restart){TravelUpdates.Log("RESTART_FAILED "+restart.Message);}}return 1;}
  finally{if(maintenance!=null){maintenance.ReleaseMutex();maintenance.Dispose();}}
 }
}
