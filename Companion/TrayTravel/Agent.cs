using System;
using System.IO;
using System.Linq;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Drawing;
using System.Web.Script.Serialization;
using DriveLinkRestart;
public class TrayReadiness { public string nonce {get;set;} public string sourceId {get;set;} public string targetId {get;set;} }
public static class TrayPolicy {
    public static void RequireIdle(bool previousBusy){if(previousBusy)throw new InvalidOperationException("The previous travel helper is still active. Finish or cancel that travel before retrying.");}
    public static bool CompletedReceipt(string path,string nonce) {
        try{if(String.IsNullOrEmpty(nonce)||!File.Exists(path)||new FileInfo(path).Length>65536)return false;
            var receipt=new JavaScriptSerializer().Deserialize<System.Collections.Generic.Dictionary<string,object>>(File.ReadAllText(path));
            return receipt.ContainsKey("nonce")&&receipt.ContainsKey("status")&&Convert.ToString(receipt["nonce"])==nonce&&Convert.ToString(receipt["status"])=="arrived";
        }catch(IOException){return false;}catch(ArgumentException){return false;}catch(InvalidOperationException){return false;}catch(UnauthorizedAccessException){return false;}
    }
    public static bool SameMods(Destination a,Destination b) {
        return a!=null&&b!=null&&a.mods!=null&&b.mods!=null&&a.mods.Select(Normalize).SequenceEqual(b.mods.Select(Normalize));
    }
    static string Normalize(string s){return Path.GetFullPath(s).TrimEnd('\\','/').ToLowerInvariant();}
    public static Destination Validate(Config c,TrayReadiness r,DateTime written) {
        if(r==null||String.IsNullOrEmpty(r.nonce)||!System.Text.RegularExpressions.Regex.IsMatch(r.nonce,"^[0-9]+-[0-9]{6}$")||DateTime.UtcNow-written>TimeSpan.FromSeconds(15)||written>DateTime.UtcNow.AddSeconds(5))throw new InvalidOperationException("Readiness request is stale or invalid");
        Protocol.Preflight(c);
        var source=c.destinations.SingleOrDefault(d=>d.id==r.sourceId);
        var target=c.destinations.SingleOrDefault(d=>d.id==r.targetId);
        if(source==null||target==null)throw new InvalidOperationException("Server is not in the installed network configuration");
        return target;
    }
}
public class TrayAgent : ApplicationContext {
    public const string StopEvent="Local\\DriveLinkTrayLocalTestStop";
    readonly Config config;readonly string configPath,ipc;readonly JavaScriptSerializer json=new JavaScriptSerializer();
    readonly NotifyIcon icon=new NotifyIcon();readonly System.Windows.Forms.Timer timer=new System.Windows.Forms.Timer();
    readonly EventWaitHandle stop=new EventWaitHandle(false,EventResetMode.AutoReset,StopEvent);
    volatile bool checking,exiting;string launchedNonce="",lastChecked="";volatile string status="Idle; waiting for DayZ travel";DateTime lastCheck=DateTime.MinValue;Process activeHelper;
    DateTime nextUpdateCheck=DateTime.UtcNow.AddSeconds(15),updateLaunchDeadline;
    Task<HelperUpdatePackage> updateCheck;HelperUpdatePackage updatePackage;Process updater;
    public TrayAgent(Config c,string file) {
        config=c;configPath=file;ipc=Path.Combine(c.clientProfile,"DriveLinkRestart");Directory.CreateDirectory(ipc);
        var menu=new ContextMenuStrip();menu.Items.Add("Status / setup location",null,delegate{MessageBox.Show(status+"\n\nInstalled helper version: "+TravelUpdates.Version+"\nInstalled agent: "+AppDomain.CurrentDomain.BaseDirectory+"\nReopen after exit from Windows Start > DriveLink Travel Helper.\nUninstall from this tray menu or Windows Settings > Apps > Installed apps > DriveLink Travel Helper.\nThe tray agent stays running after DayZ closes.","DriveLink Travel Helper");});
        menu.Items.Add("Exit for this Windows session",null,delegate{ExitAgent();});
        menu.Items.Add("Check for helper updates",null,delegate{nextUpdateCheck=DateTime.MinValue;status="Checking downloaded DriveLink helper for updates";});
        menu.Items.Add("Uninstall DriveLink Travel Helper...",null,delegate{
            try {
                Process.Start(new ProcessStartInfo(Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"DriveLinkUninstall.exe")){UseShellExecute=false});
            }catch(Exception e){MessageBox.Show(e.Message,"DriveLink Travel Helper",MessageBoxButtons.OK,MessageBoxIcon.Error);}
        });
        icon.Icon=SystemIcons.Application;icon.Text="DriveLink Travel Helper";icon.ContextMenuStrip=menu;icon.Visible=true;
        timer.Interval=500;timer.Tick+=Tick;timer.Start();Log("AGENT_STARTED version="+TravelUpdates.Version+" pid="+Process.GetCurrentProcess().Id);
    }
    void Log(string line){lock(this)File.AppendAllText(Path.Combine(ipc,"tray-agent.log"),DateTime.UtcNow.ToString("o")+" "+line+Environment.NewLine);}
    void Write(string file,object value){string path=Path.Combine(ipc,file),temp=path+"."+Guid.NewGuid().ToString("N")+".tmp";File.WriteAllText(temp,new JavaScriptSerializer().Serialize(value));if(File.Exists(path))File.Delete(path);File.Move(temp,path);}
    void ExitAgent(){
        if(TravelInstallation.HasHelpers()&&MessageBox.Show("Exit the agent and cancel the current travel helper? DayZ itself will stay open.","DriveLink Travel Helper",MessageBoxButtons.YesNo,MessageBoxIcon.Question)!=DialogResult.Yes)return;
        exiting=true;
        try{TravelInstallation.StopHelpers();}catch(Exception e){exiting=false;MessageBox.Show(e.Message,"DriveLink Travel Helper");return;}
        timer.Stop();icon.Visible=false;icon.Dispose();stop.Dispose();Log("AGENT_EXIT");ExitThread();
    }
    void Tick(object sender,EventArgs args) {
        if(exiting)return;
        if(stop.WaitOne(0)){ExitAgent();return;}
        if(updater!=null){
            if(File.Exists(Path.Combine(updatePackage.stage,"updater-ready.txt"))){exiting=true;timer.Stop();icon.Visible=false;icon.Dispose();stop.Dispose();Log("AGENT_EXIT_FOR_UPDATE version="+updatePackage.manifest.version);ExitThread();return;}
            if(updater.HasExited||DateTime.UtcNow>updateLaunchDeadline){if(!updater.HasExited){updater.Kill();updater.WaitForExit(2000);}updater.Dispose();updater=null;updatePackage=null;status="Helper update deferred; current helper remains available";Log("UPDATE_LAUNCH_FAILED");}
            else return;
        }
        if(activeHelper!=null) {
            try {
                if(!activeHelper.HasExited && TrayPolicy.CompletedReceipt(Path.Combine(ipc,"receipt-"+launchedNonce+".json"),launchedNonce)) {
                    if(DateTime.UtcNow-File.GetLastWriteTimeUtc(Path.Combine(ipc,"receipt-"+launchedNonce+".json"))>TimeSpan.FromSeconds(3)) {
                        // Only our recorded child, after a nonce-matched successful receipt; never DayZ.
                        Log("REAP_COMPLETED_HELPER pid="+activeHelper.Id+" nonce="+launchedNonce);
                        activeHelper.Kill();activeHelper.WaitForExit(2000);
                    }
                }
                if(activeHelper.HasExited){activeHelper.Dispose();activeHelper=null;status="Idle; waiting for DayZ travel";}
            }catch(InvalidOperationException){activeHelper=null;}catch(System.ComponentModel.Win32Exception e){status="Previous helper could not close: "+e.Message;Log("HELPER_CLEANUP_ERROR "+e.Message);}
        }
        bool previousBusy=activeHelper!=null;
        string readiness=Path.Combine(ipc,"readiness-request.json");
        if(!checking&&File.Exists(readiness)&&new FileInfo(readiness).Length<8192&&DateTime.UtcNow-File.GetLastWriteTimeUtc(readiness)<TimeSpan.FromSeconds(15)) {
            try {
                var r=json.Deserialize<TrayReadiness>(File.ReadAllText(readiness));
                if(r!=null&&(r.nonce!=lastChecked||DateTime.UtcNow-lastCheck>TimeSpan.FromSeconds(2))) {
                    lastChecked=r.nonce;lastCheck=DateTime.UtcNow;checking=true;DateTime written=File.GetLastWriteTimeUtc(readiness);
                    Task.Run(delegate {
                        try {
                            var target=TrayPolicy.Validate(config,r,written);
                            TrayPolicy.RequireIdle(previousBusy);
                            if(Process.GetProcessesByName("DayZ_x64").Length!=1)throw new InvalidOperationException("Exactly one DayZ client must be running");
                            if(Process.GetProcessesByName("steam").Length==0)throw new InvalidOperationException("Steam must be signed in");
                            Protocol.CheckServer(target);
                            if(exiting)return;
                            Write("readiness-response.json",new {nonce=r.nonce,status="ready",message="DriveLink tray helper ready"});status="Ready for "+target.name;
                            Log("READINESS_READY target="+r.targetId+" nonce="+r.nonce);
                        }catch(Exception e){try{Write("readiness-response.json",new {nonce=r.nonce,status="error",message=e.Message});}catch{}status=e.Message;Log("READINESS_ERROR "+e.Message);}
                        finally{checking=false;}
                    });
                }
            }catch(IOException){}catch(ArgumentException){}
        }
        string request=Path.Combine(ipc,"request.json");
        if(File.Exists(request)&&new FileInfo(request).Length<=1048576&&DateTime.UtcNow-File.GetLastWriteTimeUtc(request)<TimeSpan.FromSeconds(30)) {
            try {
                var r=json.Deserialize<Request>(File.ReadAllText(request));
                if(r!=null&&r.nonce!=launchedNonce) {
                    Protocol.Validate(config,r);
                    if(r.probe!=0||Process.GetProcessesByName("DayZ_x64").Length!=1)return;
                    if(activeHelper!=null&&!activeHelper.HasExited)return;
                    if(activeHelper!=null){activeHelper.Dispose();activeHelper=null;}
                    string exe=Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"DriveLinkRestartHelper.exe");
                    var process=Process.Start(new ProcessStartInfo(exe,"--one-transfer --vanilla-launcher --config "+Protocol.Quote(configPath)){UseShellExecute=false,CreateNoWindow=true});
                    activeHelper=process;
                    launchedNonce=r.nonce;
                    status="Travel helper active";Log("HELPER_LAUNCHED pid="+process.Id+" nonce="+r.nonce);
                }
            }catch(IOException){}catch(ArgumentException){}catch(Exception e){status=e.Message;Log("REQUEST_ERROR "+e.Message);}
        }
        CheckUpdates();
    }
    void CheckUpdates(){
        if(exiting)return;
        if(updateCheck!=null&&updateCheck.IsCompleted){
            try{updatePackage=updateCheck.GetAwaiter().GetResult();if(updatePackage!=null){status="Helper "+updatePackage.manifest.version+" ready; installs after DayZ closes";Log("UPDATE_STAGED version="+updatePackage.manifest.version);}else if(!checking&&activeHelper==null)status="No newer downloaded helper update; checking every five minutes";}
            catch(Exception e){Log("UPDATE_CHECK_DEFERRED "+e.Message);status="Helper update check deferred; will retry";}
            updateCheck=null;
        }
        if(updatePackage!=null){
            if(checking||activeHelper!=null||TravelUpdates.GameBusy())return;
            string readiness=Path.Combine(ipc,"readiness-request.json"),request=Path.Combine(ipc,"request.json");
            if((File.Exists(readiness)&&DateTime.UtcNow-File.GetLastWriteTimeUtc(readiness)<TimeSpan.FromSeconds(30))||(File.Exists(request)&&DateTime.UtcNow-File.GetLastWriteTimeUtc(request)<TimeSpan.FromSeconds(30)))return;
            try{updater=Process.Start(new ProcessStartInfo(Path.Combine(updatePackage.stage,"ApplyUpdate.exe"),Protocol.Quote(updatePackage.stage)+" "+Process.GetCurrentProcess().Id){UseShellExecute=false,CreateNoWindow=true,WorkingDirectory=updatePackage.stage});updateLaunchDeadline=DateTime.UtcNow.AddSeconds(10);}
            catch(Exception e){Log("UPDATE_LAUNCH_FAILED "+e.Message);updatePackage=null;}
            return;
        }
        if(updateCheck==null&&DateTime.UtcNow>=nextUpdateCheck){
            nextUpdateCheck=DateTime.UtcNow.AddMinutes(5);
            updateCheck=Task.Run(delegate{return TravelUpdates.Stage(TravelUpdates.Source(AppDomain.CurrentDomain.BaseDirectory),TravelUpdates.VersionCode,TravelUpdates.Cache);});
        }
    }
    [STAThread] public static int Main(string[] args) {
        bool startup=Array.IndexOf(args,"--startup")>=0;
        StartupLog("ENTRY source="+(startup?"windows-startup":"manual")+" cwd="+Environment.CurrentDirectory);
        try {
            string file=Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"helper-config.json");
            var c=new JavaScriptSerializer().Deserialize<Config>(File.ReadAllText(file));
            StartupLog("CONFIG_LOADED");
            bool owned;using(var mutex=new Mutex(true,"Local\\DriveLinkTray-"+Protocol.ClientMarker(c),out owned)) {
                if(!owned){StartupLog("ALREADY_RUNNING");return 0;}Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
                var context=new TrayAgent(c,file);int updated=Array.IndexOf(args,"--update-stage");if(updated>=0&&updated+1<args.Length)TravelUpdates.Acknowledge(args[updated+1]);Application.Run(context);
            }return 0;
        }catch(Exception e){StartupLog("FAILED "+e.GetType().Name+": "+e.Message);if(!startup)MessageBox.Show(e.Message,"DriveLink tray helper");return 1;}
    }
    static void StartupLog(string message) {
        try {string folder=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"DriveLinkTravel","LocalTest");Directory.CreateDirectory(folder);File.AppendAllText(Path.Combine(folder,"startup.log"),DateTime.UtcNow.ToString("o")+" pid="+Process.GetCurrentProcess().Id+" "+message+Environment.NewLine);}catch{}
    }
}
