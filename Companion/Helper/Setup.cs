using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text.RegularExpressions;
using System.Web.Script.Serialization;
using System.Windows.Forms;

namespace DriveLinkRestart {
public class PilotManifest {
    public int schema {get;set;}
    public string name {get;set;}
    public Destination[] destinations {get;set;}
    public Dictionary<string,string> modNames {get;set;}
}
public static class PilotSetup {
    public static Config Resolve(PilotManifest manifest,string game,string steam,string workshop,string profile,string player,bool defaults,Dictionary<string,string> local) {
        if(manifest==null||manifest.schema!=1||manifest.destinations==null)throw new InvalidOperationException("Unsupported network manifest");
        var json=new JavaScriptSerializer();
        // Do not mutate the reviewed input manifest or reuse any machine paths from it.
        var destinations=json.Deserialize<Destination[]>(json.Serialize(manifest.destinations));
        foreach(var d in destinations) {
            if(d==null||String.IsNullOrEmpty(d.id)||!Regex.IsMatch(d.id,"^[a-zA-Z0-9_-]+$")||String.IsNullOrEmpty(d.host)||d.host.StartsWith("REPLACE-"))
                throw new InvalidOperationException("Replace example addresses and supply valid destination IDs");
            if(d.modRefs==null||d.modRefs.Length==0)throw new InvalidOperationException("Each server needs an ordered modRefs list");
            var paths=new List<string>();var ids=new List<string>();var names=new List<string>();
            foreach(string reference in d.modRefs) {
                if(reference!=null&&Regex.IsMatch(reference,"^workshop:[0-9]+$")) {
                    string id=reference.Substring(9);ids.Add(id);paths.Add(Path.Combine(workshop,id));
                } else if(reference!=null&&Regex.IsMatch(reference,"^local:[a-zA-Z0-9_-]+$")&&local.ContainsKey(reference.Substring(6))) {
                    paths.Add(local[reference.Substring(6)]);
                } else throw new InvalidOperationException("Unresolved mod reference: "+reference);
            }
            d.mods=paths.ToArray();d.workshopIds=ids.ToArray();
            for(int i=0;i<d.modRefs.Length;i++){string name;names.Add(manifest.modNames!=null&&manifest.modNames.TryGetValue(d.modRefs[i],out name)?name:Protocol.ModName(d.modRefs[i],d.mods[i]));}
            d.modNames=names.ToArray();
        }
        var config=new Config {gameRoot=game,steamExe=steam,clientProfile=profile,clientName=player,useDefaultProfile=defaults,allowRemote=true,launchMode="steam-early",destinations=destinations,launchOptions=new string[0]};
        Protocol.Preflight(config);return config;
    }
    public static void Shortcut(string file,string config,string destination) {
        string exe=Assembly.GetExecutingAssembly().Location;
        object shell=Activator.CreateInstance(Type.GetTypeFromProgID("WScript.Shell"));
        object link=shell.GetType().InvokeMember("CreateShortcut",BindingFlags.InvokeMethod,null,shell,new object[]{file});
        Type type=link.GetType();
        if(File.Exists(file)) {
            string previous=(string)type.InvokeMember("TargetPath",BindingFlags.GetProperty,null,link,null);
            if(!String.Equals(previous,exe,StringComparison.OrdinalIgnoreCase))throw new InvalidOperationException("Shortcut belongs to another program: "+file);
        }
        Action<string,object> set=(key,value)=>type.InvokeMember(key,BindingFlags.SetProperty,null,link,new object[]{value});
        set("TargetPath",exe);set("Arguments","--watch --config "+Protocol.Quote(config)+(destination==null?" --vanilla-launcher":" --launch "+destination));
        set("WorkingDirectory",Path.GetDirectoryName(exe));set("Description","DriveLink: DayZ and the disclosed travel companion");
        type.InvokeMember("Save",BindingFlags.InvokeMethod,null,link,null);
        System.Runtime.InteropServices.Marshal.FinalReleaseComObject(link);
        System.Runtime.InteropServices.Marshal.FinalReleaseComObject(shell);
    }
}
public class SetupForm : Form {
    TextBox manifest=new TextBox(),game=new TextBox(),steam=new TextBox(),workshop=new TextBox(),profile=new TextBox(),player=new TextBox();
    CheckBox defaults=new CheckBox(),consent=new CheckBox();
    DataGridView paths=new DataGridView();
    PilotManifest network;
    public SetupForm() {
        Text="DriveLink companion setup";Size=new Size(880,760);MinimumSize=Size;StartPosition=FormStartPosition.CenterScreen;
        var layout=new TableLayoutPanel {Dock=DockStyle.Fill,ColumnCount=3,RowCount=12,Padding=new Padding(14),AutoScroll=true};
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,150));layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));layout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute,90));Controls.Add(layout);
        var intro=new Label {Text="One-time setup. This companion runs alongside DayZ, displays travel, and restarts DayZ with your destination mods. No service or Windows startup entry is installed. Required mods must already be downloaded.",AutoSize=true,MaximumSize=new Size(810,0)};
        layout.Controls.Add(intro,0,0);layout.SetColumnSpan(intro,3);
        AddRow(layout,1,"Network manifest",manifest,()=>{using(var dialog=new OpenFileDialog {Filter="Network JSON|*.json"})if(dialog.ShowDialog()==DialogResult.OK){manifest.Text=dialog.FileName;LoadManifest();}});
        AddRow(layout,2,"DayZ directory",game,()=>Folder(game));AddRow(layout,3,"Steam executable",steam,()=>{using(var dialog=new OpenFileDialog {Filter="Steam|steam.exe"})if(dialog.ShowDialog()==DialogResult.OK)steam.Text=dialog.FileName;});
        AddRow(layout,4,"Workshop 221100",workshop,()=>Folder(workshop));
        defaults.Text="Use my normal DayZ settings (recommended)";defaults.Checked=true;defaults.AutoSize=true;layout.Controls.Add(defaults,1,5);layout.SetColumnSpan(defaults,2);
        AddRow(layout,6,"Profile / IPC directory",profile,()=>Folder(profile));profile.Text=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"DayZ");profile.ReadOnly=true;
        defaults.CheckedChanged+=delegate {profile.ReadOnly=defaults.Checked;if(defaults.Checked)profile.Text=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"DayZ");};
        AddRow(layout,7,"Player name",player,null);player.Text=Environment.UserName;
        var explanation=new Label {Text="Local mod folders below: select a row, then Browse. DriveLinkRestart is detected from this executable. Confirm every server and ordered mod list in your administrator's JSON before installing.",AutoSize=true,MaximumSize=new Size(810,0)};
        layout.Controls.Add(explanation,0,8);layout.SetColumnSpan(explanation,3);
        paths.Dock=DockStyle.Fill;paths.Height=145;paths.AllowUserToAddRows=false;paths.AllowUserToDeleteRows=false;paths.RowHeadersVisible=false;paths.SelectionMode=DataGridViewSelectionMode.FullRowSelect;paths.MultiSelect=false;
        paths.Columns.Add(new DataGridViewTextBoxColumn {Name="Reference",ReadOnly=true,Width=160});paths.Columns.Add(new DataGridViewTextBoxColumn {Name="Folder",AutoSizeMode=DataGridViewAutoSizeColumnMode.Fill});
        layout.Controls.Add(paths,0,9);layout.SetColumnSpan(paths,2);
        var browse=new Button {Text="Browse",AutoSize=true};browse.Click+=delegate {if(paths.CurrentRow!=null)using(var dialog=new FolderBrowserDialog())if(dialog.ShowDialog()==DialogResult.OK)paths.CurrentRow.Cells[1].Value=dialog.SelectedPath;};layout.Controls.Add(browse,2,9);
        consent.Text="I trust this network manifest and understand that a background companion will run with DayZ.";consent.AutoSize=true;layout.Controls.Add(consent,0,10);layout.SetColumnSpan(consent,3);
        var install=new Button {Text="Check files and create shortcuts",AutoSize=true};install.Click+=Install;layout.Controls.Add(install,1,11);
        DetectSteam();
    }
    void AddRow(TableLayoutPanel layout,int row,string label,TextBox input,Action browse) {
        layout.Controls.Add(new Label {Text=label,AutoSize=true,Anchor=AnchorStyles.Left},0,row);input.Dock=DockStyle.Fill;layout.Controls.Add(input,1,row);
        if(browse!=null){var button=new Button {Text="Browse",AutoSize=true};button.Click+=delegate {browse();};layout.Controls.Add(button,2,row);}
    }
    void Folder(TextBox field) {using(var dialog=new FolderBrowserDialog())if(dialog.ShowDialog()==DialogResult.OK)field.Text=dialog.SelectedPath;}
    void DetectSteam() {
        using(var key=Microsoft.Win32.Registry.CurrentUser.OpenSubKey(@"Software\Valve\Steam")) {
            string root=key==null?null:key.GetValue("SteamPath") as string;if(String.IsNullOrEmpty(root))return;
            root=root.Replace('/',Path.DirectorySeparatorChar);steam.Text=Path.Combine(root,"steam.exe");var libraries=new List<string>{root};
            string vdf=Path.Combine(root,@"steamapps\libraryfolders.vdf");
            if(File.Exists(vdf))foreach(Match match in Regex.Matches(File.ReadAllText(vdf),"\"path\"\\s*\"([^\"]+)\""))libraries.Add(match.Groups[1].Value.Replace(@"\\",@"\"));
            foreach(string library in libraries)if(File.Exists(Path.Combine(library,@"steamapps\common\DayZ\DayZ_BE.exe"))) {
                game.Text=Path.Combine(library,@"steamapps\common\DayZ");workshop.Text=Path.Combine(library,@"steamapps\workshop\content\221100");break;
            }
        }
    }
    void LoadManifest() {
        try {
            network=new JavaScriptSerializer().Deserialize<PilotManifest>(File.ReadAllText(manifest.Text));
            if(network==null||network.schema!=1||network.destinations==null)throw new InvalidOperationException("Unsupported network manifest");
            paths.Rows.Clear();
            foreach(string reference in network.destinations.SelectMany(d=>d.modRefs??new string[0]).Where(r=>r!=null&&r.StartsWith("local:")).Distinct()) {
                string id=reference.Substring(6);string folder=id=="DriveLinkRestart"?Path.GetDirectoryName(AppDomain.CurrentDomain.BaseDirectory.TrimEnd(Path.DirectorySeparatorChar)):"";
                paths.Rows.Add(id,folder);
            }
        }catch(Exception e){network=null;MessageBox.Show(e.Message,Text,MessageBoxButtons.OK,MessageBoxIcon.Error);}
    }
    void Install(object sender,EventArgs args) {
        try {
            if(!consent.Checked)throw new InvalidOperationException("Read the disclosure and acknowledge the background companion first");
            if(network==null)throw new InvalidOperationException("Choose the administrator's network manifest first");
            var local=new Dictionary<string,string>();foreach(DataGridViewRow row in paths.Rows)local.Add(Convert.ToString(row.Cells[0].Value),Convert.ToString(row.Cells[1].Value));
            Config config=PilotSetup.Resolve(network,game.Text,steam.Text,workshop.Text,profile.Text,player.Text,defaults.Checked,local);
            string id=Regex.Replace(Path.GetFileNameWithoutExtension(manifest.Text),"[^a-zA-Z0-9_-]","_");
            string directory=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"DriveLinkRestart",id);Directory.CreateDirectory(directory);
            string file=Path.Combine(directory,"network.json");
            if(File.Exists(file))File.Copy(file,file+".previous-"+DateTime.UtcNow.ToString("yyyyMMddHHmmss"),false);
            File.WriteAllText(file,new JavaScriptSerializer().Serialize(config));
            string desktop=Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory);
            foreach(var d in config.destinations)PilotSetup.Shortcut(Path.Combine(desktop,"DriveLink - "+id+" - "+d.id+".lnk"),file,d.id);
            PilotSetup.Shortcut(Path.Combine(desktop,"DriveLink - "+id+" - helper only.lnk"),file,null);
            MessageBox.Show("Shortcuts created on your desktop. For the vanilla DayZ launcher, start the helper only shortcut just before joining (or after joining). Map shortcuts can also start the game directly. No game was launched. Configuration: "+file,Text);Close();
        }catch(Exception e){MessageBox.Show(e.Message,Text,MessageBoxButtons.OK,MessageBoxIcon.Error);}
    }
}
}
