// CE spike: which cars does CE create (EEOnCECreate), and what happens when we veto them.
// Settings come from $profile:DLCESpike/config.json (DLCE_Config); every line starts with [DLCE] in the script log.

class DLCE_Config
{
	ref array<string> family = new array<string>;	// car classes of one CE event (e.g. VehicleOffroadHatchback)
	int actAt = 120;		// seconds after mission start for the delete/spawn step
	int deleteCount = 0;	// delete this many CE-made family cars at actAt (= cars "leaving")
	int spawnCount = 0;		// make this many family cars by script at actAt (= cars "arriving")
	bool vetoAfterAct = false;	// after actAt, delete every new CE-made family car
	int reportEvery = 30;	// seconds between count lines
	int exitAt = 0;			// > 0: shut the server down cleanly this many seconds after start (storage is saved)
}

class DLCE
{
	static ref DLCE_Config cfg;
	static ref array<CarScript> cars = new array<CarScript>;
	static ref array<CarScript> ceMade = new array<CarScript>;
	static ref array<CarScript> scriptMade = new array<CarScript>;
	static bool scriptSpawning;
	static bool acted;
	static int vetoes;

	static void Log(string s)
	{
		Print("[DLCE] t=" + (g_Game.GetTime() / 1000).ToString() + " " + s);
	}

	static bool InFamily(string type)
	{
		return cfg && cfg.family.Find(type) >= 0;
	}

	static void Delete(Object o)
	{
		if (o)
			g_Game.ObjectDelete(o);
	}
}

modded class CarScript
{
	override void EEInit()
	{
		super.EEInit();
		if (!g_Game.IsDedicatedServer())
			return;
		DLCE.cars.Insert(this);
		string how = "loaded/other";
		if (DLCE.scriptSpawning)
		{
			how = "script";
			DLCE.scriptMade.Insert(this);
		}
		DLCE.Log("EEInit " + GetType() + " at " + GetPosition().ToString() + " (" + how + ")");
	}

	override void EEOnCECreate()
	{
		super.EEOnCECreate();
		if (!g_Game.IsDedicatedServer())
			return;
		DLCE.Log("EEOnCECreate " + GetType() + " at " + GetPosition().ToString() + " scriptSpawning=" + DLCE.scriptSpawning.ToString());
		if (DLCE.InFamily(GetType()) && !DLCE.scriptSpawning)
			DLCE.ceMade.Insert(this);
		if (DLCE.cfg && DLCE.cfg.vetoAfterAct && DLCE.acted && DLCE.InFamily(GetType()))
		{
			DLCE.vetoes++;
			DLCE.Log("VETO #" + DLCE.vetoes.ToString() + " " + GetType() + " at " + GetPosition().ToString());
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DLCE.Delete, 100, false, this);
		}
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);
		if (!g_Game.IsDedicatedServer())
			return;
		DLCE.cars.RemoveItem(this);
		DLCE.ceMade.RemoveItem(this);
		DLCE.scriptMade.RemoveItem(this);
		DLCE.Log("EEDelete " + GetType() + " at " + GetPosition().ToString());
	}
}
