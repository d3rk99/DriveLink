// DriveLink - parser self test (debug, M1). If $profile:DriveLink/selftest/ has t*.json files, the server tries to
// read each one as DL_PlayerData at boot and logs OK/FAIL + the item count. Used to find which JSON shape DayZ's
// reader rejects ("Miss fraction part in number", 2f8669a test) - the files are variants of one real snapshot.
// Remove the folder (or this file) when done.
modded class MissionServer
{
	override void OnMissionStart()
	{
		super.OnMissionStart();
		DL_SelfTest();
	}

	protected void DL_SelfTest()
	{
		string dir = DL_Const.DIR + "/selftest/";
		string name;
		FileAttr attr;
		// FindFile/FindNextFile/CloseFindFile (vanilla 1_core/proto/ensystem.c:520-522)
		FindFileHandle h = FindFile(dir + "*.json", name, attr, 0);
		if (!h)
			return;
		bool more = true;
		while (more)
		{
			FileHandle f = OpenFile(dir + name, FileMode.READ);
			string json = "";
			if (f)
			{
				// ReadFile reads the whole file in one go, like JsonFileLoader.LoadFile (jsonfileloader.c:19)
				ReadFile(f, json, 1000000);
				CloseFile(f);
			}
			DL_PlayerData d;
			string err;
			bool ok = JsonFileLoader<DL_PlayerData>.LoadData(json, d, err);
			int n = -1;
			if (ok && d)
				n = DL_Hub.CountItems(d.items);
			// also count without any recursion: top items + their direct children (ground truth for CountItems)
			int flat = -1;
			if (ok && d && d.items)
			{
				flat = d.items.Count();
				for (int k = 0; k < d.items.Count(); k++)
				{
					if (d.items[k] && d.items[k].children)
						flat += d.items[k].children.Count();
				}
			}
			Print(DL_Const.LOG_TAG + "SELFTEST " + name + " (" + json.Length().ToString() + " chars): " + ok.ToString() + ", items " + n.ToString() + ", counted flat " + flat.ToString() + " " + err);
			more = FindNextFile(h, name, attr);
		}
		CloseFindFile(h);
	}
}
