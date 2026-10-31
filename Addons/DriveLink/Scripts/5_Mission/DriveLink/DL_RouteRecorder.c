// DEV BUILDS ONLY: record a driving route for the main menu scene (Research/main-menu-scene.md). In a vehicle, F8
// starts recording, F8 again stops: every 3 m the vehicle's position and speed go into
// $profile:DriveLink/route-recorded.json (also saved every 25 points, so a crash keeps what was driven).
// Server side, -dlRecordStart=x,z,heading puts a joining player at the wheel of a hatchback there.
class DL_RoutePoint
{
	float x;
	float z;
	float v;                  // m/s
}

class DL_RecordedRoute
{
	string world;
	ref array<ref DL_RoutePoint> points = new array<ref DL_RoutePoint>;
}

#ifdef DL_DEV
modded class MissionGameplay
{
	protected bool m_DL_RecKeyDown;
	protected ref DL_RecordedRoute m_DL_Rec;
	protected vector m_DL_RecLast;
	protected float m_DL_RecLen;

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		bool key = KeyState(KeyCode.KC_F8) != 0;
		if (key && !m_DL_RecKeyDown)
			DL_RecToggle();
		m_DL_RecKeyDown = key;
		if (m_DL_Rec)
			DL_RecSample();
	}

	protected Transport DL_RecVehicle()
	{
		PlayerBase me = PlayerBase.Cast(g_Game.GetPlayer());
		if (!me || !me.GetCommand_Vehicle())
			return null;
		return me.GetCommand_Vehicle().GetTransport();
	}

	protected void DL_RecToggle()
	{
		if (m_DL_Rec)
		{
			DL_RecSave();
			NotificationSystem.AddNotificationExtended(8, "DriveLink route recorder", "Stopped: " + m_DL_Rec.points.Count().ToString() + " points, " + Math.Round(m_DL_RecLen).ToString() + " m. Saved to profile DriveLink/route-recorded.json", "");
			DL_Log.Info("route recorder: stopped, " + m_DL_Rec.points.Count().ToString() + " points, " + Math.Round(m_DL_RecLen).ToString() + " m");
			m_DL_Rec = null;
			return;
		}
		if (!DL_RecVehicle())
		{
			NotificationSystem.AddNotificationExtended(5, "DriveLink route recorder", "Get in a vehicle first.", "");
			return;
		}
		m_DL_Rec = new DL_RecordedRoute();
		m_DL_Rec.world = g_Game.GetWorldName();
		m_DL_RecLast = Vector(0, -1000, 0);
		m_DL_RecLen = 0;
		NotificationSystem.AddNotificationExtended(6, "DriveLink route recorder", "Recording. Drive the route, F8 again to stop.", "");
		DL_Log.Info("route recorder: started on " + m_DL_Rec.world);
	}

	protected void DL_RecSample()
	{
		Transport t = DL_RecVehicle();
		if (!t)
			return;
		vector p = t.GetPosition();
		float d = vector.Distance(Vector(p[0], 0, p[2]), Vector(m_DL_RecLast[0], 0, m_DL_RecLast[2]));
		if (m_DL_RecLast[1] > -999 && d < 3)
			return;
		if (m_DL_RecLast[1] > -999)
			m_DL_RecLen += d;
		m_DL_RecLast = p;
		DL_RoutePoint rp = new DL_RoutePoint();
		rp.x = p[0];
		rp.z = p[2];
		Car c = Car.Cast(t);
		if (c)
			rp.v = c.GetSpeedometerAbsolute() / 3.6;
		else
			rp.v = GetVelocity(t).Length();
		m_DL_Rec.points.Insert(rp);
		if (m_DL_Rec.points.Count() % 25 == 0)
			DL_RecSave();
	}

	protected void DL_RecSave()
	{
		MakeDirectory("$profile:DriveLink");
		string err;
		if (!JsonFileLoader<DL_RecordedRoute>.SaveFile("$profile:DriveLink/route-recorded.json", m_DL_Rec, err))
			DL_Log.Warn("route recorder: not saved: " + err);
	}
}

modded class MissionServer
{
	// -dlRecordStart=x,z,heading: whoever joins is put at the wheel of a hatchback there (route recording)
	override void OnClientReadyEvent(PlayerIdentity identity, PlayerBase player)
	{
		super.OnClientReadyEvent(identity, player);
		string v;
		if (identity && player && g_Game.CommandlineGetParam("dlRecordStart", v))
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_RecStartCar, 3000, false, identity.GetPlainId(), v);
	}

	override PlayerBase OnClientNewEvent(PlayerIdentity identity, vector pos, ParamsReadContext ctx)
	{
		PlayerBase pb = super.OnClientNewEvent(identity, pos, ctx);
		string v;
		if (identity && pb && g_Game.CommandlineGetParam("dlRecordStart", v))
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_RecStartCar, 3000, false, identity.GetPlainId(), v);
		return pb;
	}

	// each player's recorder car and when it was given (a rejoin gets a fresh one at the start; the old one goes)
	protected ref map<string, Transport> m_DL_RecCar = new map<string, Transport>;
	protected ref map<string, int> m_DL_RecAt = new map<string, int>;

	protected void DL_RecStartCar(string sid, string spec)
	{
		PlayerBase pb = DL_FindPlayer(sid);
		if (!pb)
			return;
		int now = g_Game.GetTime();
		if (m_DL_RecAt.Contains(sid) && now - m_DL_RecAt.Get(sid) < 20000)
			return;                                  // the new and the ready event of one join
		m_DL_RecAt.Set(sid, now);
		Transport old = m_DL_RecCar.Get(sid);
		if (old && old.CrewSize() > 0 && !old.CrewMember(0))
			g_Game.ObjectDelete(old);
		array<string> parts = new array<string>;
		spec.Split(",", parts);
		if (parts.Count() < 3)
			return;
		vector at = Vector(parts[0].ToFloat(), 0, parts[1].ToFloat());
		at[1] = g_Game.SurfaceY(at[0], at[2]);
		float heading = parts[2].ToFloat();
		Transport car = Transport.Cast(g_Game.CreateObjectEx("OffroadHatchback", at, ECE_SETUP | ECE_UPDATEPATHGRAPH | ECE_CREATEPHYSICS | ECE_TRACE));
		if (!car)
			return;
		car.SetOrientation(Vector(heading, 0, 0));
		car.OnDebugSpawn();                          // wheels, doors, battery, plug, radiator, fluids
		m_DL_RecCar.Set(sid, car);
		vector side = Vector(-Math.Cos(heading * Math.DEG2RAD), 0, Math.Sin(heading * Math.DEG2RAD));
		vector stand = at + side * 3;
		stand[1] = g_Game.SurfaceY(stand[0], stand[2]);
		pb.SetPosition(stand);
		DL_Log.Info("route recorder: hatchback at " + at.ToString() + " heading " + heading.ToString() + " for " + sid);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_RecSeat, 2000, false, sid, car);
	}

	protected void DL_RecSeat(string sid, Transport car)
	{
		PlayerBase pb = DL_FindPlayer(sid);
		if (pb && car)
			pb.DL_SeatIn(car, 0);
	}
}
#endif
