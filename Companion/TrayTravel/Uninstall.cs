using System;
using System.IO;
using System.Diagnostics;
using System.Threading;
using System.Windows.Forms;
using Microsoft.Win32;

public static class TravelInstallation {
 public const string Version="0.4.8-rc3";
 public const string AppKey=@"Software\Microsoft\Windows\CurrentVersion\Uninstall\DriveLinkTravel";
 public static string Installed {get{return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"DriveLinkTravel","LocalTest");}}
 public static string UninstallCommand {get{return "\""+Path.Combine(Installed,"DriveLinkUninstall.exe")+"\"";}}
 public static Process[] Helpers() {
  var owned=new System.Collections.Generic.List<Process>();
  string expected=Path.Combine(Installed,"DriveLinkRestartHelper.exe");
  foreach(var process in Process.GetProcessesByName("DriveLinkRestartHelper")) {
   try{if(!process.HasExited&&String.Equals(process.MainModule.FileName,expected,StringComparison.OrdinalIgnoreCase)){owned.Add(process);continue;}}
   catch(InvalidOperationException){}catch(System.ComponentModel.Win32Exception){}
   process.Dispose();
  }
  return owned.ToArray();
 }
 public static bool HasHelpers(){var helpers=Helpers();try{return helpers.Length>0;}finally{foreach(var p in helpers)p.Dispose();}}
 public static string StopNameForProfile(string profile) {
  using(var sha=System.Security.Cryptography.SHA256.Create())return @"Local\DriveLinkTravelHelperStop--dlrClient="+BitConverter.ToString(sha.ComputeHash(System.Text.Encoding.UTF8.GetBytes(Path.GetFullPath(profile).ToLowerInvariant()))).Replace("-","").Substring(0,16);
 }
 public static void StopHelpers() {
  try {
   var json=new System.Web.Script.Serialization.JavaScriptSerializer();
   var config=json.Deserialize<System.Collections.Generic.Dictionary<string,object>>(File.ReadAllText(Path.Combine(Installed,"helper-config.json")));
   using(var stop=EventWaitHandle.OpenExisting(StopNameForProfile((string)config["clientProfile"])))stop.Set();
  }catch(WaitHandleCannotBeOpenedException){}catch(IOException){}catch(ArgumentException){}catch(InvalidCastException){}catch(System.Collections.Generic.KeyNotFoundException){}
  foreach(var process in Helpers())using(process) {
   if(process.WaitForExit(4000))continue;
   // A confirmed exit/uninstall may clean up only our installed helper, never DayZ or Steam.
   if(!process.HasExited&&String.Equals(process.MainModule.FileName,Path.Combine(Installed,"DriveLinkRestartHelper.exe"),StringComparison.OrdinalIgnoreCase)){process.Kill();if(!process.WaitForExit(3000))throw new IOException("Travel helper could not stop");}
  }
 }
 public static void CheckRegistration() {
  using(var key=Registry.CurrentUser.OpenSubKey(AppKey)) {
   if(key!=null && ((string)key.GetValue("InstallLocation","")!=Installed || (string)key.GetValue("UninstallString","")!=UninstallCommand))throw new InvalidOperationException("Another installation owns the uninstall entry");
  }
 }
 public static void Register() {
  CheckRegistration();
  if(!File.Exists(Path.Combine(Installed,"DriveLinkUninstall.exe")))throw new InvalidOperationException("Uninstaller is missing");
  TrayStartup.ConfigureMenuShortcut(true,Installed);
  using(var key=Registry.CurrentUser.CreateSubKey(AppKey)) {
   key.SetValue("DisplayName","DriveLink Travel Helper");key.SetValue("DisplayVersion",Version);key.SetValue("Publisher","Derk");
   key.SetValue("InstallLocation",Installed);key.SetValue("UninstallString",UninstallCommand);
   key.SetValue("DisplayIcon",Path.Combine(Installed,"DriveLinkTrayAgent.exe"));
   key.SetValue("NoModify",1,RegistryValueKind.DWord);key.SetValue("NoRepair",1,RegistryValueKind.DWord);
  }
 }
 public static void ValidateTree(string path) {
  if(!Directory.Exists(path))return;
  if((File.GetAttributes(path)&FileAttributes.ReparsePoint)!=0)throw new IOException("Uninstall refuses linked folders: "+path);
  foreach(string entry in Directory.GetFileSystemEntries(path)) {
   if((File.GetAttributes(entry)&FileAttributes.ReparsePoint)!=0)throw new IOException("Uninstall refuses linked files or folders: "+entry);
   if(Directory.Exists(entry))ValidateTree(entry);
  }
 }
 public static void Remove() {
  using(var maintenance=new Mutex(false,"Local\\DriveLinkTravelMaintenance")){
   bool owned=false;try{try{owned=maintenance.WaitOne(0);}catch(AbandonedMutexException){owned=true;}if(!owned)throw new IOException("A DriveLink update or setup is running. Please retry uninstall shortly.");RemoveCore();}finally{if(owned)maintenance.ReleaseMutex();}
  }
 }
 static void RemoveCore() {
  CheckRegistration();ValidateTree(Installed);
  StopHelpers();
  TrayStartup.Configure(false,Installed);
  TrayStartup.ConfigureMenuShortcut(false,Installed);
  try{using(var stop=EventWaitHandle.OpenExisting(@"Local\DriveLinkTrayLocalTestStop"))stop.Set();}catch(WaitHandleCannotBeOpenedException){}
  for(int i=0;i<50 && Process.GetProcessesByName("DriveLinkTrayAgent").Length>0;i++)Thread.Sleep(100);
  if(Process.GetProcessesByName("DriveLinkTrayAgent").Length>0)throw new IOException("The tray agent is still closing. Retry uninstall shortly; automatic startup has been removed.");
  // Only this fixed per-user installation is removed. Workshop and DayZ profiles are outside it.
  ValidateTree(Installed);
  if(Directory.Exists(Installed))Directory.Delete(Installed,true);
  string cache=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"DriveLinkTravel","UpdateCache");ValidateTree(cache);if(Directory.Exists(cache))Directory.Delete(cache,true);
  Registry.CurrentUser.DeleteSubKeyTree(AppKey,false);
 }
}
class TravelUninstall {
 [STAThread]static int Main() {
  Application.EnableVisualStyles();
  try {
   string exe=Process.GetCurrentProcess().MainModule.FileName;
   if(String.Equals(Path.GetDirectoryName(exe),TravelInstallation.Installed,StringComparison.OrdinalIgnoreCase)) {
    string temp=Path.Combine(Path.GetTempPath(),"DriveLinkUninstall-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(temp);
    string copy=Path.Combine(temp,"DriveLinkUninstall.exe");File.Copy(exe,copy);
    Process.Start(new ProcessStartInfo(copy){UseShellExecute=false,WorkingDirectory=temp});return 0;
   }
   if(MessageBox.Show("Uninstall DriveLink Travel Helper?\n\nThis stops the agent and travel helper, cancels any active client-switching operation, removes automatic startup and the Installed Apps entry, and deletes installed helper files. DayZ itself stays open; settings and Workshop mods stay in place. Setup is required again if you return.","Uninstall DriveLink Travel Helper",MessageBoxButtons.YesNo,MessageBoxIcon.Question)!=DialogResult.Yes)return 0;
   TravelInstallation.Remove();
   MessageBox.Show("DriveLink Travel Helper has been uninstalled. It will not start with Windows.","DriveLink Travel Helper");return 0;
  }catch(Exception e){MessageBox.Show("Uninstall could not finish: "+e.Message,"DriveLink Travel Helper",MessageBoxButtons.OK,MessageBoxIcon.Error);return 1;}
 }
}
