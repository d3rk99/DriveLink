// DriveLink main menu "on the road" scene (Research/main-menu-scene.md). A trip with travel time sends the player to
// the main menu; the server sent him first who travels with him (RPC_MENU_SCENE, in parts like the film): every
// vehicle of the party with its parts, every member's character class, worn items and seat. The client keeps it in
// $profile:DriveLink/menuscene.json with the time the trip ends, so the menu shows it also after a game restart, until
// that time (DL_MenuSceneBuilder, 5_Mission, builds it in the menu's offline world).
class DL_MenuItem
{
	string type;
	string slot;              // inventory slot name ("" = let the game pick)
}

class DL_MenuVehicle
{
	string type;
	ref array<ref DL_MenuItem> parts = new array<ref DL_MenuItem>;
}

class DL_MenuMember
{
	string name;
	string charType;          // e.g. SurvivorM_Mirek
	ref array<ref DL_MenuItem> worn = new array<ref DL_MenuItem>;
	int vehicle = -1;         // index into vehicles, -1 = on foot
	int seat = -1;
}

class DL_MenuScene
{
	static const string FILE = "$profile:DriveLink/menuscene.json";
	string target;            // where the trip goes
	int seconds;              // on the road (from the server)
	int until;                // client: DL_MenuScene.Now() when the trip is over
	int you = -1;             // index of the receiving player in members
	ref array<ref DL_MenuVehicle> vehicles = new array<ref DL_MenuVehicle>;
	ref array<ref DL_MenuMember> members = new array<ref DL_MenuMember>;

	// seconds since 2000-01-01 UTC, from the PC's clock (enough to count a trip down across game restarts)
	static int Now()
	{
		int y;
		int mo;
		int d;
		int h;
		int mi;
		int s;
		GetYearMonthDayUTC(y, mo, d);
		GetHourMinuteSecondUTC(h, mi, s);
		int days = 0;
		for (int yy = 2000; yy < y; yy++)
		{
			days += 365;
			if (yy % 4 == 0)
				days++;
		}
		TIntArray monthDays = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
		for (int m = 1; m < mo; m++)
		{
			days += monthDays[m - 1];
			if (m == 2 && y % 4 == 0)
				days++;
		}
		days += d - 1;
		return days * 86400 + h * 3600 + mi * 60 + s;
	}

	int Left()
	{
		return until - Now();
	}

	void Save()
	{
		MakeDirectory("$profile:DriveLink");
		string err;
		if (!JsonFileLoader<DL_MenuScene>.SaveFile(FILE, this, err))
			DL_Log.Warn("menu scene: not saved: " + err);
	}

	// the saved scene while its trip lasts, else null (an old one is deleted)
	static DL_MenuScene Load()
	{
		if (!FileExist(FILE))
			return null;
		DL_MenuScene s;
		string err;
		if (!JsonFileLoader<DL_MenuScene>.LoadFile(FILE, s, err) || !s || !s.members)
		{
			DeleteFile(FILE);
			return null;
		}
		// a little past the end: the line says "you have arrived" until the menu is opened again
		if (s.Left() < -600)
		{
			DeleteFile(FILE);
			return null;
		}
		return s;
	}

	static void Clear()
	{
		if (FileExist(FILE))
			DeleteFile(FILE);
	}
}
