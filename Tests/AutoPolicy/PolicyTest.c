modded class MissionServer
{
    override void OnMissionStart(){super.OnMissionStart();GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL049TestPolicy,5000,false);}
    void DL049Check(bool ok,string message){if(ok)Print("[DL049 TEST PASS] "+message);else Print("[DL049 TEST FAIL] "+message);}
    void DL049TestPolicy()
    {
        DL049Check(!FileExist("$profile:DriveLinkRestart/routes.json"),"no manual routes file required");
        DL049Check(!DT_PolicyKnown("livonia"),"unknown route is not classified as changed mods");
        DL049Check(DT_PolicyReason("livonia").Contains("not heard"),"no Hub reply yet is explained");
        DT_ApplyHubPolicy("{\"ok\":true,\"serverId\":\"chernarus\",\"servers\":[]}");
        DL049Check(!DT_PolicyKnown("livonia") && DT_PolicyReason("livonia").Contains("older than 0.4.9"),"pre-0.4.9 Hub reply asks for a Hub update");
        string automatic="{\"ok\":true,\"helperPolicyVersion\":1,\"helperRoutes\":[{\"targetId\":\"livonia\",\"known\":true,\"restart\":false,\"reason\":\"matching mods\"},{\"targetId\":\"namalsk\",\"known\":true,\"restart\":true,\"reason\":\"different mods\"}]}";
        DL_OnHello(true,automatic);
        DL049Check(DT_PolicyKnown("livonia") && !DT_RestartNeeded("livonia"),"hello automatically selects direct Livonia travel");
        DL049Check(DT_PolicyKnown("namalsk") && DT_RestartNeeded("namalsk"),"hello automatically selects helper for Namalsk");
        DL049Check(FileExist("$profile:DriveLinkRestart/hub-routes.json"),"Hub policy cache created automatically");
        DL049Check(!FileExist("$profile:DriveLinkRestart/routes.json"),"automatic policy never needs the legacy file");
        DL049Check(!DT_PolicyKnown("unknown-destination"),"missing destination remains unknown");
        DL049Check(DT_PolicyReason("unknown-destination").Contains("no server with ID 'unknown-destination'"),"missing destination names the ID the Hub lacks");
        DL049Check(DT_PolicyReason("Livonia").Contains("no server with ID"),"destination IDs are case-sensitive");
        float before=m_DT_Time;m_DT_Time=m_DT_HubPolicyAt+91;
        DL049Check(!DT_PolicyKnown("livonia"),"stale policy cannot approve departure");
        DL049Check(DT_PolicyReason("livonia").Contains("stale"),"stale policy explains waiting for Hub");m_DT_Time=before;
        DT_ApplyHubPolicy("{\"helperPolicyVersion\":1}");
        DL049Check(DT_PolicyKnown("livonia"),"incomplete reply cannot erase a valid cached policy");
        DL_OnLayout(true,"{\"changed\":false,\"rev\":0,\"helperPolicyVersion\":1,\"helperRoutes\":[{\"targetId\":\"livonia\",\"known\":true,\"restart\":true,\"reason\":\"override\"},{\"targetId\":\"namalsk\",\"known\":false,\"restart\":true,\"reason\":\"UDP unavailable\"}]}");
        DL049Check(DT_RestartNeeded("livonia"),"policy refresh works with unchanged map revision");
        DL049Check(!DT_PolicyKnown("namalsk"),"query failure does not become a helper requirement");
        DL049Check(DT_PolicyReason("namalsk").Contains("UDP"),"query failure reason reaches server");
        DT_Routes stored;string err;
        bool loaded=JsonFileLoader<DT_Routes>.LoadFile("$profile:DriveLinkRestart/hub-routes.json",stored,err);
        DL049Check(loaded && stored.routes.Count()==2 && stored.routes[0].restart && !stored.routes[1].known,"automatic cache contains latest statuses");
        m_DT_HasHubPolicy=false;m_DT_Routes=new DT_Routes();DT_Route legacy=new DT_Route();legacy.targetId="livonia";legacy.restart=false;m_DT_Routes.routes.Insert(legacy);
        DL049Check(DT_PolicyKnown("livonia") && !DT_RestartNeeded("livonia"),"explicit legacy entries remain compatible with older Hubs");
        legacy.restart=true;DT_ApplyHubPolicy(automatic);
        DL049Check(!DT_RestartNeeded("livonia"),"automatic Hub policy supersedes old manual entries");
        DT_ApplyHubPolicy("{\"helperPolicyVersion\":1,\"helperRoutes\":[null]}");
        DL049Check(DT_PolicyKnown("livonia"),"invalid route entries cannot erase a valid snapshot");
        DT_ApplyHubPolicy("{\"helperPolicyVersion\":1,\"helperRoutes\":[]}");
        DL049Check(!DT_PolicyKnown("livonia") && m_DT_AutoRoutes.routes.Count()==0,"explicit empty Hub policy removes obsolete routes");
        DT_ApprovedPolicy frozen=new DT_ApprovedPolicy();frozen.target="livonia";frozen.restart=false;frozen.expires=m_DT_Time+300;m_DT_Approved.Set("fixture",frozen);
        bool frozenRestart=true;
        DL049Check(!DT_UseApprovedPolicy("fixture","livonia","owner",frozenRestart),"old approval is never used for an ownership redirect");
        DL049Check(DT_UseApprovedPolicy("fixture","livonia","zone",frozenRestart) && !frozenRestart,"approved departure keeps direct policy through a later query failure");
        DL049Check(!DT_UseApprovedPolicy("fixture","livonia","zone",frozenRestart),"approved policy is consumed once");
        DT_Pending abandoned=new DT_Pending();abandoned.sid="fixture";abandoned.members.Insert("fixture");m_DT_Pending.Set("fixture",abandoned);
        DT_Warm warm=new DT_Warm();m_DT_Warm.Set("fixture",warm);DT_Ready warmReady=new DT_Ready();m_DT_Ready.Set("fixture",warmReady);m_DT_Approved.Set("fixture",frozen);
        DT_AbortUnknownHold("fixture");
        DL049Check(!m_DT_Pending.Contains("fixture") && !m_DT_Warm.Contains("fixture") && !m_DT_Ready.Contains("fixture") && !m_DT_Approved.Contains("fixture"),"query failure cancels obsolete helper setup holds before departure");
    }
}