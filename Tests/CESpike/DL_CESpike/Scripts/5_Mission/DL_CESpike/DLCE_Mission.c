modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();
		string file = "$profile:DLCESpike/config.json";
		string err;
		if (FileExist(file))
			JsonFileLoader<DLCE_Config>.LoadFile(file, DLCE.cfg, err);
		if (!DLCE.cfg)
		{
			DLCE.cfg = new DLCE_Config();
			DLCE.Log("no config at " + file + " (" + err + "), logging only");
		}
		DLCE.Log("config: family=" + DLCE.cfg.family.Count().ToString() + " actAt=" + DLCE.cfg.actAt.ToString() + " delete=" + DLCE.cfg.deleteCount.ToString() + " spawn=" + DLCE.cfg.spawnCount.ToString() + " veto=" + DLCE.cfg.vetoAfterAct.ToString());
		DLCE_ReadFiles();
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DLCE_Act, DLCE.cfg.actAt * 1000, false);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DLCE_Report, DLCE.cfg.reportEvery * 1000, true);
		if (DLCE.cfg.exitAt > 0)
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DLCE_Exit, DLCE.cfg.exitAt * 1000, false);
	}

	// Q4: can a server mod read the mission's economy files?
	void DLCE_ReadFiles()
	{
		array<string> paths = {"$mission:db/events.xml", "$mission:db/types.xml", "$mission:cfgeconomycore.xml", "$mission:cfgeventspawns.xml"};
		foreach (string p : paths)
		{
			if (!FileExist(p))
			{
				DLCE.Log("READ " + p + ": not found");
				continue;
			}
			FileHandle fh = OpenFile(p, FileMode.READ);
			if (!fh)
			{
				DLCE.Log("READ " + p + ": exists but OpenFile failed");
				continue;
			}
			string line;
			int lines;
			int events;
			bool inHatch;
			string nominal = "?";
			while (FGets(fh, line) >= 0)
			{
				lines++;
				if (line.Contains("<event name="))
				{
					events++;
					inHatch = line.Contains("\"VehicleOffroadHatchback\"");
				}
				if (inHatch && line.Contains("<nominal>"))
				{
					nominal = line.Trim();
					inHatch = false;
				}
			}
			CloseFile(fh);
			DLCE.Log("READ " + p + ": " + lines.ToString() + " lines, " + events.ToString() + " events, VehicleOffroadHatchback " + nominal);
		}
	}

	void DLCE_Act()
	{
		int deleted;
		// family cars not made by this script: CE-made this run or loaded from storage
		for (int i = DLCE.cars.Count() - 1; i >= 0 && deleted < DLCE.cfg.deleteCount; i--)
		{
			CarScript c = DLCE.cars[i];
			if (!c || !DLCE.InFamily(c.GetType()) || DLCE.scriptMade.Find(c) >= 0)
				continue;
			DLCE.Log("ACT delete (leaving) " + c.GetType() + " at " + c.GetPosition().ToString());
			g_Game.ObjectDelete(c);
			deleted++;
		}
		DLCE.scriptSpawning = true;
		for (int s = 0; s < DLCE.cfg.spawnCount && DLCE.cfg.family.Count() > 0; s++)
		{
			// a row of "arrivals" in the open field north of Vybor airfield
			float x = 4100 + s * 15;
			float z = 10900;
			vector pos = Vector(x, g_Game.SurfaceY(x, z) + 0.5, z);
			string type = DLCE.cfg.family[s % DLCE.cfg.family.Count()];
			Object o = g_Game.CreateObjectEx(type, pos, ECE_SETUP | ECE_UPDATEPATHGRAPH | ECE_CREATEPHYSICS | ECE_TRACE);
			DLCE.Log("ACT spawn (arriving) " + type + " at " + pos.ToString() + " ok=" + (o != null).ToString());
		}
		DLCE.scriptSpawning = false;
		DLCE.acted = true;
		DLCE_Report();
	}

	void DLCE_Exit()
	{
		DLCE_Report();
		DLCE.Log("EXIT requested");
		g_Game.RequestExit(0);
	}

	void DLCE_Report()
	{
		map<string, int> counts = new map<string, int>;
		int fam;
		foreach (CarScript c : DLCE.cars)
		{
			if (!c)
				continue;
			counts.Set(c.GetType(), counts.Get(c.GetType()) + 1);
			if (DLCE.InFamily(c.GetType()))
				fam++;
		}
		string s = "COUNT all=" + DLCE.cars.Count().ToString() + " family=" + fam.ToString() + " ceMadeFamily=" + DLCE.ceMade.Count().ToString() + " scriptMade=" + DLCE.scriptMade.Count().ToString() + " vetoes=" + DLCE.vetoes.ToString() + " |";
		for (int i = 0; i < counts.Count(); i++)
			s += " " + counts.GetKey(i) + "=" + counts.GetElement(i).ToString();
		DLCE.Log(s);
	}
}
