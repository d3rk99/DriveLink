using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Management;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
using System.Windows.Forms;

namespace DriveLinkRestart {
public class Destination {
    public string id {get;set;} public string name {get;set;}
    public string host {get;set;} public int port {get;set;} public int queryPort {get;set;}
    public bool zen {get;set;} public string[] mods {get;set;}
    public bool namalskIsland {get;set;} public bool namalskSurvival {get;set;} public string world {get;set;}
    public string[] workshopIds {get;set;}
    public string[] modRefs {get;set;}
    public string[] modNames {get;set;}
}
public class Config {
    public string gameRoot {get;set;} public string steamExe {get;set;}
    public string clientProfile {get;set;} public string clientName {get;set;}
    public Destination[] destinations {get;set;}
    public string launchMode {get;set;}
    public string benchmarkLabel {get;set;}
    public bool allowRemote {get;set;}
    public string[] launchOptions {get;set;}
    public bool useDefaultProfile {get;set;}
}
public class Request {
    public string nonce {get;set;} public string targetId {get;set;}
    public int remainingHops {get;set;} public int probe {get;set;}
    public RoadFilm film {get;set;}
    public int benchmark {get;set;}
}
public class Reveal {public string nonce {get;set;} public int ready {get;set;} public string story {get;set;}}
public class ClientProgress {public string nonce {get;set;} public int stage {get;set;} public string detail {get;set;} public float fraction {get;set;}}
public sealed class TravelProgress {
    static readonly int[] Percentages={0,10,25,45,60,72,82,92,100};
    static readonly string[] Names={"Preparing travel","Closing current DayZ","Preparing Steam and mods","Launching destination client","Loading game","Connecting to server","Login countdown","Finalizing arrival","Connected — opening DayZ"};
    public int Stage {get;private set;} public string Detail {get;private set;}
    public int Percent {get{return Percentages[Stage];}}
    public string Label {get{return Names[Stage];}}
    public void Advance(int stage,string detail){lock(this){if(stage<Stage||stage<0||stage>=Percentages.Length)return;Stage=stage;Detail=detail??"";}}
    public bool Accept(ClientProgress progress,string nonce){lock(this){if(progress==null||String.IsNullOrEmpty(nonce)||progress.nonce!=nonce||progress.stage<4||progress.stage>7||progress.stage<Stage)return false;Advance(progress.stage,progress.detail);return true;}}
}
public static class GameForeground {
    // Native focus APIs can wait on the game's input queue. Never run them on our UI thread.
    public static bool RequestBounded(Func<bool> request,int milliseconds) {
        var task=Task.Run(request);
        try{return task.Wait(milliseconds)&&task.Result;}catch(AggregateException){return false;}
    }
    [System.Runtime.InteropServices.DllImport("user32.dll")]static extern bool SetForegroundWindow(IntPtr window);
    [System.Runtime.InteropServices.DllImport("user32.dll")]static extern bool ShowWindowAsync(IntPtr window,int command);
    [System.Runtime.InteropServices.DllImport("user32.dll")]static extern bool IsIconic(IntPtr window);
    [System.Runtime.InteropServices.DllImport("user32.dll")]static extern bool FlashWindowEx(ref FLASHWINFO info);
    [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential)]struct FLASHWINFO {public uint size;public IntPtr window;public uint flags,count,timeout;}
    public static bool Request(int pid){
        try{using(var process=Process.GetProcessById(pid)){process.Refresh();IntPtr window=process.MainWindowHandle;if(window==IntPtr.Zero)return false;if(IsIconic(window))ShowWindowAsync(window,9);if(SetForegroundWindow(window))return true;var flash=new FLASHWINFO{size=(uint)System.Runtime.InteropServices.Marshal.SizeOf(typeof(FLASHWINFO)),window=window,flags=2,count=3};FlashWindowEx(ref flash);}}
        catch(ArgumentException){}catch(InvalidOperationException){}return false;
    }
}
public static class TransitionWindowOrder {
    [System.Runtime.InteropServices.DllImport("user32.dll")]static extern IntPtr GetForegroundWindow();
    [System.Runtime.InteropServices.DllImport("user32.dll")]static extern uint GetWindowThreadProcessId(IntPtr window,out uint pid);
    [System.Runtime.InteropServices.DllImport("user32.dll")]static extern bool SetWindowPos(IntPtr window,IntPtr after,int x,int y,int width,int height,uint flags);
    public static bool IsDayZ(string processName) {
        return String.Equals(processName,"DayZ_x64",StringComparison.OrdinalIgnoreCase)||String.Equals(processName,"DayZLauncher",StringComparison.OrdinalIgnoreCase)||String.Equals(processName,"DayZ_BE",StringComparison.OrdinalIgnoreCase);
    }
    public static bool ShouldCover(bool visible,bool minimized,bool gameForeground,bool helperForeground,bool alreadyCovering) {
        return visible&&!minimized&&(gameForeground||(helperForeground&&alreadyCovering));
    }
    public static bool ForegroundIsDayZ(out bool helperForeground,IntPtr helper) {
        IntPtr window=GetForegroundWindow();helperForeground=window==helper;
        uint pid;GetWindowThreadProcessId(window,out pid);
        if(pid==0)return false;
        try{using(var process=Process.GetProcessById((int)pid))return IsDayZ(process.ProcessName);}
        catch(ArgumentException){}catch(InvalidOperationException){}catch(System.ComponentModel.Win32Exception){}return false;
    }
    public static bool SetCover(IntPtr window,bool cover) {
        // Change only z-order, never foreground/input ownership. Async avoids waiting on DayZ's input queue.
        const uint flags=0x0001|0x0002|0x0010|0x0400|0x4000; // NOSIZE|NOMOVE|NOACTIVATE|NOSENDCHANGING|ASYNCWINDOWPOS
        return SetWindowPos(window,new IntPtr(cover?-1:-2),0,0,0,0,flags);
    }
}
public class Arrival {
    public string nonce {get;set;} public int port {get;set;} public int zen {get;set;}
    public int namalskIsland {get;set;} public int namalskSurvival {get;set;} public string world {get;set;}
}
public static class Protocol {
    public static string HelperStopName(Config c){return @"Local\DriveLinkTravelHelperStop-"+ClientMarker(c);}
    public static string ModName(string reference,string path) {
        var known=new Dictionary<string,string>{{"1559212036","Community Framework (CF)"},{"1828439124","VPPAdminTools"},{"2289456201","Namalsk Island"},{"2289461232","Namalsk Survival"},{"3824238331","DriveLink"}};
        string id=(reference??"").Replace("workshop:","");string name;
        if(known.TryGetValue(id,out name))return name;
        if(reference=="local:DriveLinkRestart")return "DriveLink";
        try {
            string file=Path.Combine(path,"mod.cpp");
            if(File.Exists(file)&&new FileInfo(file).Length<65536){var m=System.Text.RegularExpressions.Regex.Match(File.ReadAllText(file),"(?im)^\\s*name\\s*=\\s*\"([^\"]+)\"");if(m.Success)return m.Groups[1].Value;}
        }catch(IOException){}catch(UnauthorizedAccessException){}
        return reference!=null&&reference.StartsWith("workshop:")?"Workshop mod "+id:Path.GetFileName(path);
    }
    public static bool HasMod(string path){return Directory.Exists(Path.Combine(path,"Addons"))&&Directory.GetFiles(Path.Combine(path,"Addons"),"*.pbo").Length>0;}
    public static void RequireMods(IEnumerable<Destination> destinations) {
        var missing=new HashSet<string>();
        foreach(var d in destinations)for(int i=0;i<(d.mods??new string[0]).Length;i++) {
            string path=d.mods[i];if(HasMod(path))continue;
            string reference=d.modRefs!=null&&i<d.modRefs.Length?d.modRefs[i]:null;
            string name=d.modNames!=null&&i<d.modNames.Length&&!String.IsNullOrWhiteSpace(d.modNames[i])?d.modNames[i]:ModName(reference,path);
            missing.Add(name+(reference!=null&&reference.StartsWith("workshop:")?" (Workshop "+reference.Substring(9)+")":""));
        }
        if(missing.Count>0)throw new InvalidOperationException("Required mods are missing or have not finished downloading:\n\n"+String.Join("\n",missing.OrderBy(x=>x).Select(x=>"• "+x))+"\n\nSubscribe to these mods in Steam Workshop and wait for all downloads to finish. Then close and restart DriveLink Setup and run setup again.");
    }
    public static bool MatchesClient(Config c,string args,string executable,bool vanilla,bool bootstrap) {
        if(args==null)return false;
        if(args.IndexOf(ClientMarker(c),StringComparison.OrdinalIgnoreCase)>=0 || (!c.useDefaultProfile&&args.IndexOf(c.clientProfile,StringComparison.OrdinalIgnoreCase)>=0))return true;
        if(!vanilla||!c.useDefaultProfile||!String.Equals(executable,Path.Combine(c.gameRoot,bootstrap?"DayZ_BE.exe":"DayZ_x64.exe"),StringComparison.OrdinalIgnoreCase))return false;
        // Explicit opt-in for an ordinary launcher session. A custom -profiles session
        // must use matching configuration instead of being silently adopted.
        return args.IndexOf("-profiles",StringComparison.OrdinalIgnoreCase)<0;
    }
    public static string ClientMarker(Config c) {
        using(var sha=System.Security.Cryptography.SHA256.Create())
            return "-dlrClient="+BitConverter.ToString(sha.ComputeHash(Encoding.UTF8.GetBytes(Path.GetFullPath(c.clientProfile).ToLowerInvariant()))).Replace("-","").Substring(0,16);
    }
    public static void VerifyArrival(Destination d,Arrival a) {
        if(a.port!=d.port || (a.zen!=0)!=d.zen || (a.namalskIsland!=0)!=d.namalskIsland || (a.namalskSurvival!=0)!=d.namalskSurvival ||
            (!String.IsNullOrEmpty(d.world) && !String.Equals(a.world,d.world,StringComparison.OrdinalIgnoreCase)))
            throw new InvalidOperationException("Destination/map/mod verification failed");
    }
    public static string Quote(string value) {
        if (value == null || value.IndexOfAny(new char[]{'"','\r','\n'}) >= 0)
            throw new InvalidOperationException("Invalid launch argument");
        return "\"" + value + "\"";
    }
    public static Destination Validate(Config c,Request r) {
        if (r == null || String.IsNullOrEmpty(r.nonce) || r.nonce.Length > 80 ||
            r.nonce.Any(x => !Char.IsDigit(x) && x != '-') || r.remainingHops < 0 || r.remainingHops > 2)
            throw new InvalidOperationException("Invalid local travel request");
        Destination d = c.destinations.SingleOrDefault(x => x.id == r.targetId);
        if (d == null) throw new InvalidOperationException("Destination is not in the local manifest");
        if (String.IsNullOrEmpty(d.host) || d.host.Any(x=>!Char.IsLetterOrDigit(x)&&x!='.'&&x!='-') ||
            (d.host!="127.0.0.1"&&!c.allowRemote) || d.port < 1024 || d.port > 65535 || d.queryPort < 1024 || d.queryPort > 65535)
            throw new InvalidOperationException("Destination address is invalid or remote servers are not enabled in the local manifest");
        if (d.mods == null || d.mods.Length == 0) throw new InvalidOperationException("Empty mod list");
        foreach (string mod in d.mods) {
            if (!Path.IsPathRooted(mod) || mod.Contains(";")) throw new InvalidOperationException("Invalid mod path");
            Quote(mod);
        }
        RequireMods(new[]{d});
        return d;
    }
    public static void Preflight(Config c) {
        if(c==null || String.IsNullOrWhiteSpace(c.clientProfile) || !Path.IsPathRooted(c.clientProfile) || !Directory.Exists(c.clientProfile) || String.IsNullOrWhiteSpace(c.clientName))
            throw new InvalidOperationException("An absolute player profile and player name are required");
        Quote(c.clientProfile); Quote(c.clientName);
        if(c.useDefaultProfile&&!String.Equals(Path.GetFullPath(c.clientProfile).TrimEnd('\\'),Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"DayZ").TrimEnd('\\'),StringComparison.OrdinalIgnoreCase))
            throw new InvalidOperationException("Normal settings mode uses the standard LocalAppData\\DayZ IPC directory");
        if(!Path.IsPathRooted(c.steamExe)||!Path.IsPathRooted(c.gameRoot)||!File.Exists(c.steamExe)||!File.Exists(Path.Combine(c.gameRoot,"DayZ_BE.exe")))
            throw new InvalidOperationException("Steam or the BattlEye game launcher is missing");
        if(c.destinations==null || c.destinations.Length<2 || c.destinations.Any(d=>d==null||String.IsNullOrWhiteSpace(d.id)) || c.destinations.Select(d=>d.id).Distinct().Count()!=c.destinations.Length)
            throw new InvalidOperationException("The manifest must contain at least two uniquely named destinations");
        PlayerOptions(c);
        RequireMods(c.destinations);
        if(!String.IsNullOrEmpty(c.launchMode)&&c.launchMode!="steam"&&c.launchMode!="steam-early"&&c.launchMode!="battleye")throw new InvalidOperationException("Unknown launch mode");
        foreach(var d in c.destinations)Validate(c,new Request {nonce="1-100000",targetId=d.id});
    }
    public static string PlayerOptions(Config c) {
        // Only local, explicitly configured preferences; never execute server-supplied arguments.
        string[] flags={"-window","-fullscreen","-nosplash","-skipIntro","-noPause","-dologs"};
        var result=new List<string>();
        foreach(string option in c.launchOptions??new string[0]) {
            if(option==null)throw new InvalidOperationException("Null launch option");
            bool safe=flags.Contains(option,StringComparer.OrdinalIgnoreCase);
            foreach(string prefix in new[]{"-x=","-y=","-winX=","-winY="}) {
                int value;
                if(option.StartsWith(prefix,StringComparison.OrdinalIgnoreCase)&&Int32.TryParse(option.Substring(prefix.Length),out value)&&value>=0&&value<=32768)safe=true;
            }
            if(option.StartsWith("-cfg=",StringComparison.OrdinalIgnoreCase)&&Path.IsPathRooted(option.Substring(5))&&File.Exists(option.Substring(5)))safe=true;
            if(!safe)throw new InvalidOperationException("Unsupported local launch option: "+option);
            result.Add(Quote(option));
        }
        return String.Join(" ",result);
    }
    public static void CheckServer(Destination d) {
        // A2S_INFO proves the configured query endpoint responds, not that admission is guaranteed.
        byte[] request=Encoding.ASCII.GetBytes("\xff\xff\xff\xffTSource Engine Query\0");
        request[0]=request[1]=request[2]=request[3]=255;
        using(var udp=new UdpClient()) {
            udp.Client.ReceiveTimeout=3000; udp.Connect(d.host,d.queryPort);
            udp.Send(request,request.Length);
            var sender=new IPEndPoint(IPAddress.Any,0);
            byte[] reply=udp.Receive(ref sender);
            if(reply.Length>=9 && reply[4]==0x41) {
                var challenged=new byte[request.Length+4]; Array.Copy(request,challenged,request.Length);Array.Copy(reply,5,challenged,request.Length,4);
                udp.Send(challenged,challenged.Length);reply=udp.Receive(ref sender);
            }
            if(reply.Length<6 || reply[0]!=255 || reply[1]!=255 || reply[2]!=255 || reply[3]!=255 || reply[4]!=0x49)
                throw new InvalidOperationException("Destination did not return a supported server query response");
        }
    }
    public static string Arguments(Config c,Destination d,Request r) {
        return "-applaunch 221100 -nolauncher -useBE " + GameArguments(c,d,r);
    }
    public static string GameArguments(Config c,Destination d,Request r) {
        return Quote("-mod=" + String.Join(";",d.mods)) + " " +
            (c.useDefaultProfile?"":Quote("-profiles=" + c.clientProfile)+" ") + Quote("-name=" + c.clientName) + " " + ClientMarker(c) +
            " -connect=" + d.host + ":" + d.port + ":" + d.queryPort + " -port=" + d.port + " " + PlayerOptions(c) +
            " -dlrSession=" + r.nonce + " -dlrAutoHops=" + r.remainingHops + (r.benchmark!=0?" -dlrBenchmark=1":"");
    }
}
public class Shroud : Form {
    public readonly TravelProgress Progress=new TravelProgress();
    public volatile string Status = "Preparing your journey";
    public bool Failed;
    public Action Cancel;
    private readonly Stopwatch animation = Stopwatch.StartNew();
    private readonly System.Windows.Forms.Timer animationTimer=new System.Windows.Forms.Timer();
    private readonly System.Windows.Forms.Timer coverTimer=new System.Windows.Forms.Timer();
    private bool covering;
    private bool coverReleased;
    private readonly Stopwatch filmClock=new Stopwatch();
    private FilmRenderer renderer;
    public bool HasFilm {get {return renderer!=null;}}
    public double FilmElapsed {get {return filmClock.Elapsed.TotalSeconds;}}
    public bool FilmComplete {get {return renderer==null||FilmElapsed>=renderer.Film.Duration;}}
    public void StartFilm(RoadFilm film,string world) {
        if(renderer!=null)renderer.Dispose(); renderer=null;
        if(film!=null&&film.Valid()) {
            if(String.IsNullOrEmpty(film.biomeTo))film.biomeTo=RoadFilm.Biome(world);
            renderer=new FilmRenderer(film,Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"Assets"));
        }
        filmClock.Restart();
    }
    protected override void Dispose(bool disposing) {if(disposing){coverTimer.Stop();coverTimer.Dispose();animationTimer.Stop();animationTimer.Dispose();if(renderer!=null)renderer.Dispose();}base.Dispose(disposing);}
    public void RefreshCover() {
        if(!IsHandleCreated||IsDisposed)return;
        bool ownForeground;
        bool gameForeground=TransitionWindowOrder.ForegroundIsDayZ(out ownForeground,Handle);
        bool wanted=!coverReleased&&TransitionWindowOrder.ShouldCover(Visible,WindowState==FormWindowState.Minimized,gameForeground,ownForeground,covering);
        if((wanted||wanted!=covering)&&TransitionWindowOrder.SetCover(Handle,wanted))covering=wanted;
    }
    public void ReleaseCover() {
        coverReleased=true;coverTimer.Stop();
        if(IsHandleCreated)TransitionWindowOrder.SetCover(Handle,false);
        covering=false;
    }
    protected override void OnVisibleChanged(EventArgs e) {
        base.OnVisibleChanged(e);
        if(Visible){coverReleased=false;coverTimer.Start();RefreshCover();}
        else ReleaseCover();
    }
    protected override void OnResize(EventArgs e) {base.OnResize(e);RefreshCover();}
    public Shroud() {
        Text="DriveLink travel"; FormBorderStyle=FormBorderStyle.Sizable;MinimizeBox=true;MaximizeBox=true;ShowInTaskbar=true;
        StartPosition=FormStartPosition.CenterScreen;Size=new Size(1100,800);MinimumSize=new Size(640,480);WindowState=FormWindowState.Maximized;
        TopMost=false; DoubleBuffered=true; BackColor=Color.FromArgb(12,20,25);
        coverTimer.Interval=200;coverTimer.Tick+=delegate {RefreshCover();};
        KeyPreview=true;
        KeyDown += delegate(object s,KeyEventArgs e) { if(e.KeyCode==Keys.Escape && Cancel!=null) Cancel(); };
        animationTimer.Interval=33;
        animationTimer.Tick += delegate { Invalidate(); }; animationTimer.Start();
    }
    protected override bool ShowWithoutActivation {get{return true;}}
    void DrawProgress(Graphics g,int w,int h){
        var bar=new Rectangle(w/12,h-105,w*5/6,14);
        using(var brush=new SolidBrush(Color.FromArgb(48,62,67)))g.FillRectangle(brush,bar);
        using(var brush=new SolidBrush(Color.FromArgb(113,180,140)))g.FillRectangle(brush,bar.X,bar.Y,bar.Width*Progress.Percent/100f,bar.Height);
        string text=Progress.Label+" · "+Progress.Percent+"% of stages completed";
        using(var font=new Font("Segoe UI",10))TextRenderer.DrawText(g,text,font,new Rectangle(w/12,h-86,w*5/6,28),Color.White,TextFormatFlags.HorizontalCenter);
    }
    protected override void OnPaint(PaintEventArgs e) {
        base.OnPaint(e); Graphics g=e.Graphics; g.SmoothingMode=System.Drawing.Drawing2D.SmoothingMode.AntiAlias;
        int w=ClientSize.Width,h=ClientSize.Height; float t=(float)animation.Elapsed.TotalSeconds;
        if(renderer!=null) {
            using(var font=new Font("Segoe UI",20,FontStyle.Bold)) TextRenderer.DrawText(g,"DRIVELINK · ON THE ROAD",font,new Rectangle(0,12,w,44),Color.FromArgb(224,233,218),TextFormatFlags.HorizontalCenter);
            float filmW=Math.Min(w*.8f,(h-230)*1.15f);
            renderer.DrawTravel(g,new RectangleF((w-filmW)/2,62,filmW,h-230),FilmElapsed);
            using(var font=new Font("Segoe UI",11)) TextRenderer.DrawText(g,Status,font,new Rectangle(w/12,h-155,w*5/6,46),Color.FromArgb(240,225,190),TextFormatFlags.HorizontalCenter|TextFormatFlags.WordBreak);
            DrawProgress(g,w,h);
            using(var font=new Font("Segoe UI",9))TextRenderer.DrawText(g,"Minimize or Alt+Tab at any time · DayZ is requested at arrival · Esc cancels travel",font,new Rectangle(w/12,h-45,w*5/6,30),Color.Silver,TextFormatFlags.HorizontalCenter|TextFormatFlags.WordBreak);
            return;
        }
        using(var brush=new SolidBrush(Color.FromArgb(22,42,48))) g.FillRectangle(brush,0,h*.55f,w,h*.45f);
        using(var brush=new SolidBrush(Color.FromArgb(37,48,50))) g.FillPolygon(brush,new PointF[]{new PointF(w*.43f,h*.55f),new PointF(w*.57f,h*.55f),new PointF(w*.83f,h),new PointF(w*.17f,h)});
        using(var pen=new Pen(Color.FromArgb(140,169,150),4))
            for(int i=0;i<9;i++) { float y=h*.58f+((i*75+t*95)%(h*.5f)); g.DrawLine(pen,w*.5f,y,w*.5f,y+30); }
        using(var brush=new SolidBrush(Color.FromArgb(113,146,121))) g.FillRectangle(brush,w*.5f-35,h*.75f,70,40);
        using(var font=new Font("Segoe UI",34,FontStyle.Bold)) TextRenderer.DrawText(g,"DRIVELINK",font,new Rectangle(0,h/5,w,70),Color.FromArgb(224,233,218),TextFormatFlags.HorizontalCenter);
        using(var font=new Font("Segoe UI",16)) TextRenderer.DrawText(g,Status,font,new Rectangle(w/8,h/3,w*3/4,150),Color.White,TextFormatFlags.HorizontalCenter|TextFormatFlags.WordBreak);
        DrawProgress(g,w,h);
        using(var font=new Font("Segoe UI",9))TextRenderer.DrawText(g,"Minimize or Alt+Tab at any time · Esc cancels travel",font,new Rectangle(w/8,h-45,w*3/4,30),Color.Silver,TextFormatFlags.HorizontalCenter);
    }
}
public class HelperContext : ApplicationContext {
    private readonly Config config; private readonly string ipc;
    private readonly JavaScriptSerializer json=new JavaScriptSerializer();
    private readonly Shroud shroud=new Shroud();
    private readonly System.Windows.Forms.Timer poll=new System.Windows.Forms.Timer();
    private readonly System.Windows.Forms.Timer control=new System.Windows.Forms.Timer();
    private readonly EventWaitHandle stopRequest;
    private int finishing;
    private readonly Stopwatch lifetime=Stopwatch.StartNew();
    private readonly bool watch;
    private readonly bool vanilla;
    private bool working,seenClient; private volatile bool cancelled;
    private string filmNonce;
    private string phase="preflight";
    private DateTime lastPlayback;
    public HelperContext(Config c,bool watchMode,bool vanillaMode=false) {
        config=c; watch=watchMode; vanilla=vanillaMode; ipc=Path.Combine(c.clientProfile,"DriveLinkRestart");
        stopRequest=new EventWaitHandle(false,EventResetMode.AutoReset,Protocol.HelperStopName(c));
        Directory.CreateDirectory(ipc); IntPtr handle=shroud.Handle;
        shroud.Cancel=delegate { Finish("USER_CANCEL"); };
        control.Interval=250;control.Tick+=delegate {if(stopRequest.WaitOne(0))Finish("STOP_REQUESTED");else if(lifetime.Elapsed.TotalMinutes>15)Finish("LIFETIME_LIMIT_EXIT");};control.Start();
        poll.Interval=250; poll.Tick+=Tick; poll.Start(); Log("HELPER_STARTED mode="+(vanilla?"vanilla-launcher":(watch?"watch":"on-demand")));
    }
    private void Ui(Action action) {try{if(!shroud.IsDisposed&&shroud.IsHandleCreated)shroud.BeginInvoke(action);}catch(InvalidOperationException){}}
    private void Log(string message) {
        lock(this) File.AppendAllText(Path.Combine(ipc,"helper.log"),DateTime.UtcNow.ToString("o")+" "+message+Environment.NewLine);
    }
    private void Write(string file,object value) {
        string path=Path.Combine(ipc,file),temp=path+".tmp";
        File.WriteAllText(temp,json.Serialize(value));
        if(File.Exists(path)) File.Delete(path);
        File.Move(temp,path);
    }
    private T Read<T>(string path) { return json.Deserialize<T>(File.ReadAllText(path)); }
    private List<int> Clients() {
        var found=new List<int>();
        var hidden=new List<int>();
        int total=0;
        using(var query=new ManagementObjectSearcher("SELECT ProcessId,CommandLine,ExecutablePath FROM Win32_Process WHERE Name='DayZ_x64.exe'"))
            foreach(ManagementObject entry in query.Get()) {
                total++;
                string args=entry["CommandLine"] as string;
                if(Protocol.MatchesClient(config,args,entry["ExecutablePath"] as string,vanilla,false))
                    found.Add(Convert.ToInt32(entry["ProcessId"]));
                else if(args==null) hidden.Add(Convert.ToInt32(entry["ProcessId"]));
            }
        if(total!=1)return new List<int>();
        // BattlEye hides the game's command line. Only accept this case when there is
        // one game and one BE launcher, with the launcher using our dedicated profile.
        if(found.Count==0 && hidden.Count==1) {
            using(var launchers=new ManagementObjectSearcher("SELECT CommandLine,ExecutablePath FROM Win32_Process WHERE Name='DayZ_BE.exe'")) {
                var rows=launchers.Get().Cast<ManagementObject>().ToList();
                if(rows.Count==1) {
                    string args=rows[0]["CommandLine"] as string;
                    if(Protocol.MatchesClient(config,args,rows[0]["ExecutablePath"] as string,vanilla,true)) found.Add(hidden[0]);
                }
            }
        }
        return found;
    }
    private void Tick(object sender,EventArgs args) {
        if(working) return;
        string path=Path.Combine(ipc,"request.json");
        if(File.Exists(path) && DateTime.UtcNow-File.GetLastWriteTimeUtc(path)<TimeSpan.FromSeconds(30)) {
            Request request;
            try { request=Read<Request>(path); } catch { return; }
            working=true; poll.Stop();
            Task.Run(delegate { Transfer(request); }); return;
        }
        if(watch) {
            var clients=Clients();bool running=clients.Count>0;
            if(running&&!seenClient){seenClient=true;Log("CLIENT_ATTACHED pid="+clients[0]+" vanilla="+vanilla);}
            if((seenClient&&!running)||(!seenClient&&lifetime.Elapsed.TotalSeconds>180)) Finish("WATCH_EXIT");
        } else if(lifetime.Elapsed.TotalSeconds>30) Finish("NO_REQUEST_EXIT");
    }
    private void WaitUntil(Func<bool> condition,int seconds,string failure) {
        var timeout=Stopwatch.StartNew();
        while(true) {
            if(cancelled) throw new OperationCanceledException();
            if(condition())return;
            UpdatePlayback();
            if(timeout.Elapsed.TotalSeconds>seconds) throw new TimeoutException(failure);
            Thread.Sleep(250);
        }
    }
    private void UpdatePlayback() {
        if(String.IsNullOrEmpty(filmNonce)||DateTime.UtcNow-lastPlayback<TimeSpan.FromSeconds(1))return;
        // Old clients must not suppress their story or vote it complete.
        Write("playback.json",new {nonce=filmNonce,hasFilm=false,elapsed=0,complete=false});
        lastPlayback=DateTime.UtcNow;
    }
    private bool GameExists(int pid) {
        using(var query=new ManagementObjectSearcher("SELECT ProcessId FROM Win32_Process WHERE Name='DayZ_x64.exe' AND ProcessId="+pid))
            return query.Get().Count!=0;
    }
    private void CloseVanillaLauncher() {
        string expected=Path.GetFullPath(Path.Combine(config.gameRoot,"DayZLauncher.exe"));
        var launchers=new List<int>();
        using(var query=new ManagementObjectSearcher("SELECT ProcessId,ExecutablePath FROM Win32_Process WHERE Name='DayZLauncher.exe'"))
            foreach(ManagementObject entry in query.Get()) {
                string path=entry["ExecutablePath"] as string;
                if(path==null||!String.Equals(Path.GetFullPath(path),expected,StringComparison.OrdinalIgnoreCase))
                    throw new InvalidOperationException("Close the DayZ launcher before continuing: its installation could not be verified");
                launchers.Add(Convert.ToInt32(entry["ProcessId"]));
            }
        foreach(int id in launchers) {
            try {using(var launcher=Process.GetProcessById(id)) {
                if(launcher.HasExited)continue;
                Log("LAUNCHER_CLOSE_REQUEST pid="+id);
                if(!launcher.CloseMainWindow())throw new InvalidOperationException("Please close the DayZ launcher; it has no window that can be closed normally");
            }}catch(ArgumentException){}
        }
        if(launchers.Count>0) {
            phase="waiting-launcher-exit";
            WaitUntil(delegate {
                foreach(int id in launchers)try{using(var p=Process.GetProcessById(id))if(!p.HasExited)return false;}catch(ArgumentException){}
                return true;
            },30,"The DayZ launcher is still open. Close it normally; no forced termination was attempted");
            Log("VANILLA_LAUNCHER_EXITED");
            Thread.Sleep(2000);
        }
    }
    private void Finish(string reason) {
        if(Interlocked.Exchange(ref finishing,1)!=0)return;
        cancelled=true;Log(reason);
        Ui(delegate {poll.Stop();control.Stop();shroud.Hide();shroud.Close();ExitThread();});
    }
    protected override void Dispose(bool disposing) {
        if(disposing){cancelled=true;poll.Dispose();control.Dispose();stopRequest.Dispose();shroud.Dispose();}
        base.Dispose(disposing);
    }
    private void Transfer(Request r) {
        phase="preflight";
        try {
            if(r==null || String.IsNullOrEmpty(r.nonce)) throw new InvalidOperationException("Missing request nonce");
            if(r.probe!=0) {
                Log("PROTOCOL_ACTIVATED nonce="+r.nonce);
                Write("response.json",new {nonce=r.nonce,status="ready",message="Protocol activation verified"});
                WaitUntil(delegate {return !File.Exists(Path.Combine(ipc,"request.json"));},25,"Probe did not acknowledge");
                Finish("PROBE_COMPLETE"); return;
            }
            Destination d=Protocol.Validate(config,r);
            Protocol.Preflight(config);
            if(Process.GetProcessesByName("steam").Length==0)throw new InvalidOperationException("Start Steam and sign in before transferring; DayZ was not closed");
            if(d.host!="127.0.0.1") {
                try {Protocol.CheckServer(d);}catch(Exception e){throw new InvalidOperationException("Destination query failed before closing DayZ: "+e.Message);}
            }
            if(!String.IsNullOrEmpty(config.launchMode)&&config.launchMode!="steam"&&config.launchMode!="battleye"&&config.launchMode!="steam-early")throw new InvalidOperationException("Unknown launch mode");
            var clock=Stopwatch.StartNew();var timing=new Dictionary<string,double>();
            List<int> clients=Clients();
            if(clients.Count!=1) throw new InvalidOperationException("Could not identify exactly one matching DayZ client. For ordinary launcher joins, start the helper-only shortcut and use normal profile settings.");
            int oldId=clients[0];
            shroud.Status="Travelling to "+d.name+"\nPreparing the destination mod set";
            shroud.Progress.Advance(1,"Waiting for the source client to close");
            Ui(delegate { shroud.StartFilm(r.film,d.world);shroud.Show(); });
            // Acknowledge only after the UI-thread has actually displayed its cover.
            WaitUntil(delegate {return shroud.Visible;},10,"Travel screen could not open");
            filmNonce=r.nonce;lastPlayback=DateTime.MinValue;UpdatePlayback();
            if(shroud.HasFilm)Log("GENERIC_TRAVEL_STARTED nonce="+r.nonce+" vehicles="+r.film.Looks.Length+" crew="+r.film.crew.Length);
            if(shroud.HasFilm)Write("journey-"+r.nonce+".json",r.film);
            Write("response.json",new {nonce=r.nonce,status="ready",message="Helper ready"});
            Log("READY oldPid="+oldId+" destination="+d.id+" zen="+d.zen);
            phase="waiting-old-client-exit";
            timing["ready"]=clock.Elapsed.TotalSeconds;
            WaitUntil(delegate {return !GameExists(oldId);},60,"DayZ did not close; no force termination was attempted");
            Log("OLD_CLIENT_EXITED pid="+oldId);
            timing["oldClientExit"]=clock.Elapsed.TotalSeconds;
            shroud.Progress.Advance(2,"Source client closed; preparing Steam and destination mods");
            // Steam tracks the vanilla launcher as part of the running app. A normal
            // launcher session must end too before Steam can launch a new mod set.
            CloseVanillaLauncher();
            if(cancelled)throw new OperationCanceledException();
            bool early=config.launchMode=="steam-early";
            bool direct=config.launchMode=="battleye";
            if(!early)WaitUntil(delegate {
                using(var query=new ManagementObjectSearcher("SELECT ProcessId FROM Win32_Process WHERE Name='DayZ_BE.exe'"))
                    return query.Get().Count==0;
            },30,"The BattlEye launcher has not finished closing");
            if(!early){timing["bootstrapExit"]=clock.Elapsed.TotalSeconds;Log("BOOTSTRAP_EXITED nonce="+r.nonce);}
            else Log("EARLY_STEAM_REQUEST game exit confirmed; old bootstrap may still be closing");
            if(!direct&&!early)Thread.Sleep(2000); // Baseline Steam path waits for the exit notification.
            shroud.Status="Travelling to "+d.name+"\nStarting DayZ with "+d.mods.Length+" mods";
            timing["launchRequest"]=clock.Elapsed.TotalSeconds;
            phase="launch-requested";
            shroud.Progress.Advance(3,"Launch request sent to Steam");
            string mode=direct?"battleye":(early?"steam-early":"steam");
            Log("LAUNCH_REQUEST mode="+mode+" nonce="+r.nonce);
            if(cancelled)throw new OperationCanceledException();
            if(direct) {
                if(Process.GetProcessesByName("steam").Length==0)throw new InvalidOperationException("Steam must be running and signed in");
                Process.Start(new ProcessStartInfo(Path.Combine(config.gameRoot,"DayZ_BE.exe"),"0 1 0 -exe DayZ_x64.exe "+Protocol.GameArguments(config,d,r)){WorkingDirectory=config.gameRoot,UseShellExecute=false,CreateNoWindow=true,WindowStyle=ProcessWindowStyle.Hidden});
            } else Process.Start(new ProcessStartInfo(config.steamExe,Protocol.Arguments(config,d,r)){UseShellExecute=false,CreateNoWindow=true});
            int next=0;
            bool retried=false;var launchWait=Stopwatch.StartNew();
            WaitUntil(delegate {
                next=Clients().FirstOrDefault(p=>p!=oldId);
                // Steam can reject an early request while finishing the old app. Retry once,
                // only when no game, launcher or bootstrap exists; never create a second game.
                if(next==0&&early&&!retried&&launchWait.Elapsed.TotalSeconds>20) {
                    using(var query=new ManagementObjectSearcher("SELECT ProcessId FROM Win32_Process WHERE Name='DayZ_x64.exe' OR Name='DayZ_BE.exe' OR Name='DayZLauncher.exe'"))
                        if(query.Get().Count==0) {
                            retried=true;Log("EARLY_STEAM_RETRY nonce="+r.nonce);
                            Process.Start(new ProcessStartInfo(config.steamExe,Protocol.Arguments(config,d,r)){UseShellExecute=false,CreateNoWindow=true});
                        }
                }
                return next!=0;
            },90,"Steam did not launch the replacement client");
            Log("NEW_CLIENT pid="+next+" destination="+d.id);
            phase="waiting-arrival";
            timing["clientProcess"]=clock.Elapsed.TotalSeconds;
            shroud.Status="Connecting to "+d.name+"\nWaiting for the player to arrive";
            shroud.Progress.Advance(4,"Destination process is running");
            string arrivalPath=Path.Combine(ipc,"arrival.json");
            WaitUntil(delegate {
                if(!GameExists(next)) throw new InvalidOperationException("The replacement client closed before arrival");
                string progressPath=Path.Combine(ipc,"progress.json");
                if(File.Exists(progressPath)&&new FileInfo(progressPath).Length<16384&&DateTime.UtcNow-File.GetLastWriteTimeUtc(progressPath)<TimeSpan.FromSeconds(30)) {
                    try{var progress=Read<ClientProgress>(progressPath);if(shroud.Progress.Accept(progress,r.nonce)){if(!String.IsNullOrWhiteSpace(progress.detail))shroud.Status=progress.detail.Length>160?progress.detail.Substring(0,160):progress.detail;}}catch(IOException){}catch(ArgumentException){}
                }
                if(!File.Exists(arrivalPath)) return false;
                Arrival a;
                try {a=Read<Arrival>(arrivalPath);} catch {return false;}
                if(a==null||a.nonce!=r.nonce) return false;
                Protocol.VerifyArrival(d,a);
                return true;
            },180,"Arrival was not confirmed. The hub transfer remains recoverable.");
            Log("ARRIVAL_VERIFIED pid="+next+" destination="+d.id+" zen="+d.zen+" world="+d.world+" namalskIsland="+d.namalskIsland+" namalskSurvival="+d.namalskSurvival);
            timing["arrival"]=clock.Elapsed.TotalSeconds;
            shroud.Progress.Advance(8,"Transfer arrival verified");
            var hidden=new ManualResetEventSlim(false);
            Ui(delegate {shroud.ReleaseCover();shroud.Hide();hidden.Set();});
            hidden.Wait(2000);
            bool focused=GameForeground.RequestBounded(delegate{return GameForeground.Request(next);},1000);
            Log("DAYZ_FOREGROUND bounded requested="+focused);
            Log("HANDOFF_TO_GAME_STORY nonce="+r.nonce);
            timing["reveal"]=clock.Elapsed.TotalSeconds;
            var receipt=new {nonce=r.nonce,destination=d.id,oldPid=oldId,newPid=next,zen=d.zen,world=d.world,namalskIsland=d.namalskIsland,namalskSurvival=d.namalskSurvival,status="arrived",launchMode=mode,label=config.benchmarkLabel,hasFilm=shroud.HasFilm,timing=timing};
            Write("receipt.json",receipt);Write("receipt-"+r.nonce+".json",receipt);
            Log("TIMING "+json.Serialize(timing));filmNonce=null;
            if(watch) {
                Log("TRANSFER_COMPLETE_WATCHING");
                Ui(delegate {shroud.Hide(); working=false; seenClient=true; poll.Start();});
            } else Finish("TRANSFER_COMPLETE_HELPER_EXIT");
        } catch(Exception error) {
            if(cancelled||Volatile.Read(ref finishing)!=0)return;
            Log("ERROR "+error.Message);
            try {Write("response.json",new {nonce=r==null?"":r.nonce,status="error",message=error.Message});} catch {}
            try {Write("failure.json",new {nonce=r==null?"":r.nonce,targetId=r==null?"":r.targetId,phase=phase,time=DateTime.UtcNow.ToString("o"),message=error.Message,recovery="Dismiss with Esc. If DayZ is still open, inspect it. If closed, ask the admin to check hub ownership before using the destination shortcut. Do not reset the hub transfer or return to the source."});} catch {}
            shroud.Status="Travel stopped\n"+error.Message+"\nPress Esc to dismiss";
            Ui(delegate {shroud.Failed=true;shroud.Show();});
        }
    }
}
public static class Program {
    [STAThread] public static int Main(string[] args) {
        if(args.Length==0||Array.IndexOf(args,"--setup")>=0) {
            Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);Application.Run(new SetupForm());return 0;
        }
        string configPath=Path.Combine(AppDomain.CurrentDomain.BaseDirectory,"helper-config.json");
        int index=Array.IndexOf(args,"--config"); if(index>=0 && index+1<args.Length) configPath=args[index+1];
        try {
            var json=new JavaScriptSerializer();
            Config config=json.Deserialize<Config>(File.ReadAllText(configPath));
            Protocol.Preflight(config);
            int check=Array.IndexOf(args,"--preflight");
            if(check>=0) {
                if(check+1>=args.Length)throw new InvalidOperationException("--preflight requires a report path");
                File.WriteAllText(args[check+1],json.Serialize(new {status="ready",time=DateTime.UtcNow.ToString("o"),profile=config.clientProfile,destinations=config.destinations.Select(d=>d.id).ToArray()}));
                return 0;
            }
            bool owned;
            string name="Local\\DriveLinkRestartPrototype-"+Math.Abs(config.clientProfile.ToLowerInvariant().GetHashCode());
            using(var mutex=new Mutex(true,name,out owned)) {
                if(!owned) return 0;
                Application.EnableVisualStyles(); Application.SetCompatibleTextRenderingDefault(false);
                bool vanilla=Array.IndexOf(args,"--vanilla-launcher")>=0;
                using(var context=new HelperContext(config,Array.IndexOf(args,"--one-transfer")<0&&(Array.IndexOf(args,"--watch")>=0||vanilla),vanilla)) {
                int launch=Array.IndexOf(args,"--launch");
                if(launch>=0) {
                    if(launch+1>=args.Length)throw new InvalidOperationException("--launch requires a destination ID");
                    if(Process.GetProcessesByName("DayZ_x64").Length!=0)throw new InvalidOperationException("Close the existing DayZ client normally before using the pilot shortcut");
                    if(Process.GetProcessesByName("steam").Length==0)throw new InvalidOperationException("Start Steam and sign in first");
                    var request=new Request {nonce="0-100000",targetId=args[launch+1]};
                    var destination=Protocol.Validate(config,request);
                    if(destination.host!="127.0.0.1")Protocol.CheckServer(destination);
                    string folder=Path.Combine(config.clientProfile,"DriveLinkRestart");
                    foreach(string file in new[]{"request.json","response.json","arrival.json","reveal.json","playback.json"}) {
                        string path=Path.Combine(folder,file);if(File.Exists(path))File.Delete(path);
                    }
                    Process.Start(new ProcessStartInfo(config.steamExe,Protocol.Arguments(config,destination,request)){UseShellExecute=false,CreateNoWindow=true});
                }
                Application.Run(context);
                }
            }
            return 0;
        } catch(Exception e) {
            int report=Array.IndexOf(args,"--preflight");
            if(report>=0&&report+1<args.Length) {
                try {File.WriteAllText(args[report+1],new JavaScriptSerializer().Serialize(new {status="error",message=e.Message}));}catch {}
                return 1;
            }
            MessageBox.Show(e.Message,"DriveLink restart prototype",MessageBoxButtons.OK,MessageBoxIcon.Error); return 1;
        }
    }
}
}
