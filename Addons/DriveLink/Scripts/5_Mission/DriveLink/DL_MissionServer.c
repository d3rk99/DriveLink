// DriveLink - server side: arrivals, departures, zone watching.
// Vanilla login order (5_mission/mission/missionserver.c:300-365): ClientPrepare -> OnClientPrepareEvent (:436) ->
// ClientNew -> OnClientNewEvent (:536, builds a NEW character) or ClientReady (:586, existing character from the
// game's own database). The hub answer comes back asynchronously, so we ask it in the Prepare step (the client is
// still loading then) and use the answer in New/Ready. Same idea as DaemonForge MapLink (OnClientPrepareEvent loads,
// ClientNew builds) - not its code (AGPL).
// Departure: vanilla HandleBody kills restrained/unconscious players who log out (:713-751). A traveller must
// leave NOTHING behind (or its gear would exist twice), so the old character is deleted, not killed.

// What the hub said about a player who is logging in.
// action: "" = no answer yet, "fetching" = transfer offered, snapshot on its way (/api/payload),
//         "transfer" = snapshot parsed, build him from it, "failed" = the transfer can't be built here,
//         "redirect" | "normal" = as the hub said.
class DL_Pending
{
	string action;
	string reason;             // "failed": why
	bool loggedIn;             // the game made/loaded his character before the answer came (late answer)
	ref DL_ArriveResp resp;
}

// M3: a car leaving with passengers. Their trips go first; the driver's (with the car) when all are answered.
class DL_CrewTrip
{
	string driverSid;
	Transport car;
	ref DL_Zone zone;
	int waiting;                               // passenger answers still to come
	int total;                                 // players in the car, driver included
	ref DL_TravelPrice price;                  // what the trip costs each of them (null = free)
	ref DL_Journey journey;                    // weakened mode: the road, played once for all of them
	ref DL_Film film;                          // the road film (any trip with a distance)
	ref array<string> refused = new array<string>;
}

// Group travel (Research/group-travel.md): players who come into the same zone while its countdown runs leave
// together - a group on foot (up to 8) or a convoy (up to 4 cars, each with its crew). Every newcomer starts the
// countdown again. One journey for all of them, they arrive together; the cars park one behind the other.
class DL_Party
{
	int id;
	ref DL_Zone zone;
	bool convoy;                                    // vehicles (each with its crew) / on foot
	string kind;                                    // "foot" | "land" (cars, trucks, motorbikes) | "water" (boats): only one kind travels together
	ref array<string> leaders = new array<string>;  // drivers (convoy) or walkers, in the order they came
	ref map<string, string> prices = new map<string, string>; // leader -> his trip's price text
	int since;                                      // g_Game.GetTime() the countdown (re)started
}

// M3, target side: a passenger who arrived before his driver's car exists here.
class DL_SeatWait
{
	string group;                              // the driver's steam id
	int seat;
	int since;                                 // g_Game.GetTime() when he arrived
}

// M3 waiting room, target side: one per car crew (key = the driver's steam id). Each member is built on foot beside
// the spot where the car will stand, black screen, no damage. When all who were in the car are here (or after
// crewWaitSeconds) the car is built, everyone is seated, the driver's engine starts and the screen clears.
class DL_Room
{
    bool finalConstructed;
    bool ShouldRetrySeat(string sid, int now, bool serverSeated)
    {
        if(!deliveryVisible.Contains(sid))return false;
        if(serverSeated && deliveryAcks.Contains(sid))return false;
        return now>=deliverySeatRetry.Get(sid);
    }
    bool staged;
    vector finalCarPos;
    float finalCarYaw;
    vector finalLandPos;
    float finalLandYaw;
    int deliveryEpoch;
    int deliveryAfter;
    vector deliveryPos;
    vector deliveryOri;
    int deliverySeatAfter;
    bool deliveryCorrectionSent;
    ref map<string,bool> deliveryAcks = new map<string,bool>;
    ref map<string,bool> deliveryVisible = new map<string,bool>;
    ref map<string,int> deliverySeatRetry = new map<string,int>;
    string deliveryBlocked;
	string group;                                   // the driver's steam id
	int expected;                                   // players in the car when it left (driver included)
	ref array<string> here = new array<string>;     // members built here so far
	ref map<string, int> seats = new map<string, int>;
	ref DL_CarData car;                             // from the driver's snapshot; null = driver not here yet
	string driverToken;                             // his transfer is confirmed only once the car exists
	vector carPos;
	float carYaw;
	int since;                                      // g_Game.GetTime() when the first member came
	bool going;                                     // car being built / crew being seated
	string story;                                   // what happened on the road (weakened mode), shown on the black screen
	float filmSeconds;                              // the longest road film a member is watching
	int revealAt;                                   // g_Game.GetTime(): not before the last member's film / story ends
	string convoyId;                                // a convoy car: its convoy (rooms of a convoy are revealed together)
	int convoyCars;
	bool convoyReady;                               // built and seated, waiting for the rest of the convoy
	bool boat;                                      // a boat: the crew waits on land, the boat goes on the water
	bool air;                                       // a plane: the crew waits on land, the plane is built in the air
	vector landPos;                                 // a boat's crew: where they wait (the arrival point)
	float landYaw;
	int seatedAt;                                   // g_Game.GetTime() when everyone is seated (0 = not yet going)
	Transport builtCar;
}

// target side: the cars of one convoy, each in its own waiting room; the black screen lifts for all at once
class DL_ConvoyArrival
{
    bool releasing;
	string id;
	int expected;                                   // cars that arrive (the ones left on the road don't)
	ref map<string, Transport> ready = new map<string, Transport>; // room group -> its car, built and seated
	int firstReady;                                 // g_Game.GetTime() the first car was ready (give-up timer)
}

modded class MissionServer
{
    protected ref map<string, ref DL_ArrivalGuard> m_DL_ArrivalGuards = new map<string, ref DL_ArrivalGuard>;
    protected ref map<Object, ref DL_ArrivalGuard> m_DL_GuardOf = new map<Object, ref DL_ArrivalGuard>;

    void DL_GuardObject(DL_ArrivalGuard guard, Object entity)
    {
        if(!guard || !entity)return;
        if(!guard.originalDamage.Contains(entity))guard.originalDamage.Set(entity,entity.GetAllowDamage());
        m_DL_GuardOf.Set(entity,guard);entity.SetAllowDamage(false);
    }
    void DL_GuardJoin(PlayerBase player, DL_PlayerData data, Transport car = null)
    {
        if(!player || !player.GetIdentity() || !data)return;
        string sid=player.GetIdentity().GetPlainId();string key=data.partyId;
        if(key=="")key=data.convoyId;
        if(key=="")key=data.carGroup;
        if(key=="")key=sid;
        DL_ArrivalGuard guard=m_DL_ArrivalGuards.Get(key);
        if(!guard){guard=new DL_ArrivalGuard();guard.timeoutAt=GetGame().GetTime()+Math.Max(120,m_DL_Cfg.crewWaitSeconds)*1000;m_DL_ArrivalGuards.Set(key,guard);}
        guard.expected=Math.Max(guard.expected,Math.Max(1,data.crewCount));
        if(data.film)guard.expected=Math.Max(guard.expected,data.film.crew.Count());
        guard.expiresAt=0;guard.members.Set(sid,-1);
        DL_GuardObject(guard,player);DL_GuardObject(guard,car);
    }
    void DL_GuardReady(PlayerBase player, int delayMs = 0)
    {
        DL_ArrivalGuard guard=m_DL_GuardOf.Get(player);
        if(guard && player && player.GetIdentity())guard.members.Set(player.GetIdentity().GetPlainId(),GetGame().GetTime()+delayMs);
    }
    void DL_GuardVehicle(DL_Room room, Transport car)
    {
        foreach(string member:room.here){PlayerBase player=DL_FindPlayer(member);DL_ArrivalGuard guard=m_DL_GuardOf.Get(player);if(guard){DL_GuardObject(guard,car);return;}}
    }
    void DL_GuardTick()
    {
        int now=GetGame().GetTime();array<string> finished=new array<string>;
        foreach(string key,DL_ArrivalGuard guard:m_DL_ArrivalGuards)
        {
            bool pending=false;
            foreach(string sid,int readyAt:guard.members){PlayerBase member=DL_FindPlayer(sid);if(member && member.IsAlive() && (readyAt<0 || now<readyAt))pending=true;}
            // Never time out an active assembly. Missing travellers use the existing crew timeout.
            int previousDeadline=guard.expiresAt;
            bool expired=guard.Expired(now,pending,m_DL_Cfg.arrivalProtectSeconds);
            if(previousDeadline==0 && guard.expiresAt>0)DL_Log.Info("arrival protection: party ready; grace period started");
            if(!expired)continue;
            foreach(Object entity,bool allowed:guard.originalDamage){if(entity){entity.SetAllowDamage(allowed);m_DL_GuardOf.Remove(entity);}}
            finished.Insert(key);
        }
        foreach(string done:finished)m_DL_ArrivalGuards.Remove(done);
    }

    float DL_DistanceXZ(vector a,vector b){a[1]=0;b[1]=0;return vector.Distance(a,b);}
    bool DL_StagingHasObservers(vector at,string ownGroup)
    {
        array<Man> people=new array<Man>;GetGame().GetPlayers(people);
        foreach(Man person:people)
        {
            if(!person.GetIdentity() || person.GetIdentity().GetPlainId()==ownGroup)continue;
            bool traveller=false;
            foreach(string roomKey,DL_Room waiting:m_DL_Rooms)if(waiting.here.Find(person.GetIdentity().GetPlainId())>=0)traveller=true;
            if(!traveller && DL_DistanceXZ(person.GetPosition(),at)<100)return true;
        }
        return false;
    }
    bool DL_AssemblyGroundSpot(vector want,float yaw,DL_PlayerData d,string group,float spacing,out vector found)
    {
        string type="Truck_01_Covered";if(d.car && d.car.type!="")type=d.car.type;
        float width,length;DL_CarBox(type,width,length);
        vector forward=Vector(yaw,0,0).AnglesToVector();vector right=Vector(yaw+90,0,0).AnglesToVector();
        for(int slot=0;slot<24;slot++)
        {
            int lane=slot/8;int back=slot-lane*8;
            vector candidate=want-forward*(spacing*back)+right*(spacing*lane);candidate[1]=GetGame().SurfaceY(candidate[0],candidate[2]);
            bool reserved=false;
            foreach(string key,DL_Room other:m_DL_Rooms)if(key!=group && DL_DistanceXZ(candidate,other.carPos)<12)reserved=true;
            if(!reserved && !DL_StagingHasObservers(candidate,group) && DL_SpotFree(candidate,yaw,width,length)){found=candidate;return true;}
        }
        return false;
    }
    void DL_SendStageControl(DL_Room room,Transport car,bool pause,int epoch)
    {
        if(!room || !car)return;
        foreach(string member:room.here)
        {
            PlayerBase player=DL_FindPlayer(member);if(!player || !player.GetIdentity())continue;
            vector position=car.GetPosition();vector orientation=car.GetOrientation();
            if(epoch>0){position=room.deliveryPos;orientation=room.deliveryOri;}
            ScriptRPC rpc=new ScriptRPC();rpc.Write(car);rpc.Write(position);rpc.Write(orientation);rpc.Write(pause);rpc.Write(epoch);rpc.Write(room.staged);
            rpc.Send(player,DL_Const.RPC_STAGE_CONTROL,true,player.GetIdentity());
        }
    }
    void DL_StageAck(string payload)
    {
        array<string> parts=new array<string>;payload.Split("|",parts);if(parts.Count()!=2)return;
        int epoch=parts[1].ToInt();
        foreach(string key,DL_Room room:m_DL_Rooms)
        {
            if(room.deliveryEpoch>0 && room.deliveryEpoch==epoch && room.here.Find(parts[0])>=0)room.deliveryAcks.Set(parts[0],true);
        }
    }
    void DL_StageVisible(string payload)
    {
        array<string> parts=new array<string>;payload.Split("|",parts);if(parts.Count()!=2)return;
        int epoch=parts[1].ToInt();
        foreach(string key,DL_Room room:m_DL_Rooms)
        {
            if(room.deliveryEpoch>0 && room.deliveryEpoch==epoch && room.here.Find(parts[0])>=0)room.deliveryVisible.Set(parts[0],true);
        }
    }
    bool DL_StageCrewReady(DL_Room room)
    {
        if(!room.builtCar)return true;
        if(room.staged && GetGame().GetTime()<room.deliverySeatAfter)return false;
        foreach(string sid:room.here)
        {
            PlayerBase pb=DL_FindPlayer(sid);if(!pb || !pb.IsAlive() || !pb.GetIdentity())continue;
            int now=GetGame().GetTime();
            if(!room.deliveryVisible.Contains(sid))
            {
                room.deliveryBlocked="Loading the destination vehicle on your client...";
                if(now>=room.deliverySeatRetry.Get(sid))
                {
                    // Do not possess the car until the unseated player's client can resolve it.
                    if(room.builtCar.CrewMemberIndex(pb)<0)pb.ForceCorrection();
                    room.deliverySeatRetry.Set(sid,now+3000);
                    DL_Log.Info("arrival awaiting client vehicle visibility: "+sid);
                }
                return false;
            }
            if(!room.deliveryAcks.Contains(sid) || room.builtCar.CrewMemberIndex(pb)<0)
            {
                room.deliveryBlocked="Seating and confirming your client...";
                if(room.ShouldRetrySeat(sid,now,room.builtCar.CrewMemberIndex(pb)>=0))
                {
                    DL_SeatTraveller(sid,room.builtCar,room.seats.Get(sid));
                    room.deliverySeatRetry.Set(sid,now+3000);
                    DL_Log.Info("arrival seat retry awaiting client acknowledgement: "+sid);
                }
                return false;
            }
        }
        return true;
    }
    bool DL_FinalDeliverySpot(DL_Room room,map<string,vector> planned,out vector result)
    {
        vector forward=Vector(room.finalCarYaw,0,0).AnglesToVector();vector right=Vector(room.finalCarYaw+90,0,0).AnglesToVector();
        float width,length;DL_CarBox(room.car.type,width,length);
        for(int slot=0;slot<25;slot++)
        {
            int lane=slot/5;int back=slot-lane*5;
            float side=lane*6;if(lane==2 || lane==4)side=-side;
            vector p=room.finalCarPos-forward*(12*back)+right*side;p[1]=GetGame().SurfaceY(p[0],p[2]);
            bool reserved=false;
            foreach(string plannedGroup,vector reservedPos:planned)if(DL_DistanceXZ(p,reservedPos)<12)reserved=true;
            if(reserved)continue;
            if(room.boat){if(!DL_WaterFree(p,room.finalCarYaw))continue;p=DL_OnWater(p);}
            else if(!DL_SpotFree(p,room.finalCarYaw,width,length))continue;
            result=p;return true;
        }
        return false;
    }
    void DL_ReleaseArrivalBatch(array<string> candidates)
    {
        array<string> handled=new array<string>;
        foreach(string first:candidates)
        {
            DL_Room initial=m_DL_Rooms.Get(first);if(!initial)continue;
            string batchId=initial.convoyId;if(batchId=="")batchId=first;
            if(handled.Find(batchId)>=0)continue;handled.Insert(batchId);
            array<string> batch=new array<string>;
            foreach(string next:candidates){DL_Room matching=m_DL_Rooms.Get(next);if(!matching)continue;string matchId=matching.convoyId;if(matchId=="")matchId=next;if(matchId==batchId)batch.Insert(next);}
            DL_DeliverReadyBatch(batch);
        }
    }
    void DL_DeliverReadyBatch(array<string> batch)
    {
        ref map<string,vector> planned=new map<string,vector>;
        // Reserve every destination before moving any member of this convoy.
        foreach(string planGroup:batch)
        {
            DL_Room plan=m_DL_Rooms.Get(planGroup);if(!plan || plan.deliveryEpoch>0 || plan.air)continue;
            if(!plan.builtCar && (!plan.staged || !plan.car))continue;
            vector destination=plan.finalCarPos;if(plan.builtCar)destination=plan.builtCar.GetPosition();
            if(plan.staged && plan.builtCar && plan.builtCar.HasCrew())
            {
                plan.deliveryBlocked="Staging vehicle is unexpectedly occupied; waiting for it to be cleared before delivery.";
                return;
            }
            if(plan.staged && !plan.finalConstructed && !DL_FinalDeliverySpot(plan,planned,destination))
            {
                foreach(string blockedGroup:batch){DL_Room blocked=m_DL_Rooms.Get(blockedGroup);if(blocked)blocked.deliveryBlocked="Final arrival is obstructed. Waiting safely in staging for a clear spot.";}
                return;
            }
            planned.Set(planGroup,destination);
        }
        // Create each vehicle once, directly at its reserved destination. No network vehicle ever
        // exists at staging, so there is no staging-origin owner/proxy state to replay later.
        foreach(string buildGroup:batch)
        {
            DL_Room toBuild=m_DL_Rooms.Get(buildGroup);
            if(!toBuild || !toBuild.staged || toBuild.deliveryEpoch>0 || toBuild.finalConstructed || !toBuild.car)continue;
            if(!DL_CreateFinalRoomCar(buildGroup,toBuild,planned.Get(buildGroup)))
            {
                toBuild.deliveryBlocked="Could not restore the destination vehicle. Remaining protected at staging for retry.";
                return;
            }
        }
        foreach(string moveGroup:batch)
        {
            DL_Room moving=m_DL_Rooms.Get(moveGroup);if(!moving || moving.deliveryEpoch>0 || !moving.builtCar || moving.air)continue;
            Transport vehicle=moving.builtCar;
            if(!moving.staged){vehicle.DisableSimulation(true);dBodyActive(vehicle,ActiveState.INACTIVE);SetVelocity(vehicle,vector.Zero);dBodySetAngularVelocity(vehicle,vector.Zero);}
            if(moving.staged)
            {
                moving.carPos=vehicle.GetPosition();moving.carYaw=moving.finalCarYaw;moving.landPos=moving.finalLandPos;moving.landYaw=moving.finalLandYaw;
            }
            if(!moving.staged)DL_ArrivalPose.Commit(vehicle,vehicle.GetPosition(),vehicle.GetOrientation());
            if(moving.staged)
            {
                DL_MoveRoomCrew(moving);
                moving.deliverySeatAfter=GetGame().GetTime()+2000;
                DL_Log.Info("arrival delivery: destination-born vehicle; crew moved independently for final seating");
            }
            moving.deliveryPos=vehicle.GetPosition();moving.deliveryOri=vehicle.GetOrientation();
            vehicle.SetSynchDirty();moving.deliveryEpoch=Math.Max(1,GetGame().GetTime());moving.deliveryAfter=GetGame().GetTime()+2500;
            moving.deliveryBlocked="Arrival prepared. Synchronizing the crew...";
            DL_SendStageControl(moving,vehicle,true,moving.deliveryEpoch);
            DL_Log.Info("arrival delivery "+moveGroup+" epoch "+moving.deliveryEpoch.ToString()+" at "+vehicle.GetPosition().ToString());
        }
        foreach(string waitGroup:batch)
        {
            DL_Room waiting=m_DL_Rooms.Get(waitGroup);if(!waiting || !waiting.builtCar || waiting.air)continue;
            if(waiting.staged)
            {
                // Observe natural settling. No repeated teleport or forced owner rewind.
                waiting.builtCar.Update();waiting.deliveryPos=waiting.builtCar.GetPosition();waiting.deliveryOri=waiting.builtCar.GetOrientation();
            }
            else DL_ArrivalPose.Commit(waiting.builtCar,waiting.deliveryPos,waiting.deliveryOri);
            if(!waiting.staged && waiting.builtCar.CrewDriver() && !waiting.deliveryCorrectionSent)
            {
                waiting.builtCar.ForceCorrection();waiting.deliveryCorrectionSent=true;
            }
            DL_SendStageControl(waiting,waiting.builtCar,true,waiting.deliveryEpoch);
            if(GetGame().GetTime()<waiting.deliveryAfter || !DL_StageCrewReady(waiting))return;
        }
        foreach(string revealGroup:batch)
        {
            DL_Room ready=m_DL_Rooms.Get(revealGroup);if(!ready)continue;
            // Failed vehicle creation must not strand a restored crew in an assembly area.
            if(ready.staged && !ready.builtCar)
            {
                vector foot=DL_LandNear(ready.finalLandPos,revealGroup);
                foreach(string walker:ready.here){PlayerBase survivor=DL_FindPlayer(walker);if(survivor)survivor.SetPosition(foot);}
            }
            DL_RoomReveal(revealGroup,ready.builtCar);
        }
    }

    bool DL_CreateFinalRoomCar(string group,DL_Room room,vector position)
    {
        if(room.finalConstructed)return room.builtCar!=null;
        int made,dropped;m_DL_CarSnap.holdArrival=false;m_DL_CarSnap.surfaceArrival=true;
        Transport car=m_DL_CarSnap.Build(room.car,position,Vector(room.finalCarYaw,0,0),made,dropped);
        m_DL_CarSnap.holdArrival=false;m_DL_CarSnap.surfaceArrival=false;
        if(!car){DL_Log.Warn("arrival final vehicle creation failed; snapshot retained for "+group);return false;}
        room.builtCar=car;room.finalConstructed=true;
        DL_GuardVehicle(room,car);car.SetAllowDamage(false);DL_Veh.SetHidden(car,false);
        DL_ArrivalPose.Resume(car);
        m_DL_GroupCar.Set(group,car);
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_ForgetGroupCar,m_DL_Cfg.crewWaitSeconds*2000,false,group,car);
        if(room.driverToken!=""){DL_Confirm(group,room.driverToken);room.driverToken="";}
        DL_Log.Info("arrival vehicle born at final point: "+car.GetNetworkIDString()+" "+car.GetPosition().ToString()+", restored "+made.ToString()+", dropped "+dropped.ToString());
        return true;
    }


	protected ref map<string, ref DL_Room> m_DL_Rooms = new map<string, ref DL_Room>;
	protected bool m_DL_RoomTicking;
	protected ref map<string, ref DL_CrewTrip> m_DL_TripOf = new map<string, ref DL_CrewTrip>; // passenger sid -> his car trip
	protected ref map<string, Transport> m_DL_GroupCar = new map<string, Transport>;         // driver sid -> the car he brought here
	protected ref map<string, ref DL_SeatWait> m_DL_SeatWait = new map<string, ref DL_SeatWait>; // passenger sid -> waiting for that car
	protected ref DL_Config m_DL_Cfg;
	protected ref map<string, ref DL_Film> m_DL_TransitionOf = new map<string, ref DL_Film>;
	protected ref map<string, ref DL_Pending> m_DL_Pending = new map<string, ref DL_Pending>;    // steamId -> hub answer
	protected ref array<ref DL_Party> m_DL_Parties = new array<ref DL_Party>;                   // forming in the zones
	protected int m_DL_PartyN;
	protected ref map<string, int> m_DL_SlotOf = new map<string, int>;                          // refuse mode: convoy place / group place of a leaving player
	protected ref map<string, ref DL_ConvoyArrival> m_DL_Convoys = new map<string, ref DL_ConvoyArrival>;
	protected ref map<string, string> m_DL_PartyIdOf = new map<string, string>;   // leaving sid -> his party's id
	protected ref map<string, string> m_DL_FilmKey = new map<string, string>;     // arrived sid -> the film he watches (party id or his own sid)
	protected ref map<string, ref array<string>> m_DL_FilmVoted = new map<string, ref array<string>>; // film key -> who voted to skip
	protected ref map<string, bool> m_DL_StoryCut = new map<string, bool>;        // a skipped black screen: no more WAIT_SHOW
	protected ref map<string, string> m_DL_ConvoyOf = new map<string, string>;  // leaving sid -> his convoy's id
	protected ref map<string, int> m_DL_ConvoyCarsOf = new map<string, int>;
	static const int DL_CONVOY_MAX = 4;
	static const int DL_GROUP_MAX = 8;
	protected ref map<string, bool> m_DL_Leaving = new map<string, bool>;  // steamIds being sent away (zone check skips them)
	protected ref map<string, bool> m_DL_MustLeaveZone = new map<string, bool>; // trip failed: no new countdown until he steps out
	static const float DL_AIR_HEIGHT = 300;      // a plane comes in this high above the ground (plane entry not set)
	static const float DL_AIR_MIN_SPEED = 45;    // m/s (162 km/h): at least this, so it doesn't stall
	protected ref map<string, ref DL_ServerInfo> m_DL_GoHome = new map<string, ref DL_ServerInfo>; // died here, not the starting server: his respawn goes there
	protected ref map<PlayerBase, bool> m_DL_Doomed = new map<PlayerBase, bool>; // characters that live elsewhere now: delete + CharacterKill on logout
	protected float m_DL_Tick;
	protected ref DL_Snapshot m_DL_Snap = new DL_Snapshot();
	protected string m_DL_BuildError;      // why the last DL_BuildArrival returned null
	protected ref DL_Debug m_DL_Debug;     // debug.json, null when the file is not there
	protected ref DL_CarSnapshot m_DL_CarSnap = new DL_CarSnapshot();
	protected ref map<Transport, int> m_DL_CarInZoneSince = new map<Transport, int>;
	protected ref map<Transport, bool> m_DL_CarDone = new map<Transport, bool>; // rebuilt cars: not again until they leave the circle
	protected ref map<string, Transport> m_DL_TravelCar = new map<string, Transport>; // steamId -> the car he is taking along, until the hub answers

	override void OnMissionStart()
	{
		super.OnMissionStart();
		DL_LoadConfig();
	}

	protected void DL_LoadConfig()
	{
		MakeDirectory(DL_Const.DIR);
		m_DL_Cfg = new DL_Config();
		string err;
		if (FileExist(DL_Const.CONFIG))
		{
			DL_Config loaded;
			if (JsonFileLoader<DL_Config>.LoadFile(DL_Const.CONFIG, loaded, err) && loaded)
				m_DL_Cfg = loaded;
			else
				DL_Log.Warn("config.json unreadable (" + err + "), DriveLink is OFF");
		}
		else
		{
			// zones + arrival points are drawn in the hub's web app; only the hub's address + key go here
			JsonFileLoader<DL_Config>.SaveFile(DL_Const.CONFIG, m_DL_Cfg, err);
			DL_Log.Info("wrote " + DL_Const.CONFIG + " - put in hubUrl + hubKey (the hub's web app, Setup page shows both), restart, then approve this server on the hub's Servers page");
		}
		// a config.json from before a setting existed loads it as 0
		if (m_DL_Cfg.crewWaitSeconds <= 0)
			m_DL_Cfg.crewWaitSeconds = 120;
		if (!m_DL_Cfg.travel)
			m_DL_Cfg.travel = new DL_TravelRules();
		if (m_DL_Cfg.joinId == "")
		{
			Math.Randomize(-1);
			m_DL_Cfg.joinId = Math.RandomInt(0, int.MAX).ToString() + "-" + Math.RandomInt(0, int.MAX).ToString() + "-" + g_Game.GetTime().ToString();
			JsonFileLoader<DL_Config>.SaveFile(DL_Const.CONFIG, m_DL_Cfg, err);
		}
		DL_CarLedger.Start(m_DL_Cfg.carEconomy, m_DL_Cfg.serverId);
		if (m_DL_Cfg.hubKey != "")
			DL_SendHello();
#ifdef DL_DEV
		DL_LoadDebug();                 // debug.json test switches: dev builds only
#endif
		// log level: "quiet" (warnings only) | "normal" | "verbose"; a config.json from before it has none = normal
		if (m_DL_Cfg.log == "")
		{
			m_DL_Cfg.log = "normal";
			JsonFileLoader<DL_Config>.SaveFile(DL_Const.CONFIG, m_DL_Cfg, err);
		}
		DL_Log.s_Level = 1;
		if (m_DL_Cfg.log == "quiet")
			DL_Log.s_Level = 0;
		else if (m_DL_Cfg.log == "verbose")
			DL_Log.s_Level = 2;
		string dlMsg184 = "v" + DL_Const.VERSION + " server '" + m_DL_Cfg.serverId;
		dlMsg184 += "', hub " + m_DL_Cfg.hubUrl + ", " + m_DL_Cfg.zones.Count().ToString();
		dlMsg184 += " zones, " + m_DL_Cfg.arrivals.Count().ToString() + " arrival points, key set " + (m_DL_Cfg.hubKey != "").ToString();
		DL_Log.Info(dlMsg184);
		// once the world is up: are the arrival points' car spots (and a convoy's line behind them) free?
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_CheckCarSpots, 20000, false);
	}

	// ---------------- M2 spikes (debug.json only) ----------------

	protected void DL_LoadDebug()
	{
		if (!FileExist(DL_Const.DEBUG_FILE))
			return;
		string err;
		if (!JsonFileLoader<DL_Debug>.LoadFile(DL_Const.DEBUG_FILE, m_DL_Debug, err) || !m_DL_Debug)
		{
			DL_Log.Warn("DEBUG debug.json unreadable: " + err);
			m_DL_Debug = null;
			return;
		}
		string zone = "off";
		if (m_DL_Debug.carRebuildZone && m_DL_Debug.carRebuildZone.radius > 0)
			zone = m_DL_Debug.carRebuildZone.center.ToString() + " r " + m_DL_Debug.carRebuildZone.radius.ToString();
		DL_Log.Detail("DEBUG debug.json: carRebuildZone " + zone + ", seatTestCar '" + m_DL_Debug.seatTestCar + "'");
	}

	// Spike 1: an empty car 5 s in the circle -> snapshot -> delete -> JSON text -> rebuilt on the same spot 3 s later.
	protected void DL_DebugTick()
	{
		if (!m_DL_Debug || !m_DL_Debug.carRebuildZone || m_DL_Debug.carRebuildZone.radius <= 0)
			return;
		array<Object> objs = new array<Object>;
		array<CargoBase> cargos = new array<CargoBase>;
		// CGame.GetObjectsAtPosition (3_game/global/game.c:922) searches a SPHERE: centre it on the ground (test
		// 2026-10-18: y 0 under ground at 6 m missed the parked car), then keep cars within the radius on the map.
		vector c0 = m_DL_Debug.carRebuildZone.center;
		float r = m_DL_Debug.carRebuildZone.radius;
		c0[1] = g_Game.SurfaceY(c0[0], c0[2]);
		g_Game.GetObjectsAtPosition(c0, r + 5, objs, cargos);
		map<Transport, bool> seen = new map<Transport, bool>;
		int now = g_Game.GetTime();
		foreach (Object o : objs)
		{
			Transport car = Transport.Cast(o);
			if (!car)
				continue;
			vector cp = car.GetPosition();
			float dx = cp[0] - c0[0];
			float dz = cp[2] - c0[2];
			if (dx * dx + dz * dz > r * r)
				continue;
			seen.Set(car, true);
			if (m_DL_CarDone.Contains(car))
				continue;
			bool empty = true;
			for (int c = 0; c < car.CrewSize(); c++)
			{
				if (car.CrewMember(c))
					empty = false;
			}
			int since;
			if (!empty || !m_DL_CarInZoneSince.Find(car, since))
			{
				m_DL_CarInZoneSince.Set(car, now); // occupied: restart the 5 s
				continue;
			}
			if (now - since >= 5000)
			{
				m_DL_CarInZoneSince.Remove(car);
				DL_CarSpike(car);
			}
		}
		// cars that left the circle may be tested again
		array<Transport> gone = new array<Transport>;
		foreach (Transport k, bool v : m_DL_CarDone)
		{
			if (!k || !seen.Contains(k))
				gone.Insert(k);
		}
		foreach (Transport g : gone)
			m_DL_CarDone.Remove(g);
	}

	protected void DL_CarSpike(Transport car)
	{
		DL_CarData d = m_DL_CarSnap.Capture(car);
		string json;
		string err;
		JsonFileLoader<DL_CarData>.MakeData(d, json, err, false);
		JsonFileLoader<DL_CarData>.SaveFile(DL_Const.DIR + "/car-snapshot.json", d, err);
		string dlMsg272 = "CAR SPIKE " + d.type + " at " + d.pos.ToString();
		dlMsg272 += ": " + d.items.Count().ToString() + " parts/items, health " + d.health.ToString();
		dlMsg272 += ", " + DL_CarSnapshot.FluidText(d.fluids) + ", " + d.zones.Count().ToString();
		dlMsg272 += " zones, json " + json.Length().ToString() + " bytes (saved car-snapshot.json); deleting it, rebuild in 3 s";
		DL_Log.Detail(dlMsg272);
		car.Delete();
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_CarSpikeBuild, 3000, false, json);
	}

	protected void DL_CarSpikeBuild(string json)
	{
		DL_CarData d;
		string err;
		if (!JsonFileLoader<DL_CarData>.LoadData(json, d, err) || !d)
		{
			DL_Log.Warn("CAR SPIKE FAILED: snapshot unreadable after the JSON round trip: " + err);
			return;
		}
		int made;
		int dropped;
		Transport car = m_DL_CarSnap.Build(d, d.pos, d.ori, made, dropped);
		if (!car)
		{
			DL_Log.Warn("CAR SPIKE FAILED: could not create " + d.type);
			return;
		}
		m_DL_CarDone.Set(car, true);
		string dlMsg295 = "CAR SPIKE rebuilt " + d.type + ": " + made.ToString();
		dlMsg295 += "/" + d.items.Count().ToString() + " parts/items (" + dropped.ToString();
		dlMsg295 += " on the ground), health " + car.GetHealth("", "Health").ToString() + ", now " + DL_CarSnapshot.FluidText(DL_CarSnapshot.FluidsOf(car));
		DL_Log.Detail(dlMsg295);
	}

	// Spike 2: a fresh car next to a player who just logged in / arrived, then he is seated as driver.
	protected void DL_SeatSpikeLater(string sid)
	{
		if (m_DL_Debug && m_DL_Debug.seatTestCar != "")
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SeatSpikeSpawn, 3000, false, sid);
	}

	protected void DL_SeatSpikeSpawn(string sid)
	{
		PlayerBase pb = DL_FindPlayer(sid);
		if (!pb)
			return;
		vector pos = pb.GetPosition() + pb.GetDirection() * 5;
		Transport car = Transport.Cast(g_Game.CreateObjectEx(m_DL_Debug.seatTestCar, pos, ECE_SETUP | ECE_UPDATEPATHGRAPH | ECE_CREATEPHYSICS | ECE_TRACE));
		if (!car)
		{
			DL_Log.Warn("SEAT SPIKE FAILED: could not create '" + m_DL_Debug.seatTestCar + "' (not a Transport class?)");
			return;
		}
		car.SetOrientation(pb.GetOrientation());
		car.OnDebugSpawn(); // vanilla kit: wheels, doors, battery, plugs, fluids (e.g. offroadhatchback.c:457-477)
		string dlMsg319 = "SEAT SPIKE spawned " + car.GetType() + " at " + pos.ToString();
		dlMsg319 += " for " + sid + ", seating him in 2 s";
		DL_Log.Detail(dlMsg319);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SeatSpikeSeat, 2000, false, sid, car);
	}

	protected void DL_SeatSpikeSeat(string sid, Transport car)
	{
		PlayerBase pb = DL_FindPlayer(sid);
		if (!pb || !car)
			return;
		pb.DL_SeatIn(car, 0);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SeatSpikeCheck, 4000, false, sid, car);
	}

	protected void DL_SeatSpikeCheck(string sid, Transport car)
	{
		PlayerBase pb = DL_FindPlayer(sid);
		if (!pb || !car)
			return;
		bool inCmd = pb.GetCommand_Vehicle() != null;
		bool isDriver = car.CrewDriver() == pb;
		string dlMsg339 = "SEAT SPIKE result for " + sid + ": vehicle command " + inCmd.ToString();
		dlMsg339 += ", driver of " + car.GetType() + " " + isDriver.ToString();
		dlMsg339 += ", crew index " + car.CrewMemberIndex(pb).ToString();
		DL_Log.Detail(dlMsg339);
	}

	// /api/hello: on boot, and again until the hub answers OK (hub down, or this server not approved yet). It carries
	// this server's zones/arrival points (the hub imports them once, see DL_HelloReq) and who it is (port, map).
	protected bool m_DL_Polling;
	protected bool m_DL_SaidPending;

	protected void DL_SendHello()
	{
		DL_HelloReq h = new DL_HelloReq();
		h.serverId = m_DL_Cfg.serverId;
		h.key = m_DL_Cfg.hubKey;
		h.claim = m_DL_Cfg.joinId;
		h.zones = m_DL_Cfg.zones;
		h.arrivals = m_DL_Cfg.arrivals;
		string p;
		h.port = 2302;
		if (g_Game.CommandlineGetParam("port", p) && p.ToInt() > 0)
			h.port = p.ToInt();
		// serverDZ.cfg steamQueryPort is what DayZ uses; -steamQueryPort on the command line is IGNORED by DayZ 1.30
		// (2026-10-19: -steamQueryPort=27802 still listened on 27016), so it is only a last guess
		h.queryPort = g_Game.ServerConfigGetInt("steamQueryPort");
		string qp;
		if (h.queryPort <= 0 && g_Game.CommandlineGetParam("steamQueryPort", qp))
			h.queryPort = qp.ToInt();
		if (h.queryPort <= 0)
			h.queryPort = 27016; // DayZ's default
		string world;
		g_Game.GetWorldName(world);
		world.ToLower();
		h.world = world;
		if (g_Game.GetWorld())
			h.worldSize = g_Game.GetWorld().GetWorldSize();
		string hj;
		string herr;
		JsonFileLoader<DL_HelloReq>.MakeData(h, hj, herr, false);
		DL_Hub.Post(m_DL_Cfg.hubUrl, "/api/hello?serverId=" + m_DL_Cfg.serverId + "&key=" + m_DL_Cfg.hubKey, hj, new DL_HelloWaiter(this), "OnAnswer", "hello");
	}

	void DL_OnHello(bool ok, string body)
	{
		if (!ok)
		{
			DL_Log.Warn("hub hello FAILED - check hubUrl/hubKey, that the hub runs, and the hub's log; trying again in 30 s");
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SendHello, 30000, false);
			return;
		}
		DL_HelloResp r;
		string err;
		JsonSerializer js = new JsonSerializer();
		if (!js.ReadFromString(r, body, err) || !r)
		{
			DL_Log.Warn("hub hello: unreadable answer (" + err + "), trying again in 30 s");
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SendHello, 30000, false);
			return;
		}
		if (r.incompatible)
		{
			// DL_Hub.Handshake said why; ask again now and then (the hub may be updated while this server runs)
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SendHello, 60000, false);
			return;
		}
		if (r.pending)
		{
			if (!m_DL_SaidPending)
				DL_Log.Info("the hub knows the key but not this server yet: approve it on the hub's Servers page (asking again every 15 s)");
			m_DL_SaidPending = true;
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SendHello, 15000, false);
			return;
		}
		DL_Log.Info("hub hello OK: " + body.Substring(0, Math.Min(160, body.Length())));
		if (r.serverId != "" && r.serverId != m_DL_Cfg.serverId)
		{
			DL_Log.Info("the hub knows this server as '" + r.serverId + "' (was '" + m_DL_Cfg.serverId + "'), saved in config.json");
			m_DL_Cfg.serverId = r.serverId;
			JsonFileLoader<DL_Config>.SaveFile(DL_Const.CONFIG, m_DL_Cfg, err);
		}
		if (r.layout)
			DL_ApplyLayout(r.layout, "hello");
		DL_CarLedger.SetServerId(m_DL_Cfg.serverId);
		if (DL_CarLedger.On() && !m_DL_CarReports)
		{
			m_DL_CarReports = true;
			// the first report after a minute: by then the world storage has loaded every car (their tags come back
			// in AfterStoreLoad); until then vetoes use the hub's last answer from car-ledger.json
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_ReportCars, DL_CarLedger.REPORT_MS, true);
		}
		// from now on: ask for edits every 30 s (only a rev number goes over the wire when nothing changed)
		if (!m_DL_Polling)
		{
			m_DL_Polling = true;
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_PollLayout, 30000, true);
		}
	}

	// ---------------- vehicle economy (DL_CarLedger, Research/vehicle-economy.md) ----------------

	protected bool m_DL_CarReports;

	// every 60 s: the travelled cars here and the ones that left; the hub answers how many of ours are away
	void DL_ReportCars()
	{
		string json = DL_CarLedger.ReportJson(m_DL_Cfg.hubKey);
		if (json != "")
			DL_Hub.Post(m_DL_Cfg.hubUrl, "/api/cars?serverId=" + m_DL_Cfg.serverId + "&key=" + m_DL_Cfg.hubKey, json, new DL_CarReportWaiter(), "OnAnswer", "cars");
	}

	// ---------------- zones + arrival points from the hub (map editor) ----------------

	protected int m_DL_LayoutRev;

	protected void DL_PollLayout()
	{
		DL_LayoutReq q = new DL_LayoutReq();
		q.serverId = m_DL_Cfg.serverId;
		q.key = m_DL_Cfg.hubKey;
		q.rev = m_DL_LayoutRev;
		string json;
		string err;
		JsonFileLoader<DL_LayoutReq>.MakeData(q, json, err, false);
		DL_Hub.Post(m_DL_Cfg.hubUrl, "/api/layout?serverId=" + m_DL_Cfg.serverId + "&key=" + m_DL_Cfg.hubKey, json, new DL_LayoutWaiter(this), "OnAnswer", "layout");
	}

	void DL_OnLayout(bool ok, string body)
	{
		if (!ok)
			return; // hub down: keep what we have
		DL_Layout l;
		string err;
		JsonSerializer js = new JsonSerializer();
		if (js.ReadFromString(l, body, err) && l && l.changed)
			DL_ApplyLayout(l, "edit on the hub");
	}

	// The hub's copy replaces the zones/arrival points; config.json gets it too, so a boot without the hub uses
	// the latest layout. Zones only need x/z; arrival heights are taken from the terrain when someone arrives.
	protected void DL_ApplyLayout(DL_Layout l, string why)
	{
		if (!l || l.rev <= 0 || !l.zones || !l.arrivals || l.rev == m_DL_LayoutRev)
			return;
		m_DL_LayoutRev = l.rev;
		m_DL_Cfg.zones = l.zones;
		m_DL_Cfg.arrivals = l.arrivals;
		if (l.travel && l.travel.mode != "") // an older hub sends none: keep what config.json has
			m_DL_Cfg.travel = l.travel;
		m_DL_Parties.Clear(); // countdowns of zones that may be gone now
		string err;
		JsonFileLoader<DL_Config>.SaveFile(DL_Const.CONFIG, m_DL_Cfg, err);
		string dlMsg462 = "layout rev " + l.rev.ToString() + " from the hub (" + why;
		dlMsg462 += "): " + m_DL_Cfg.zones.Count().ToString() + " zones, " + m_DL_Cfg.arrivals.Count().ToString();
		dlMsg462 += " arrival points";
		DL_Log.Info(dlMsg462);
		foreach (DL_Zone z : m_DL_Cfg.zones)
		{
			if (z)
			{
				string dlMsg466 = "  zone '" + z.name + "' " + z.center.ToString();
				dlMsg466 += " r " + z.radius.ToString() + " -> " + z.to;
				dlMsg466 += " / " + z.arrival + ", " + z.distanceKm.ToString();
				dlMsg466 += " km";
				DL_Log.Detail(dlMsg466);
			}
		}
		foreach (DL_ArrivalPoint a : m_DL_Cfg.arrivals)
		{
			if (a)
				DL_Log.Detail("  arrival '" + a.name + "' " + a.pos.ToString() + " yaw " + a.yaw.ToString());
		}
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(DL_CheckCarSpots);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_CheckCarSpots, 5000, false);
		DL_TravelRules t = m_DL_Cfg.travel;
		if (t)
		{
			string dlMsg477 = "  travel rules: " + t.mode + ", per 10 km foot " + t.foodFootKcal.ToString();
			dlMsg477 += " kcal/" + t.waterFootMl.ToString() + " ml, vehicle " + t.foodCarKcal.ToString();
			dlMsg477 += " kcal/" + t.waterCarMl.ToString() + " ml + " + t.fuelLiters.ToString();
			dlMsg477 += " L (per vehicle " + t.fuelPerVehicle.ToString() + " x" + t.fuelScale.ToString();
			dlMsg477 += ", boats x" + t.fuelBoatScale.ToString() + "), wear " + t.wearEnginePct.ToString();
			dlMsg477 += "/" + t.wearWheelsPct.ToString() + "%, time " + t.timeEnabled.ToString();
			dlMsg477 += " (" + t.speedFootKmh.ToString() + "/" + t.speedCarKmh.ToString();
			dlMsg477 += " km/h)";
			DL_Log.Detail(dlMsg477);
		}
	}

	protected bool DL_On()
	{
		// no id = not approved on the hub yet; another build than the hub's = no travel (DL_Hub.Handshake)
		return m_DL_Cfg && m_DL_Cfg.hubKey != "" && m_DL_Cfg.serverId != "" && !DL_Hub.s_Incompatible;
	}

	// ---------------- arrival ----------------

	override void OnClientPrepareEvent(PlayerIdentity identity, out bool useDB, out vector pos, out float yaw, out int preloadTimeout)
	{
		super.OnClientPrepareEvent(identity, useDB, pos, yaw, preloadTimeout);
		if (!DL_On() || !identity)
			return;
		string sid = identity.GetPlainId();
		DL_Pending p = new DL_Pending();
		m_DL_Pending.Set(sid, p);
		DL_ArriveReq q = new DL_ArriveReq();
		q.serverId = m_DL_Cfg.serverId;
		q.key = m_DL_Cfg.hubKey;
		q.steamId = sid;
		q.name = identity.GetName(); // for the hub's Players page
		DL_Hub.Post(m_DL_Cfg.hubUrl, "/api/arrive?serverId=" + m_DL_Cfg.serverId + "&key=" + m_DL_Cfg.hubKey, DL_Hub.ArriveJson(q), new DL_ArriveWaiter(this, sid), "OnAnswer", "arrive " + sid);
		// the client keeps loading while we wait; give the hub time before the character is made
		if (preloadTimeout < 10)
			preloadTimeout = 10;
	}

	void DL_OnArriveAnswer(string sid, bool ok, string body)
	{
		DL_Pending p = m_DL_Pending.Get(sid);
		if (!p)
			return;
		DL_ArriveResp r;
		string err;
		JsonSerializer js = new JsonSerializer();
		if (!ok || !js.ReadFromString(r, body, err) || !r || !r.ok)
		{
			// hub down or confused: let the player play here rather than lock him out
			if (!ok)
				DL_Log.Warn("hub call failed for " + sid + " (refused or unreachable, see the hub window), normal login");
			else
			{
				string dlMsg520 = "hub answer for " + sid + " unusable: " + err;
				dlMsg520 += " " + body.Substring(0, Math.Min(200, body.Length())) + ", normal login";
				DL_Log.Warn(dlMsg520);
			}
			p.action = "normal";
			if (p.loggedIn)
				DL_LateAnswer(sid, p);
			return;
		}
		p.resp = r;
		DL_Log.Detail("hub says " + r.action + " for " + sid);
		if (r.action == "transfer")
		{
			// second step: the snapshot itself, as the whole body of /api/payload (see DL_Hub.ParsePayload)
			p.action = "fetching";
			DL_TokenReq q = DL_TokenRequest(sid, r.token);
			DL_Hub.Post(m_DL_Cfg.hubUrl, "/api/payload?serverId=" + m_DL_Cfg.serverId + "&key=" + m_DL_Cfg.hubKey, DL_Hub.TokenJson(q), new DL_PayloadWaiter(this, sid), "OnAnswer", "payload " + sid);
			return;
		}
		p.action = r.action;
		if (p.loggedIn)
			DL_LateAnswer(sid, p);
	}

	void DL_OnPayloadAnswer(string sid, bool ok, string body)
	{
		DL_Pending p = m_DL_Pending.Get(sid);
		if (!p || !p.resp)
			return;
		if (!ok)
		{
			// hub gone between the two calls: nothing is built, the trip stays pending on the hub
			DL_MarkFailed(sid, p, "could not fetch the travel data from the hub");
		}
		else
		{
			string err;
			p.resp.payload = DL_Hub.ParsePayload(body, err);
			string sizes = body.Length().ToString() + " of " + p.resp.bytes.ToString() + " bytes";
			if (!p.resp.payload)
				DL_MarkFailed(sid, p, "travel data unreadable (" + sizes + "): " + err);
			else
			{
				p.action = "transfer";
				string dlMsg561 = "transfer for " + sid + ": " + sizes;
				dlMsg561 += ", " + DL_Hub.CountItems(p.resp.payload.items).ToString() + " items in the snapshot";
				DL_Log.Info(dlMsg561);
			}
		}
		if (p.loggedIn)
			DL_LateAnswer(sid, p);
	}

	protected DL_TokenReq DL_TokenRequest(string sid, string token)
	{
		DL_TokenReq q = new DL_TokenReq();
		q.serverId = m_DL_Cfg.serverId;
		q.key = m_DL_Cfg.hubKey;
		q.steamId = sid;
		q.token = token;
		return q;
	}

	// The transfer can't be built here. He must NOT play here with another character (his real one is in the
	// hub, and departing would be refused anyway: the hub has him in transit). Tell the hub why (it keeps the trip
	// pending; admin: "DriveLinkHub.exe transfers" / "return <steamId>"), and send him away with the reason.
	protected void DL_MarkFailed(string sid, DL_Pending p, string reason)
	{
		p.action = "failed";
		p.reason = reason;
		DL_Log.Warn("ARRIVAL FAILED for " + sid + ": " + reason);
		DL_TokenReq q = DL_TokenRequest(sid, p.resp.token);
		q.reason = reason;
		DL_Hub.Post(m_DL_Cfg.hubUrl, "/api/fail?serverId=" + m_DL_Cfg.serverId + "&key=" + m_DL_Cfg.hubKey, DL_Hub.TokenJson(q), new DL_ConfirmWaiter(sid, "failure report"), "OnAnswer", "fail " + sid);
	}

	// The game made/loaded his character before the hub answered (OnClientPrepareEvent gives the hub
	// preloadTimeout seconds). Before Phase A the late answer was dropped: a pending trip was silently skipped and
	// he played a wrong character on the target.
	protected void DL_LateAnswer(string sid, DL_Pending p)
	{
		PlayerBase pb = DL_FindPlayer(sid);
		if (!pb || !pb.GetIdentity())
		{
			m_DL_Pending.Remove(sid);
			return;
		}
		DL_Log.Detail("late hub answer for " + sid + ": " + p.action);
		if (p.action == "transfer")
		{
			// rebuilt on his next login (the hub still offers the trip; the ClientReady swap replaces this character)
			m_DL_Pending.Remove(sid);
			DL_Kick(pb, "Your travel data arrived late. Wait 30 seconds, then join this server again to finish the trip.");
			return;
		}
		DL_AfterLogin(pb.GetIdentity(), pb, p);
	}

	// Client leaves by itself on RPC_KICK (DL_Client) and shows the text in the main menu. The character logs out
	// the normal way (saved as it is: it is not the travelling one), protected and kept out of the zones meanwhile.
	// Sent 2 s later: at login (OnClientNewEvent) vanilla hasn't run InvokeOnConnect yet, give the client time.
	protected void DL_Kick(PlayerBase pb, string text)
	{
		if (!pb || !pb.GetIdentity())
			return;
		string sid = pb.GetIdentity().GetPlainId();
		m_DL_Leaving.Set(sid, true);
		pb.SetAllowDamage(false);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SendKick, 2000, false, sid, text);
	}

	protected void DL_SendKick(string sid, string text)
	{
		PlayerBase pb = DL_FindPlayer(sid);
		if (!pb || !pb.GetIdentity())
			return;
		DL_Log.Info("sending " + sid + " away: " + text);
		ScriptRPC rpc = new ScriptRPC();
		rpc.Write("DriveLink");
		rpc.Write(text);
		rpc.Send(null, DL_Const.RPC_KICK, true, pb.GetIdentity());
	}

	override PlayerBase OnClientNewEvent(PlayerIdentity identity, vector pos, ParamsReadContext ctx)
	{
		DL_Pending p;
		if (DL_On() && identity)
			p = m_DL_Pending.Get(identity.GetPlainId());
		if (p && p.action == "transfer" && p.resp && p.resp.payload)
		{
			PlayerBase built = DL_BuildArrival(identity, p.resp);
			if (built)
			{
				m_DL_Pending.Remove(identity.GetPlainId());
				return built;
			}
			// vanilla needs a character here (a null one breaks the login), so he gets a normal one and is sent away
			DL_MarkFailed(identity.GetPlainId(), p, m_DL_BuildError);
		}
		PlayerBase player = super.OnClientNewEvent(identity, pos, ctx);
		DL_AfterLogin(identity, player, p);
		return player;
	}

	// M1 test 2026-10-13: he came back to a server where the game's OWN database still had an alive character for
	// him (a copy from before he left, or one we failed to remove). The engine loads that one and sends
	// ClientReadyEvent with it (missionserver.c:334-350, ClientReadyEventParams = Param2<PlayerIdentity, Man>,
	// gameplay.c:422). A transfer must win: before vanilla handles the event, swap the loaded copy for the
	// character built from the transfer, and remove + CharacterKill the copy so it is never loaded again.
	override void OnEvent(EventType eventTypeId, Param params)
	{
		if (eventTypeId == ClientReadyEventTypeID && DL_On())
		{
			ClientReadyEventParams rp;
			Class.CastTo(rp, params);
			if (rp && rp.param1)
			{
				DL_Pending p = m_DL_Pending.Get(rp.param1.GetPlainId());
				if (p && p.action == "transfer" && p.resp && p.resp.payload)
				{
					PlayerBase old = PlayerBase.Cast(rp.param2);
					DL_Log.Info(rp.param1.GetPlainId() + " arrives by transfer; this server had an old character for him, replacing it");
					PlayerBase built = DL_BuildArrival(rp.param1, p.resp, old);
					if (built)
					{
						m_DL_Pending.Remove(rp.param1.GetPlainId());
						// vanilla's ClientNew path, so the new character gets the normal connect handling
						InvokeOnConnect(built, rp.param1);
						SyncEvents.SendPlayerList();
						ControlPersonalLight(built);
						SyncGlobalLighting(built);
						return;
					}
					// DL_BuildArrival only removes the old copy once the new character exists, so he keeps it and is sent away
					DL_MarkFailed(rp.param1.GetPlainId(), p, m_DL_BuildError);
				}
			}
		}
		super.OnEvent(eventTypeId, params);
	}

	override void OnClientReadyEvent(PlayerIdentity identity, PlayerBase player)
	{
		super.OnClientReadyEvent(identity, player);
		DL_Pending p;
		if (DL_On() && identity)
			p = m_DL_Pending.Get(identity.GetPlainId());
		DL_AfterLogin(identity, player, p);
	}

	protected void DL_AfterLogin(PlayerIdentity identity, PlayerBase player, DL_Pending p)
	{
		if (!identity)
			return;
		string sid = identity.GetPlainId();
		// M1 test 2026-10-13: a leaving-flag left from an earlier visit made the zone check skip him forever
		m_DL_Leaving.Remove(sid);
		DL_ServerInfo home;
		if (m_DL_GoHome.Find(sid, home) && home)
		{
			// he died here and this is not the starting server: his new life starts there
			m_DL_GoHome.Remove(sid);
			m_DL_Pending.Remove(sid);
			DL_Log.Info(sid + " respawns: sent to the starting server " + home.name);
			NotificationSystem.SendNotificationToPlayerIdentityExtended(identity, 8, "DriveLink", "You died. Your next life starts on " + home.name + ". Taking you there...", "");
			DL_SendRedirect(identity, home, "died");
			if (player)
			{
				player.SetAllowDamage(false);
				m_DL_Doomed.Set(player, true);
			}
			return;
		}
		if (p && (p.action == "" || p.action == "fetching"))
		{
			// the hub hasn't fully answered yet: keep the entry, DL_LateAnswer handles the answer when it comes
			p.loggedIn = true;
			m_DL_Leaving.Set(sid, true); // no zone travel until we know where his character really is
			DL_Log.Warn("WARNING " + sid + " logged in before the hub answered (" + p.action + "), waiting for it");
			return;
		}
		m_DL_Pending.Remove(sid);
		if (p && p.action == "wait")
		{
			string where = "the next server";
			if (p.resp && p.resp.target)
				where = p.resp.target.name;
			int left = 0;
			if (p.resp)
				left = p.resp.seconds;
			DL_Log.Info(identity.GetPlainId() + " is still on the road to " + where + " (" + left.ToString() + " s), sent back to the menu");
			if (player)
			{
				player.SetAllowDamage(false);
				m_DL_Doomed.Set(player, true); // his real character is on the road: nothing of this one may stay
			}
			DL_Kick(player, "You are still on the road to " + where + ": about " + DL_TravelCost.Duration(left) + " left. Join " + where + " after that.");
			return;
		}
		if (p && p.action == "failed")
		{
			string text = "Your character could not be brought to this server (" + p.reason + "). It is safe on the DriveLink hub. Ask an admin, or try again later.";
			DL_Kick(player, text);
			return;
		}
		if (p && p.action == "redirect" && p.resp && p.resp.target)
		{
			DL_Log.Info(identity.GetPlainId() + " belongs on " + p.resp.target.name + ", redirecting");
			NotificationSystem.SendNotificationToPlayerIdentityExtended(identity, 8, "DriveLink", "Your character is on " + p.resp.target.name + ". Taking you there...", "");
			DL_SendRedirect(identity, p.resp.target, "owner");
			if (player)
			{
				player.SetAllowDamage(false);
				m_DL_Doomed.Set(player, true);
			}
			return;
		}
#ifdef DL_DEV
		DL_SeatSpikeLater(sid); // debug.json only (dev builds)
#endif
	}

	// Returns null (and says why in m_DL_BuildError) when he can't be built here; nothing is changed then.
	protected PlayerBase DL_BuildArrival(PlayerIdentity identity, DL_ArriveResp r, PlayerBase oldChar = null)
	{
		DL_PlayerData d = r.payload;
		if (d.diedOnRoad)
			return DL_BuildDiedOnRoad(identity, r, oldChar);
		DL_ArrivalPoint ap = m_DL_Cfg.FindArrival(r.arrival);
		if (!ap && m_DL_Cfg.arrivals.Count() > 0)
		{
			// "" = a trip the hub admin turned around ("DriveLinkHub.exe return"): first point is the normal answer
			if (r.arrival != "")
				DL_Log.Warn("arrival point '" + r.arrival + "' not in config.json, using the first one");
			ap = m_DL_Cfg.arrivals[0];
		}
		if (!ap)
		{
			m_DL_BuildError = "this server has no arrival points in config.json";
			return null;
		}
		// M3 waiting room: a car crew (driver with the car, or passenger) whose snapshot says how many were in it.
		// A passenger whose driver's car is already standing here (came late) takes the old path below.
		bool roomCar = d.car != null && d.car.type != "";
		string roomGroup = "";
		if (roomCar)
			roomGroup = identity.GetPlainId();
		else if (d.carGroup != "")
			roomGroup = d.carGroup;
		bool lateRider = !roomCar && !m_DL_Rooms.Contains(roomGroup) && m_DL_GroupCar.Get(roomGroup) != null;
		if (roomGroup != "" && d.crewCount > 0 && !lateRider)
			return DL_BuildIntoRoom(identity, r, ap, oldChar, roomGroup);
		vector pos = ap.pos;
		float yaw = ap.yaw;
		// on foot at an arrival point out on the water (a boat party that swam the rest): he comes ashore
		if (!(d.car && d.car.type != "") && d.carGroup == "")
			pos = DL_LandNear(pos, identity.GetPlainId());
		// M2: his car first, at carPos or else at the arrival point (he then stands 3 m to its left = driver side)
		Transport car;
		string carText = "";
		if (d.car && d.car.type != "")
		{
			vector carPos = ap.carPos;
			float carYaw = ap.carYaw;
			bool onPoint = carPos[0] == 0 && carPos[2] == 0;
			if (onPoint)
			{
				carPos = ap.pos;
				carYaw = ap.yaw;
			}
			carPos[1] = g_Game.SurfaceY(carPos[0], carPos[2]);
			carPos = DL_FreeCarSpot(carPos, carYaw, d.car.type, identity.GetPlainId());
			int carMade;
			int carDropped;
			car = m_DL_CarSnap.Build(d.car, carPos, Vector(carYaw, 0, 0), carMade, carDropped);
			if (car)
			{
				if (onPoint)
					pos = carPos + Vector(carYaw - 90, 0, 0).AnglesToVector() * 3;
				carText = ", with his " + car.GetType() + " (" + carMade.ToString() + "/" + d.car.items.Count().ToString() + " parts/items, " + carDropped.ToString() + " on the ground)";
			}
			else
			{
				carText = ", WITHOUT his car: could not create " + d.car.type;
				DL_Log.Warn("WARNING could not create the car " + d.car.type + " for " + identity.GetPlainId() + ", he arrives on foot");
			}
		}
		// M3: a passenger. His driver's car is here already -> next to it (right side) and seated; else he waits on
		// foot 6 m to the right of the arrival point (where the car will appear) and is brought in when it comes.
		Transport rideCar;
		bool waitForCar;
		if (!car && d.carGroup != "")
		{
			rideCar = m_DL_GroupCar.Get(d.carGroup);
			if (rideCar)
			{
				vector rideOri = rideCar.GetOrientation();
				pos = rideCar.GetPosition() + Vector(rideOri[0] + 90, 0, 0).AnglesToVector() * 3;
				carText = ", passenger: " + d.carGroup + "'s " + rideCar.GetType() + " is here, seat " + d.carSeat.ToString();
			}
			else
			{
				waitForCar = true;
				pos = ap.pos + Vector(ap.yaw + 90, 0, 0).AnglesToVector() * 6;
				carText = ", passenger: waiting for " + d.carGroup + "'s car (seat " + d.carSeat.ToString() + ")";
			}
		}
		// a group on foot: side by side, 1.5 m apart, alternating left and right of the point
		if (!car && d.carGroup == "" && d.partySlot > 0)
		{
			float side = 1.5 * ((d.partySlot + 1) / 2);
			if (d.partySlot % 2 == 0)
				side = -side;
			pos = pos + Vector(yaw + 90, 0, 0).AnglesToVector() * side;
		}
		pos[1] = g_Game.SurfaceY(pos[0], pos[2]);
		// CreateCharacter (missionserver.c:486): CreatePlayer + SelectPlayer, same as a fresh spawn but his own body
		PlayerBase player = CreateCharacter(identity, pos, null, d.charType);
		if (!player)
		{
			m_DL_BuildError = "could not create the character " + d.charType;
			if (car)
				car.Delete(); // the trip stays pending with the car in it: a later build makes it again
			return null;
		}
		player.DL_MarkTransferArrival();
		DL_GuardJoin(player,d,car);
		if (oldChar)
		{
			// only now that the new one exists: the stale copy must not live on in this server's database (or he
			// gets it back next time)
			if (GetHive())
				GetHive().CharacterKill(oldChar);
			oldChar.Delete();
		}
		player.SetOrientation(Vector(yaw, 0, 0));
		int dropped;
		int made = m_DL_Snap.Apply(player, d, dropped);
		// weapons whose magazine was attached after them: refresh their state once everything exists
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(m_DL_Snap.FixWeapons, 500, false, player);
		if (m_DL_Cfg.arrivalProtectSeconds > 0)
		{
			player.SetAllowDamage(false);
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_EndProtection, m_DL_Cfg.arrivalProtectSeconds * 1000, false, player);
		}
		m_DL_Leaving.Remove(identity.GetPlainId());
		DL_Confirm(identity.GetPlainId(), r.token);
		string dlMsg901 = identity.GetPlainId() + " arrived from " + r.from + " at ";
		dlMsg901 += ap.name + " " + pos.ToString() + ", ";
		dlMsg901 += made.ToString() + "/" + DL_Hub.CountItems(d.items).ToString() + " items restored (";
		dlMsg901 += dropped.ToString() + " on the ground, no room), health " + d.health.ToString() + ", bleeding ";
		dlMsg901 += d.bleeding.Count().ToString() + ", modifiers " + d.modifiers.Count().ToString() + ", stomach ";
		dlMsg901 += d.stomach.Count().ToString() + ", cuffed " + d.restrained.ToString() + carText;
		DL_Log.Info(dlMsg901);
		NotificationSystem.SendNotificationToPlayerIdentityExtended(identity, 6, "DriveLink", "Welcome from " + r.from, "");
		string sid = identity.GetPlainId();
		float filmSec = DL_SendFilmLater(sid, d);
		int protectedStorySeconds = 0;
		if (!car && d.carGroup == "")
			protectedStorySeconds = DL_StoryScreen(sid, d.journey, filmSec);
		DL_GuardReady(player,2000+protectedStorySeconds*1000);
		if(rideCar)DL_GuardObject(m_DL_GuardOf.Get(player),rideCar);
		if (car)
		{
			// seated once his client is in (same juncture path as the seat spike, DL_PlayerSeat.c)
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SeatTraveller, 3000, false, sid, car, d.carSeat);
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_EndCarProtection, (m_DL_Cfg.arrivalProtectSeconds + 3) * 1000, false, car);
			// M3: his passengers find the car here; the ones already waiting are brought in after him
			m_DL_GroupCar.Set(sid, car);
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_BringWaitingCrew, 5000, false, sid, car);
		}
		else if (rideCar)
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SeatTraveller, 3000, false, sid, rideCar, d.carSeat);
		else if (waitForCar)
		{
			DL_SeatWait w = new DL_SeatWait();
			w.group = d.carGroup;
			w.seat = d.carSeat;
			w.since = g_Game.GetTime();
			m_DL_SeatWait.Set(sid, w);
			NotificationSystem.SendNotificationToPlayerIdentityExtended(identity, 10, "DriveLink", "Your driver is still on the way. You get your seat when the car arrives.", "");
		}
		return player;
	}

	// ---------------- travel step 4: the road's story, death on the road ----------------

	// He died on the way: the road kept his gear. A fresh character (vanilla starting gear) at the arrival point, the
	// trip is claimed (he is here now), and the black screen tells what happened.
	protected PlayerBase DL_BuildDiedOnRoad(PlayerIdentity identity, DL_ArriveResp r, PlayerBase oldChar)
	{
		DL_PlayerData d = r.payload;
		string sid = identity.GetPlainId();
		vector pos = "0 0 0";
		DL_ArrivalPoint ap = m_DL_Cfg.FindArrival(r.arrival);
		if (!ap && m_DL_Cfg.arrivals.Count() > 0)
			ap = m_DL_Cfg.arrivals[0];
		if (ap)
			pos = ap.pos;
		pos[1] = g_Game.SurfaceY(pos[0], pos[2]);
		PlayerBase player = CreateCharacter(identity, pos, null, d.charType);
		if (!player)
		{
			m_DL_BuildError = "could not create the character " + d.charType;
			return null;
		}
		if (oldChar)
		{
			if (GetHive())
				GetHive().CharacterKill(oldChar);
			oldChar.Delete();
		}
		// he died on the way: a bare, hidden body for the black screen (story + film), then a real death (death
		// screen, the game's own respawn); the hub sends that respawn to the starting server (DL_TakeDeaths)
		player.SetAllowDamage(false);
		m_DL_Leaving.Set(sid, true);              // no zone travel for the dead
		DL_Confirm(sid, r.token);
		DL_Log.Info(sid + " died on the road from " + r.from + ": dies here once the film / story is over");
		int secs = DL_StoryScreen(sid, "You died on the road.\n" + d.journey, DL_SendFilmLater(sid, d), true);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_RoadDeath, 2000 + 1000 * secs + 300, false, sid);
		return player;
	}

	protected void DL_RoadDeath(string sid)
	{
		PlayerBase pb = DL_FindPlayer(sid);
		if (!pb || !pb.IsAlive())
			return;
		m_DL_Leaving.Remove(sid);
		pb.SetAllowDamage(true);
		pb.SetHealth("", "", 0);
		DL_Log.Info(sid + " died on the road: killed");
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_RemoveBody, 60000, false, pb);
	}

	// the road death's bare body: nobody should find it at the arrival point
	protected void DL_RemoveBody(PlayerBase pb)
	{
		if (pb && !pb.IsAlive())
			pb.Delete();
	}

	// deaths on this server -> the hub; it answers with the starting server when that is another one
	protected void DL_TakeDeaths()
	{
		array<string> q = DL_Deaths.Queue();
		if (q.Count() == 0)
			return;
		foreach (string sid : q)
		{
			DL_TokenReq dr = DL_TokenRequest(sid, "");
			DL_Hub.Post(m_DL_Cfg.hubUrl, "/api/died?serverId=" + m_DL_Cfg.serverId + "&key=" + m_DL_Cfg.hubKey, DL_Hub.TokenJson(dr), new DL_DiedWaiter(this, sid), "OnAnswer", "died " + sid);
		}
		q.Clear();
	}

	void DL_OnDied(string sid, bool ok, string body)
	{
		if (!ok)
		{
			DL_Log.Warn("WARNING hub did not take the death of " + sid);
			return;
		}
		DL_DiedResp resp;
		string err;
		JsonSerializer js = new JsonSerializer();
		if (!js.ReadFromString(resp, body, err) || !resp)
			return;
		if (resp.home && resp.home.id != "")
		{
			m_DL_GoHome.Set(sid, resp.home);
			DL_Log.Info(sid + " died here: his next life starts on " + resp.home.name);
		}
		else
			DL_Log.Info(sid + " died here: respawns here");
	}

	// the black screen with the road's story (and film) for a while (arrivals that don't go through a waiting room)
	// returns how long it shows, s (0 = nothing to show); staysHidden: the body is never shown again (a road death)
	protected int DL_StoryScreen(string sid, string story, float filmSeconds, bool staysHidden = false)
	{
		if (story == "" && filmSeconds <= 0)
			return 0;
		int secs = Math.Max(9, Math.Ceil(filmSeconds) + 4);
		PlayerBase pb = DL_FindPlayer(sid);
		if (pb)
		{
			pb.DL_SetHidden(staysHidden); // living arrivals remain visible and protected
		}
		m_DL_StoryCut.Remove(sid);
		for (int i = 0; i < secs; i++)
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_StoryShow, 2000 + 1000 * i, false, sid, story);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SendWait, 2000 + 1000 * secs, false, sid, DL_Const.WAIT_DONE, "", false, "");
		return secs;
	}

	protected void DL_StoryShow(string sid, string story)
	{
		if (!m_DL_StoryCut.Contains(sid))
			DL_SendWait(sid, DL_Const.WAIT_SHOW, "On the road", false, story);
	}

	// ---------------- skipping the film: everyone watching it votes ----------------

	protected void DL_ForgetFilm(string sid, string key)
	{
		if (m_DL_FilmKey.Get(sid) == key)
			m_DL_FilmKey.Remove(sid);
		bool any = false;
		foreach (string s2, string k2 : m_DL_FilmKey)
		{
			if (k2 == key)
				any = true;
		}
		if (!any)
			m_DL_FilmVoted.Remove(key);
	}

	// the votes the clients sent (DL_FilmVotes, queued by PlayerBase.OnRPC)
	protected void DL_TakeFilmVotes()
	{
		array<string> q = DL_FilmVotes.Queue();
		if (q.Count() == 0)
			return;
		array<string> votes = new array<string>;
		votes.Copy(q);
		q.Clear();
		foreach (string v : votes)
			DL_FilmVote(v);
	}

	// one more vote: all of them in = everyone's film stops and their black screens lift (as soon as they are seated)
	protected void DL_FilmVote(string sid)
	{
		string key;
		if (!m_DL_FilmKey.Find(sid, key))
			return;
		array<string> voted = m_DL_FilmVoted.Get(key);
		if (!voted)
		{
			voted = new array<string>;
			m_DL_FilmVoted.Set(key, voted);
		}
		if (voted.Find(sid) < 0)
			voted.Insert(sid);
		array<string> watching = new array<string>;
		foreach (string w, string wk : m_DL_FilmKey)
		{
			if (wk == key && DL_FindPlayer(w))
				watching.Insert(w);
		}
		int yes = 0;
		foreach (string ws : watching)
		{
			if (voted.Find(ws) >= 0)
				yes++;
		}
		bool skip = yes >= watching.Count();
		foreach (string to : watching)
			DL_SendSkip(to, yes, watching.Count(), skip);
		if (!skip)
			return;
		DL_Log.Detail("road film " + key + " skipped: all " + watching.Count().ToString() + " voted");
		int now = g_Game.GetTime();
		foreach (string ss : watching)
		{
			m_DL_FilmKey.Remove(ss);
			// a waiting room: it lifts as soon as everyone is seated (DL_RoomTick)
			foreach (string g, DL_Room room : m_DL_Rooms)
			{
				if (room.here.Find(ss) >= 0)
					room.revealAt = Math.Min(room.revealAt, now);
			}
			// on foot: the black screen goes now
			if (!DL_InRoom(ss))
			{
				m_DL_StoryCut.Set(ss, true);
				DL_SendWait(ss, DL_Const.WAIT_DONE, "", false, "");
				PlayerBase pb = DL_FindPlayer(ss);
				if (pb)
					pb.DL_SetHidden(false);
			}
		}
		m_DL_FilmVoted.Remove(key);
	}

	protected bool DL_InRoom(string sid)
	{
		foreach (string g, DL_Room room : m_DL_Rooms)
		{
			if (room.here.Find(sid) >= 0)
				return true;
		}
		return false;
	}

	protected void DL_SendSkip(string sid, int votes, int total, bool skip)
	{
		PlayerBase pb = DL_FindPlayer(sid);
		if (!pb || !pb.GetIdentity())
			return;
		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(votes);
		rpc.Write(total);
		rpc.Write(skip);
		rpc.Send(null, DL_Const.RPC_FILM_SKIP, true, pb.GetIdentity());
	}

	// ---------------- road film ----------------

	// this server's look in the film: the Travel setting, else from the map
	protected string DL_MyBiome()
	{
		if (m_DL_Cfg.travel && m_DL_Cfg.travel.biome != "")
			return m_DL_Cfg.travel.biome;
		string world;
		g_Game.GetWorldName(world);
		return DL_Film.BiomeOfWorld(world);
	}

	// no road events (refuse mode): the crew simply drives (or walks) the distance
	protected DL_Film DL_PlainFilm(DL_Zone z, array<PlayerBase> crew, Transport car)
	{
		DL_Film f = new DL_Film();
		f.km = z.distanceKm;
		if (car)
			f.car = DL_FilmCast.CarSprite(car.GetType());
		foreach (PlayerBase m : crew)
		{
			if (m && m.GetIdentity())
				f.crew.Insert(DL_FilmCast.From(m.GetIdentity().GetName(), m.GetIdentity().GetPlainId(), m_DL_Snap.Capture(m)));
		}
		f.km = DL_FilmKm(z);
		if (car)
			f.Add("drive", f.km);
		else
			f.Add("walk", f.km);
		return f;
	}

	// the distance a film shows: the zone's, at least 1 km (a free zone with no distance set still plays a short trip)
	protected float DL_FilmKm(DL_Zone z)
	{
		if (z.distanceKm < 1)
			return 1;
		return z.distanceKm;
	}

	// the arriving traveller's client gets his film once it has loaded (sent twice, the client plays it once);
	// returns how long it plays (0 = no film)
	protected float DL_SendFilmLater(string sid, DL_PlayerData d)
	{
		if (!d || !d.film || d.film.events.Count() == 0)
			return 0;
		if (m_DL_Cfg.travel && m_DL_Cfg.travel.filmOff)
			return 0;                                   // Travel settings: no film here, the story only
		d.film.biomeTo = DL_MyBiome();
		d.film.skip = "vote";
		if (m_DL_Cfg.travel && m_DL_Cfg.travel.voteSkipOff)
			d.film.skip = "none";
		// who watches this film together: his party, else him alone (a skip needs every one of them)
		string key = d.partyId;
		if (key == "")
			key = sid;
		m_DL_FilmKey.Set(sid, key);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_ForgetFilm, 300000, false, sid, key);
		string json;
		string err;
		JsonFileLoader<DL_Film>.MakeData(d.film, json, err, false);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SendFilm, 2500, false, sid, json);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SendFilm, 4500, false, sid, json);
		string dlMsg1218 = sid + "'s road film: " + d.film.events.Count().ToString() + " scenes, ";
		dlMsg1218 += d.film.biomeFrom + " -> " + d.film.biomeTo + ", ";
		dlMsg1218 += d.film.Duration().ToString() + " s";
		DL_Log.Detail(dlMsg1218);
		return d.film.Duration();
	}

	// the film's JSON in FILM_CHUNK pieces, one RPC each (guaranteed, in order); the client puts them together
	// CallLater dispatches dynamically and does not supply default arguments.
	// Keep the original two-argument callback, including its retry sends.
	protected void DL_SendFilm(string sid, string json)
	{
		DL_SendFilmRpc(sid, json, DL_Const.RPC_FILM);
	}

	protected void DL_SendFilmRpc(string sid, string json, int rpcType)
	{
		PlayerBase pb = DL_FindPlayer(sid);
		if (!pb || !pb.GetIdentity())
			return;
		int len = json.Length();
		int parts = (len + DL_Const.FILM_CHUNK - 1) / DL_Const.FILM_CHUNK;
		if (parts < 1)
			parts = 1;
		int id = json.Hash();
		for (int i = 0; i < parts; i++)
		{
			int at = i * DL_Const.FILM_CHUNK;
			int n = len - at;
			if (n > DL_Const.FILM_CHUNK)
				n = DL_Const.FILM_CHUNK;
			ScriptRPC rpc = new ScriptRPC();
			rpc.Write(id);
			rpc.Write(i);
			rpc.Write(parts);
			rpc.Write(json.Substring(at, n));
			rpc.Send(null, rpcType, true, pb.GetIdentity());
		}
		DL_Log.Detail(sid + "'s road film sent: " + len.ToString() + " chars in " + parts.ToString() + " parts");
	}

	// ---------------- M3 waiting room ----------------

	protected PlayerBase DL_BuildIntoRoom(PlayerIdentity identity, DL_ArriveResp r, DL_ArrivalPoint ap, PlayerBase oldChar, string group)
	{
		DL_PlayerData d = r.payload;
		string sid = identity.GetPlainId();
		bool isDriver = d.car != null && d.car.type != "";
		DL_Room room = m_DL_Rooms.Get(group);
		bool newRoom = room == null;
		if (newRoom)
		{
			room = new DL_Room();
			room.group = group;
			room.since = g_Game.GetTime();
			// the car will stand on carPos (or the arrival point), the crew waits beside that spot
			room.carPos = ap.carPos;
			room.carYaw = ap.carYaw;
			if (room.carPos[0] == 0 && room.carPos[2] == 0)
			{
				room.carPos = ap.pos;
				room.carYaw = ap.yaw;
			}
			// a convoy: each car one length behind the one before it
			if (d.convoyIdx > 0)
				room.carPos = room.carPos - Vector(room.carYaw, 0, 0).AnglesToVector() * (Math.Max(12,ap.convoySpacing) * d.convoyIdx);
			room.carPos[1] = g_Game.SurfaceY(room.carPos[0], room.carPos[2]);
			room.convoyId = d.convoyId;
			room.convoyCars = d.convoyCars;
			room.boat = d.vehKind == "boat" || (d.car && d.car.type != "" && DL_Veh.KindOf(d.car.type) == "boat");
			room.landPos = ap.pos;
			room.landYaw = ap.yaw;
			room.air = d.vehKind == "plane" || (d.car && d.car.type != "" && DL_Veh.KindOf(d.car.type) == "plane");
			if (room.air)
			{
				room.landPos = DL_LandNear(ap.pos, group);
				room.carPos = DL_AirSpot(ap, d.convoyIdx);
				room.carYaw = ap.airYaw;
				if (ap.airPos[0] == 0 && ap.airPos[2] == 0)
					room.carYaw = ap.yaw;
			}
			if (room.boat)
			{
				// the crew waits on dry land: an arrival point set out on the water would build them on the sea floor
				room.landPos = DL_LandNear(ap.pos, group);
				float byaw;
				room.carPos = DL_BoatSpot(ap, d.convoyIdx, group, byaw);
				room.carYaw = byaw;
			}
		}
		if (newRoom && d.car && d.car.type != "" && !room.boat && !room.air)
			room.carPos = DL_FreeCarSpot(room.carPos, room.carYaw, d.car.type, group);
        bool stagingForKind=ap.stagingEnabled && !room.air;
        if(room.boat && ap.stagingBoatPos[0]==0 && ap.stagingBoatPos[2]==0)stagingForKind=false;
        if(!room.boat && ap.stagingPos[0]==0 && ap.stagingPos[2]==0)stagingForKind=false;
        if (newRoom && stagingForKind)
        {
            room.finalCarPos=room.carPos;room.finalCarYaw=room.carYaw;room.finalLandPos=room.landPos;room.finalLandYaw=room.landYaw;
            vector assembly=ap.stagingPos;float assemblyYaw=ap.stagingYaw;
            if(room.boat){assembly=ap.stagingBoatPos;assemblyYaw=ap.stagingBoatYaw;}
            if((assembly[0]==0 && assembly[2]==0) || DL_DistanceXZ(assembly,room.finalCarPos)<100)
            {
                m_DL_BuildError="Arrival staging needs a configured point at least 100 m from the final arrival";return null;
            }
            if(DL_StagingHasObservers(assembly,group))
            {
                m_DL_BuildError="Arrival staging is occupied by another player; clear the area and retry";return null;
            }
            room.staged=true;room.carYaw=assemblyYaw;room.landYaw=assemblyYaw;
            float spacing=Math.Max(15,ap.stagingSpacing);
            assembly=assembly-Vector(assemblyYaw,0,0).AnglesToVector()*(spacing*d.convoyIdx);
            if(room.boat){room.carPos=DL_WaterNear(assembly,assemblyYaw,group);room.landPos=DL_LandNear(assembly,group);}
            else {assembly[1]=g_Game.SurfaceY(assembly[0],assembly[2]);vector stageSpot;if(!DL_AssemblyGroundSpot(assembly,assemblyYaw,d,group,spacing,stageSpot)){m_DL_BuildError="No clear, flat ground at the configured staging area";return null;}room.carPos=stageSpot;room.landPos=room.carPos;}
            if(room.boat && !DL_WaterFree(room.carPos,room.carYaw)){m_DL_BuildError="No clear water at the configured boat staging area";return null;}
            DL_Log.Info("arrival staging of "+group+" at "+room.carPos.ToString()+"; final "+room.finalCarPos.ToString());
        }
		if (d.crewCount > room.expected)
			room.expected = d.crewCount;
		if (d.journey != "")
			room.story = d.journey;
		float filmSec = DL_SendFilmLater(sid, d);
		room.filmSeconds = Math.Max(room.filmSeconds, filmSec);
		// the film starts once his client is in (sent at 2.5 and 4.5 s); the black screen stays until it ends
		int until = g_Game.GetTime() + 2000;
		if (filmSec > 0)
			until = g_Game.GetTime() + 4500 + filmSec * 1000 + 500;
		else if (d.journey != "")
			until = g_Game.GetTime() + 9000;              // time to read the story
		room.revealAt = Math.Max(room.revealAt, until);
		vector fwd = Vector(room.carYaw, 0, 0).AnglesToVector();
		vector right = Vector(room.carYaw + 90, 0, 0).AnglesToVector();
		vector pos;
		if (room.boat || room.air)
		{
			// a boat's (or plane's) crew waits on dry land at the arrival point (side by side) and is seated from
			// there: nobody goes into the water, or falls
			vector landRight = Vector(room.landYaw + 90, 0, 0).AnglesToVector();
			pos = room.landPos + landRight * (1.5 * room.here.Count());
			if (DL_Veh.WaterY(pos[0], pos[2], 0.05) >= 0)
				pos = room.landPos;                 // the row would step into the water: wait on the same dry spot
		}
		else if (isDriver)
			pos = room.carPos - right * 3;                                           // driver side
		else
			pos = room.carPos + right * 3 + fwd * (1.5 - 1.5 * room.here.Count());   // passengers in a row on the right
		pos[1] = g_Game.SurfaceY(pos[0], pos[2]);
		if (room.boat || room.air)
			pos[1] = Math.Max(pos[1], room.landPos[1]);   // never under the water surface
		PlayerBase player = CreateCharacter(identity, pos, null, d.charType);
		if (!player)
		{
			m_DL_BuildError = "could not create the character " + d.charType;
			return null;
		}
		player.DL_MarkTransferArrival();
		DL_GuardJoin(player,d);
		if (newRoom)
			m_DL_Rooms.Set(group, room);
		if (oldChar)
		{
			// only now that the new one exists (same as DL_BuildArrival)
			if (GetHive())
				GetHive().CharacterKill(oldChar);
			oldChar.Delete();
		}
		player.SetOrientation(Vector(room.carYaw, 0, 0));
		int dropped;
		int made = m_DL_Snap.Apply(player, d, dropped);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(m_DL_Snap.FixWeapons, 500, false, player);
		player.SetAllowDamage(false); // until the black screen lifts (+ arrivalProtectSeconds)
		player.DL_SetHidden(false);   // assembly is visible; staging supplies separation from the final arrival
		m_DL_Leaving.Remove(sid);
		if (room.here.Find(sid) < 0)
			room.here.Insert(sid);
		room.seats.Set(sid, d.carSeat);
		string carText;
		if (isDriver)
		{
			room.car = d.car;
			room.driverToken = r.token; // confirmed in DL_RoomGo: until then a crash replays the whole trip, car included
			carText = ", driving " + d.car.type + " (" + d.car.items.Count().ToString() + " parts/items, built once the crew is in)";
		}
		else
		{
			DL_Confirm(sid, r.token);
			carText = ", passenger in seat " + d.carSeat.ToString() + " of " + group + "'s car";
		}
		string dlMsg1369 = sid + " arrived from " + r.from + " at ";
		dlMsg1369 += ap.name + " " + pos.ToString() + ", ";
		dlMsg1369 += made.ToString() + "/" + DL_Hub.CountItems(d.items).ToString() + " items restored (";
		dlMsg1369 += dropped.ToString() + " on the ground, no room), health " + d.health.ToString() + ", bleeding ";
		dlMsg1369 += d.bleeding.Count().ToString() + ", modifiers " + d.modifiers.Count().ToString() + ", stomach ";
		dlMsg1369 += d.stomach.Count().ToString() + ", cuffed " + d.restrained.ToString() + carText;
		dlMsg1369 += ", waiting room " + room.here.Count().ToString() + "/" + room.expected.ToString();
		DL_Log.Info(dlMsg1369);
		NotificationSystem.SendNotificationToPlayerIdentityExtended(identity, 6, "DriveLink", "Welcome from " + r.from, "");
		if (!room.going && room.car && room.here.Count() >= room.expected)
		{
			room.going = true;
			// 2 s: his client is still loading; the seat juncture needs the character to exist there. The car is built
			// and everyone seated (invisible) while the story / road film plays: the black screen is the loading screen,
			// DL_RoomGo lifts it at room.revealAt at the earliest
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_RoomGo, 2000, false, group);
		}
		if (!m_DL_RoomTicking)
		{
			m_DL_RoomTicking = true;
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_RoomTick, 1000, true);
		}
		return player;
	}

	// every second: tell each member how many are in (a client that is still loading misses some; the next one comes),
	// and end rooms that waited too long
	protected void DL_RoomTick()
	{
		int now = g_Game.GetTime();
		array<string> release = new array<string>;
		array<string> startNow = new array<string>;
		foreach (string g, DL_Room room : m_DL_Rooms)
		{
			if (!room.going && now - room.since > m_DL_Cfg.crewWaitSeconds * 1000)
			{
				if (room.car)
					startNow.Insert(g);
				else
					release.Insert(g);
				continue;
			}
			string text = "Waiting for your crew: " + room.here.Count().ToString() + "/" + room.expected.ToString() + " connected";
			if (room.going)
				text = "Everyone is here. Getting in...";
			DL_ConvoyArrival ca = m_DL_Convoys.Get(room.convoyId);
			if (room.convoyReady && ca)
				text = "Waiting for the convoy: " + ca.ready.Count().ToString() + "/" + ca.expected.ToString() + " cars ready";
            if(room.deliveryBlocked!="")text=room.deliveryBlocked;
            if(room.builtCar && !room.air && room.deliveryEpoch==0)DL_SendStageControl(room,room.builtCar,true,0);
			foreach (string m : room.here)
				DL_SendWait(m, DL_Const.WAIT_SHOW, text, false, room.story);
		}
		// seated rooms: alone -> lift when the film is over; a convoy car -> ready, the convoy lifts all together
		array<string> reveal = new array<string>;
		foreach (string sg, DL_Room sroom : m_DL_Rooms)
		{
			if (!sroom.going || sroom.seatedAt == 0 || now < sroom.seatedAt)
				continue;
			if (sroom.convoyId != "" && sroom.convoyCars > 1)
			{
				if (!sroom.convoyReady)
					DL_ConvoyRoomReady(sg, sroom.builtCar);
			}
			else if (now >= sroom.revealAt)
				reveal.Insert(sg);
		}
		DL_ConvoyTick(now, reveal);
        DL_ReleaseArrivalBatch(reveal);
		foreach (string gs : startNow)
		{
			DL_Room rs = m_DL_Rooms.Get(gs);
			string dlMsg1437 = "waiting room of " + gs + ": waited " + m_DL_Cfg.crewWaitSeconds.ToString();
			dlMsg1437 += " s, going with " + rs.here.Count().ToString() + "/" + rs.expected.ToString();
			DL_Log.Detail(dlMsg1437);
			rs.going = true;
			DL_RoomGo(gs);
		}
		foreach (string gr : release)
			DL_RoomRelease(gr, "your driver did not arrive in time");
		if (m_DL_Rooms.Count() == 0)
		{
			m_DL_RoomTicking = false;
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(DL_RoomTick);
		}
	}

	protected void DL_SendWait(string sid, int phase, string text, bool startEngine, string story)
	{
		PlayerBase pb = DL_FindPlayer(sid);
		if (!pb || !pb.GetIdentity())
			return;
		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(phase);
		rpc.Write(text);
		rpc.Write(startEngine);
		rpc.Write(story); // older clients stop reading after startEngine
		rpc.Send(null, DL_Const.RPC_CREW_WAIT, true, pb.GetIdentity());
	}

	// all in (or waited long enough): build the car, confirm the driver's trip, seat the driver then each passenger
	protected void DL_RoomGo(string group)
	{
		DL_Room room = m_DL_Rooms.Get(group);
		if (!room)
			return;
		room.going = true;
		if(room.staged)
		{
			// Stage the crew and serialized vehicle data, not a live transport.
			room.seatedAt=GetGame().GetTime()+2000;
			DL_Log.Info("arrival staging: vehicle snapshot retained; live vehicle deferred until final destination");
			return;
		}
		if (room.air)
		{
		}
		else if (room.boat)
		{
			if (!DL_WaterFree(room.carPos, room.carYaw))
				room.carPos = DL_WaterNear(room.carPos, room.carYaw, group);
		}
		else
		{
			vector spot = DL_FreeCarSpot(room.carPos, room.carYaw, room.car.type, group);
			if (vector.Distance(spot, room.carPos) > 0.5)
			{
				room.carPos = spot;
				DL_MoveRoomCrew(room);
			}
		}
		int made;
		int dropped;
        m_DL_CarSnap.holdArrival=true;
		Transport car = m_DL_CarSnap.Build(room.car, room.carPos, Vector(room.carYaw, 0, 0), made, dropped);
        m_DL_CarSnap.holdArrival=false;
		string carText;
		if (car)
		{
			DL_GuardVehicle(room,car);
			car.SetAllowDamage(false);
			DL_Veh.SetHidden(car, false);
			if(room.staged){car.DisableSimulation(false);dBodyActive(car,ActiveState.ACTIVE);}
			if (room.air)
			{
				// up in the air: held where it is (no falling) while hidden; let go with its speed at the reveal
				car.SetPosition(room.carPos);
				car.SetOrientation(Vector(room.carYaw, 0, 0));
				dBodyActive(car, ActiveState.INACTIVE);
				car.DisableSimulation(true);
			}
			m_DL_GroupCar.Set(group, car);
			// passengers who come after this get seated in it (old path) for a while, then the tag is forgotten
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_ForgetGroupCar, m_DL_Cfg.crewWaitSeconds * 2000, false, group, car);
			carText = car.GetType() + " built (" + made.ToString() + "/" + room.car.items.Count().ToString() + " parts/items, " + dropped.ToString() + " on the ground)";
		}
		else
			carText = "could NOT create the car " + room.car.type + ", they arrive on foot";
		if (room.driverToken != "")
		{
			DL_Confirm(group, room.driverToken);
			room.driverToken = "";
		}
		string dlMsg1515 = "waiting room of " + group + ": " + room.here.Count().ToString();
		dlMsg1515 += "/" + room.expected.ToString() + " here, " + carText;
		DL_Log.Detail(dlMsg1515);
		int i = 0;
		if (car && !room.staged)
		{
			array<string> order = new array<string>;
			if (room.here.Find(group) >= 0)
				order.Insert(group);
			foreach (string m : room.here)
			{
				if (m != group)
					order.Insert(m);
			}
			foreach (string o : order)
			{
				g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SeatTraveller, 1500 + 800 * i, false, o, car, room.seats.Get(o));
				i++;
			}
		}
		// seated by then; the black screen lifts when DL_RoomTick sees that time and the film's end (room.revealAt, a
		// unanimous skip brings it forward) - for a convoy when all its cars are seated
		room.seatedAt = g_Game.GetTime() + 1500 + 800 * i + 2500;
		room.builtCar = car;
	}

	// a convoy car is built and seated: when all of the convoy's cars are (or after the convoy wait), all its rooms
	// lift their black screen together (DL_ConvoyTick)
	protected void DL_ConvoyRoomReady(string group, Transport car)
	{
		DL_Room room = m_DL_Rooms.Get(group);
		if (!room)
			return;
		room.convoyReady = true;
		DL_ConvoyArrival ca = m_DL_Convoys.Get(room.convoyId);
		if (!ca)
		{
			ca = new DL_ConvoyArrival();
			ca.id = room.convoyId;
			ca.expected = room.convoyCars;
			ca.firstReady = g_Game.GetTime();
			m_DL_Convoys.Set(ca.id, ca);
		}
		ca.ready.Set(group, car);
		string dlMsg1557 = "convoy " + ca.id + ": " + group;
		dlMsg1557 += "'s car is ready, " + ca.ready.Count().ToString() + "/" + ca.expected.ToString();
		DL_Log.Info(dlMsg1557);
	}

	// convoys whose cars are all seated (or waited long enough) and whose films are over (or skipped): their rooms
	// go into reveal, all at once
	protected void DL_ConvoyTick(int now, array<string> reveal)
	{
		array<string> done = new array<string>;
		foreach (string id, DL_ConvoyArrival ca : m_DL_Convoys)
		{
			bool all = ca.ready.Count() >= ca.expected;
			if (!all && now - ca.firstReady < DL_ConvoyWaitMs())
				continue;
			int at = 0;
			foreach (string g, Transport c : ca.ready)
			{
				DL_Room r = m_DL_Rooms.Get(g);
				if (r)
					at = Math.Max(at, r.revealAt);
			}
			if (now < at)
				continue;
            if(!ca.releasing){DL_Log.Detail("convoy "+id+": all available cars ready; preparing coordinated delivery");ca.releasing=true;}
            bool any=false;
            foreach(string rg,Transport rc:ca.ready)if(m_DL_Rooms.Contains(rg)){reveal.Insert(rg);any=true;}
            if(!any)done.Insert(id);
		}
		foreach (string d : done)
			m_DL_Convoys.Remove(d);
	}

	// ---------------- where a boat can float ----------------

	// the water spot for the k-th boat arriving at ap: the point's boat spot (and the places behind it) when set,
	// else the nearest free water around the arrival point; yaw = which way it faces
	protected vector DL_BoatSpot(DL_ArrivalPoint ap, int k, string who, out float yaw)
	{
		vector base = ap.pos;
		yaw = ap.yaw;
		if (ap.boatPos[0] != 0 || ap.boatPos[2] != 0)
		{
			base = ap.boatPos;
			yaw = ap.boatYaw;
		}
		vector want = base - Vector(yaw, 0, 0).AnglesToVector() * (9 * k);
		if (DL_WaterFree(want, yaw))
			return DL_OnWater(want);
		return DL_WaterNear(base, yaw, who);
	}

	// as DL_BoatSpot for the first boat, without logging; "0 0 0" = no free water near
	protected vector DL_BoatSpotQuiet(DL_ArrivalPoint ap, out float yaw)
	{
		vector base = ap.pos;
		yaw = ap.yaw;
		if (ap.boatPos[0] != 0 || ap.boatPos[2] != 0)
		{
			base = ap.boatPos;
			yaw = ap.boatYaw;
		}
		if (DL_WaterFree(base, yaw))
			return base;
		for (float r = 6; r <= 400; r += 6)
		{
			int n = Math.Max(8, Math.Floor(r / 4));
			for (int i = 0; i < n; i++)
			{
				vector q = base + Vector(i * 360.0 / n, 0, 0).AnglesToVector() * r;
				if (DL_WaterFree(q, yaw))
					return q;
			}
		}
		return "0 0 0";
	}

	// the nearest free water around p (rings of 6 m, up to 400 m), else p itself (logged)
	protected vector DL_WaterNear(vector p, float yaw, string who)
	{
		for (float r = 6; r <= 400; r += 6)
		{
			int n = Math.Max(8, Math.Floor(r / 4));
			for (int i = 0; i < n; i++)
			{
				vector q = p + Vector(i * 360.0 / n, 0, 0).AnglesToVector() * r;
				if (DL_WaterFree(q, yaw))
				{
					DL_Log.Detail(who + "'s boat goes on the water " + Math.Round(r).ToString() + " m from its spot");
					return DL_OnWater(q);
				}
			}
		}
		DL_Log.Warn("WARNING " + who + "'s boat: no free water within 400 m of " + p.ToString() + ", putting it there anyway");
		return DL_OnWater(p);
	}

	// water at least 1 m deep under the whole boat (2.6 x 6.5 m, so it isn't beached), and a box of the boat's size
	// over the water touches nothing but players and loose items (piers, rocks, other boats count)
	protected bool DL_WaterFree(vector p, float yaw)
	{
		vector fwd = Vector(yaw, 0, 0).AnglesToVector();
		vector right = Vector(yaw + 90, 0, 0).AnglesToVector();
		float y = DL_Veh.WaterY(p[0], p[2], 1.0);
		if (y < 0)
			return false;
		array<vector> corners = {p + fwd * 3.3, p - fwd * 3.3, p + right * 1.3, p - right * 1.3};
		foreach (vector c : corners)
		{
			if (DL_Veh.WaterY(c[0], c[2], 0.6) < 0)
				return false;
		}
		vector center = Vector(p[0], y + 1.2, p[2]);
		array<Object> hits = new array<Object>;
		array<Object> none = new array<Object>;
		if (!g_Game.IsBoxCollidingGeometry(center, Vector(yaw, 0, 0), Vector(3.2, 2, 7.3), ObjIntersectGeom, ObjIntersectView, none, hits))
			return true;
		foreach (Object o : hits)
		{
			if (!o || o.IsInherited(Man))
				continue;
			if (o.IsInherited(ItemBase) && !o.IsInherited(Transport))
				continue;
			return false;
		}
		return true;
	}

	// p if it is dry land, else the nearest dry ground within 150 m (rings of 5 m, 16 directions); else on the water
	protected vector DL_LandNear(vector p, string who)
	{
		if (DL_Veh.WaterY(p[0], p[2], 0.05) < 0)
			return p;
		for (float r = 5; r <= 150; r += 5)
		{
			for (int i = 0; i < 16; i++)
			{
				vector q = p + Vector(i * 22.5, 0, 0).AnglesToVector() * r;
				if (DL_Veh.WaterY(q[0], q[2], 0.05) < 0)
				{
					q[1] = g_Game.SurfaceY(q[0], q[2]);
					DL_Log.Info(who + ": the arrival point is on the water, the boat crew waits on the shore " + r.ToString() + " m away " + q.ToString());
					return q;
				}
			}
		}
		DL_Log.Warn(who + ": no dry land within 150 m of the arrival point, the boat crew waits on the water");
		return DL_OnWater(p);
	}

	// p on the water surface (a little above it, it settles)
	protected vector DL_OnWater(vector p)
	{
		vector q = p;
		float y = DL_Veh.WaterY(p[0], p[2], 0);
		if (y < 0)
			y = g_Game.SurfaceY(p[0], p[2]);
		q[1] = y + 0.2;
		return q;
	}

	// after the arrival: dry (a boat crew never touches the water, but nobody should stay wet from the trip)
	protected void DL_Dry(PlayerBase pb)
	{
		if (!pb)
			return;
		if (pb.GetStatWet())
			pb.GetStatWet().Set(0);
		array<EntityAI> worn = new array<EntityAI>;
		pb.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, worn);
		foreach (EntityAI e : worn)
		{
			ItemBase ib = ItemBase.Cast(e);
			if (ib && ib != pb && ib.GetWet() > 0)
				ib.SetWet(0);
		}
	}

	// ---------------- where a car can stand ----------------

	// log, per arrival point, which of the 4 convoy places behind its car spot a car fits on (admin info)
	protected void DL_CheckCarSpots()
	{
		DL_SpotsReq report = new DL_SpotsReq();
		foreach (DL_ArrivalPoint ap : m_DL_Cfg.arrivals)
		{
			DL_SpotReport rep = new DL_SpotReport();
			rep.arrival = ap.name;
			vector base = ap.carPos;
			float yaw = ap.carYaw;
			if (base[0] == 0 && base[2] == 0)
			{
				base = ap.pos;
				yaw = ap.yaw;
			}
			string line = "";
			for (int k = 0; k < 4; k++)
			{
				vector p = base - Vector(yaw, 0, 0).AnglesToVector() * (Math.Max(12,ap.convoySpacing) * k);
				p[1] = g_Game.SurfaceY(p[0], p[2]);
				string car = "free";
				string why;
				if (!DL_SpotFreeWhy(p, yaw, 2.2, 4.8, why))
					car = "blocked by " + why;
				string truck = "free";
				string twhy;
				if (!DL_SpotFreeWhy(p, yaw, 2.7, 7.8, twhy))
					truck = "blocked by " + twhy;
				DL_SpotPlace place = new DL_SpotPlace();
				place.car = car;
				place.truck = truck;
				rep.places.Insert(place);
				line += " | car " + (k + 1).ToString() + ": " + car + " (truck " + truck + ")";
			}
			// a boat: the pinned spot, or the nearest free water
			float byaw;
			vector bspot = DL_BoatSpotQuiet(ap, byaw);
			if (bspot[0] == 0 && bspot[2] == 0)
				rep.boat = "no free water within 400 m";
			else
			{
				rep.boatAt = bspot;
				if (ap.boatPos[0] != 0 || ap.boatPos[2] != 0)
				{
					if (vector.Distance(Vector(bspot[0], 0, bspot[2]), Vector(ap.boatPos[0], 0, ap.boatPos[2])) < 1)
						rep.boat = "its boat spot is free";
					else
						rep.boat = "its boat spot is taken or dry: free water " + Math.Round(vector.Distance(Vector(bspot[0], 0, bspot[2]), Vector(ap.boatPos[0], 0, ap.boatPos[2]))).ToString() + " m from it";
				}
				else
					rep.boat = "free water " + Math.Round(vector.Distance(Vector(bspot[0], 0, bspot[2]), Vector(ap.pos[0], 0, ap.pos[2]))).ToString() + " m from the point";
			}
			DL_Log.Detail("arrival '" + ap.name + "' car spots" + line + " | boat: " + rep.boat);
			report.spots.Insert(rep);
		}
		// to the hub: its Map page shows the places in green / red
		report.serverId = m_DL_Cfg.serverId;
		report.key = m_DL_Cfg.hubKey;
		string json;
		string err;
		JsonFileLoader<DL_SpotsReq>.MakeData(report, json, err, false);
		DL_Hub.Post(m_DL_Cfg.hubUrl, "/api/spots?serverId=" + m_DL_Cfg.serverId + "&key=" + m_DL_Cfg.hubKey, json, new DL_SpotsWaiter(), "OnAnswer", "spot check");
	}

	// the car's box (width, length, metres) by its class
	protected void DL_CarBox(string type, out float wid, out float len)
	{
		string t = type;
		t.ToLower();
		wid = 2.2;
		len = 4.8;
		if (t.Contains("truck"))
		{
			wid = 2.7;
			len = 7.8;
		}
		else if (t.Contains("offroad_02"))
		{
			wid = 2.4;
			len = 5.2;
		}
	}

	// want if a car of this type fits there, else the nearest spot that is free: further back in the same lane, then
	// a lane to the side (4.5 m, 9 m). Free = on dry land, not too steep, and a box of the car's size (from 0.5 m
	// above the ground) touches nothing but players and loose items (CGame.IsBoxCollidingGeometry, as base building
	// placement does).
	protected vector DL_FreeCarSpot(vector want, float yaw, string type, string who)
	{
		float wid;
		float len;
		DL_CarBox(type, wid, len);
		vector fwd = Vector(yaw, 0, 0).AnglesToVector();
		vector right = Vector(yaw + 90, 0, 0).AnglesToVector();
		array<float> sides = {0.0, 4.5, -4.5, 9.0, -9.0};
		array<float> backs = {0.0, 4.0, 8.0, 12.0, 16.0};
		foreach (float side : sides)
		{
			foreach (float back : backs)
			{
				vector p = want - fwd * back + right * side;
				p[1] = g_Game.SurfaceY(p[0], p[2]);
				if (DL_SpotFree(p, yaw, wid, len))
				{
					if (side != 0 || back != 0)
					{
						string dlMsg1840 = who + "'s " + type + ": its spot was taken, standing ";
						dlMsg1840 += back.ToString() + " m back, " + side.ToString() + " m to the side";
						DL_Log.Detail(dlMsg1840);
					}
					return p;
				}
			}
		}
		string dlMsg1845 = "WARNING " + who + "'s " + type;
		dlMsg1845 += ": no free spot near " + want.ToString() + ", using it anyway (damage is off for a while)";
		DL_Log.Warn(dlMsg1845);
		return want;
	}

	protected bool DL_SpotFree(vector p, float yaw, float wid, float len)
	{
		string why;
		return DL_SpotFreeWhy(p, yaw, wid, len, why);
	}

	// why = what is in the way ("water", "slope", or the object's class / model)
	protected bool DL_SpotFreeWhy(vector p, float yaw, float wid, float len, out string why)
	{
		why = "";
		if (g_Game.SurfaceIsSea(p[0], p[2]) || g_Game.SurfaceIsPond(p[0], p[2]))
		{
			why = "water";
			return false;
		}
		vector fwd = Vector(yaw, 0, 0).AnglesToVector();
		vector right = Vector(yaw + 90, 0, 0).AnglesToVector();
		vector f = p + fwd * (len / 2);
		vector b = p - fwd * (len / 2);
		vector l = p - right * (wid / 2);
		vector r = p + right * (wid / 2);
		if (Math.AbsFloat(g_Game.SurfaceY(f[0], f[2]) - g_Game.SurfaceY(b[0], b[2])) > len * 0.35 || Math.AbsFloat(g_Game.SurfaceY(l[0], l[2]) - g_Game.SurfaceY(r[0], r[2])) > wid * 0.35)
		{
			why = "slope";
			return false;
		}
		vector center = p;
		center[1] = p[1] + 1.5;                         // a 2 m tall box from 0.5 m up: bumps in the road don't count
		array<Object> hits = new array<Object>;
		array<Object> none = new array<Object>;
		if (!g_Game.IsBoxCollidingGeometry(center, Vector(yaw, 0, 0), Vector(wid + 0.6, 2, len + 0.8), ObjIntersectGeom, ObjIntersectView, none, hits))
			return true;
		foreach (Object o : hits)
		{
			if (!o || o.IsInherited(Man))
				continue;                               // the crew waiting there (they move with the car)
			if (o.IsInherited(ItemBase) && !o.IsInherited(Transport))
				continue;                               // loose loot
			why = o.GetType();
			if (why == "")
			{
				why = o.GetModelName();
				if (why == "")
					why = "an object";
			}
			return false;
		}
		return true;
	}

	// the car of a waiting room moved: its crew stand beside the new spot (driver left, passengers right)
	protected void DL_MoveRoomCrew(DL_Room room)
	{
		vector fwd = Vector(room.carYaw, 0, 0).AnglesToVector();
		vector right = Vector(room.carYaw + 90, 0, 0).AnglesToVector();
		int n = 0;
		foreach (string m : room.here)
		{
			PlayerBase pb = DL_FindPlayer(m);
			if (!pb)
				continue;
			vector pos;
			if (m == room.group)
				pos = room.carPos - right * 3;
			else
			{
				pos = room.carPos + right * 3 + fwd * (1.5 - 1.5 * n);
				n++;
			}
			pos[1] = g_Game.SurfaceY(pos[0], pos[2]);
			if(room.boat)
			{
				pos=room.landPos+Vector(room.landYaw+90,0,0).AnglesToVector()*(1.5*n);n++;
				if(DL_Veh.WaterY(pos[0],pos[2],0.05)>=0)pos=room.landPos;
				pos[1]=Math.Max(g_Game.SurfaceY(pos[0],pos[2]),room.landPos[1]);
			}
			pb.SetPosition(pos);
			pb.Update();
			pb.ForceCorrection();
		}
	}

	// everyone seated: lift the black screen; the driver's client starts the engine (PHYSICS cars are simulated on the
	// driver's machine, so vanilla ActionStartEngine starts them there too, actionstartengine.c:51-65)
	protected void DL_RoomReveal(string group, Transport car)
	{
		DL_Room room = m_DL_Rooms.Get(group);
		if (!room)
			return;
		m_DL_Rooms.Remove(group);
		bool serverStarts = car != null && car.GetNetworkMoveStrategy() != NetworkMoveStrategy.PHYSICS;
		if (car)
			DL_Veh.SetHidden(car, false);
        if(car && !room.air && !room.staged)
        {
            SetVelocity(car,vector.Zero);dBodySetAngularVelocity(car,vector.Zero);
            car.Synchronize();car.DisableSimulation(false);dBodyActive(car,ActiveState.ACTIVE);
            DL_SendStageControl(room,car,false,room.deliveryEpoch);
        }
        if(car && room.staged){DL_ArrivalPose.Resume(car);DL_SendStageControl(room,car,false,room.deliveryEpoch);}
		int seated = 0;
		foreach (string m : room.here)
		{
			PlayerBase pb = DL_FindPlayer(m);
			if (!pb)
				continue;
			pb.DL_SetHidden(false);
			DL_Dry(pb);
			if (car && car.CrewMemberIndex(pb) >= 0)
				seated++;
			bool engine = car != null && m == group && car.CrewDriver() == pb;
			DL_SendWait(m, DL_Const.WAIT_DONE, "", engine, "");
			DL_GuardReady(pb);
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SendWait, 1500, false, m, DL_Const.WAIT_DONE, "", engine, ""); // in case the first is lost
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_EndProtection, m_DL_Cfg.arrivalProtectSeconds * 1000, false, pb);
		}
		if (serverStarts && car.CrewDriver())
			DL_Veh.EngineStart(car);
		if (car && room.air)
			DL_LetFly(group, car, room);
		string how = "driver's client";
		if (serverStarts)
			how = "server";
		string dlMsg1956 = "waiting room of " + group + ": " + seated.ToString();
		dlMsg1956 += "/" + room.here.Count().ToString() + " seated, black screen lifted, engine start by the " + how;
		DL_Log.Detail(dlMsg1956);
		if (car)
		{
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_EndCarProtection, m_DL_Cfg.arrivalProtectSeconds * 1000, false, car);
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_RoomEngineCheck, 4000, false, group, car);
		}
	}

	// a plane is let go in the air: engine on, flying the way it faces at the speed it left with (at least a safe
	// cruise), set here and on the pilot's client (cars are simulated by the driver's client)
	protected void DL_LetFly(string group, Transport car, DL_Room room)
	{
		car.DisableSimulation(false);
		dBodyActive(car, ActiveState.ACTIVE);
		float speed = 0;
		if (room.car)
			speed = room.car.vel.Length();
		if (speed < DL_AIR_MIN_SPEED)
			speed = DL_AIR_MIN_SPEED;
		vector vel = car.GetDirection() * speed;
		DL_Veh.EngineStart(car);
		Car pc = Car.Cast(car);
		if (pc)
			pc.ShiftTo(CarGear.FIRST);           // flying forward (planes only change gear standing still)
		SetVelocity(car, vel);
		PlayerBase pilot = PlayerBase.Cast(car.CrewDriver());
		if (pilot && pilot.GetIdentity())
		{
			ScriptRPC rpc = new ScriptRPC();
			rpc.Write(vel);
			rpc.Send(null, DL_Const.RPC_FLY, true, pilot.GetIdentity());
		}
		string dlMsg1988 = "waiting room of " + group + ": " + car.GetType();
		dlMsg1988 += " let go in the air at " + car.GetPosition().ToString() + ", " + Math.Round(speed * 3.6).ToString();
		dlMsg1988 += " km/h, engine " + DL_Veh.EngineIsOn(car).ToString();
		DL_Log.Detail(dlMsg1988);
	}

	// where the k-th plane of a formation comes in: the plane entry (or above the arrival point), at its height
	// above the ground (or the sea), each one further back and off to the right
	protected vector DL_AirSpot(DL_ArrivalPoint ap, int k)
	{
		vector p = ap.airPos;
		float yaw = ap.airYaw;
		if (p[0] == 0 && p[2] == 0)
		{
			p = ap.pos;
			yaw = ap.yaw;
		}
		float height = ap.airPos[1];
		if (height < 50)
			height = DL_AIR_HEIGHT;
		vector fwd = Vector(yaw, 0, 0).AnglesToVector();
		vector right = Vector(yaw + 90, 0, 0).AnglesToVector();
		p = p - fwd * (60 * k) + right * (30 * k);
		float below = g_Game.SurfaceY(p[0], p[2]);
		if (g_Game.SurfaceIsSea(p[0], p[2]))
			below = Math.Max(below, g_Game.SurfaceGetSeaLevel());
		p[1] = below + height + 10 * k;
		return p;
	}

	protected void DL_RoomEngineCheck(string group, Transport car)
	{
		if (car)
		{
			string dlMsg2018 = "waiting room of " + group + ": engine of the " + car.GetType();
			dlMsg2018 += " running " + DL_Veh.EngineIsOn(car).ToString() + ", " + DL_CarSnapshot.FluidText(DL_CarSnapshot.FluidsOf(car));
			DL_Log.Detail(dlMsg2018);
		}
	}

	// no car is coming: the passengers go on on foot
	protected void DL_RoomRelease(string group, string why)
	{
		DL_Room room = m_DL_Rooms.Get(group);
		if (!room)
			return;
		if(room.staged)
		{
			room.carPos=room.finalCarPos;room.carYaw=room.finalCarYaw;
			room.landPos=room.finalLandPos;room.landYaw=room.finalLandYaw;
			DL_MoveRoomCrew(room);
		}
		m_DL_Rooms.Remove(group);
		string dlMsg2028 = "waiting room of " + group + " closed: " + why;
		dlMsg2028 += " (" + room.here.Count().ToString() + "/" + room.expected.ToString();
		dlMsg2028 += " here, on foot)";
		DL_Log.Detail(dlMsg2028);
		foreach (string m : room.here)
		{
			PlayerBase pb = DL_FindPlayer(m);
			if (!pb)
				continue;
			pb.DL_SetHidden(false);
			DL_SendWait(m, DL_Const.WAIT_DONE, "", false, "");
			DL_GuardReady(pb);
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_EndProtection, m_DL_Cfg.arrivalProtectSeconds * 1000, false, pb);
			NotificationSystem.SendNotificationToPlayerIdentityExtended(pb.GetIdentity(), 10, "DriveLink", "No car: " + why + ". You continue on foot.", "");
		}
	}

	protected void DL_ForgetGroupCar(string group, Transport car)
	{
		if (m_DL_GroupCar.Get(group) == car)
			m_DL_GroupCar.Remove(group);
	}

	// M3: the driver's car just arrived; seat the passengers who got here before it (one per second)
	protected void DL_BringWaitingCrew(string driverSid, Transport car)
	{
		if (!car)
			return;
		array<string> mine = new array<string>;
		foreach (string psid, DL_SeatWait w : m_DL_SeatWait)
		{
			if (w && w.group == driverSid)
				mine.Insert(psid);
		}
		int k = 0;
		foreach (string p : mine)
		{
			DL_SeatWait sw = m_DL_SeatWait.Get(p);
			m_DL_SeatWait.Remove(p);
			if (g_Game.GetTime() - sw.since > 300000)
				continue; // waited over 5 min: he has walked off by now
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_BringToCar, 1000 * k, false, p, car, sw.seat);
			k++;
		}
		if (mine.Count() > 0)
			DL_Log.Info(driverSid + "'s car is here, bringing in " + k.ToString() + " waiting passenger(s)");
	}

	protected void DL_BringToCar(string psid, Transport car, int seat)
	{
		PlayerBase pb = DL_FindPlayer(psid);
		if (!pb || !car || pb.GetCommand_Vehicle())
			return;
		vector ori = car.GetOrientation();
		pb.SetPosition(car.GetPosition() + Vector(ori[0] + 90, 0, 0).AnglesToVector() * 3);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SeatTraveller, 1500, false, psid, car, seat);
	}

	protected void DL_SeatTraveller(string sid, Transport car, int seat)
	{
		PlayerBase pb = DL_FindPlayer(sid);
		if (!pb || !car)
		{
			DL_Log.Warn("could not seat " + sid + ": player or car gone");
			return;
		}
		if (seat < 0)
			seat = 0;
		pb.DL_SeatIn(car, seat);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SeatTravellerCheck, 4000, false, sid, car);
	}

	protected void DL_SeatTravellerCheck(string sid, Transport car)
	{
		PlayerBase pb = DL_FindPlayer(sid);
		if (pb && car)
		{
			string dlMsg2100 = sid + " seated in his " + car.GetType() + ": ";
			dlMsg2100 += (car.CrewMemberIndex(pb) >= 0).ToString() + " (seat " + car.CrewMemberIndex(pb).ToString() + "), ";
			dlMsg2100 += DL_CarSnapshot.FluidText(DL_CarSnapshot.FluidsOf(car));
			DL_Log.Detail(dlMsg2100);
		}
	}

	protected void DL_EndCarProtection(Transport car)
	{
		if(m_DL_GuardOf.Contains(car))return;
		if (car)
			car.SetAllowDamage(true);
	}

	// Tell the hub the character exists here now (hub /api/confirm claims the transfer + sets the owner).
	protected void DL_Confirm(string sid, string token)
	{
		DL_TokenReq c = DL_TokenRequest(sid, token);
		DL_Hub.Post(m_DL_Cfg.hubUrl, "/api/confirm?serverId=" + m_DL_Cfg.serverId + "&key=" + m_DL_Cfg.hubKey, DL_Hub.TokenJson(c), new DL_ConfirmWaiter(sid, "arrival"), "OnAnswer", "confirm " + sid);
	}

	// Object.SetAllowDamage is proto native (3_game/entities/object.c:1192); call it from a script method for the queue.
	protected void DL_EndProtection(PlayerBase player)
	{
		if(m_DL_GuardOf.Contains(player))return;
		if (player)
			player.SetAllowDamage(true);
	}

	// ---------------- departure ----------------

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (!DL_On())
			return;
		m_DL_Tick += timeslice;
		if (m_DL_Tick < m_DL_Cfg.checkEverySeconds)
			return;
		m_DL_Tick = 0;
		DL_GuardTick();
		DL_CheckZones();
#ifdef DL_DEV
		DL_DebugTick();
#endif
		DL_TakeFilmVotes();
		DL_TakeDeaths();
	}

	protected void DL_CheckZones()
	{
		array<Man> men = new array<Man>;
		g_Game.GetPlayers(men);
		int now = g_Game.GetTime();
		foreach (Man m : men)
		{
			PlayerBase pb = PlayerBase.Cast(m);
			if (!pb || !pb.GetIdentity())
				continue;
			string sid = pb.GetIdentity().GetPlainId();
			if (m_DL_Leaving.Contains(sid))
				continue;
			DL_Zone z = DL_ZoneFor(pb);
			DL_Party party = DL_PartyOf(sid);
			if (!z || !pb.IsAlive())
			{
				m_DL_MustLeaveZone.Remove(sid);
				continue;                       // out of the zone: DL_TendParties drops him
			}
			// a failed trip doesn't retry by itself (test 2026-10-18: "Travel failed" every 5 s while standing in it)
			if (m_DL_MustLeaveZone.Contains(sid))
				continue;
			// M3: in a car the DRIVER leads the whole crew; a passenger with a player at the wheel goes with him
			if (DL_IsCrewPassenger(pb))
				continue;
			string carProblem = DL_CarProblem(pb);
			if (carProblem != "")
			{
				m_DL_MustLeaveZone.Set(sid, true);
				NotificationSystem.SendNotificationToPlayerIdentityExtended(pb.GetIdentity(), 8, "DriveLink", carProblem, "");
				continue;
			}
			if (party)
				continue;
			// travel cost (Research/travel-cost-design.md): the price goes on the countdown; in refuse mode a crew
			// that can't pay doesn't get one
			DL_TravelPrice price = DL_TravelCost.Price(m_DL_Cfg.travel, z, DL_CarOf(pb) != null, DL_CarOf(pb));
			string missing = DL_TravelCost.Shortfall(price, DL_CrewOf(pb), DL_CarOf(pb), m_DL_Snap, m_DL_CarSnap);
			if (missing != "" && DL_RefusesShortTrips())
			{
				m_DL_MustLeaveZone.Set(sid, true);
				string dlMsg2184 = sid + " can't pay the trip to " + z.to + " (";
				dlMsg2184 += price.Text() + "): " + missing;
				DL_Log.Info(dlMsg2184);
				DL_CrewNotify(pb, 12, "Not enough for " + z.name, price.Text() + ". Missing: " + missing + ". Step out of the zone and come back when you have it.");
				continue;
			}
			string priceText = "";
			if (price)
				priceText = price.Text();
			DL_JoinParty(pb, z, priceText, now);
		}
		DL_TendParties(now);
	}

	protected int DL_GroupMax()
	{
		if (m_DL_Cfg.travel && m_DL_Cfg.travel.groupMax >= 2)
			return m_DL_Cfg.travel.groupMax;
		return DL_GROUP_MAX;
	}

	protected int DL_ConvoyMax()
	{
		if (m_DL_Cfg.travel && m_DL_Cfg.travel.convoyMax >= 2)
			return m_DL_Cfg.travel.convoyMax;
		return DL_CONVOY_MAX;
	}

	// may this kind travel together (Travel settings: groups on foot / convoys on)?
	protected bool DL_Together(bool convoy)
	{
		if (!m_DL_Cfg.travel)
			return true;
		if (convoy)
			return !m_DL_Cfg.travel.noConvoys;
		return !m_DL_Cfg.travel.noGroups;
	}

	protected int DL_ConvoyWaitMs()
	{
		if (m_DL_Cfg.travel && m_DL_Cfg.travel.convoyWaitSeconds >= 10)
			return m_DL_Cfg.travel.convoyWaitSeconds * 1000;
		return m_DL_Cfg.crewWaitSeconds * 1000;
	}

	// his party forming in a zone, or null
	protected DL_Party DL_PartyOf(string sid)
	{
		foreach (DL_Party p : m_DL_Parties)
		{
			if (p.leaders.Find(sid) >= 0)
				return p;
		}
		return null;
	}

	// into the zone: with the party counting down there (same kind, room left) - its countdown starts again - or a
	// new one
	protected void DL_JoinParty(PlayerBase pb, DL_Zone z, string priceText, int now)
	{
		string sid = pb.GetIdentity().GetPlainId();
		bool convoy = DL_CarOf(pb) != null;
		string kind = DL_PartyKind(pb);
		DL_Party party;
		if (DL_Together(convoy))
		{
			foreach (DL_Party p : m_DL_Parties)
			{
				int room = DL_GroupMax();
				if (p.convoy)
					room = DL_ConvoyMax();
				if (p.zone == z && p.kind == kind && p.leaders.Count() < room)
				{
					party = p;
					break;
				}
			}
		}
		if (!party)
		{
			party = new DL_Party();
			m_DL_PartyN++;
			party.id = m_DL_PartyN;
			party.zone = z;
			party.convoy = convoy;
			party.kind = kind;
			m_DL_Parties.Insert(party);
		}
		party.leaders.Insert(sid);
		party.prices.Set(sid, priceText);
		party.since = now;
		if (party.leaders.Count() > 1)
		{
			string dlMsg2274 = sid + " joins the " + DL_PartyWord(party) + " ";
			dlMsg2274 += party.id.ToString() + " in " + z.name + ": ";
			dlMsg2274 += party.leaders.Count().ToString() + " now, countdown again";
			DL_Log.Info(dlMsg2274);
		}
		DL_PartyCountdown(party, m_DL_Cfg.countdownSeconds);
	}

	// on foot, or a land vehicle (car, truck, motorbike: they convoy together), or a boat (only with boats)
	protected string DL_PartyKind(PlayerBase pb)
	{
		Transport t = DL_CarOf(pb);
		if (!t)
			return "foot";
		if (DL_Veh.IsBoat(t))
			return "water";
		if (DL_Veh.IsPlane(t))
			return "air";                // planes fly together, nothing else with them
		return "land";
	}

	protected string DL_PartyWord(DL_Party party)
	{
		if (party.convoy)
			return "convoy";
		return "group";
	}

	// the countdown (0 = cancelled) to every member of the party, with how many are going together
	protected void DL_PartyCountdown(DL_Party party, int seconds)
	{
		string zoneName = party.zone.name;
		if (party.leaders.Count() > 1)
		{
			if (party.kind == "water")
				zoneName += " - convoy of " + party.leaders.Count().ToString() + " boats";
			else if (party.kind == "air")
				zoneName += " - formation of " + party.leaders.Count().ToString() + " planes";
			else if (party.convoy)
				zoneName += " - convoy of " + party.leaders.Count().ToString() + " vehicles";
			else
				zoneName += " - group of " + party.leaders.Count().ToString();
		}
		foreach (string l : party.leaders)
		{
			PlayerBase pb = DL_FindPlayer(l);
			if (pb)
				DL_CrewCountdown(pb, seconds, zoneName, party.prices.Get(l));
		}
	}

	// every tick: drop who left (the zone, the car, the wheel), send the parties whose countdown ran out
	protected void DL_TendParties(int now)
	{
		array<DL_Party> go = new array<DL_Party>;
		for (int i = m_DL_Parties.Count() - 1; i >= 0; i--)
		{
			DL_Party party = m_DL_Parties[i];
			bool dropped = false;
			for (int k = party.leaders.Count() - 1; k >= 0; k--)
			{
				string l = party.leaders[k];
				PlayerBase pb = DL_FindPlayer(l);
				bool inCar = false;
				if (pb)
					inCar = DL_CarOf(pb) != null;
				string nowKind = "";
				if (pb)
					nowKind = DL_PartyKind(pb);
				bool stays = pb && pb.IsAlive() && !m_DL_Leaving.Contains(l) && !m_DL_MustLeaveZone.Contains(l) && DL_ZoneFor(pb) == party.zone && !DL_IsCrewPassenger(pb) && nowKind == party.kind;
				if (stays)
					continue;
				party.leaders.RemoveOrdered(k);
				party.prices.Remove(l);
				dropped = true;
				if (pb && !m_DL_Leaving.Contains(l))
					DL_CrewCountdown(pb, 0, "", "");
			}
			if (party.leaders.Count() == 0)
			{
				m_DL_Parties.RemoveOrdered(i);
				continue;
			}
			if (dropped && party.leaders.Count() > 0)
			{
				int left = m_DL_Cfg.countdownSeconds - (now - party.since) / 1000;
				if (left < 1)
					left = 1;
				DL_PartyCountdown(party, left);
			}
			if (now - party.since >= m_DL_Cfg.countdownSeconds * 1000)
			{
				go.Insert(party);
				m_DL_Parties.RemoveOrdered(i);
			}
		}
		foreach (DL_Party gp : go)
			DL_DepartParty(gp);
	}

	// the countdown of a party ran out: one unit = the usual trip; more = one road for all, each unit sent as usual
	protected void DL_DepartParty(DL_Party party)
	{
		DL_Zone z = party.zone;
		array<ref DL_PartyUnit> units = new array<ref DL_PartyUnit>;
		foreach (string l : party.leaders)
		{
			PlayerBase pb = DL_FindPlayer(l);
			if (!pb)
				continue;
			DL_PartyUnit u = new DL_PartyUnit();
			u.leader = pb;
			u.car = DL_CarOf(pb);
			array<PlayerBase> pbCrew = DL_CrewOf(pb);
			foreach (PlayerBase c : pbCrew)
				u.crew.Insert(c);
			units.Insert(u);
		}
		if (units.Count() == 0)
			return;
		if (units.Count() == 1)
		{
			DL_Depart(units[0].leader, z);
			return;
		}
		// each unit pays its own way (refuse mode: one that can't stays here, the others go)
		array<ref DL_PartyUnit> going = new array<ref DL_PartyUnit>;
		array<ref DL_TravelPrice> prices = new array<ref DL_TravelPrice>;
		foreach (DL_PartyUnit gu : units)
		{
			DL_TravelPrice price = DL_TravelCost.Price(m_DL_Cfg.travel, z, gu.car != null, gu.car);
			string missing = DL_TravelCost.Shortfall(price, gu.crew, gu.car, m_DL_Snap, m_DL_CarSnap);
			if (missing != "" && DL_RefusesShortTrips())
			{
				string gsid = gu.leader.GetIdentity().GetPlainId();
				m_DL_MustLeaveZone.Set(gsid, true);
				string dlMsg2406 = gsid + " can't pay the trip to " + z.to + " any more (";
				dlMsg2406 += price.Text() + "): " + missing + " - the ";
				dlMsg2406 += DL_PartyWord(party) + " goes without him";
				DL_Log.Info(dlMsg2406);
				DL_CrewNotify(gu.leader, 12, "Not enough for " + z.name, price.Text() + ". Missing: " + missing + ". The others went on.");
				continue;
			}
			going.Insert(gu);
			prices.Insert(price);
		}
		if (going.Count() == 0)
			return;
		if (going.Count() == 1)
		{
			DL_Depart(going[0].leader, z);
			return;
		}
		// weakened mode: the road is played now, once for the whole party
		DL_Journey journey;
		if (prices[0] && !DL_RefusesShortTrips())
		{
			journey = DL_Journey.RunParty(m_DL_Cfg.travel, z, going, m_DL_Snap, m_DL_CarSnap, true);
			string jtext = journey.Story();
			if (jtext == "")
				jtext = "(an uneventful trip)";
			jtext.Replace("\n", " | ");
			string dlMsg2429 = DL_PartyWord(party) + " " + party.id.ToString() + "'s road to ";
			dlMsg2429 += z.to + ": driven " + Math.Round(journey.drivenKm).ToString() + " km, walked ";
			dlMsg2429 += Math.Round(journey.walkedKm).ToString() + " km, " + journey.Alive().ToString() + "/";
			dlMsg2429 += journey.crew.Count().ToString() + " alive, all cars lost " + journey.carLost.ToString() + ", ";
			dlMsg2429 += journey.seconds.ToString() + " s: " + jtext;
			DL_Log.Info(dlMsg2429);
		}
		DL_Film film;
		if (journey)
			film = journey.film;
		else
			film = DL_PartyFilm(z, going);      // also a free (0 km) trip, e.g. a short boat crossing: they still see it
		if (film)
		{
			film.biomeFrom = DL_MyBiome();
			film.biomeVia = z.viaBiome;
		}
		string pid = m_DL_Cfg.serverId + "-" + party.id.ToString() + "-" + g_Game.GetTime().ToString();
		foreach (DL_PartyUnit pu : going)
		{
			foreach (PlayerBase pm : pu.crew)
			{
				if (pm && pm.GetIdentity())
					m_DL_PartyIdOf.Set(pm.GetIdentity().GetPlainId(), pid);
			}
		}
		if (film)
			film.biomeVia = z.viaBiome;
		if (party.convoy)
		{
			string cid = pid;
			int cars = 0;
			if (journey)
			{
				foreach (DL_JVehicle jv : journey.vehicles)
				{
					if (!jv.lost)
						cars++;
				}
			}
			else
			{
				foreach (DL_PartyUnit cu : going)
				{
					if (cu.car)
						cars++;
				}
			}
			foreach (DL_PartyUnit mu : going)
			{
				foreach (PlayerBase mm : mu.crew)
				{
					if (mm && mm.GetIdentity())
					{
						m_DL_ConvoyOf.Set(mm.GetIdentity().GetPlainId(), cid);
						m_DL_ConvoyCarsOf.Set(mm.GetIdentity().GetPlainId(), cars);
					}
				}
			}
		}
		int carIdx = 0;
		int footIdx = 0;
		for (int i = 0; i < going.Count(); i++)
		{
			DL_PartyUnit du = going[i];
			int slot = footIdx;
			if (du.car)
				slot = carIdx;
			foreach (PlayerBase cm : du.crew)
			{
				if (cm && cm.GetIdentity())
					m_DL_SlotOf.Set(cm.GetIdentity().GetPlainId(), slot);
			}
			if (du.car)
				carIdx++;
			else
				footIdx++;
			DL_DepartUnit(du.leader, z, du.car, du.crew, prices[i], journey, film);
		}
		string dlMsg2503 = DL_PartyWord(party) + " " + party.id.ToString() + " leaves for ";
		dlMsg2503 += z.to + ": " + going.Count().ToString() + " ";
		dlMsg2503 += DL_PartyWord(party) + " units together";
		DL_Log.Info(dlMsg2503);
	}

	// refuse mode (no road events): the plain film of a party - every car with its crew, or the group walking
	protected DL_Film DL_PartyFilm(DL_Zone z, array<ref DL_PartyUnit> units)
	{
		DL_Film f = new DL_Film();
		f.km = z.distanceKm;
		bool cars = false;
		foreach (DL_PartyUnit u : units)
		{
			int idx = -1;
			if (u.car)
			{
				idx = f.cars.Count();
				f.cars.Insert(DL_FilmCast.CarSprite(u.car.GetType()));
				cars = true;
			}
			foreach (PlayerBase m : u.crew)
			{
				if (!m || !m.GetIdentity())
					continue;
				DL_FilmActor a = DL_FilmCast.From(m.GetIdentity().GetName(), m.GetIdentity().GetPlainId(), m_DL_Snap.Capture(m));
				a.car = idx;
				f.crew.Insert(a);
			}
		}
		if (f.cars.Count() == 1)
			f.car = f.cars[0];
		f.km = DL_FilmKm(z);
		if (cars)
			f.Add("drive", f.km);
		else
			f.Add("walk", f.km);
		return f;
	}

	// The countdown (or its cancel, seconds 0) goes to him and, at the wheel of a car, to everyone in it.
	protected void DL_CrewCountdown(PlayerBase pb, int seconds, string zoneName, string priceText)
	{
		array<PlayerBase> crew = DL_CrewOf(pb);
		foreach (PlayerBase c : crew)
		{
			if (c && c.GetIdentity())
				DL_Countdown(c.GetIdentity(), seconds, zoneName, priceText);
		}
	}

	protected void DL_CrewNotify(PlayerBase pb, float seconds, string title, string text)
	{
		array<PlayerBase> crew = DL_CrewOf(pb);
		foreach (PlayerBase c : crew)
		{
			if (c && c.GetIdentity())
				NotificationSystem.SendNotificationToPlayerIdentityExtended(c.GetIdentity(), seconds, "DriveLink: " + title, text, "");
		}
	}

	// travel rules: "refuse" (default) = a crew short of food/water/fuel can't leave; "weakened" = it can (step 4 adds
	// the road events; until then the cost is simply taken, never below 0)
	protected bool DL_RefusesShortTrips()
	{
		return !m_DL_Cfg.travel || m_DL_Cfg.travel.mode != "weakened";
	}

	// pb plus, when he drives a car, every other player in it (seat order)
	protected array<PlayerBase> DL_CrewOf(PlayerBase pb)
	{
		array<PlayerBase> crew = new array<PlayerBase>;
		crew.Insert(pb);
		Transport car = DL_CarOf(pb);
		if (!car || car.CrewDriver() != pb)
			return crew;
		for (int c = 0; c < car.CrewSize(); c++)
		{
			PlayerBase m = PlayerBase.Cast(car.CrewMember(c));
			if (m && m != pb)
				crew.Insert(m);
		}
		return crew;
	}

	// a passenger of a car with a player at the wheel: that driver's trip takes him
	protected bool DL_IsCrewPassenger(PlayerBase pb)
	{
		Transport car = DL_CarOf(pb);
		if (!car || car.CrewDriver() == pb)
			return false;
		PlayerBase driver = PlayerBase.Cast(car.CrewDriver());
		return driver && driver.GetIdentity() && driver.IsAlive();
	}

	// The car he drives, or null (on foot / not a car). HumanCommandVehicle.GetTransport (human.c:716 in 1.30).
	protected Transport DL_CarOf(PlayerBase pb)
	{
		HumanCommandVehicle vc = pb.GetCommand_Vehicle();
		if (!vc)
			return null;
		return DL_Veh.VehicleOf(pb);
	}

	// "" = may travel (on foot, or at the wheel of a car whose crew are all players who can travel), else the reason.
	protected string DL_CarProblem(PlayerBase pb)
	{
		HumanCommandVehicle vc = pb.GetCommand_Vehicle();
		if (!vc)
			return "";
		Transport car = DL_Veh.VehicleOf(pb);
		if (!car)
			return "Only cars, trucks, motorbikes, boats and planes can travel. Get out to travel on foot.";
		if (car.CrewDriver() != pb)
			return "Only a car with a player at the wheel can travel. Get out to travel on foot.";
		for (int c = 0; c < car.CrewSize(); c++)
		{
			Human h = car.CrewMember(c);
			if (!h || h == pb)
				continue;
			PlayerBase m = PlayerBase.Cast(h);
			if (!m || !m.GetIdentity() || !m.IsAlive() || m_DL_Leaving.Contains(m.GetIdentity().GetPlainId()))
				return "Everyone in the car must be a living player who can travel.";
		}
		return "";
	}

	// the zone at pos for this kind of traveller: a plane in flight only plane zones, everyone else only the others
	protected DL_Zone DL_ZoneAt(vector pos, bool flying = false)
	{
		foreach (DL_Zone z : m_DL_Cfg.zones)
		{
			if (!z || z.air != flying)
				continue;
			float dx = pos[0] - z.center[0];
			float dz = pos[2] - z.center[2];
			if (z.shape == "rect")
			{
				if (Math.AbsFloat(dx) <= z.width / 2 && Math.AbsFloat(dz) <= z.depth / 2)
					return z;
			}
			else if (dx * dx + dz * dz <= z.radius * z.radius)
				return z;
		}
		return null;
	}

	// the zone he is in: flying a plane = plane zones only; a plane on the ground travels nowhere
	protected DL_Zone DL_ZoneFor(PlayerBase pb)
	{
		Transport t = DL_Veh.VehicleOf(pb);
		if (DL_Veh.IsPlane(t))
		{
			if (!DL_Veh.Flying(t))
				return null;
			return DL_ZoneAt(pb.GetPosition(), true);
		}
		return DL_ZoneAt(pb.GetPosition(), false);
	}

	protected void DL_Countdown(PlayerIdentity id, int seconds, string zoneName, string priceText)
	{
		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(seconds);
		rpc.Write(zoneName);
		rpc.Write(priceText); // older clients read only the first two
		rpc.Send(null, DL_Const.RPC_COUNTDOWN, true, id);
	}

	// On foot or alone in a car: one trip. With passengers (M3): the passengers' trips go first, the driver's (with
	// the car) only once the hub has taken ALL of them - a refused passenger cancels the driver's trip, so the car
	// never leaves without its crew being accounted for. Passengers who were taken already arrive on foot then.
	protected void DL_Depart(PlayerBase pb, DL_Zone z)
	{
		Transport car = DL_CarOf(pb);
		array<PlayerBase> crew = DL_CrewOf(pb);
		string sid = pb.GetIdentity().GetPlainId();
		// the price again: someone may have eaten, drunk or burnt fuel during the countdown
		DL_TravelPrice price = DL_TravelCost.Price(m_DL_Cfg.travel, z, car != null, car);
		string missing = DL_TravelCost.Shortfall(price, crew, car, m_DL_Snap, m_DL_CarSnap);
		if (missing != "" && DL_RefusesShortTrips())
		{
			m_DL_MustLeaveZone.Set(sid, true);
			string dlMsg2683 = sid + " can't pay the trip to " + z.to + " any more (";
			dlMsg2683 += price.Text() + "): " + missing;
			DL_Log.Info(dlMsg2683);
			DL_CrewNotify(pb, 12, "Not enough for " + z.name, price.Text() + ". Missing: " + missing + ".");
			return;
		}
		// weakened mode: the road is played now, once for the whole crew (Research/travel-cost-design.md section 4)
		DL_Journey journey;
		if (price && !DL_RefusesShortTrips())
		{
			journey = DL_Journey.Run(m_DL_Cfg.travel, z, crew, car, m_DL_Snap, m_DL_CarSnap, true);
			string jtext = journey.Story();
			if (jtext == "")
				jtext = "(an uneventful trip)";
			jtext.Replace("\n", " | ");
			string dlMsg2696 = sid + "'s road to " + z.to + ": driven ";
			dlMsg2696 += Math.Round(journey.drivenKm).ToString() + " km, walked " + Math.Round(journey.walkedKm).ToString() + " km, ";
			dlMsg2696 += journey.Alive().ToString() + "/" + journey.crew.Count().ToString() + " alive, car lost ";
			dlMsg2696 += journey.carLost.ToString() + ", " + journey.seconds.ToString() + " s: ";
			dlMsg2696 += jtext;
			DL_Log.Info(dlMsg2696);
		}
		// the road film: the journey's scenes, or (refuse mode / no events) the plain drive or walk with the crew
		DL_Film film;
		if (journey)
			film = journey.film;
		else
			film = DL_PlainFilm(z, crew, car);  // also a free (0 km) trip, e.g. a short boat crossing
		if (film)
		{
			film.biomeFrom = DL_MyBiome();
			film.biomeVia = z.viaBiome;
		}
		m_DL_SlotOf.Remove(sid);
		// a crew in one car is a party too (the hub's Players page keeps them together)
		if (crew.Count() > 1)
		{
			string cpid = m_DL_Cfg.serverId + "-" + sid + "-" + g_Game.GetTime().ToString();
			foreach (PlayerBase cm : crew)
			{
				if (cm && cm.GetIdentity())
					m_DL_PartyIdOf.Set(cm.GetIdentity().GetPlainId(), cpid);
			}
		}
		DL_DepartUnit(pb, z, car, crew, price, journey, film);
	}

	// one unit leaves (a walker, or a car with its crew): its road may be shared with a party (journey, film)
	protected void DL_DepartUnit(PlayerBase pb, DL_Zone z, Transport car, array<PlayerBase> crew, DL_TravelPrice price, DL_Journey journey, DL_Film film)
	{
		string sid = pb.GetIdentity().GetPlainId();
		m_DL_Leaving.Set(sid, true);
		pb.SetAllowDamage(false);
		if (car)
		{
			// stop it where it is while the hub answers: engine off, handbrake, no damage (Car.EngineStop/SetHandbrake,
			// car.c:236/209 in 1.30). Anything that changes after the driver's snapshot doesn't travel.
			DL_Veh.EngineStop(car);
			DL_Veh.Handbrake(car, 1);
			car.SetAllowDamage(false);
		}
		if (!car || crew.Count() <= 1)
		{
			int alone = 0;
			if (car)
				alone = 1;
			DL_SendDepart(pb, z, car, "", -1, alone, price, journey, film);
			return;
		}
		DL_CrewTrip trip = new DL_CrewTrip();
		trip.driverSid = sid;
		trip.car = car;
		trip.zone = z;
		trip.total = crew.Count();
		trip.price = price;
		trip.journey = journey;
		trip.film = film;
		for (int i = 1; i < crew.Count(); i++)
		{
			PlayerBase m = crew[i];
			string msid = m.GetIdentity().GetPlainId();
			m_DL_Leaving.Set(msid, true);
			m.SetAllowDamage(false);
			trip.waiting++;
			m_DL_TripOf.Set(msid, trip);
			DL_SendDepart(m, z, null, sid, car.CrewMemberIndex(m), crew.Count(), price, journey, film);
		}
		string dlMsg2763 = sid + " drives " + car.GetType() + " to ";
		dlMsg2763 += z.to + " with " + trip.waiting.ToString() + " passenger(s): their trips first, his own once all are taken";
		DL_Log.Info(dlMsg2763);
	}

	// One player's trip. car = the car he drives (it goes inside his trip), carGroup/seat = a passenger's tag.
	// crewCount = players in the car (driver included) for the target's waiting room, 0 = on foot.
	// price = the trip's cost (null = free): taken off the snapshot that travels, never off the live character/car
	// journey != null (weakened mode): his snapshot was made and charged by the road already, use it as it is
	protected void DL_SendDepart(PlayerBase pb, DL_Zone z, Transport car, string carGroup, int seat, int crewCount, DL_TravelPrice price, DL_Journey journey, DL_Film film)
	{
		string sid = pb.GetIdentity().GetPlainId();
		DL_DepartReq q = new DL_DepartReq();
		q.serverId = m_DL_Cfg.serverId;
		q.key = m_DL_Cfg.hubKey;
		q.steamId = sid;
		q.to = z.to;
		q.arrival = z.arrival;
		DL_Traveller jt;
		if (journey)
			jt = journey.Find(sid);
		if (jt)
		{
			q.payload = jt.d;
			q.travelSeconds = journey.seconds;
		}
		else
		{
			q.payload = m_DL_Snap.Capture(pb);
			q.payload.crewCount = crewCount;
			if (price)
				q.travelSeconds = price.seconds;
			int slot;
			if (m_DL_SlotOf.Find(sid, slot))
			{
				// a party in refuse mode: the car's place in the convoy, or his place in the group on foot
				if (crewCount > 0 || car || carGroup != "")
					q.payload.convoyIdx = slot;
				else
					q.payload.partySlot = slot;
				m_DL_SlotOf.Remove(sid);
			}
		}
		if (film)
			q.payload.film = film;
		if (film && (!m_DL_Cfg.travel || !m_DL_Cfg.travel.filmOff))
			m_DL_TransitionOf.Set(sid, film);
		if (q.payload.vehKind == "")
		{
			Transport ride = DL_CarOf(pb);
			if (ride)
				q.payload.vehKind = DL_Veh.Kind(ride);
		}
		string ppid;
		if (m_DL_PartyIdOf.Find(sid, ppid))
		{
			q.payload.partyId = ppid;
			m_DL_PartyIdOf.Remove(sid);
		}
		string cid;
		if (m_DL_ConvoyOf.Find(sid, cid))
		{
			q.payload.convoyId = cid;
			q.payload.convoyCars = m_DL_ConvoyCarsOf.Get(sid);
			m_DL_ConvoyOf.Remove(sid);
			m_DL_ConvoyCarsOf.Remove(sid);
		}
		string carText = "on foot";
		if (car && jt)
		{
			// the road decided about the car (in his snapshot, a passenger's, or left behind); it still goes from here
			m_DL_TravelCar.Set(sid, car);
			carText = "driving " + car.GetType() + " (road: car lost " + journey.carLost.ToString() + ")";
		}
		else if (car)
		{
			q.payload.car = m_DL_CarSnap.Capture(car);
			q.payload.carSeat = car.CrewMemberIndex(pb);
			m_DL_TravelCar.Set(sid, car);
			carText = "driving " + car.GetType() + " (" + q.payload.car.items.Count().ToString() + " parts/items, " + DL_CarSnapshot.FluidText(q.payload.car.fluids) + ")";
		}
		else if (carGroup != "" && !jt)
		{
			q.payload.carGroup = carGroup;
			q.payload.carSeat = seat;
			carText = "passenger in seat " + seat.ToString() + " of " + carGroup + "'s car";
		}
		string dlMsg2846 = sid + " leaving to " + z.to + " (";
		dlMsg2846 += z.arrival + "), " + q.payload.items.Count().ToString() + " items, ";
		dlMsg2846 += q.payload.bleeding.Count().ToString() + " bleeding, " + q.payload.modifiers.Count().ToString() + " modifiers, ";
		dlMsg2846 += carText;
		DL_Log.Info(dlMsg2846);
		if (price && !jt)
		{
			// the driver's snapshot holds the car: he eats from its trunk too, canisters top up the tank
			string paid = DL_TravelCost.ChargePlayer(q.payload, price, q.payload.car);
			if (car)
				paid += "; car: " + DL_TravelCost.ChargeCar(q.payload.car, car, price, q.payload);
			DL_Log.Info(sid + " pays the trip (" + price.Text() + "): " + paid);
		}
		DL_Hub.Post(m_DL_Cfg.hubUrl, "/api/depart?serverId=" + m_DL_Cfg.serverId + "&key=" + m_DL_Cfg.hubKey, DL_Hub.DepartJson(q), new DL_DepartWaiter(this, sid), "OnAnswer", "depart " + sid);
	}

	// a passenger's answer is in: once all are in, send the driver (all taken) or call the trip off for him
	protected void DL_CrewAnswered(DL_CrewTrip trip, string psid, bool taken)
	{
		trip.waiting--;
		if (!taken)
			trip.refused.Insert(psid);
		if (trip.waiting > 0)
			return;
		PlayerBase driver = DL_FindPlayer(trip.driverSid);
		if (trip.refused.Count() == 0 && driver && trip.car && DL_CarOf(driver) == trip.car)
		{
			DL_SendDepart(driver, trip.zone, trip.car, "", -1, trip.total, trip.price, trip.journey, trip.film);
			return;
		}
		string why = "a passenger could not travel";
		if (!driver || DL_CarOf(driver) != trip.car)
			why = "the driver is no longer at the wheel";
		DL_Log.Warn(trip.driverSid + "'s car trip called off: " + why + " (refused: " + trip.refused.Count().ToString() + ")");
		m_DL_Leaving.Remove(trip.driverSid);
		if (trip.car)
			trip.car.SetAllowDamage(true);
		if (driver)
		{
			driver.SetAllowDamage(true);
			m_DL_MustLeaveZone.Set(trip.driverSid, true);
			NotificationSystem.SendNotificationToPlayerIdentityExtended(driver.GetIdentity(), 8, "DriveLink", "Car trip called off: " + why + ". Step out of the zone and back in to try again.", "");
		}
	}

	void DL_OnDepartAnswer(string sid, bool ok, string body)
	{
		DL_Film connectingFilm = m_DL_TransitionOf.Get(sid);
		m_DL_TransitionOf.Remove(sid);
		PlayerBase pb = DL_FindPlayer(sid);
		Transport car = m_DL_TravelCar.Get(sid);
		m_DL_TravelCar.Remove(sid);
		DL_DepartResp r;
		string err;
		JsonSerializer js = new JsonSerializer();
		if (ok)
			js.ReadFromString(r, body, err);
		DL_CrewTrip trip = m_DL_TripOf.Get(sid);
		if (trip)
		{
			m_DL_TripOf.Remove(sid);
			DL_CrewAnswered(trip, sid, ok && r && r.ok && r.target);
		}
		if (!ok || !r || !r.ok || !r.target)
		{
			// Nothing was handed over: he stays here with everything he has, the car too.
			m_DL_Leaving.Remove(sid);
			if (car)
				car.SetAllowDamage(true);
			// RestApi gives the same OnError for "no connection" and "hub said no" (HTTP 4xx), so don't guess.
			string why = "hub refused or unreachable (see the hub window)";
			if (r && r.error != "")
				why = r.error;
			DL_Log.Info(sid + " NOT sent: " + why);
			if (pb)
			{
				pb.SetAllowDamage(true);
				m_DL_MustLeaveZone.Set(sid, true);
			NotificationSystem.SendNotificationToPlayerIdentityExtended(pb.GetIdentity(), 8, "DriveLink", "Travel failed: " + why + ". Step out of the zone and back in to try again.", "");
			}
			return;
		}
		if (car)
		{
			// The hub holds the car now (inside his trip). Remember it BEFORE it goes (crash guard, DL_CarGuard), and
			// delete it after he has been removed from the seat (DL_RemoveTraveller runs at 1.5 s).
			DL_CarGuard.Remember(car);
			DL_CarLedger.Left(car); // vehicle economy: it stays counted at its home
			DL_ParkCar(car);
			DL_Log.Detail(car.GetType() + " " + DL_CarGuard.IdOf(car) + " travels with " + sid + ", removing it here once its seats are empty");
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_RemoveCar, 3000, false, car, 0);
		}
		if (!pb)
		{
			// He logged out between the snapshot and now: the hub already holds him in transit, he will arrive on
			// the target next time he joins anywhere (the hub redirects him there).
			DL_Log.Info(sid + " left before the redirect; in transit to " + r.target.name);
			return;
		}
		if (r.readyIn > 0)
		{
			// travel time: he is on the road. Back to the main menu with the time; every server refuses him until then
			DL_SendKick(sid, "On the road to " + r.target.name + ": about " + DL_TravelCost.Duration(r.readyIn) + ". Join " + r.target.name + " when that time has passed (no server lets you in before).");
			DL_Log.Info(sid + " is on the road to " + r.target.name + " for " + r.readyIn.ToString() + " s");
		}
		else
		{
			DL_SendTransition(sid, connectingFilm);
			DL_SendRedirect(pb.GetIdentity(), r.target, "zone");
		}
		m_DL_Doomed.Set(pb, true);
		DL_Log.Info(sid + " handed over to " + r.target.name + ", removing the character here");
		// give the RPC a moment to reach the client, then remove the character WITHOUT killing it (nothing may
		// stay behind: no body, no dropped gear)
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_RemoveTraveller, 1500, false, sid);
	}

	// Derk 2026-10-18: move the departed car (with the logging-out driver still in it) out of everyone's reach until
	// it can be deleted. Not 0 0 0: that is under the ground / at the world's edge, and an engine delete of an occupied
	// car is what crashed the server. The position is logged 1 s later to see that the move held.
	protected void DL_ParkCar(Transport car)
	{
		vector p = m_DL_Cfg.parkingSpot;
		if (p[0] == 0 && p[2] == 0)
			return;
		p[1] = g_Game.SurfaceY(p[0], p[2]) + 0.5;
		vector from = car.GetPosition();
		car.SetPosition(p);
		car.SetOrientation(car.GetOrientation());
		car.SetSynchDirty();
		DL_Log.Detail(car.GetType() + " parked: " + from.ToString() + " -> " + p.ToString());
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_LogCarPos, 1000, false, car);
	}

	protected void DL_LogCarPos(Transport car)
	{
		if (car)
			DL_Log.Detail(car.GetType() + " 1 s after parking is at " + car.GetPosition().ToString());
	}

	// NEVER delete a car with someone in a seat: test 2026-10-18 20:06:25 the Chernarus server crashed (access
	// violation, crash_2026-10-18_20-02-09.log) deleting the car while his logging-out character (vanilla keeps it in
	// the world for the logout time, 15 s) still sat in it. Wait for the seats to empty: his character goes when the
	// logout ends (HandleBody -> doomed -> DL_Forget) or at once via DL_RemoveTraveller.
	protected void DL_RemoveCar(Transport car, int tries = 0)
	{
		if (!car)
			return;
		int crew = 0;
		for (int c = 0; c < car.CrewSize(); c++)
		{
			if (car.CrewMember(c))
				crew++;
		}
		if (crew > 0)
		{
			if (tries < 60)
			{
				g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_RemoveCar, 1000, false, car, tries + 1);
				return;
			}
			// a minute and still occupied: leave it (a duplicate is better than a crash); the crash guard list has it,
			// so it is removed at this server's next start
			DL_Log.Warn("WARNING " + car.GetType() + " still occupied after 60 s, NOT removed now (removed at next start)");
			return;
		}
		DL_Log.Detail(car.GetType() + " removed here (seats empty after " + tries.ToString() + " s)");
		car.Delete();
	}

	// Normally the client is gone by now and vanilla's logout ends in PlayerDisconnected -> HandleBody (doomed ->
	// DL_Forget). If he is STILL connected, run that same vanilla path ourselves (missionserver.c PlayerDisconnected:
	// save, CharacterExit, HandleBody, g_Game.DisconnectPlayer). Test 2026-10-18 19:03: a bare Delete() here while his
	// logout was still running made vanilla log "Skipping player, already removed" and never call DisconnectPlayer,
	// so the engine kept his UID "in game" and the server refused him ("Player with same UID is already in game")
	// until a restart.
	protected void DL_RemoveTraveller(string sid)
	{
		m_DL_Leaving.Remove(sid);
		PlayerBase pb = DL_FindPlayer(sid);
		if (!pb || !pb.GetIdentity())
			return;
		DL_Log.Info(sid + " still connected after the hand-over, disconnecting him the vanilla way");
		m_DL_Doomed.Set(pb, true); // HandleBody -> DL_Forget
		PlayerDisconnected(pb, pb.GetIdentity(), pb.GetIdentity().GetId());
	}

	// Remove a character that now lives on another server. It must also be marked dead in THIS server's own
	// character database, or he would get this old copy back the next time he joins here (= duplicated gear).
	// Hive.CharacterKill (3_game/hive/hive.c:20) is what vanilla calls when a player dies (playerbase.c:1211-1213).
	protected void DL_Forget(PlayerBase pb)
	{
		if (!pb)
			return;
		if (GetHive())
			GetHive().CharacterKill(pb);
		pb.Delete();
	}

	override void HandleBody(PlayerBase player)
	{
		if (player && m_DL_Doomed.Contains(player))
		{
			// travelling: never kill (vanilla kills cuffed/unconscious loggers-out) and never leave a body
			m_DL_Doomed.Remove(player);
			DL_Forget(player);
			return;
		}
		super.HandleBody(player);
	}

	// Only a plain driving/walking cast is sent before disconnect, after the Hub accepts the transfer.
	// Road events and skip voting remain under the destination server's existing arrival flow.
	protected void DL_SendTransition(string sid, DL_Film source)
	{
		if (!source)
			return;
		DL_Film preview = new DL_Film();
		preview.car = source.car;
		foreach (string look : source.cars)
			preview.cars.Insert(look);
		foreach (DL_FilmActor actor : source.crew)
			preview.crew.Insert(actor);
		preview.biomeFrom = source.biomeFrom;
		preview.biomeTo = source.biomeFrom;
		preview.km = 1;
		preview.skip = "none";
		string kind = "walk";
		if (preview.car != "" || preview.cars.Count() > 0)
			kind = "drive";
		preview.Add(kind, 1);
		string json, err;
		if (!JsonFileLoader<DL_Film>.MakeData(preview, json, err, false))
			return;
		DL_SendFilmRpc(sid, json, DL_Const.RPC_TRANSITION);
	}

	protected void DL_SendRedirect(PlayerIdentity id, DL_ServerInfo target, string reason)
	{
		DL_Redirect r = new DL_Redirect();
		r.target = target;
		r.reason = reason;
		ScriptRPC rpc = new ScriptRPC();
		string json;
		string err;
		if (!JsonFileLoader<DL_Redirect>.MakeData(r, json, err, false))
			DL_Log.Info("redirect json: " + err);
		rpc.Write(json);
		rpc.Send(null, DL_Const.RPC_REDIRECT, true, id);
	}

	protected PlayerBase DL_FindPlayer(string sid)
	{
		array<Man> men = new array<Man>;
		g_Game.GetPlayers(men);
		foreach (Man m : men)
		{
			if (m && m.GetIdentity() && m.GetIdentity().GetPlainId() == sid)
				return PlayerBase.Cast(m);
		}
		return null;
	}
}

// The hub callback target must be a Managed object (DL_Hub.Post); MissionServer is not one (8b74f1f compile error
// "Types 'MissionServer' and 'Managed' are unrelated"), so these small helpers carry the answer back to it.
class DL_HelloWaiter : Managed
{
	protected MissionServer m_M;
	void DL_HelloWaiter(MissionServer m) { m_M = m; }
	void OnAnswer(bool ok, string body) { if (m_M) m_M.DL_OnHello(ok, body); }
}

class DL_DiedResp
{
	bool ok;
	ref DL_ServerInfo home;   // the starting server, when he must start his next life there (not this server)
}

class DL_DiedWaiter : Managed
{
	protected MissionServer m_M;
	protected string m_Sid;
	void DL_DiedWaiter(MissionServer m, string sid) { m_M = m; m_Sid = sid; }
	void OnAnswer(bool ok, string body) { if (m_M) m_M.DL_OnDied(m_Sid, ok, body); }
}

class DL_ConfirmWaiter : Managed
{
	protected string m_Sid;
	protected string m_What;
	void DL_ConfirmWaiter(string sid, string what) { m_Sid = sid; m_What = what; }
	void OnAnswer(bool ok, string body)
	{
		if (ok)
			DL_Log.Detail("hub took the " + m_What + " of " + m_Sid);
		else
			DL_Log.Warn("WARNING hub did NOT take the " + m_What + " of " + m_Sid + " (see hub window)");
	}
}

class DL_PayloadWaiter : Managed
{
	protected MissionServer m_M;
	protected string m_Sid;
	void DL_PayloadWaiter(MissionServer m, string sid) { m_M = m; m_Sid = sid; }
	void OnAnswer(bool ok, string body) { if (m_M) m_M.DL_OnPayloadAnswer(m_Sid, ok, body); }
}

// These also carry the steam id along.
class DL_ArriveWaiter : Managed
{
	protected MissionServer m_M;
	protected string m_Sid;
	void DL_ArriveWaiter(MissionServer m, string sid) { m_M = m; m_Sid = sid; }
	void OnAnswer(bool ok, string body) { if (m_M) m_M.DL_OnArriveAnswer(m_Sid, ok, body); }
}

class DL_DepartWaiter : Managed
{
	protected MissionServer m_M;
	protected string m_Sid;
	void DL_DepartWaiter(MissionServer m, string sid) { m_M = m; m_Sid = sid; }
	void OnAnswer(bool ok, string body) { if (m_M) m_M.DL_OnDepartAnswer(m_Sid, ok, body); }
}

class DL_LayoutWaiter : Managed
{
	protected MissionServer m_M;
	void DL_LayoutWaiter(MissionServer m) { m_M = m; }
	void OnAnswer(bool ok, string body) { if (m_M) m_M.DL_OnLayout(ok, body); }
}

// the arrival spot check for the hub (POST /api/spots)
class DL_SpotPlace
{
	string car;
	string truck;
}

class DL_SpotReport
{
	string arrival;
	string boat;                  // where a boat would go: "pinned spot free" / "water 35 m away" / "no water within 400 m"
	vector boatAt;                // that spot (0 0 0 = none)
	ref array<ref DL_SpotPlace> places = new array<ref DL_SpotPlace>;
}

class DL_SpotsReq
{
	string serverId;
	string key;
	ref array<ref DL_SpotReport> spots = new array<ref DL_SpotReport>;
}

class DL_SpotsWaiter : Managed
{
	void OnAnswer(bool ok, string body) {}
}
