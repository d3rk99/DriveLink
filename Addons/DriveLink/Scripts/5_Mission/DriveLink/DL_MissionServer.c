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
	CarScript car;
	ref DL_Zone zone;
	int waiting;                               // passenger answers still to come
	int total;                                 // players in the car, driver included
	ref array<string> refused = new array<string>;
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
}

modded class MissionServer
{
	protected ref map<string, ref DL_Room> m_DL_Rooms = new map<string, ref DL_Room>;
	protected bool m_DL_RoomTicking;
	protected ref map<string, ref DL_CrewTrip> m_DL_TripOf = new map<string, ref DL_CrewTrip>; // passenger sid -> his car trip
	protected ref map<string, CarScript> m_DL_GroupCar = new map<string, CarScript>;         // driver sid -> the car he brought here
	protected ref map<string, ref DL_SeatWait> m_DL_SeatWait = new map<string, ref DL_SeatWait>; // passenger sid -> waiting for that car
	protected ref DL_Config m_DL_Cfg;
	protected ref map<string, ref DL_Pending> m_DL_Pending = new map<string, ref DL_Pending>;    // steamId -> hub answer
	protected ref map<string, int> m_DL_InZoneSince = new map<string, int>;                     // steamId -> ms
	protected ref map<string, bool> m_DL_Leaving = new map<string, bool>;  // steamIds being sent away (zone check skips them)
	protected ref map<string, bool> m_DL_MustLeaveZone = new map<string, bool>; // trip failed: no new countdown until he steps out
	protected ref map<PlayerBase, bool> m_DL_Doomed = new map<PlayerBase, bool>; // characters that live elsewhere now: delete + CharacterKill on logout
	protected float m_DL_Tick;
	protected ref DL_Snapshot m_DL_Snap = new DL_Snapshot();
	protected string m_DL_BuildError;      // why the last DL_BuildArrival returned null
	protected ref DL_Debug m_DL_Debug;     // debug.json, null when the file is not there
	protected ref DL_CarSnapshot m_DL_CarSnap = new DL_CarSnapshot();
	protected ref map<CarScript, int> m_DL_CarInZoneSince = new map<CarScript, int>;
	protected ref map<CarScript, bool> m_DL_CarDone = new map<CarScript, bool>; // rebuilt cars: not again until they leave the circle
	protected ref map<string, CarScript> m_DL_TravelCar = new map<string, CarScript>; // steamId -> the car he is taking along, until the hub answers

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
				Print(DL_Const.LOG_TAG + "config.json unreadable (" + err + "), DriveLink is OFF");
		}
		else
		{
			// zones + arrival points are drawn in the hub's web app; only the hub's address + key go here
			JsonFileLoader<DL_Config>.SaveFile(DL_Const.CONFIG, m_DL_Cfg, err);
			Print(DL_Const.LOG_TAG + "wrote " + DL_Const.CONFIG + " - put in hubUrl + hubKey (the hub's web app, Setup page shows both), restart, then approve this server on the hub's Servers page");
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
		if (m_DL_Cfg.hubKey != "")
			DL_SendHello();
		DL_LoadDebug();
		Print(DL_Const.LOG_TAG + "v" + DL_Const.VERSION + " server '" + m_DL_Cfg.serverId + "', hub " + m_DL_Cfg.hubUrl + ", " + m_DL_Cfg.zones.Count().ToString() + " zones, " + m_DL_Cfg.arrivals.Count().ToString() + " arrival points, key set " + (m_DL_Cfg.hubKey != "").ToString());
	}

	// ---------------- M2 spikes (debug.json only) ----------------

	protected void DL_LoadDebug()
	{
		if (!FileExist(DL_Const.DEBUG_FILE))
			return;
		string err;
		if (!JsonFileLoader<DL_Debug>.LoadFile(DL_Const.DEBUG_FILE, m_DL_Debug, err) || !m_DL_Debug)
		{
			Print(DL_Const.LOG_TAG + "DEBUG debug.json unreadable: " + err);
			m_DL_Debug = null;
			return;
		}
		string zone = "off";
		if (m_DL_Debug.carRebuildZone && m_DL_Debug.carRebuildZone.radius > 0)
			zone = m_DL_Debug.carRebuildZone.center.ToString() + " r " + m_DL_Debug.carRebuildZone.radius.ToString();
		Print(DL_Const.LOG_TAG + "DEBUG debug.json: carRebuildZone " + zone + ", seatTestCar '" + m_DL_Debug.seatTestCar + "'");
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
		map<CarScript, bool> seen = new map<CarScript, bool>;
		int now = g_Game.GetTime();
		foreach (Object o : objs)
		{
			CarScript car = CarScript.Cast(o);
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
		array<CarScript> gone = new array<CarScript>;
		foreach (CarScript k, bool v : m_DL_CarDone)
		{
			if (!k || !seen.Contains(k))
				gone.Insert(k);
		}
		foreach (CarScript g : gone)
			m_DL_CarDone.Remove(g);
	}

	protected void DL_CarSpike(CarScript car)
	{
		DL_CarData d = m_DL_CarSnap.Capture(car);
		string json;
		string err;
		JsonFileLoader<DL_CarData>.MakeData(d, json, err, false);
		JsonFileLoader<DL_CarData>.SaveFile(DL_Const.DIR + "/car-snapshot.json", d, err);
		Print(DL_Const.LOG_TAG + "CAR SPIKE " + d.type + " at " + d.pos.ToString() + ": " + d.items.Count().ToString() + " parts/items, health " + d.health.ToString() + ", " + DL_CarSnapshot.FluidText(d.fluids) + ", " + d.zones.Count().ToString() + " zones, json " + json.Length().ToString() + " bytes (saved car-snapshot.json); deleting it, rebuild in 3 s");
		car.Delete();
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_CarSpikeBuild, 3000, false, json);
	}

	protected void DL_CarSpikeBuild(string json)
	{
		DL_CarData d;
		string err;
		if (!JsonFileLoader<DL_CarData>.LoadData(json, d, err) || !d)
		{
			Print(DL_Const.LOG_TAG + "CAR SPIKE FAILED: snapshot unreadable after the JSON round trip: " + err);
			return;
		}
		int made;
		int dropped;
		CarScript car = m_DL_CarSnap.Build(d, d.pos, d.ori, made, dropped);
		if (!car)
		{
			Print(DL_Const.LOG_TAG + "CAR SPIKE FAILED: could not create " + d.type);
			return;
		}
		m_DL_CarDone.Set(car, true);
		Print(DL_Const.LOG_TAG + "CAR SPIKE rebuilt " + d.type + ": " + made.ToString() + "/" + d.items.Count().ToString() + " parts/items (" + dropped.ToString() + " on the ground), health " + car.GetHealth("", "Health").ToString() + ", now " + DL_CarSnapshot.FluidText(DL_CarSnapshot.FluidsOf(car)));
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
		CarScript car = CarScript.Cast(g_Game.CreateObjectEx(m_DL_Debug.seatTestCar, pos, ECE_SETUP | ECE_UPDATEPATHGRAPH | ECE_CREATEPHYSICS | ECE_TRACE));
		if (!car)
		{
			Print(DL_Const.LOG_TAG + "SEAT SPIKE FAILED: could not create '" + m_DL_Debug.seatTestCar + "' (not a CarScript class?)");
			return;
		}
		car.SetOrientation(pb.GetOrientation());
		car.OnDebugSpawn(); // vanilla kit: wheels, doors, battery, plugs, fluids (e.g. offroadhatchback.c:457-477)
		Print(DL_Const.LOG_TAG + "SEAT SPIKE spawned " + car.GetType() + " at " + pos.ToString() + " for " + sid + ", seating him in 2 s");
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SeatSpikeSeat, 2000, false, sid, car);
	}

	protected void DL_SeatSpikeSeat(string sid, CarScript car)
	{
		PlayerBase pb = DL_FindPlayer(sid);
		if (!pb || !car)
			return;
		pb.DL_SeatIn(car, 0);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SeatSpikeCheck, 4000, false, sid, car);
	}

	protected void DL_SeatSpikeCheck(string sid, CarScript car)
	{
		PlayerBase pb = DL_FindPlayer(sid);
		if (!pb || !car)
			return;
		bool inCmd = pb.GetCommand_Vehicle() != null;
		bool isDriver = car.CrewDriver() == pb;
		Print(DL_Const.LOG_TAG + "SEAT SPIKE result for " + sid + ": vehicle command " + inCmd.ToString() + ", driver of " + car.GetType() + " " + isDriver.ToString() + ", crew index " + car.CrewMemberIndex(pb).ToString());
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
		string hj;
		string herr;
		JsonFileLoader<DL_HelloReq>.MakeData(h, hj, herr, false);
		DL_Hub.Post(m_DL_Cfg.hubUrl, "/api/hello?serverId=" + m_DL_Cfg.serverId + "&key=" + m_DL_Cfg.hubKey, hj, new DL_HelloWaiter(this), "OnAnswer", "hello");
	}

	void DL_OnHello(bool ok, string body)
	{
		if (!ok)
		{
			Print(DL_Const.LOG_TAG + "hub hello FAILED - check hubUrl/hubKey, that the hub runs, and the hub's log; trying again in 30 s");
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SendHello, 30000, false);
			return;
		}
		DL_HelloResp r;
		string err;
		JsonSerializer js = new JsonSerializer();
		if (!js.ReadFromString(r, body, err) || !r)
		{
			Print(DL_Const.LOG_TAG + "hub hello: unreadable answer (" + err + "), trying again in 30 s");
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SendHello, 30000, false);
			return;
		}
		if (r.pending)
		{
			if (!m_DL_SaidPending)
				Print(DL_Const.LOG_TAG + "the hub knows the key but not this server yet: approve it on the hub's Servers page (asking again every 15 s)");
			m_DL_SaidPending = true;
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SendHello, 15000, false);
			return;
		}
		Print(DL_Const.LOG_TAG + "hub hello OK: " + body.Substring(0, Math.Min(160, body.Length())));
		if (r.serverId != "" && r.serverId != m_DL_Cfg.serverId)
		{
			Print(DL_Const.LOG_TAG + "the hub knows this server as '" + r.serverId + "' (was '" + m_DL_Cfg.serverId + "'), saved in config.json");
			m_DL_Cfg.serverId = r.serverId;
			JsonFileLoader<DL_Config>.SaveFile(DL_Const.CONFIG, m_DL_Cfg, err);
		}
		if (r.layout)
			DL_ApplyLayout(r.layout, "hello");
		// from now on: ask for edits every 30 s (only a rev number goes over the wire when nothing changed)
		if (!m_DL_Polling)
		{
			m_DL_Polling = true;
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_PollLayout, 30000, true);
		}
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
		m_DL_InZoneSince.Clear(); // countdowns of zones that may be gone now
		string err;
		JsonFileLoader<DL_Config>.SaveFile(DL_Const.CONFIG, m_DL_Cfg, err);
		Print(DL_Const.LOG_TAG + "layout rev " + l.rev.ToString() + " from the hub (" + why + "): " + m_DL_Cfg.zones.Count().ToString() + " zones, " + m_DL_Cfg.arrivals.Count().ToString() + " arrival points");
		foreach (DL_Zone z : m_DL_Cfg.zones)
		{
			if (z)
				Print(DL_Const.LOG_TAG + "  zone '" + z.name + "' " + z.center.ToString() + " r " + z.radius.ToString() + " -> " + z.to + " / " + z.arrival + ", " + z.distanceKm.ToString() + " km");
		}
		foreach (DL_ArrivalPoint a : m_DL_Cfg.arrivals)
		{
			if (a)
				Print(DL_Const.LOG_TAG + "  arrival '" + a.name + "' " + a.pos.ToString() + " yaw " + a.yaw.ToString());
		}
		DL_TravelRules t = m_DL_Cfg.travel;
		if (t)
			Print(DL_Const.LOG_TAG + "  travel rules: " + t.mode + ", per 10 km foot " + t.foodFootPct.ToString() + "/" + t.waterFootPct.ToString() + "% food/water, car " + t.foodCarPct.ToString() + "/" + t.waterCarPct.ToString() + "% + " + t.fuelLiters.ToString() + " L, wear " + t.wearEnginePct.ToString() + "/" + t.wearWheelsPct.ToString() + "%, time " + t.timeEnabled.ToString() + " (" + t.speedFootKmh.ToString() + "/" + t.speedCarKmh.ToString() + " km/h)");
	}

	protected bool DL_On()
	{
		return m_DL_Cfg && m_DL_Cfg.hubKey != "" && m_DL_Cfg.serverId != ""; // no id = not approved on the hub yet
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
				Print(DL_Const.LOG_TAG + "hub call failed for " + sid + " (refused or unreachable, see the hub window), normal login");
			else
				Print(DL_Const.LOG_TAG + "hub answer for " + sid + " unusable: " + err + " " + body.Substring(0, Math.Min(200, body.Length())) + ", normal login");
			p.action = "normal";
			if (p.loggedIn)
				DL_LateAnswer(sid, p);
			return;
		}
		p.resp = r;
		Print(DL_Const.LOG_TAG + "hub says " + r.action + " for " + sid);
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
				Print(DL_Const.LOG_TAG + "transfer for " + sid + ": " + sizes + ", " + DL_Hub.CountItems(p.resp.payload.items).ToString() + " items in the snapshot");
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
		Print(DL_Const.LOG_TAG + "ARRIVAL FAILED for " + sid + ": " + reason);
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
		Print(DL_Const.LOG_TAG + "late hub answer for " + sid + ": " + p.action);
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
		Print(DL_Const.LOG_TAG + "sending " + sid + " away: " + text);
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
					Print(DL_Const.LOG_TAG + rp.param1.GetPlainId() + " arrives by transfer; this server had an old character for him, replacing it");
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
		if (p && (p.action == "" || p.action == "fetching"))
		{
			// the hub hasn't fully answered yet: keep the entry, DL_LateAnswer handles the answer when it comes
			p.loggedIn = true;
			m_DL_Leaving.Set(sid, true); // no zone travel until we know where his character really is
			Print(DL_Const.LOG_TAG + "WARNING " + sid + " logged in before the hub answered (" + p.action + "), waiting for it");
			return;
		}
		m_DL_Pending.Remove(sid);
		if (p && p.action == "failed")
		{
			string text = "Your character could not be brought to this server (" + p.reason + "). It is safe on the DriveLink hub. Ask an admin, or try again later.";
			DL_Kick(player, text);
			return;
		}
		if (p && p.action == "redirect" && p.resp && p.resp.target)
		{
			Print(DL_Const.LOG_TAG + identity.GetPlainId() + " belongs on " + p.resp.target.name + ", redirecting");
			NotificationSystem.SendNotificationToPlayerIdentityExtended(identity, 8, "DriveLink", "Your character is on " + p.resp.target.name + ". Taking you there...", "");
			DL_SendRedirect(identity, p.resp.target, "owner");
			if (player)
			{
				player.SetAllowDamage(false);
				m_DL_Doomed.Set(player, true);
			}
			return;
		}
		DL_SeatSpikeLater(sid); // debug.json only
	}

	// Returns null (and says why in m_DL_BuildError) when he can't be built here; nothing is changed then.
	protected PlayerBase DL_BuildArrival(PlayerIdentity identity, DL_ArriveResp r, PlayerBase oldChar = null)
	{
		DL_PlayerData d = r.payload;
		DL_ArrivalPoint ap = m_DL_Cfg.FindArrival(r.arrival);
		if (!ap && m_DL_Cfg.arrivals.Count() > 0)
		{
			// "" = a trip the hub admin turned around ("DriveLinkHub.exe return"): first point is the normal answer
			if (r.arrival != "")
				Print(DL_Const.LOG_TAG + "arrival point '" + r.arrival + "' not in config.json, using the first one");
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
		// M2: his car first, at carPos or else at the arrival point (he then stands 3 m to its left = driver side)
		CarScript car;
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
			int carMade;
			int carDropped;
			car = m_DL_CarSnap.Build(d.car, carPos, Vector(carYaw, 0, 0), carMade, carDropped);
			if (car)
			{
				car.SetAllowDamage(false);
				if (onPoint)
					pos = carPos + Vector(carYaw - 90, 0, 0).AnglesToVector() * 3;
				carText = ", with his " + car.GetType() + " (" + carMade.ToString() + "/" + d.car.items.Count().ToString() + " parts/items, " + carDropped.ToString() + " on the ground)";
			}
			else
			{
				carText = ", WITHOUT his car: could not create " + d.car.type;
				Print(DL_Const.LOG_TAG + "WARNING could not create the car " + d.car.type + " for " + identity.GetPlainId() + ", he arrives on foot");
			}
		}
		// M3: a passenger. His driver's car is here already -> next to it (right side) and seated; else he waits on
		// foot 6 m to the right of the arrival point (where the car will appear) and is brought in when it comes.
		CarScript rideCar;
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
		Print(DL_Const.LOG_TAG + identity.GetPlainId() + " arrived from " + r.from + " at " + ap.name + " " + pos.ToString() + ", " + made.ToString() + "/" + DL_Hub.CountItems(d.items).ToString() + " items restored (" + dropped.ToString() + " on the ground, no room), health " + d.health.ToString() + ", bleeding " + d.bleeding.Count().ToString() + ", modifiers " + d.modifiers.Count().ToString() + ", stomach " + d.stomach.Count().ToString() + ", cuffed " + d.restrained.ToString() + carText);
		NotificationSystem.SendNotificationToPlayerIdentityExtended(identity, 6, "DriveLink", "Welcome from " + r.from, "");
		string sid = identity.GetPlainId();
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
			room.carPos[1] = g_Game.SurfaceY(room.carPos[0], room.carPos[2]);
		}
		if (d.crewCount > room.expected)
			room.expected = d.crewCount;
		vector fwd = Vector(room.carYaw, 0, 0).AnglesToVector();
		vector right = Vector(room.carYaw + 90, 0, 0).AnglesToVector();
		vector pos;
		if (isDriver)
			pos = room.carPos - right * 3;                                           // driver side
		else
			pos = room.carPos + right * 3 + fwd * (1.5 - 1.5 * room.here.Count());   // passengers in a row on the right
		pos[1] = g_Game.SurfaceY(pos[0], pos[2]);
		PlayerBase player = CreateCharacter(identity, pos, null, d.charType);
		if (!player)
		{
			m_DL_BuildError = "could not create the character " + d.charType;
			return null;
		}
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
		Print(DL_Const.LOG_TAG + sid + " arrived from " + r.from + " at " + ap.name + " " + pos.ToString() + ", " + made.ToString() + "/" + DL_Hub.CountItems(d.items).ToString() + " items restored (" + dropped.ToString() + " on the ground, no room), health " + d.health.ToString() + ", bleeding " + d.bleeding.Count().ToString() + ", modifiers " + d.modifiers.Count().ToString() + ", stomach " + d.stomach.Count().ToString() + ", cuffed " + d.restrained.ToString() + carText + ", waiting room " + room.here.Count().ToString() + "/" + room.expected.ToString());
		NotificationSystem.SendNotificationToPlayerIdentityExtended(identity, 6, "DriveLink", "Welcome from " + r.from, "");
		if (!room.going && room.car && room.here.Count() >= room.expected)
		{
			room.going = true;
			// 2 s: his client is still loading; the seat juncture needs the character to exist there
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
			foreach (string m : room.here)
				DL_SendWait(m, DL_Const.WAIT_SHOW, text, false);
		}
		foreach (string gs : startNow)
		{
			DL_Room rs = m_DL_Rooms.Get(gs);
			Print(DL_Const.LOG_TAG + "waiting room of " + gs + ": waited " + m_DL_Cfg.crewWaitSeconds.ToString() + " s, going with " + rs.here.Count().ToString() + "/" + rs.expected.ToString());
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

	protected void DL_SendWait(string sid, int phase, string text, bool startEngine)
	{
		PlayerBase pb = DL_FindPlayer(sid);
		if (!pb || !pb.GetIdentity())
			return;
		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(phase);
		rpc.Write(text);
		rpc.Write(startEngine);
		rpc.Send(null, DL_Const.RPC_CREW_WAIT, true, pb.GetIdentity());
	}

	// all in (or waited long enough): build the car, confirm the driver's trip, seat the driver then each passenger
	protected void DL_RoomGo(string group)
	{
		DL_Room room = m_DL_Rooms.Get(group);
		if (!room)
			return;
		room.going = true;
		int made;
		int dropped;
		CarScript car = m_DL_CarSnap.Build(room.car, room.carPos, Vector(room.carYaw, 0, 0), made, dropped);
		string carText;
		if (car)
		{
			car.SetAllowDamage(false);
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
		Print(DL_Const.LOG_TAG + "waiting room of " + group + ": " + room.here.Count().ToString() + "/" + room.expected.ToString() + " here, " + carText);
		int i = 0;
		if (car)
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
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_RoomReveal, 1500 + 800 * i + 2500, false, group, car);
	}

	// everyone seated: lift the black screen; the driver's client starts the engine (PHYSICS cars are simulated on the
	// driver's machine, so vanilla ActionStartEngine starts them there too, actionstartengine.c:51-65)
	protected void DL_RoomReveal(string group, CarScript car)
	{
		DL_Room room = m_DL_Rooms.Get(group);
		if (!room)
			return;
		m_DL_Rooms.Remove(group);
		bool serverStarts = car != null && car.GetNetworkMoveStrategy() != NetworkMoveStrategy.PHYSICS;
		int seated = 0;
		foreach (string m : room.here)
		{
			PlayerBase pb = DL_FindPlayer(m);
			if (!pb)
				continue;
			if (car && car.CrewMemberIndex(pb) >= 0)
				seated++;
			bool engine = car != null && m == group && car.CrewDriver() == pb;
			DL_SendWait(m, DL_Const.WAIT_DONE, "", engine);
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SendWait, 1500, false, m, DL_Const.WAIT_DONE, "", engine); // in case the first is lost
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_EndProtection, m_DL_Cfg.arrivalProtectSeconds * 1000, false, pb);
		}
		if (serverStarts && car.CrewDriver())
			car.EngineStart();
		string how = "driver's client";
		if (serverStarts)
			how = "server";
		Print(DL_Const.LOG_TAG + "waiting room of " + group + ": " + seated.ToString() + "/" + room.here.Count().ToString() + " seated, black screen lifted, engine start by the " + how);
		if (car)
		{
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_EndCarProtection, m_DL_Cfg.arrivalProtectSeconds * 1000, false, car);
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_RoomEngineCheck, 4000, false, group, car);
		}
	}

	protected void DL_RoomEngineCheck(string group, CarScript car)
	{
		if (car)
			Print(DL_Const.LOG_TAG + "waiting room of " + group + ": engine of the " + car.GetType() + " running " + car.EngineIsOn().ToString() + ", " + DL_CarSnapshot.FluidText(DL_CarSnapshot.FluidsOf(car)));
	}

	// no car is coming: the passengers go on on foot
	protected void DL_RoomRelease(string group, string why)
	{
		DL_Room room = m_DL_Rooms.Get(group);
		if (!room)
			return;
		m_DL_Rooms.Remove(group);
		Print(DL_Const.LOG_TAG + "waiting room of " + group + " closed: " + why + " (" + room.here.Count().ToString() + "/" + room.expected.ToString() + " here, on foot)");
		foreach (string m : room.here)
		{
			PlayerBase pb = DL_FindPlayer(m);
			if (!pb)
				continue;
			DL_SendWait(m, DL_Const.WAIT_DONE, "", false);
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_EndProtection, m_DL_Cfg.arrivalProtectSeconds * 1000, false, pb);
			NotificationSystem.SendNotificationToPlayerIdentityExtended(pb.GetIdentity(), 10, "DriveLink", "No car: " + why + ". You continue on foot.", "");
		}
	}

	protected void DL_ForgetGroupCar(string group, CarScript car)
	{
		if (m_DL_GroupCar.Get(group) == car)
			m_DL_GroupCar.Remove(group);
	}

	// M3: the driver's car just arrived; seat the passengers who got here before it (one per second)
	protected void DL_BringWaitingCrew(string driverSid, CarScript car)
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
			Print(DL_Const.LOG_TAG + driverSid + "'s car is here, bringing in " + k.ToString() + " waiting passenger(s)");
	}

	protected void DL_BringToCar(string psid, CarScript car, int seat)
	{
		PlayerBase pb = DL_FindPlayer(psid);
		if (!pb || !car || pb.GetCommand_Vehicle())
			return;
		vector ori = car.GetOrientation();
		pb.SetPosition(car.GetPosition() + Vector(ori[0] + 90, 0, 0).AnglesToVector() * 3);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SeatTraveller, 1500, false, psid, car, seat);
	}

	protected void DL_SeatTraveller(string sid, CarScript car, int seat)
	{
		PlayerBase pb = DL_FindPlayer(sid);
		if (!pb || !car)
		{
			Print(DL_Const.LOG_TAG + "could not seat " + sid + ": player or car gone");
			return;
		}
		if (seat < 0)
			seat = 0;
		pb.DL_SeatIn(car, seat);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SeatTravellerCheck, 4000, false, sid, car);
	}

	protected void DL_SeatTravellerCheck(string sid, CarScript car)
	{
		PlayerBase pb = DL_FindPlayer(sid);
		if (pb && car)
			Print(DL_Const.LOG_TAG + sid + " seated in his " + car.GetType() + ": " + (car.CrewMemberIndex(pb) >= 0).ToString() + " (seat " + car.CrewMemberIndex(pb).ToString() + "), " + DL_CarSnapshot.FluidText(DL_CarSnapshot.FluidsOf(car)));
	}

	protected void DL_EndCarProtection(CarScript car)
	{
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
		DL_CheckZones();
		DL_DebugTick();
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
			DL_Zone z = DL_ZoneAt(pb.GetPosition());
			int since;
			bool had = m_DL_InZoneSince.Find(sid, since);
			if (!z || !pb.IsAlive())
			{
				m_DL_MustLeaveZone.Remove(sid);
				if (had)
				{
					m_DL_InZoneSince.Remove(sid);
					DL_CrewCountdown(pb, 0, "");
				}
				continue;
			}
			// a failed trip doesn't retry by itself (test 2026-10-18: "Travel failed" every 5 s while standing in it)
			if (m_DL_MustLeaveZone.Contains(sid))
				continue;
			// M3: in a car the DRIVER's countdown sends the whole crew; a passenger with a player at the wheel waits
			if (DL_IsCrewPassenger(pb))
			{
				if (had)
					m_DL_InZoneSince.Remove(sid);
				continue;
			}
			string carProblem = DL_CarProblem(pb);
			if (carProblem != "")
			{
				m_DL_MustLeaveZone.Set(sid, true);
				if (had)
					m_DL_InZoneSince.Remove(sid);
				NotificationSystem.SendNotificationToPlayerIdentityExtended(pb.GetIdentity(), 8, "DriveLink", carProblem, "");
				continue;
			}
			if (!had)
			{
				m_DL_InZoneSince.Set(sid, now);
				DL_CrewCountdown(pb, m_DL_Cfg.countdownSeconds, z.name);
				continue;
			}
			if (now - since >= m_DL_Cfg.countdownSeconds * 1000)
			{
				m_DL_InZoneSince.Remove(sid);
				DL_Depart(pb, z);
			}
		}
		// a driver who left the zone (or got out) cancelled it for his passengers too
		array<string> cancel = new array<string>;
		foreach (string csid, int cs : m_DL_InZoneSince)
		{
			PlayerBase cpb = DL_FindPlayer(csid);
			if (!cpb || !DL_ZoneAt(cpb.GetPosition()))
				cancel.Insert(csid);
		}
		foreach (string gone : cancel)
		{
			m_DL_InZoneSince.Remove(gone);
			PlayerBase gpb = DL_FindPlayer(gone);
			if (gpb)
				DL_CrewCountdown(gpb, 0, "");
		}
	}

	// The countdown (or its cancel, seconds 0) goes to him and, at the wheel of a car, to everyone in it.
	protected void DL_CrewCountdown(PlayerBase pb, int seconds, string zoneName)
	{
		array<PlayerBase> crew = DL_CrewOf(pb);
		foreach (PlayerBase c : crew)
		{
			if (c && c.GetIdentity())
				DL_Countdown(c.GetIdentity(), seconds, zoneName);
		}
	}

	// pb plus, when he drives a car, every other player in it (seat order)
	protected array<PlayerBase> DL_CrewOf(PlayerBase pb)
	{
		array<PlayerBase> crew = new array<PlayerBase>;
		crew.Insert(pb);
		CarScript car = DL_CarOf(pb);
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
		CarScript car = DL_CarOf(pb);
		if (!car || car.CrewDriver() == pb)
			return false;
		PlayerBase driver = PlayerBase.Cast(car.CrewDriver());
		return driver && driver.GetIdentity() && driver.IsAlive();
	}

	// The car he drives, or null (on foot / not a car). HumanCommandVehicle.GetTransport (human.c:716 in 1.30).
	protected CarScript DL_CarOf(PlayerBase pb)
	{
		HumanCommandVehicle vc = pb.GetCommand_Vehicle();
		if (!vc)
			return null;
		return CarScript.Cast(vc.GetTransport());
	}

	// "" = may travel (on foot, or at the wheel of a car whose crew are all players who can travel), else the reason.
	protected string DL_CarProblem(PlayerBase pb)
	{
		HumanCommandVehicle vc = pb.GetCommand_Vehicle();
		if (!vc)
			return "";
		CarScript car = CarScript.Cast(vc.GetTransport());
		if (!car)
			return "Only cars can travel for now. Get out to travel on foot.";
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

	protected DL_Zone DL_ZoneAt(vector pos)
	{
		foreach (DL_Zone z : m_DL_Cfg.zones)
		{
			if (!z)
				continue;
			float dx = pos[0] - z.center[0];
			float dz = pos[2] - z.center[2];
			if (dx * dx + dz * dz <= z.radius * z.radius)
				return z;
		}
		return null;
	}

	protected void DL_Countdown(PlayerIdentity id, int seconds, string zoneName)
	{
		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(seconds);
		rpc.Write(zoneName);
		rpc.Send(null, DL_Const.RPC_COUNTDOWN, true, id);
	}

	// On foot or alone in a car: one trip. With passengers (M3): the passengers' trips go first, the driver's (with
	// the car) only once the hub has taken ALL of them - a refused passenger cancels the driver's trip, so the car
	// never leaves without its crew being accounted for. Passengers who were taken already arrive on foot then.
	protected void DL_Depart(PlayerBase pb, DL_Zone z)
	{
		CarScript car = DL_CarOf(pb);
		array<PlayerBase> crew = DL_CrewOf(pb);
		string sid = pb.GetIdentity().GetPlainId();
		m_DL_Leaving.Set(sid, true);
		pb.SetAllowDamage(false);
		if (car)
		{
			// stop it where it is while the hub answers: engine off, handbrake, no damage (Car.EngineStop/SetHandbrake,
			// car.c:236/209 in 1.30). Anything that changes after the driver's snapshot doesn't travel.
			car.EngineStop();
			car.SetHandbrake(1);
			car.SetAllowDamage(false);
		}
		if (!car || crew.Count() <= 1)
		{
			int alone = 0;
			if (car)
				alone = 1;
			DL_SendDepart(pb, z, car, "", -1, alone);
			return;
		}
		DL_CrewTrip trip = new DL_CrewTrip();
		trip.driverSid = sid;
		trip.car = car;
		trip.zone = z;
		trip.total = crew.Count();
		for (int i = 1; i < crew.Count(); i++)
		{
			PlayerBase m = crew[i];
			string msid = m.GetIdentity().GetPlainId();
			m_DL_Leaving.Set(msid, true);
			m.SetAllowDamage(false);
			trip.waiting++;
			m_DL_TripOf.Set(msid, trip);
			DL_SendDepart(m, z, null, sid, car.CrewMemberIndex(m), crew.Count());
		}
		Print(DL_Const.LOG_TAG + sid + " drives " + car.GetType() + " to " + z.to + " with " + trip.waiting.ToString() + " passenger(s): their trips first, his own once all are taken");
	}

	// One player's trip. car = the car he drives (it goes inside his trip), carGroup/seat = a passenger's tag.
	// crewCount = players in the car (driver included) for the target's waiting room, 0 = on foot.
	protected void DL_SendDepart(PlayerBase pb, DL_Zone z, CarScript car, string carGroup, int seat, int crewCount)
	{
		string sid = pb.GetIdentity().GetPlainId();
		DL_DepartReq q = new DL_DepartReq();
		q.serverId = m_DL_Cfg.serverId;
		q.key = m_DL_Cfg.hubKey;
		q.steamId = sid;
		q.to = z.to;
		q.arrival = z.arrival;
		q.payload = m_DL_Snap.Capture(pb);
		q.payload.crewCount = crewCount;
		string carText = "on foot";
		if (car)
		{
			q.payload.car = m_DL_CarSnap.Capture(car);
			q.payload.carSeat = car.CrewMemberIndex(pb);
			m_DL_TravelCar.Set(sid, car);
			carText = "driving " + car.GetType() + " (" + q.payload.car.items.Count().ToString() + " parts/items, " + DL_CarSnapshot.FluidText(q.payload.car.fluids) + ")";
		}
		else if (carGroup != "")
		{
			q.payload.carGroup = carGroup;
			q.payload.carSeat = seat;
			carText = "passenger in seat " + seat.ToString() + " of " + carGroup + "'s car";
		}
		Print(DL_Const.LOG_TAG + sid + " leaving to " + z.to + " (" + z.arrival + "), " + q.payload.items.Count().ToString() + " items, " + q.payload.bleeding.Count().ToString() + " bleeding, " + q.payload.modifiers.Count().ToString() + " modifiers, " + carText);
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
			DL_SendDepart(driver, trip.zone, trip.car, "", -1, trip.total);
			return;
		}
		string why = "a passenger could not travel";
		if (!driver || DL_CarOf(driver) != trip.car)
			why = "the driver is no longer at the wheel";
		Print(DL_Const.LOG_TAG + trip.driverSid + "'s car trip called off: " + why + " (refused: " + trip.refused.Count().ToString() + ")");
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
		PlayerBase pb = DL_FindPlayer(sid);
		CarScript car = m_DL_TravelCar.Get(sid);
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
			Print(DL_Const.LOG_TAG + sid + " NOT sent: " + why);
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
			DL_ParkCar(car);
			Print(DL_Const.LOG_TAG + car.GetType() + " " + DL_CarGuard.IdOf(car) + " travels with " + sid + ", removing it here once its seats are empty");
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_RemoveCar, 3000, false, car, 0);
		}
		if (!pb)
		{
			// He logged out between the snapshot and now: the hub already holds him in transit, he will arrive on
			// the target next time he joins anywhere (the hub redirects him there).
			Print(DL_Const.LOG_TAG + sid + " left before the redirect; in transit to " + r.target.name);
			return;
		}
		DL_SendRedirect(pb.GetIdentity(), r.target, "zone");
		m_DL_Doomed.Set(pb, true);
		Print(DL_Const.LOG_TAG + sid + " handed over to " + r.target.name + ", removing the character here");
		// give the RPC a moment to reach the client, then remove the character WITHOUT killing it (nothing may
		// stay behind: no body, no dropped gear)
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_RemoveTraveller, 1500, false, sid);
	}

	// Derk 2026-10-18: move the departed car (with the logging-out driver still in it) out of everyone's reach until
	// it can be deleted. Not 0 0 0: that is under the ground / at the world's edge, and an engine delete of an occupied
	// car is what crashed the server. The position is logged 1 s later to see that the move held.
	protected void DL_ParkCar(CarScript car)
	{
		vector p = m_DL_Cfg.parkingSpot;
		if (p[0] == 0 && p[2] == 0)
			return;
		p[1] = g_Game.SurfaceY(p[0], p[2]) + 0.5;
		vector from = car.GetPosition();
		car.SetPosition(p);
		car.SetOrientation(car.GetOrientation());
		car.SetSynchDirty();
		Print(DL_Const.LOG_TAG + car.GetType() + " parked: " + from.ToString() + " -> " + p.ToString());
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_LogCarPos, 1000, false, car);
	}

	protected void DL_LogCarPos(CarScript car)
	{
		if (car)
			Print(DL_Const.LOG_TAG + car.GetType() + " 1 s after parking is at " + car.GetPosition().ToString());
	}

	// NEVER delete a car with someone in a seat: test 2026-10-18 20:06:25 the Chernarus server crashed (access
	// violation, crash_2026-10-18_20-02-09.log) deleting the car while his logging-out character (vanilla keeps it in
	// the world for the logout time, 15 s) still sat in it. Wait for the seats to empty: his character goes when the
	// logout ends (HandleBody -> doomed -> DL_Forget) or at once via DL_RemoveTraveller.
	protected void DL_RemoveCar(CarScript car, int tries = 0)
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
			Print(DL_Const.LOG_TAG + "WARNING " + car.GetType() + " still occupied after 60 s, NOT removed now (removed at next start)");
			return;
		}
		Print(DL_Const.LOG_TAG + car.GetType() + " removed here (seats empty after " + tries.ToString() + " s)");
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
		Print(DL_Const.LOG_TAG + sid + " still connected after the hand-over, disconnecting him the vanilla way");
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

	protected void DL_SendRedirect(PlayerIdentity id, DL_ServerInfo target, string reason)
	{
		DL_Redirect r = new DL_Redirect();
		r.target = target;
		r.reason = reason;
		ScriptRPC rpc = new ScriptRPC();
		string json;
		string err;
		if (!JsonFileLoader<DL_Redirect>.MakeData(r, json, err, false))
			Print(DL_Const.LOG_TAG + "redirect json: " + err);
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

class DL_ConfirmWaiter : Managed
{
	protected string m_Sid;
	protected string m_What;
	void DL_ConfirmWaiter(string sid, string what) { m_Sid = sid; m_What = what; }
	void OnAnswer(bool ok, string body)
	{
		if (ok)
			Print(DL_Const.LOG_TAG + "hub took the " + m_What + " of " + m_Sid);
		else
			Print(DL_Const.LOG_TAG + "WARNING hub did NOT take the " + m_What + " of " + m_Sid + " (see hub window)");
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
