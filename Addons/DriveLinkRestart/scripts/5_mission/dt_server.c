class DT_Route { string targetId; bool restart=true; bool known; string reason; }
class DT_Routes { ref array<ref DT_Route> routes=new array<ref DT_Route>; }
class DT_HubPolicies { int helperPolicyVersion; ref array<ref DT_Route> helperRoutes; }
class DT_Warm { string nonce;string target;float expires;float protectAt; }
class DT_SetupProtection { PlayerBase player;ref DL_Zone zone;bool restoreDamage; }
class DT_Ready { string target;float expires; }
class DT_ApprovedPolicy { string target;bool restart;float expires; }
class DT_Pending { string sid;ref DL_Zone zone;ref DL_Party party;float expires;ref array<string> members=new array<string>; }
modded class MissionServer
{
    protected ref DT_Routes m_DT_Routes;
    protected ref DT_Routes m_DT_AutoRoutes;
    protected bool m_DT_HasHubPolicy;
    protected float m_DT_HubPolicyAt;
    protected string m_DT_HubPolicySignature;
    protected ref map<string,ref DT_Warm> m_DT_Warm=new map<string,ref DT_Warm>;
    protected ref map<string,ref DT_Ready> m_DT_Ready=new map<string,ref DT_Ready>;
    protected ref map<string,ref DT_ApprovedPolicy> m_DT_Approved=new map<string,ref DT_ApprovedPolicy>;
    protected ref map<string,ref DT_Pending> m_DT_Pending=new map<string,ref DT_Pending>;
    protected ref map<DL_Party,bool> m_DT_Cancelled=new map<DL_Party,bool>;
    protected ref map<string,ref DT_SetupProtection> m_DT_Protection=new map<string,ref DT_SetupProtection>;
    protected float m_DT_Time;
    protected float m_DT_Check;
    override void DL_OnHello(bool ok,string body)
    {
        super.DL_OnHello(ok,body);
        if(ok)DT_ApplyHubPolicy(body);
    }
    override void DL_OnLayout(bool ok,string body)
    {
        super.DL_OnLayout(ok,body);
        // Policy updates are independent of the map-layout revision.
        if(ok)DT_ApplyHubPolicy(body);
    }
    void DT_ApplyHubPolicy(string body)
    {
        // Enforce may deserialize an omitted array as empty. Missing policy must not erase a valid snapshot.
        if(!body.Contains("\"helperRoutes\""))return;
        DT_HubPolicies reply;string err;JsonSerializer parser=new JsonSerializer();
        if(!parser.ReadFromString(reply,body,err)||!reply||reply.helperPolicyVersion!=1||!reply.helperRoutes)return;
        array<string> targets=new array<string>;
        foreach(DT_Route entry:reply.helperRoutes){if(!entry||entry.targetId==""||targets.Find(entry.targetId)>=0)return;targets.Insert(entry.targetId);}
        m_DT_AutoRoutes=new DT_Routes();m_DT_AutoRoutes.routes=reply.helperRoutes;
        m_DT_HasHubPolicy=true;m_DT_HubPolicyAt=m_DT_Time;
        string signature;
        foreach(DT_Route route:reply.helperRoutes)signature+=route.targetId+"|"+route.known.ToString()+"|"+route.restart.ToString()+"|"+route.reason+";";
        if(signature==m_DT_HubPolicySignature)return;
        m_DT_HubPolicySignature=signature;
        foreach(DT_Route changed:reply.helperRoutes)Print("[DT] HUB_POLICY target="+changed.targetId+" known="+changed.known.ToString()+" restart="+changed.restart.ToString()+" reason="+changed.reason);
        MakeDirectory("$profile:DriveLinkRestart");
        JsonFileLoader<DT_Routes>.SaveFile("$profile:DriveLinkRestart/hub-routes.json",m_DT_AutoRoutes,err);
    }
    DT_Route DT_FindPolicy(string target)
    {
        if(m_DT_HasHubPolicy){foreach(DT_Route automatic:m_DT_AutoRoutes.routes)if(automatic.targetId==target)return automatic;return null;}
        // Explicit legacy entries are retained only for an older Hub; unknown routes no longer imply different mods.
        if(!m_DT_Routes){string err;if(!JsonFileLoader<DT_Routes>.LoadFile("$profile:DriveLinkRestart/routes.json",m_DT_Routes,err)||!m_DT_Routes)m_DT_Routes=new DT_Routes();}
        foreach(DT_Route legacy:m_DT_Routes.routes)if(legacy.targetId==target)return legacy;
        return null;
    }
    bool DT_PolicyKnown(string target)
    {
        DT_Route route=DT_FindPolicy(target);
        if(!route)return false;
        if(m_DT_HasHubPolicy)return route.known && m_DT_Time-m_DT_HubPolicyAt<=90;
        return true;
    }
    string DT_PolicyReason(string target)
    {
        if(m_DT_HasHubPolicy && m_DT_Time-m_DT_HubPolicyAt>90)return "The Hub's mod report is stale. Wait for a fresh query, then leave and re-enter the travel zone.";
        DT_Route route=DT_FindPolicy(target);
        if(route && route.reason!="")return route.reason+". Wait for the Hub, then leave and re-enter the travel zone.";
        return "The Hub has not supplied this route's client-mod policy. Update DriveLink Hub and check its Servers page. Your character has not departed.";
    }
    void DT_ApproveCrew(PlayerBase leader,string target,bool restart)
    {
        DT_ApprovedPolicy approval=new DT_ApprovedPolicy();approval.target=target;approval.restart=restart;approval.expires=m_DT_Time+300;
        array<PlayerBase> members=DL_CrewOf(leader);
        foreach(PlayerBase member:members)if(member&&member.GetIdentity())m_DT_Approved.Set(member.GetIdentity().GetPlainId(),approval);
    }
    bool DT_UseApprovedPolicy(string sid,string target,string reason,out bool restart)
    {
        if(reason!="zone")return false;
        DT_ApprovedPolicy approval;
        if(!m_DT_Approved.Find(sid,approval))return false;
        m_DT_Approved.Remove(sid);
        if(approval.target!=target||approval.expires<m_DT_Time)return false;
        restart=approval.restart;return true;
    }
    void DT_AbortUnknownHold(string sid)
    {
        DT_Pending pending;if(!m_DT_Pending.Find(sid,pending))return;
        if(pending.party)m_DT_Cancelled.Set(pending.party,true);
        DT_EndHold(pending);
    }
    void DT_ProtectSetup(string sid,PlayerBase player,DL_Zone zone)
    {
        if(!player || !player.IsAlive())return;
        DT_SetupProtection protection;
        if(!m_DT_Protection.Find(sid,protection))
        {
            protection=new DT_SetupProtection();protection.player=player;protection.zone=zone;protection.restoreDamage=player.GetAllowDamage();m_DT_Protection.Set(sid,protection);
            Print("[DT] SETUP_PROTECTION_ON sid="+sid);
        }
        // Preserve any later request to resume damage until our setup wait ends.
        if(player.GetAllowDamage())protection.restoreDamage=true;
        player.SetAllowDamage(false);
    }
    void DT_RestoreSetup(string sid)
    {
        DT_SetupProtection protection;
        if(!m_DT_Protection.Find(sid,protection))return;
        m_DT_Protection.Remove(sid);
        if(protection.player)protection.player.SetAllowDamage(protection.restoreDamage);
        Print("[DT] SETUP_PROTECTION_OFF sid="+sid);
    }
    override protected void DL_EndProtection(PlayerBase player)
    {
        foreach(string sid,DT_SetupProtection protection:m_DT_Protection)
        {
            if(protection.player==player){protection.restoreDamage=true;return;}
        }
        super.DL_EndProtection(player);
    }
    override protected void DL_SendRedirect(PlayerIdentity id,DL_ServerInfo target,string reason)
    {
        bool approvedRestart;
        if(id && target && DT_UseApprovedPolicy(id.GetPlainId(),target.id,reason,approvedRestart))
        {
            // Once departure is accepted, a later query/policy refresh must not change its handoff.
            GetGame().RPCSingleParam(null,DayZGame.DT_POLICY,new Param2<string,bool>(target.id,approvedRestart),true,id);
            super.DL_SendRedirect(id,target,reason);return;
        }
        if(id && target && !DT_PolicyKnown(target.id))
        {
            ScriptRPC blocked=new ScriptRPC();blocked.Write("Travel not ready");blocked.Write(DT_PolicyReason(target.id));blocked.Send(null,DL_Const.RPC_KICK,true,id);return;
        }
        if(id && target)GetGame().RPCSingleParam(null,DayZGame.DT_POLICY,new Param2<string,bool>(target.id,DT_RestartNeeded(target.id)),true,id);
        super.DL_SendRedirect(id,target,reason);
    }
    bool DT_RestartNeeded(string target)
    {
        DT_Route route=DT_FindPolicy(target);if(route)return route.restart;
        return true;
    }
    bool DT_Ensure(PlayerBase pb,DL_Zone zone,DL_Party party=null)
    {
        if(!pb||!pb.GetIdentity()||!pb.IsAlive()||!zone)return false;
        string sid=pb.GetIdentity().GetPlainId();
        if(!DT_PolicyKnown(zone.to))
        {
            DT_AbortUnknownHold(sid);
            m_DL_MustLeaveZone.Set(sid,true);
            DL_CrewNotify(pb,12,"Travel not ready",DT_PolicyReason(zone.to));
            Print("[DT] POLICY_UNAVAILABLE target="+zone.to+"; source retained; no helper prompt");return false;
        }
        if(!DT_RestartNeeded(zone.to))
        {
            DT_Pending directPending;if(m_DT_Pending.Find(sid,directPending))DT_EndHold(directPending);
            array<PlayerBase> directCrew=DL_CrewOf(pb);
            foreach(PlayerBase directMember:directCrew)if(directMember&&directMember.GetIdentity())m_DL_MustLeaveZone.Remove(directMember.GetIdentity().GetPlainId());
            DT_ApproveCrew(pb,zone.to,false);
            GetGame().RPCSingleParam(null,DayZGame.DT_POLICY,new Param2<string,bool>(zone.to,false),true,pb.GetIdentity());return true;
        }
        bool allReady=true;
        array<PlayerBase> crew=DL_CrewOf(pb);
        foreach(PlayerBase member:crew)
        {
            if(!member||!member.GetIdentity())continue;
            string msid=member.GetIdentity().GetPlainId();
            DT_Ready ready;
            if(m_DT_Ready.Find(msid,ready)&&ready.target==zone.to&&ready.expires>m_DT_Time)continue;
            allReady=false;DT_Warm request;
            if(!m_DT_Warm.Find(msid,request)||request.target!=zone.to||request.expires<m_DT_Time)
            {
                request=new DT_Warm();int stamp=GetGame().GetTime();int random=Math.RandomInt(100000,999999);
                request.nonce=stamp.ToString()+"-"+random.ToString();request.target=zone.to;request.expires=m_DT_Time+180;request.protectAt=m_DT_Time+4;
                m_DT_Warm.Set(msid,request);
                GetGame().RPCSingleParam(null,DayZGame.DT_CHALLENGE,new Param3<string,string,string>(m_DL_Cfg.serverId,zone.to,request.nonce),true,member.GetIdentity());
                Print("[DT] READINESS_CHALLENGE sid="+msid+" target="+zone.to);
            }
            m_DL_MustLeaveZone.Set(msid,true);
        }
        if(allReady)
        {
            DT_ApproveCrew(pb,zone.to,true);
            if(!party)
            {
                m_DT_Pending.Remove(sid);
                foreach(PlayerBase allowed:crew)if(allowed&&allowed.GetIdentity())
                {
                    string allowedSid=allowed.GetIdentity().GetPlainId();DT_RestoreSetup(allowedSid);m_DL_MustLeaveZone.Remove(allowedSid);
                }
            }
            return true;
        }
        DT_Pending pending;
        if(!m_DT_Pending.Find(sid,pending))
        {
            pending=new DT_Pending();pending.sid=sid;pending.zone=zone;pending.party=party;pending.expires=m_DT_Time+180;m_DT_Pending.Set(sid,pending);
            foreach(PlayerBase held:crew)if(held&&held.GetIdentity())pending.members.Insert(held.GetIdentity().GetPlainId());
            Print("[DT] DEPARTURE_HELD sid="+sid+"; no hub departure or source deletion");
        }
        return false;
    }
    override protected void DL_DepartParty(DL_Party party)
    {
        if(!party||m_DT_Cancelled.Contains(party))return;
        bool ready=true;
        foreach(string sid:party.leaders)if(!DT_Ensure(DL_FindPlayer(sid),party.zone,party))ready=false;
        if(ready)
        {
            foreach(string leader:party.leaders)
            {
                m_DT_Pending.Remove(leader);
                array<PlayerBase> crew=DL_CrewOf(DL_FindPlayer(leader));
                foreach(PlayerBase member:crew)if(member&&member.GetIdentity())
                {
                    string memberSid=member.GetIdentity().GetPlainId();DT_RestoreSetup(memberSid);m_DL_MustLeaveZone.Remove(memberSid);
                }
            }
            Print("[DT] PARTY_READY; allowing normal departure");super.DL_DepartParty(party);
        }
        else foreach(string waiting:party.leaders)m_DL_MustLeaveZone.Set(waiting,true);
    }
    override protected void DL_Depart(PlayerBase pb,DL_Zone z)
    {
        if(!DT_Ensure(pb,z))return;
        Print("[DT] DEPARTURE_READY; allowing normal departure");super.DL_Depart(pb,z);
    }
    void DT_OnResponse(string payload)
    {
        array<string> fields=new array<string>;payload.Split("|",fields);if(fields.Count()!=3)return;
        string sid=fields[0];string nonce=fields[1];bool readyResult=(fields[2]=="1" || fields[2]=="true");
        DT_Warm request;
        if(!m_DT_Warm.Find(sid,request)||request.nonce!=nonce||request.expires<m_DT_Time)return;
        if(readyResult)
        {
            DT_Ready ready=new DT_Ready();ready.target=request.target;ready.expires=m_DT_Time+12;m_DT_Ready.Set(sid,ready);
            Print("[DT] READINESS_ACK sid="+sid);return;
        }
        m_DT_Warm.Remove(sid);m_DT_Ready.Remove(sid);
        array<string> leaders=m_DT_Pending.GetKeyArray();
        foreach(string leader:leaders)
        {
            DT_Pending p;if(!m_DT_Pending.Find(leader,p))continue;
            if(p.members.Find(sid)>=0)
            {
                if(p.party)m_DT_Cancelled.Set(p.party,true);
                DT_EndHold(p);Print("[DT] SETUP_CANCELLED; source character retained");
            }
        }
    }
    void DT_EndHold(DT_Pending p)
    {
        foreach(string member:p.members)
        {
            DT_RestoreSetup(member);
            m_DT_Warm.Remove(member);m_DT_Ready.Remove(member);
            m_DT_Approved.Remove(member);
            PlayerBase pb=DL_FindPlayer(member);
            if(pb&&pb.GetIdentity())GetGame().RPCSingleParam(null,DayZGame.DT_ABORT,new Param1<bool>(true),true,pb.GetIdentity());
        }
        m_DT_Pending.Remove(p.sid);
    }
    override void OnMissionFinish()
    {
        array<string> members=m_DT_Protection.GetKeyArray();
        foreach(string sid:members)DT_RestoreSetup(sid);
        super.OnMissionFinish();
    }
    override void OnUpdate(float timeslice)
    {
        m_DT_Time+=timeslice;super.OnUpdate(timeslice);m_DT_Check+=timeslice;
        if(m_DT_Check<1)return;m_DT_Check=0;
        array<string> approvedMembers=m_DT_Approved.GetKeyArray();
        foreach(string approvedSid:approvedMembers){DT_ApprovedPolicy oldApproval=m_DT_Approved.Get(approvedSid);if(!oldApproval||oldApproval.expires<m_DT_Time)m_DT_Approved.Remove(approvedSid);}
        array<string> leaders=m_DT_Pending.GetKeyArray();
        foreach(string sid:leaders)
        {
            DT_Pending p;if(!m_DT_Pending.Find(sid,p))continue;
            PlayerBase player=DL_FindPlayer(sid);
            if(!player||!player.IsAlive()||!player.GetIdentity()||p.expires<m_DT_Time||DL_ZoneFor(player)!=p.zone||(p.party&&m_DT_Cancelled.Contains(p.party)))
            {
                if(p.party)m_DT_Cancelled.Set(p.party,true);
                DT_EndHold(p);Print("[DT] SETUP_HOLD_ENDED; no departure committed");continue;
            }
            // Start just before the client's five-second first-time setup prompt.
            // An already-running helper normally acknowledges before this point.
            foreach(string memberSid:p.members)
            {
                DT_Warm warm;PlayerBase member=DL_FindPlayer(memberSid);
                if(member&&DL_ZoneFor(member)==p.zone&&m_DT_Warm.Find(memberSid,warm)&&m_DT_Time>=warm.protectAt)DT_ProtectSetup(memberSid,member,p.zone);
                else DT_RestoreSetup(memberSid);
            }
            if(p.party)DL_DepartParty(p.party);
            else if(DT_Ensure(player,p.zone))DL_Depart(player,p.zone);
        }
        // Restore even if a crew member disconnects or leaves independently.
        array<string> protectedMembers=m_DT_Protection.GetKeyArray();
        foreach(string protectedSid:protectedMembers)
        {
            DT_SetupProtection active;DT_Warm activeWarm;
            if(!m_DT_Protection.Find(protectedSid,active))continue;
            if(!active.player||!active.player.GetIdentity()||!active.player.IsAlive()||!m_DT_Warm.Find(protectedSid,activeWarm)||activeWarm.expires<m_DT_Time||DL_ZoneFor(active.player)!=active.zone)DT_RestoreSetup(protectedSid);
        }
    }
}
