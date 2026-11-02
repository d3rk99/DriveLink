// Interactive trail test driver (dev builds only, debug.json trailTest): every player who logs in is put in the
// first zone a moment later with a compass, epoxy putty and a bandage (so the cards show what helps), and the server
// shuts down cleanly trailTestExitAt seconds after it started. The client side is -dlTrailAuto (DL_TrailClient).
// The file name sorts after DL_MissionServer.c, so this modded MissionServer sees its members.
#ifdef DL_DEV
modded class MissionServer
{
	protected ref map<PlayerBase, bool> m_DL_TrailPlaced = new map<PlayerBase, bool>; // per character (each login is a new one)
	protected float m_DL_TrailTestT;

	override void OnMissionStart()
	{
		super.OnMissionStart();
		if (!m_DL_Debug || !m_DL_Debug.trailTest)
			return;
		DL_Log.Info("TRAIL test: players go to the first zone after login, exit at " + m_DL_Debug.trailTestExitAt.ToString() + " s");
		vector st = m_DL_Debug.trailTestStaging;
		if (st[0] != 0 || st[2] != 0)
			m_DL_Cfg.trail.stagingPos = st;
		if (m_DL_Debug.trailTestExitAt > 0)
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_TrailTestExit, m_DL_Debug.trailTestExitAt * 1000, false);
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (!m_DL_Debug || !m_DL_Debug.trailTest)
			return;
		m_DL_TrailTestT += timeslice;
		if (m_DL_TrailTestT < 5)
			return;
		m_DL_TrailTestT = 0;
		array<Man> men = new array<Man>;
		g_Game.GetPlayers(men);
		foreach (Man m : men)
		{
			PlayerBase pb = PlayerBase.Cast(m);
			if (!pb || !pb.GetIdentity() || !pb.IsAlive())
				continue;
			string sid = pb.GetIdentity().GetPlainId();
			if (m_DL_TrailPlaced.Contains(pb))
				continue;
			m_DL_TrailPlaced.Set(pb, true);
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_TrailTestPlace, 15000, false, sid);
		}
	}

	protected void DL_TrailTestPlace(string sid)
	{
		PlayerBase pb = DL_FindPlayer(sid);
		if (!pb || !pb.IsAlive())
			return;
		foreach (DL_Zone z : m_DL_Cfg.zones)
		{
			if (z.air)
				continue;
			vector p = z.center;
			p[1] = g_Game.SurfaceY(p[0], p[2]);
			pb.SetPosition(p);
			if (m_DL_Debug.trailTestCar != "")
			{
				vector cp = p;
				cp[0] = cp[0] + 4;
				cp[1] = g_Game.SurfaceY(cp[0], cp[2]);
				Transport car = Transport.Cast(g_Game.CreateObjectEx(m_DL_Debug.trailTestCar, cp, ECE_SETUP | ECE_UPDATEPATHGRAPH | ECE_CREATEPHYSICS | ECE_TRACE));
				if (car)
				{
					car.OnDebugSpawn();
					g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_TrailTestSeat, 2000, false, sid, car);
				}
			}
			pb.GetInventory().CreateInInventory("Compass");
			pb.GetInventory().CreateInInventory("EpoxyPutty");
			pb.GetInventory().CreateInInventory("BandageDressing");
			DL_Log.Info("TRAIL test: " + sid + " put in zone " + z.name + " at " + p.ToString());
			return;
		}
	}

	protected void DL_TrailTestSeat(string sid, Transport car)
	{
		PlayerBase pb = DL_FindPlayer(sid);
		if (pb && car)
		{
			pb.DL_SeatIn(car, 0);
			DL_Log.Info("TRAIL test: " + sid + " at the wheel of " + car.GetType());
		}
	}

	protected void DL_TrailTestExit()
	{
		DL_Log.Info("TRAIL test: exit");
		g_Game.RequestExit(0);
	}
}
#endif
