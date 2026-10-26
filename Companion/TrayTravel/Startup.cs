using System;
using System.IO;
using Microsoft.Win32;
public static class TrayStartup {
 public static string MenuShortcutPath {get{return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Programs),"DriveLink Travel Helper.lnk");}}
 public static void ConfigureMenuShortcut(bool enabled,string installed) {
  string path=MenuShortcutPath,exe=Path.Combine(installed,"DriveLinkTrayAgent.exe");
  object shell=Activator.CreateInstance(Type.GetTypeFromProgID("WScript.Shell")),link=null;
  try {
   if(File.Exists(path)){link=Link(shell,path);if(!String.Equals(Get(link,"TargetPath"),exe,StringComparison.OrdinalIgnoreCase))throw new InvalidOperationException("Start menu shortcut belongs to another installation");System.Runtime.InteropServices.Marshal.FinalReleaseComObject(link);link=null;}
   if(enabled){
    if(!File.Exists(exe))throw new IOException("Installed tray agent is missing");Directory.CreateDirectory(Path.GetDirectoryName(path));
    link=Link(shell,path);Set(link,"TargetPath",exe);Set(link,"Arguments","");Set(link,"WorkingDirectory",installed);Set(link,"Description","Start the DriveLink tray helper for this Windows session");link.GetType().InvokeMember("Save",System.Reflection.BindingFlags.InvokeMethod,null,link,null);
   }else if(File.Exists(path))File.Delete(path);
  }finally{if(link!=null)System.Runtime.InteropServices.Marshal.FinalReleaseComObject(link);System.Runtime.InteropServices.Marshal.FinalReleaseComObject(shell);}
 }
 const string Run=@"Software\Microsoft\Windows\CurrentVersion\Run";
 public static string ShortcutPath {get{return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Startup),"DriveLink Travel Helper.lnk");}}
 static object Link(object shell,string path){return shell.GetType().InvokeMember("CreateShortcut",System.Reflection.BindingFlags.InvokeMethod,null,shell,new object[]{path});}
 static string Get(object link,string name){return (string)link.GetType().InvokeMember(name,System.Reflection.BindingFlags.GetProperty,null,link,null);}
 static void Set(object link,string name,object value){link.GetType().InvokeMember(name,System.Reflection.BindingFlags.SetProperty,null,link,new object[]{value});}
 public static void Configure(bool enabled,string installed) {
  string exe=Path.Combine(installed,"DriveLinkTrayAgent.exe"),command=("\""+exe+"\""),path=ShortcutPath;
  if(String.IsNullOrEmpty(Path.GetDirectoryName(path)))throw new InvalidOperationException("Windows Startup folder is unavailable");
  using(var k=Registry.CurrentUser.OpenSubKey(Run)){string v=k==null?null:k.GetValue("DriveLinkTravel") as string;if(v!=null&&v!=command)throw new InvalidOperationException("Another installation owns the Run entry");}
  object shell=Activator.CreateInstance(Type.GetTypeFromProgID("WScript.Shell"));object link=null;
  try {
   if(File.Exists(path)) {
    link=Link(shell,path);string target=Get(link,"TargetPath");
    if(!String.Equals(target,exe,StringComparison.OrdinalIgnoreCase))throw new InvalidOperationException("Startup shortcut belongs to another installation");
    System.Runtime.InteropServices.Marshal.FinalReleaseComObject(link);link=null;
   }
   if(enabled){
    if(!File.Exists(exe))throw new InvalidOperationException("Installed tray agent is missing");Directory.CreateDirectory(Path.GetDirectoryName(path));
    link=Link(shell,path);Set(link,"TargetPath",exe);Set(link,"Arguments","--startup");Set(link,"WorkingDirectory",installed);Set(link,"WindowStyle",7);Set(link,"Description","DriveLink disclosed background travel helper; starts at Windows sign-in");link.GetType().InvokeMember("Save",System.Reflection.BindingFlags.InvokeMethod,null,link,null);
    System.Runtime.InteropServices.Marshal.FinalReleaseComObject(link);link=Link(shell,path);
    if(!String.Equals(Get(link,"TargetPath"),exe,StringComparison.OrdinalIgnoreCase)||Get(link,"Arguments")!="--startup"||!String.Equals(Get(link,"WorkingDirectory"),installed,StringComparison.OrdinalIgnoreCase))throw new InvalidOperationException("Windows startup shortcut verification failed");
   }else if(File.Exists(path))File.Delete(path);
   using(var k=Registry.CurrentUser.OpenSubKey(Run,true)){if(k!=null&&(k.GetValue("DriveLinkTravel") as string)==command)k.DeleteValue("DriveLinkTravel",false);}
  }finally{if(link!=null)System.Runtime.InteropServices.Marshal.FinalReleaseComObject(link);System.Runtime.InteropServices.Marshal.FinalReleaseComObject(shell);}
 }
}
