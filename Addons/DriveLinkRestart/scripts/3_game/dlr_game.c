class DT_Readiness { string nonce; string sourceId; string targetId; }
class DT_Response { string nonce; string status; string message; }
class DLR_Request
{
    string nonce;
    string targetId;
    int remainingHops;
    bool probe;
    ref DL_Film film;
    bool benchmark;
}
class DLR_Response
{
    string nonce;
    string status;
    string message;
}
class DLR_Arrival
{
    string nonce;
    int port;
    bool zen;
    bool namalskIsland;
    bool namalskSurvival;
    string world;
}
class DLR_Playback
{
    string nonce;
    bool hasFilm;
    float elapsed;
    bool complete;
}
class DLR_Progress
{
    string nonce;
    int stage;
    string detail;
    float fraction;
}
class DLR_Reveal
{
    string nonce;
    bool ready;
    string story;
}
modded class DayZGame
{

    protected ref DLR_Request m_DLR_Request;
    protected float m_DLR_Clock;
    protected float m_DLR_Poll;
    protected float m_DLR_SentAt;
    protected float m_DLR_PlayerSince;
    protected bool m_DLR_Key;
    protected bool m_DLR_AutoSent;
    protected bool m_DLR_Arrived;
    protected bool m_DLR_TransferReady;
    protected int m_DLR_LoadStage;
    void DLR_WriteProgress(int stage, string detail, float fraction=0)
    {
        string session;
        if(IsDedicatedServer() || stage<m_DLR_LoadStage || !CommandlineGetParam("dlrSession",session))return;
        m_DLR_LoadStage=stage;
        MakeDirectory("$profile:DriveLinkRestart");
        DLR_Progress progress=new DLR_Progress();progress.nonce=session;progress.stage=stage;progress.detail=detail;progress.fraction=fraction;
        string error;JsonFileLoader<DLR_Progress>.SaveFile("$profile:DriveLinkRestart/progress.json",progress,error);
    }
    override void OnEvent(EventType eventTypeId, Param params)
    {
        super.OnEvent(eventTypeId,params);
        if(eventTypeId==ConnectingStartEventTypeID)DLR_WriteProgress(5,"Connecting to the destination server");
        if(eventTypeId==LoginTimeEventTypeID)
        {
            LoginTimeEventParams loginTimeParams;
            if(Class.CastTo(loginTimeParams,params))DLR_WriteProgress(6,"Server login countdown: "+loginTimeParams.param1.ToString()+" seconds");
        }
    }
    override void LoadProgressUpdate(int progressState, float progress, string title)
    {
        super.LoadProgressUpdate(progressState,progress,title);
        DLR_WriteProgress(4,"Loading destination game data",progress);
    }
    void DLR_ConfirmTransferReady() { m_DLR_TransferReady = true; }
    protected bool m_DLR_ProbeSent;
    protected ref DL_Film m_DLR_Film;
    protected ref array<string> m_DLR_FilmParts = new array<string>;
    protected int m_DLR_FilmId;
    protected int m_DLR_FilmGot;

    override void DL_Reconnect(UIScriptedMenu parent)
    {
        if (!m_DL_Next || m_DLR_Request)
            return;
        if (!DT_RequiresRestart(m_DL_Next.id)) { super.DL_Reconnect(parent); return; }
        // The hub already owns the transfer; leave it recoverable on helper failure.
        DLR_Begin(m_DL_Next.id, false);
    }

    void DLR_Begin(string targetId, bool probe)
    {
        MakeDirectory("$profile:DriveLinkRestart");
        DeleteFile("$profile:DriveLinkRestart/response.json");
        DeleteFile("$profile:DriveLinkRestart/arrival.json");
        m_DLR_Request = new DLR_Request();
        int nonceTime = GetTime();
        int nonceRandom = Math.RandomInt(100000,999999);
        m_DLR_Request.nonce = nonceTime.ToString() + "-" + nonceRandom.ToString();
        m_DLR_Request.targetId = targetId;
        m_DLR_Request.probe = probe;
        m_DLR_Request.film = m_DLR_Film;
        string err;
        if (!JsonFileLoader<DLR_Request>.SaveFile("$profile:DriveLinkRestart/request.json", m_DLR_Request, err))
        {
            Print("[DLR] REQUEST_WRITE_FAILED " + err);
            m_DLR_Request = null;
            return;
        }
        m_DLR_SentAt = m_DLR_Clock;
        Print("[DLR] REQUEST target=" + targetId + " nonce=" + m_DLR_Request.nonce);
        // The installed tray agent observes the nonce-bound local travel request. No browser is used.
    }

    override void OnUpdate(bool doSim, float timeslice)
    {
        super.OnUpdate(doSim,timeslice);
        if (IsDedicatedServer()) return;
        DT_Update(doSim,timeslice);
        m_DLR_Clock += timeslice;
        m_DLR_Poll += timeslice;

        if (m_DLR_Poll < 0.5) return;
        m_DLR_Poll = 0;
        if (m_DLR_Request)
        {
            DLR_Response reply;
            string err;
            if (FileExist("$profile:DriveLinkRestart/response.json") && JsonFileLoader<DLR_Response>.LoadFile("$profile:DriveLinkRestart/response.json",reply,err) && reply && reply.nonce == m_DLR_Request.nonce)
            {
                if (reply.status == "ready")
                {
                    Print("[DLR] HELPER_READY nonce=" + reply.nonce);
                    if (m_DLR_Request.probe)
                    {
                        Print("[DLR] WAKE_PROBE_SUCCESS");
                        DeleteFile("$profile:DriveLinkRestart/request.json");
                        m_DLR_Request = null;
                        RequestExit(0);
                        return;
                    }
                    DeleteFile("$profile:DriveLinkRestart/request.json");
                    m_DLR_Request = null;
                    m_DL_Next = null;
                    Print("[DLR] GRACEFUL_EXIT_FOR_TRANSFER");
                    RequestExit(0);
                    return;
                }
                if (reply.status == "error" || reply.status == "cancelled")
                {
                    Print("[DLR] HELPER_REFUSED " + reply.message);
                    m_DLR_Request = null;
                    m_DL_Next = null;
                    DL_Transition().Stop("DriveLink companion refused the restart. Your transfer remains pending; contact the server admin.");
                }
            }
            if (m_DLR_Request && m_DLR_Clock-m_DLR_SentAt > 25)
            {
                Print("[DLR] HELPER_TIMEOUT client remains open; hub transfer can be retried");
                m_DLR_Request = null;
                m_DL_Next = null;
                DL_Transition().Stop("DriveLink companion did not respond. Start the installed DriveLink tray helper; your transfer is pending. Your transfer remains pending.");
            }
        }
        if (GetPlayer() && GetMission() && !IsMissionMainMenu())
        {
            if (m_DLR_PlayerSince == 0) m_DLR_PlayerSince = m_DLR_Clock;
            string nonce;
            if (!m_DLR_Arrived && m_DLR_TransferReady && m_DLR_Clock-m_DLR_PlayerSince > 1 && CommandlineGetParam("dlrSession",nonce))
            {
                string host;
                int port;
                GetHostAddress(host,port);
                DLR_Arrival arrival = new DLR_Arrival();
                arrival.nonce = nonce;
                arrival.port = port;
                arrival.zen = ConfigIsExisting("CfgPatches ZenImmersiveLogin");
                arrival.namalskIsland = ConfigIsExisting("CfgWorlds namalsk");
                arrival.namalskSurvival = ConfigIsExisting("CfgPatches namalsk_survival");
                GetWorldName(arrival.world);
                string saveErr;
                if (JsonFileLoader<DLR_Arrival>.SaveFile("$profile:DriveLinkRestart/arrival.json",arrival,saveErr))
                {
                    m_DLR_Arrived = true;
                    Print("[DLR] ARRIVED port=" + port.ToString() + " zen=" + arrival.zen.ToString());
                }
            }

        }
    }

    override void OnRPC(PlayerIdentity sender,Object target,int rpc_type,ParamsReadContext ctx)
    {
        if (DT_OnRpc(sender,target,rpc_type,ctx)) return;
        if (rpc_type == 4810092 && IsClient())
        {
            int id, part, count;
            string chunk;
            if (!ctx.Read(id) || !ctx.Read(part) || !ctx.Read(count) || !ctx.Read(chunk)) return;
            if (count < 1 || count > 128 || part < 0 || part >= count || chunk.Length() > 900) return;
            if (id != m_DLR_FilmId || count != m_DLR_FilmParts.Count())
            {
                m_DLR_FilmId = id;
                m_DLR_FilmGot = 0;
                m_DLR_FilmParts.Clear();
                for (int i=0; i<count; i++) m_DLR_FilmParts.Insert("");
            }
            if (m_DLR_FilmParts[part] == "") m_DLR_FilmGot++;
            m_DLR_FilmParts[part] = chunk;
            if (m_DLR_FilmGot == count)
            {
                string json, err;
                foreach (string piece : m_DLR_FilmParts) json += piece;
                JsonSerializer serializer = new JsonSerializer();
                if (serializer.ReadFromString(m_DLR_Film,json,err) && m_DLR_Film)
                    Print("[DLR] JOURNEY_FILM_RECEIVED scenes=" + m_DLR_Film.events.Count().ToString());
                m_DLR_FilmParts.Clear();
                m_DLR_FilmGot = 0;
            }
            return;
        }
        super.OnRPC(sender,target,rpc_type,ctx);
    }

    static const int DT_CHALLENGE=4810191;
    static const int DT_REPLY=4810192;
    static const int DT_POLICY=4810193;
    static const int DT_MENU=4810194;
    static const int DT_ABORT=4810195;
    protected ref map<string,bool> m_DT_Restart=new map<string,bool>;
    protected ref DT_Readiness m_DT_Request;
    protected float m_DT_Clock;
    protected float m_DT_Poll;
    protected float m_DT_Since;
    protected float m_DT_Refresh;
    protected bool m_DT_Prompted;
    bool DT_RequiresRestart(string target) { bool value; return m_DT_Restart.Find(target,value) && value; }
    void DT_WriteReadiness()
    {
        if (!m_DT_Request) return;
        MakeDirectory("$profile:DriveLinkRestart");
        string err;
        JsonFileLoader<DT_Readiness>.SaveFile("$profile:DriveLinkRestart/readiness-request.json",m_DT_Request,err);
        m_DT_Refresh=m_DT_Clock;
    }
    void DT_CloseSetup()
    {
        UIScriptedMenu menu=GetUIManager().GetMenu();
        if (menu && menu.GetID()==DT_MENU) GetUIManager().Back();
    }
    void DT_AbandonSetup()
    {
        if (m_DT_Request) RPCSingleParam(null,DT_REPLY,new Param2<string,bool>(m_DT_Request.nonce,false),true);
        m_DT_Request=null;
        DeleteFile("$profile:DriveLinkRestart/readiness-request.json");
    }
    void DT_CancelSetup()
    {
        DT_AbandonSetup();DT_CloseSetup();
    }
    void DT_Update(bool doSim,float timeslice)
    {

        if (IsDedicatedServer()) return;
        m_DT_Clock+=timeslice; m_DT_Poll+=timeslice;
        if (!m_DT_Request || m_DT_Poll<0.5) return;
        m_DT_Poll=0;
        if (m_DT_Clock-m_DT_Refresh>3) DT_WriteReadiness();
        DT_Response reply; string err;
        if (FileExist("$profile:DriveLinkRestart/readiness-response.json") && JsonFileLoader<DT_Response>.LoadFile("$profile:DriveLinkRestart/readiness-response.json",reply,err) && reply && reply.nonce==m_DT_Request.nonce && reply.status=="ready")
        {
            Print("[DT] CLIENT_READY target="+m_DT_Request.targetId+" nonce="+m_DT_Request.nonce);
            RPCSingleParam(null,DT_REPLY,new Param2<string,bool>(m_DT_Request.nonce,true),true);
            m_DT_Request=null;
            DeleteFile("$profile:DriveLinkRestart/readiness-request.json");
            DT_CloseSetup();return;
        }
        if (!m_DT_Prompted && m_DT_Clock-m_DT_Since>5 && !GetUIManager().GetMenu())
        {
            m_DT_Prompted=true;GetUIManager().EnterScriptedMenu(DT_MENU,null);
            Print("[DT] SETUP_REQUIRED source character retained; departure not committed");
        }
        if (m_DT_Clock-m_DT_Since>175) DT_CancelSetup();
    }
    bool DT_OnRpc(PlayerIdentity sender,Object target,int rpc_type,ParamsReadContext ctx)
    {
        if(rpc_type==DT_ABORT && IsClient())
        {
            m_DT_Request=null;DeleteFile("$profile:DriveLinkRestart/readiness-request.json");DT_CloseSetup();return true;
        }
        if (rpc_type==DT_POLICY && IsClient())
        {
            Param2<string,bool> policy;
            if(ctx.Read(policy))m_DT_Restart.Set(policy.param1,policy.param2);
            return true;
        }
        if (rpc_type==DT_CHALLENGE && IsClient())
        {
            Param3<string,string,string> challenge;
            if(!ctx.Read(challenge))return true;
            m_DT_Request=new DT_Readiness();m_DT_Request.nonce=challenge.param3;m_DT_Request.sourceId=challenge.param1;m_DT_Request.targetId=challenge.param2;
            m_DT_Restart.Set(challenge.param2,true);m_DT_Since=m_DT_Clock;m_DT_Prompted=false;
            DeleteFile("$profile:DriveLinkRestart/readiness-response.json");DT_WriteReadiness();return true;
        }
        if(rpc_type==DT_REPLY && IsDedicatedServer() && sender)
        {
            Param2<string,bool> response;
            if(!ctx.Read(response))return true;
            Mission mission=GetMission();
            if(mission&&mission.MissionScript)mission.MissionScript.CallFunction(mission,"DT_OnResponse",null,sender.GetPlainId()+"|"+response.param1+"|"+response.param2.ToString());
            return true;
        }
        return false;
    }
}