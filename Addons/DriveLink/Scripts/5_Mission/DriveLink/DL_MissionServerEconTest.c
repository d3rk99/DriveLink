// Vehicle economy test driver (dev builds only, debug.json econ* switches, Tests/EconomyTest/README.md).
// Two servers and a hub, no players: cars "travel" as snapshot files the test runner copies from one server's
// $profile:DriveLink/econ-outbox.json to the other's econ-inbox.json. Departure and arrival use the real code
// (DL_CarSnapshot.Capture/Build -> DL_CarLedger.Stamp/Arrived, DL_CarLedger.Left), only the trip itself is a file.
// The file name sorts after DL_MissionServer.c, so this modded MissionServer sees its members.
#ifdef DL_DEV
modded class MissionServer
{
	override void OnMissionStart()
	{
		super.OnMissionStart();
		if (!m_DL_Debug || m_DL_Debug.econEvent == "")
			return;
		string dlMsg14 = "ECON test: event ";
		dlMsg14 += m_DL_Debug.econEvent + ", send " + m_DL_Debug.econSendCount.ToString();
		dlMsg14 += " at " + m_DL_Debug.econSendAt.ToString() + " s, receive at ";
		dlMsg14 += m_DL_Debug.econReceiveAt.ToString() + " s, ruin " + m_DL_Debug.econRuinCount.ToString();
		dlMsg14 += " at " + m_DL_Debug.econRuinAt.ToString() + " s, exit at ";
		dlMsg14 += m_DL_Debug.econExitAt.ToString() + " s";
		DL_Log.Info(dlMsg14);
		ScriptCallQueue q = g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM);
		if (m_DL_Debug.econSendAt > 0)
			q.CallLater(DL_EconSend, m_DL_Debug.econSendAt * 1000, false);
		if (m_DL_Debug.econReceiveAt > 0)
			q.CallLater(DL_EconReceive, m_DL_Debug.econReceiveAt * 1000, false);
		if (m_DL_Debug.econRuinAt > 0)
			q.CallLater(DL_EconRuin, m_DL_Debug.econRuinAt * 1000, false);
		if (m_DL_Debug.econExitAt > 0)
			q.CallLater(DL_EconExit, m_DL_Debug.econExitAt * 1000, false);
		if (m_DL_Debug.econStatusEvery > 0)
			q.CallLater(DL_EconStatus, m_DL_Debug.econStatusEvery * 1000, true);
	}

	void DL_EconStatus()
	{
		DL_Log.Info("ECON status " + DL_CarLedger.Status(m_DL_Debug.econEvent));
	}

	// cars of the event that belong here (never travelled, or back home); econSendForeign: those from other servers
	protected void DL_EconOwn(array<Transport> own)
	{
		array<Transport> live = DL_CarLedger.Live();
		for (int i = 0; i < live.Count(); i++)
		{
			Transport c = live.Get(i);
			if (!c || DL_CarLedger.IsLeaving(c) || DL_CarLedger.EventOf(c.GetType()) != m_DL_Debug.econEvent)
				continue;
			string home = DL_CarLedger.HomeOf(c);
			bool mine = home == "" || home == m_DL_Cfg.serverId;
			if (mine == m_DL_Debug.econSendForeign)
				continue;
			own.Insert(c);
		}
	}

	void DL_EconSend()
	{
		array<Transport> own = new array<Transport>;
		DL_EconOwn(own);
		DL_EconBox box = new DL_EconBox();
		int sent = 0;
		foreach (Transport car : own)
		{
			if (sent >= m_DL_Debug.econSendCount)
				break;
			if (!car)
				continue;
			DL_CarData d = m_DL_CarSnap.Capture(car);
			box.cars.Insert(d);
			DL_CarGuard.Remember(car);
			DL_CarLedger.Left(car);
			DL_Log.Info("ECON sent " + d.type + " " + d.dlid + " at " + car.GetPosition().ToString());
			g_Game.ObjectDelete(car);
			sent++;
		}
		string err;
		if (!JsonFileLoader<DL_EconBox>.SaveFile(DL_Const.DIR + "/econ-outbox.json", box, err))
			DL_Log.Warn("ECON could not write the outbox: " + err);
		DL_EconStatus();
	}

	void DL_EconReceive()
	{
		string file = DL_Const.DIR + "/econ-inbox.json";
		DL_EconBox box;
		string err;
		if (!FileExist(file) || !JsonFileLoader<DL_EconBox>.LoadFile(file, box, err) || !box)
		{
			DL_Log.Warn("ECON no inbox to receive (" + err + ")");
			return;
		}
		for (int i = 0; i < box.cars.Count(); i++)
		{
			DL_CarData d = box.cars[i];
			// a row of arrivals in the open field north of Vybor airfield
			float x = 4100 + i * 15;
			float z = 10900;
			vector pos = Vector(x, g_Game.SurfaceY(x, z) + 0.5, z);
			int made;
			int dropped;
			Transport car = m_DL_CarSnap.Build(d, pos, vector.Zero, made, dropped);
			string dlMsg88 = "ECON received ";
			dlMsg88 += d.type + " " + d.dlid;
			dlMsg88 += " (home " + d.home + "): built ";
			dlMsg88 += (car != null).ToString();
			DL_Log.Info(dlMsg88);
		}
		DeleteFile(file);
		DL_EconStatus();
	}

	void DL_EconRuin()
	{
		// a copy: deleting removes cars from the live list
		array<Transport> all = new array<Transport>;
		all.Copy(DL_CarLedger.Live());
		int n = 0;
		for (int i = 0; i < all.Count() && n < m_DL_Debug.econRuinCount; i++)
		{
			Transport c = all.Get(i);
			if (!c)
				continue;
			string home = DL_CarLedger.HomeOf(c);
			if (home == "" || home == m_DL_Cfg.serverId)
				continue;
			DL_Log.Info("ECON ruined " + c.GetType() + " " + DL_CarLedger.DlidOf(c) + " (home " + home + ")");
			g_Game.ObjectDelete(c);
			n++;
		}
		DL_EconStatus();
	}

	void DL_EconExit()
	{
		DL_EconStatus();
		DL_Log.Info("ECON exit");
		g_Game.RequestExit(0);
	}
}
#endif
