// DriveLink - server side: arrivals, departures, zone watching.
// Vanilla login order (5_mission/mission/missionserver.c:300-365): ClientPrepare -> OnClientPrepareEvent (:436) ->
// ClientNew -> OnClientNewEvent (:536, builds a NEW character) or ClientReady (:586, existing character from the
// game's own database). The hub answer comes back asynchronously, so we ask it in the Prepare step (the client is
// still loading then) and use the answer in New/Ready. Same idea as DaemonForge MapLink (OnClientPrepareEvent loads,
// ClientNew builds) - not its code (AGPL).
// Departure: vanilla HandleBody kills restrained/unconscious players who log out (:713-751). A traveller must
// leave NOTHING behind (or its gear would exist twice), so the old character is deleted, not killed.

class DL_Pending
{
	string action;             // "transfer" | "redirect" | "normal" | "" = no answer yet
	ref DL_ArriveResp resp;
}

modded class MissionServer
{
	protected ref DL_Config m_DL_Cfg;
	protected ref map<string, ref DL_Pending> m_DL_Pending = new map<string, ref DL_Pending>;    // steamId -> hub answer
	protected ref map<string, int> m_DL_InZoneSince = new map<string, int>;                     // steamId -> ms
	protected ref map<string, bool> m_DL_Leaving = new map<string, bool>;                                   // steamIds being sent away
	protected float m_DL_Tick;
	protected ref DL_Snapshot m_DL_Snap = new DL_Snapshot();

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
			// example so the owner sees the shape; travel stays off until hubKey is filled in
			DL_Zone z = new DL_Zone();
			z.name = "West edge";
			z.center = "100 0 7500";
			z.radius = 40;
			z.to = "livonia";
			z.arrival = "east_road";
			m_DL_Cfg.zones.Insert(z);
			DL_ArrivalPoint a = new DL_ArrivalPoint();
			a.name = "west_road";
			a.pos = "200 0 7500";
			a.yaw = 90;
			m_DL_Cfg.arrivals.Insert(a);
			JsonFileLoader<DL_Config>.SaveFile(DL_Const.CONFIG, m_DL_Cfg, err);
			Print(DL_Const.LOG_TAG + "wrote example " + DL_Const.CONFIG + " - fill in hubKey, serverId, zones, arrivals");
		}
		Print(DL_Const.LOG_TAG + "v0.1 server '" + m_DL_Cfg.serverId + "', hub " + m_DL_Cfg.hubUrl + ", " + m_DL_Cfg.zones.Count().ToString() + " zones, " + m_DL_Cfg.arrivals.Count().ToString() + " arrival points, key set " + (m_DL_Cfg.hubKey != "").ToString());
	}

	protected bool DL_On()
	{
		return m_DL_Cfg && m_DL_Cfg.hubKey != "";
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
		DL_Hub.Post(m_DL_Cfg.hubUrl, "/api/arrive", DL_Hub.ToJson(q), new DL_ArriveWaiter(this, sid), "OnAnswer", "arrive " + sid);
		// the client keeps loading while we wait; give the hub time before the character is made
		if (preloadTimeout < 10)
			preloadTimeout = 10;
	}

	void DL_OnArriveAnswer(string sid, bool ok, string body)
	{
		DL_Pending p = m_DL_Pending.Get(sid);
		if (!p)
			return;
		if (!ok)
		{
			p.action = "normal"; // hub down: let the player play here rather than lock him out
			Print(DL_Const.LOG_TAG + "hub unreachable for " + sid + ", normal login");
			return;
		}
		DL_ArriveResp r;
		string err;
		JsonSerializer js = new JsonSerializer();
		if (!js.ReadFromString(r, body, err) || !r || !r.ok)
		{
			p.action = "normal";
			Print(DL_Const.LOG_TAG + "hub answer for " + sid + " unusable: " + err + " " + body.Substring(0, Math.Min(200, body.Length())));
			return;
		}
		p.resp = r;
		p.action = r.action;
		Print(DL_Const.LOG_TAG + "hub says " + r.action + " for " + sid);
	}

	override PlayerBase OnClientNewEvent(PlayerIdentity identity, vector pos, ParamsReadContext ctx)
	{
		DL_Pending p;
		if (DL_On() && identity)
			p = m_DL_Pending.Get(identity.GetPlainId());
		if (p && p.action == "transfer" && p.resp && p.resp.payload)
		{
			m_DL_Pending.Remove(identity.GetPlainId());
			return DL_BuildArrival(identity, p.resp);
		}
		PlayerBase player = super.OnClientNewEvent(identity, pos, ctx);
		DL_AfterLogin(identity, player, p);
		return player;
	}

	override void OnClientReadyEvent(PlayerIdentity identity, PlayerBase player)
	{
		super.OnClientReadyEvent(identity, player);
		DL_Pending p;
		if (DL_On() && identity)
			p = m_DL_Pending.Get(identity.GetPlainId());
		if (p && p.action == "transfer")
		{
			// He still had a character in this server's own database (e.g. he played here before) AND arrives by
			// transfer. The transfer wins: remove the old one and send him through a fresh login.
			// Should not happen any more: when a character leaves a server, DL_Forget marks it dead in that server's
			// database. If it does (old data from before DriveLink), he keeps the old one and we say so loudly.
			Print(DL_Const.LOG_TAG + "WARNING " + identity.GetPlainId() + " arrives by transfer but this server still had an ALIVE old character for him; the old one was loaded, the transfer is NOT applied");
		}
		DL_AfterLogin(identity, player, p);
	}

	protected void DL_AfterLogin(PlayerIdentity identity, PlayerBase player, DL_Pending p)
	{
		if (!identity)
			return;
		m_DL_Pending.Remove(identity.GetPlainId());
		if (p && p.action == "redirect" && p.resp && p.resp.target)
		{
			Print(DL_Const.LOG_TAG + identity.GetPlainId() + " belongs on " + p.resp.target.name + ", redirecting");
			NotificationSystem.SendNotificationToPlayerIdentityExtended(identity, 8, "DriveLink", "Your character is on " + p.resp.target.name + ". Taking you there...", "");
			DL_SendRedirect(identity, p.resp.target, "owner");
			if (player)
			{
				player.SetAllowDamage(false);
				m_DL_Leaving.Set(identity.GetPlainId(), true);
			}
		}
	}

	protected PlayerBase DL_BuildArrival(PlayerIdentity identity, DL_ArriveResp r)
	{
		DL_PlayerData d = r.payload;
		DL_ArrivalPoint ap = m_DL_Cfg.FindArrival(r.arrival);
		vector pos = "0 0 0";
		float yaw = 0;
		if (ap)
		{
			pos = ap.pos;
			yaw = ap.yaw;
		}
		else
			Print(DL_Const.LOG_TAG + "arrival point '" + r.arrival + "' not in config.json, using the first one");
		if (!ap && m_DL_Cfg.arrivals.Count() > 0)
		{
			pos = m_DL_Cfg.arrivals[0].pos;
			yaw = m_DL_Cfg.arrivals[0].yaw;
		}
		pos[1] = g_Game.SurfaceY(pos[0], pos[2]);
		// CreateCharacter (missionserver.c:486): CreatePlayer + SelectPlayer, same as a fresh spawn but his own body
		PlayerBase player = CreateCharacter(identity, pos, null, d.charType);
		if (!player)
		{
			Print(DL_Const.LOG_TAG + "could not create " + d.charType + " for " + identity.GetPlainId());
			return null;
		}
		player.SetOrientation(Vector(yaw, 0, 0));
		int made = m_DL_Snap.Apply(player, d);
		if (m_DL_Cfg.arrivalProtectSeconds > 0)
		{
			player.SetAllowDamage(false);
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_EndProtection, m_DL_Cfg.arrivalProtectSeconds * 1000, false, player);
		}
		Print(DL_Const.LOG_TAG + identity.GetPlainId() + " arrived from " + r.from + " at " + r.arrival + " " + pos.ToString() + ", " + made.ToString() + " items, health " + d.health.ToString() + ", cuffed " + d.restrained.ToString());
		NotificationSystem.SendNotificationToPlayerIdentityExtended(identity, 6, "DriveLink", "Welcome from " + r.from, "");
		return player;
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
				if (had)
				{
					m_DL_InZoneSince.Remove(sid);
					DL_Countdown(pb.GetIdentity(), 0, "");
				}
				continue;
			}
			if (!had)
			{
				m_DL_InZoneSince.Set(sid, now);
				DL_Countdown(pb.GetIdentity(), m_DL_Cfg.countdownSeconds, z.name);
				continue;
			}
			if (now - since >= m_DL_Cfg.countdownSeconds * 1000)
			{
				m_DL_InZoneSince.Remove(sid);
				DL_Depart(pb, z);
			}
		}
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

	protected void DL_Depart(PlayerBase pb, DL_Zone z)
	{
		string sid = pb.GetIdentity().GetPlainId();
		m_DL_Leaving.Set(sid, true);
		pb.SetAllowDamage(false);
		DL_DepartReq q = new DL_DepartReq();
		q.serverId = m_DL_Cfg.serverId;
		q.key = m_DL_Cfg.hubKey;
		q.steamId = sid;
		q.to = z.to;
		q.arrival = z.arrival;
		q.payload = m_DL_Snap.Capture(pb);
		Print(DL_Const.LOG_TAG + sid + " leaving to " + z.to + " (" + z.arrival + "), " + q.payload.items.Count().ToString() + " top items");
		DL_Hub.Post(m_DL_Cfg.hubUrl, "/api/depart", DL_Hub.ToJson(q), new DL_DepartWaiter(this, sid), "OnAnswer", "depart " + sid);
	}

	void DL_OnDepartAnswer(string sid, bool ok, string body)
	{
		PlayerBase pb = DL_FindPlayer(sid);
		DL_DepartResp r;
		string err;
		JsonSerializer js = new JsonSerializer();
		if (ok)
			js.ReadFromString(r, body, err);
		if (!ok || !r || !r.ok || !r.target)
		{
			// Nothing was handed over: he stays here with everything he has.
			m_DL_Leaving.Remove(sid);
			string why = "hub not reachable";
			if (r && r.error != "")
				why = r.error;
			Print(DL_Const.LOG_TAG + sid + " NOT sent: " + why);
			if (pb)
			{
				pb.SetAllowDamage(true);
				NotificationSystem.SendNotificationToPlayerIdentityExtended(pb.GetIdentity(), 6, "DriveLink", "Travel failed: " + why, "");
			}
			return;
		}
		if (!pb)
		{
			// He logged out between the snapshot and now: the hub already holds him in transit, he will arrive on
			// the target next time he joins anywhere (the hub redirects him there).
			Print(DL_Const.LOG_TAG + sid + " left before the redirect; in transit to " + r.target.name);
			return;
		}
		DL_SendRedirect(pb.GetIdentity(), r.target, "zone");
		Print(DL_Const.LOG_TAG + sid + " handed over to " + r.target.name + ", removing the character here");
		// give the RPC a moment to reach the client, then remove the character WITHOUT killing it (nothing may
		// stay behind: no body, no dropped gear)
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_RemoveTraveller, 1500, false, sid);
	}

	protected void DL_RemoveTraveller(string sid)
	{
		DL_Forget(DL_FindPlayer(sid));
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
		if (player && player.GetIdentity() && m_DL_Leaving.Contains(player.GetIdentity().GetPlainId()))
		{
			// travelling: never kill (vanilla kills cuffed/unconscious loggers-out) and never leave a body
			m_DL_Leaving.Remove(player.GetIdentity().GetPlainId());
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
		rpc.Write(DL_Hub.ToJson(r));
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

// The hub callback hands back (ok, body); these carry the steam id along.
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
