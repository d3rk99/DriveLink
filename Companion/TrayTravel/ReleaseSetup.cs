using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Drawing;
using System.Diagnostics;
using System.Threading;
using System.Windows.Forms;
using System.Web.Script.Serialization;
using Microsoft.Win32;
using DriveLinkRestart;
public static class ReleaseSetupResolver {
 public static string NameFromLauncherFile(string file) {
  try {
   var settings=new System.Xml.XmlReaderSettings{DtdProcessing=System.Xml.DtdProcessing.Prohibit,XmlResolver=null};
   var xml=new System.Xml.XmlDocument{XmlResolver=null};
   using(var reader=System.Xml.XmlReader.Create(file,settings))xml.Load(reader);
   var enabled=xml.SelectSingleNode("//setting[@name='Name']/value");
   var value=xml.SelectSingleNode("//setting[@name='NameValue']/value");
   if(enabled!=null&&String.Equals(enabled.InnerText.Trim(),"true",StringComparison.OrdinalIgnoreCase)&&value!=null) {
    string name=value.InnerText.Trim();if(name.Length>0&&name.Length<=128){Protocol.Quote(name);return name;}
   }
  }catch(IOException){}catch(UnauthorizedAccessException){}catch(System.Xml.XmlException){}catch(InvalidOperationException){}
  return "";
 }
 public static string DefaultPlayerName() {
  var files=new List<string>();
  foreach(string root in new[]{"Bohemia_Interactive_a.s","Bohemia_Interactive"}) {
   string folder=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),root);
   try{if(Directory.Exists(folder))foreach(string app in Directory.GetDirectories(folder,"DayZLauncher.exe_*"))files.AddRange(Directory.GetFiles(app,"user.config",SearchOption.AllDirectories));}catch(IOException){}catch(UnauthorizedAccessException){}
  }
  string latest=files.OrderByDescending(File.GetLastWriteTimeUtc).FirstOrDefault();
  string name=latest==null?"":NameFromLauncherFile(latest);
  return String.IsNullOrEmpty(name)?"Survivor (change me)":name;
 }
 public static string[] Discover() {
  string steam="";using(var key=Registry.CurrentUser.OpenSubKey(@"Software\Valve\Steam")){if(key!=null)steam=key.GetValue("SteamPath","") as string;}
  steam=(steam??"").Replace('/',Path.DirectorySeparatorChar);var libs=new List<string>();if(steam!="")libs.Add(steam);
  string vdf=Path.Combine(steam,@"steamapps\libraryfolders.vdf");if(File.Exists(vdf))foreach(Match m in Regex.Matches(File.ReadAllText(vdf),"\"path\"\\s*\"([^\"]+)\""))libs.Add(m.Groups[1].Value.Replace(@"\\",@"\"));
  string game=libs.Select(l=>Path.Combine(l,@"steamapps\common\DayZ")).FirstOrDefault(p=>File.Exists(Path.Combine(p,"DayZ_BE.exe")))??"";
  string workshop=libs.Select(l=>Path.Combine(l,@"steamapps\workshop\content\221100")).FirstOrDefault(Directory.Exists)??"";
  return new[]{game,steam==""?"":Path.Combine(steam,"steam.exe"),workshop};
 }
 public static Config Resolve(string helper,string manifest,string game,string steam,string workshop,string player) {
  var network=new JavaScriptSerializer().Deserialize<PilotManifest>(File.ReadAllText(manifest));
  string profile=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"DayZ");Directory.CreateDirectory(profile);
  string mod=Path.GetDirectoryName(helper.TrimEnd('\\'));
  if(!File.Exists(Path.Combine(mod,"Addons","DriveLinkRestart.pbo"))) {
   string saved=Path.Combine(helper,"helper-config.json");
   if(File.Exists(saved)) {
    var previous=new JavaScriptSerializer().Deserialize<Config>(File.ReadAllText(saved));
    mod=previous.destinations.SelectMany(d=>d.mods).FirstOrDefault(p=>File.Exists(Path.Combine(p,"Addons","DriveLinkRestart.pbo")))??mod;
   }
  }
  var local=new Dictionary<string,string>{{"DriveLinkRestart",mod}};
  return PilotSetup.Resolve(network,game,steam,workshop,profile,player,true,local);
 }
}
class ReleaseSetup {
 static string Installed=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"DriveLinkTravel","LocalTest");
 const string Startup=@"Software\Microsoft\Windows\CurrentVersion\Run";
 static string Command {get{return Protocol.Quote(Path.Combine(Installed,"DriveLinkTrayAgent.exe"));}}
 static void Stop(){try{using(var e=EventWaitHandle.OpenExisting(@"Local\DriveLinkTrayLocalTestStop"))e.Set();Thread.Sleep(1500);}catch(WaitHandleCannotBeOpenedException){}}
 static void CopyTree(string a,string b){Directory.CreateDirectory(b);foreach(string f in Directory.GetFiles(a))File.Copy(f,Path.Combine(b,Path.GetFileName(f)),true);foreach(string d in Directory.GetDirectories(a))CopyTree(d,Path.Combine(b,Path.GetFileName(d)));}
 [STAThread]static void Main(){
  Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);string root=AppDomain.CurrentDomain.BaseDirectory;
  var form=new Form{Text="DriveLink Travel RC1 — one-time setup",Size=new Size(780,635),StartPosition=FormStartPosition.CenterScreen};
  form.Controls.Add(new Label{Location=new Point(20,15),Size=new Size(720,150),Text="Install once, then launch DayZ normally. This background tray application starts at Windows sign-in and waits for DriveLink travel. It closes DayZ and its launcher normally and restarts with the destination mods, keeping normal settings. It also checks this downloaded mod's Helper folder every five minutes for helper updates, installing them automatically after DayZ closes. Steam handles Workshop downloads.\n\nNo browser extension, service, scheduled task or per-session shortcut is required. Required Workshop mods must already be downloaded. Cancel travel setup to stay on the source server; your character remains in the live world."});
  var detected=ReleaseSetupResolver.Discover();var fields=new TextBox[4];string[] names={"DayZ folder","Steam executable","Workshop 221100","Player name"};
  for(int i=0;i<4;i++){int n=i;form.Controls.Add(new Label{Text=names[i],Location=new Point(20,180+i*36),Size=new Size(135,24)});fields[i]=new TextBox{Text=i<3?detected[i]:ReleaseSetupResolver.DefaultPlayerName(),Location=new Point(155,177+i*36),Size=new Size(495,25)};form.Controls.Add(fields[i]);if(i<3){var browse=new Button{Text="Browse",Location=new Point(660,176+i*36),Size=new Size(80,27)};browse.Click+=delegate{if(n==1){using(var d=new OpenFileDialog{Filter="Steam|steam.exe"})if(d.ShowDialog()==DialogResult.OK)fields[n].Text=d.FileName;}else using(var d=new FolderBrowserDialog())if(d.ShowDialog()==DialogResult.OK)fields[n].Text=d.SelectedPath;};form.Controls.Add(browse);}}
  form.Controls.Add(new Label{Text="Test network",Location=new Point(20,325),Size=new Size(135,24)});
  var network=new ComboBox{DropDownStyle=ComboBoxStyle.DropDownList,Location=new Point(155,322),Size=new Size(585,28)};network.Items.AddRange(new object[]{"Local — these servers run on this PC","Public — joining the owner's test servers"});network.SelectedIndex=1;form.Controls.Add(network);
  var summary=new Label{Location=new Point(20,365),Size=new Size(720,65)};Action refresh=delegate{try{string file=Path.Combine(root,"Networks",network.SelectedIndex==0?"network.local.json":"network.public.json");var m=new JavaScriptSerializer().Deserialize<PilotManifest>(File.ReadAllText(file));summary.Text=String.Join("\n",m.destinations.Select(d=>d.name+" — "+d.host+":"+d.port+" ("+String.Join(", ",d.modRefs)+")").ToArray());}catch(Exception e){summary.Text=e.Message;}};network.SelectedIndexChanged+=delegate{refresh();};refresh();form.Controls.Add(summary);
  var startup=new CheckBox{Text="Start the DriveLink tray helper when I sign in to Windows",Checked=true,Location=new Point(20,440),Size=new Size(720,26)};
  var consent=new CheckBox{Text="I trust the displayed network and agree to install this background application",Location=new Point(20,475),Size=new Size(720,26)};
  var install=new Button{Text="Install and start",Location=new Point(20,520),Size=new Size(180,40)};
  var remove=new Button{Text="Uninstall helper...",Location=new Point(220,520),Size=new Size(250,40)};
  install.Click+=delegate{try{
   if(!consent.Checked)throw new InvalidOperationException("Please acknowledge the background application and network disclosure");
   if(TravelInstallation.HasHelpers()) {
    if(MessageBox.Show("Stop the existing travel helper and continue setup? This cancels any current client-switching operation. DayZ itself will stay open.",form.Text,MessageBoxButtons.YesNo,MessageBoxIcon.Question)!=DialogResult.Yes)return;
    TravelInstallation.StopHelpers();
   }
   string file=Path.Combine(root,"Networks",network.SelectedIndex==0?"network.local.json":"network.public.json");
   Config c=ReleaseSetupResolver.Resolve(root,file,fields[0].Text.Trim(),fields[1].Text.Trim(),fields[2].Text.Trim(),fields[3].Text.Trim());
   using(var maintenance=TravelUpdates.LockMaintenance())try{
   using(var k=Registry.CurrentUser.OpenSubKey(Startup)){var v=k==null?null:k.GetValue("DriveLinkTravel") as string;if(v!=null&&v!=Command)throw new InvalidOperationException("Another installation owns the startup entry");}
   TravelInstallation.CheckRegistration();Stop();if(Process.GetProcessesByName("DriveLinkTrayAgent").Length>0)throw new InvalidOperationException("The tray agent is still closing; try again shortly");Directory.CreateDirectory(Installed);
   if(!String.Equals(root.TrimEnd('\\'),Installed.TrimEnd('\\'),StringComparison.OrdinalIgnoreCase)){foreach(string name in TravelUpdates.Executables)File.Copy(Path.Combine(root,name),Path.Combine(Installed,name),true);CopyTree(Path.Combine(root,"Assets"),Path.Combine(Installed,"Assets"));CopyTree(Path.Combine(root,"Networks"),Path.Combine(Installed,"Networks"));File.Copy(Path.Combine(root,"helper-update.json"),Path.Combine(Installed,"helper-update.json"),true);TravelUpdates.RememberSource(Installed,root);}
   File.WriteAllText(Path.Combine(Installed,"helper-config.json"),new JavaScriptSerializer().Serialize(c));
   TrayStartup.Configure(startup.Checked,Installed);
   TravelInstallation.Register();
   Process.Start(new ProcessStartInfo(Path.Combine(Installed,"DriveLinkTrayAgent.exe")){UseShellExecute=false,CreateNoWindow=true});MessageBox.Show("Installed. Launch DayZ normally with the RC1 mod enabled. No special shortcut is needed.",form.Text);form.Close();
   }finally{maintenance.ReleaseMutex();}
  }catch(Exception e){MessageBox.Show(e.Message,form.Text,MessageBoxButtons.OK,MessageBoxIcon.Error);}};
  remove.Click+=delegate{try{Process.Start(new ProcessStartInfo(Path.Combine(Installed,"DriveLinkUninstall.exe")){UseShellExecute=false});form.Close();}catch(Exception e){MessageBox.Show(e.Message);}};
  form.Controls.AddRange(new Control[]{startup,consent,install,remove});Application.Run(form);
 }
}
