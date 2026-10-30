// DriveLink vehicle economy (Research/vehicle-economy.md). Server only.
// Every car that travels gets a DriveLink id (dlid) and a home: the server whose Central Economy spawned it. While it
// is away (on another server or on the road) it keeps its place at home: the home server's mod vetoes the CE respawns
// of its vehicle event that would replace it. The hub keeps the ledger (POST /api/cars, Hub/src/cars.rs).
//
// Spike results (Tests/CESpike, 2026-10-29):
// - Vehicles come from events.xml events (VehicleOffroadHatchback: nominal 8, min 5); types.xml nominal is 0.
//   A server mod can read $mission:db/events.xml.
// - CE calls EEOnCECreate (entityai.c:1414) for every event vehicle it spawns, after EEInit. CarScript, BoatScript
//   and MotorbikeScript override it without calling super, so the hook sits on each of them (end of this file).
// - A car made by script (an arrival, or a car back home) is NOT counted by its event, also after restarts
//   (Tests/EconomyTest). So CE's own count can't be trusted: this file counts the cars itself.
//
// The rule, per event E on server S, when CE spawns a car of E:
//   (cars of E here whose home is S) + (cars of E with home S that are away) >= nominal(E)  ->  delete the new car.
// Untagged cars (never travelled) count as home = S. Cars from another server don't count here: they are owed to
// their own home.
//
// carEconomy in config.json: "off" (default) | "track" (report to the hub, log the vetoes it would make) | "veto".
// Tags are kept in $profile:DriveLink/car-ledger.json by persistent id (no change to the game's storage format).

class DL_CarTag
{
	string pid;               // EntityAI.GetPersistentID as "b1-b2-b3-b4" ("" until the game first saved the car)
	string dlid;
	string type;
	string home;
}

// a live car's tag (held by DL_CarLedger, not on the entity: Transport can't be modded)
class DL_CarState
{
	string dlid;
	string home;
	bool leaving;
}

class DL_CarAway
{
	string type;
	int n;
}

class DL_CarLedgerFile
{
	ref array<ref DL_CarTag> tags = new array<ref DL_CarTag>;      // travelled cars on this server
	ref array<ref DL_CarTag> left = new array<ref DL_CarTag>;      // left this server, the hub has not taken it yet
	ref array<ref DL_CarAway> away = new array<ref DL_CarAway>;    // the hub's last answer (vetoes work before it answers)
}

class DL_CarsReq
{
	string serverId;
	string key;
	ref array<ref DL_CarTag> left;
	ref array<ref DL_CarTag> here;
}

class DL_CarsResp
{
	bool ok;
	ref array<ref DL_CarAway> away;
}

class DL_CarLedger
{
	static const string FILE = "$profile:DriveLink/car-ledger.json";
	static const int REPORT_MS = 60000;

	static string s_Mode = "off";
	static string s_ServerId;
	protected static ref DL_CarLedgerFile s_File;
	protected static ref array<Transport> s_Live = new array<Transport>;
	protected static ref map<Transport, ref DL_CarState> s_State = new map<Transport, ref DL_CarState>; // tagged cars
	protected static ref map<string, string> s_EventOf = new map<string, string>;  // car class -> event name
	protected static ref map<string, int> s_Nominal = new map<string, int>;        // event name -> nominal
	protected static ref map<string, int> s_Away = new map<string, int>;           // car class -> own cars away
	protected static int s_Sent;      // left[] entries in the report on its way (removed when the hub took them)
	protected static int s_Vetoes;
	protected static int s_Made;      // ids made this session

	static bool On()
	{
		return s_Mode == "track" || s_Mode == "veto";
	}

	// MissionServer, once config.json is loaded
	static void Start(string mode, string serverId)
	{
		s_Mode = mode;
		s_ServerId = serverId;
		if (!On())
			return;
		Math.Randomize(-1);
		Load();
		ReadEvents();
		string dlMsg95 = "vehicle economy: ";
		dlMsg95 += s_Mode + ", " + s_Nominal.Count().ToString();
		dlMsg95 += " vehicle events, " + s_File.tags.Count().ToString() + " travelled cars, ";
		dlMsg95 += s_Away.Count().ToString() + " classes with cars away";
		DL_Log.Info(dlMsg95);
	}

	static void SetServerId(string id)
	{
		s_ServerId = id;
	}

	// ---------------- events.xml: which event a car class belongs to, and its nominal ----------------

	// cfgeconomycore.xml can add more event files: <ce folder="x"><file name="y.xml" type="events"/></ce>
	protected static void ReadEvents()
	{
		s_EventOf.Clear();
		s_Nominal.Clear();
		ReadEventFile("$mission:db/events.xml");
		FileHandle fh = OpenFile("$mission:cfgeconomycore.xml", FileMode.READ);
		if (!fh)
			return;
		string line;
		string folder = "";
		while (FGets(fh, line) >= 0)
		{
			if (line.Contains("<ce ") && line.Contains("folder="))
				folder = Attr(line, "folder");
			if (line.Contains("<file ") && Attr(line, "type") == "events" && folder != "")
				ReadEventFile("$mission:" + folder + "/" + Attr(line, "name"));
		}
		CloseFile(fh);
	}

	protected static void ReadEventFile(string path)
	{
		FileHandle fh = OpenFile(path, FileMode.READ);
		if (!fh)
		{
			DL_Log.Warn("vehicle economy: cannot read " + path);
			return;
		}
		string line;
		string ev = "";
		int nominal = 0;
		ref array<string> children = new array<string>;
		while (FGets(fh, line) >= 0)
		{
			if (line.Contains("<event ") && line.Contains("name="))
			{
				ev = Attr(line, "name");
				nominal = 0;
				children.Clear();
			}
			else if (ev != "" && line.Contains("<nominal>"))
			{
				string v = line;
				v.Replace("<nominal>", "");
				v.Replace("</nominal>", "");
				v = v.Trim();
				nominal = v.ToInt();
			}
			else if (ev != "" && line.Contains("<child ") && line.Contains("type="))
				children.Insert(Attr(line, "type"));
			else if (ev != "" && line.Contains("</event>"))
			{
				// only events that make vehicles matter here; a class in two events keeps the first one
				bool vehicles = false;
				foreach (string c : children)
				{
					if (g_Game.IsKindOf(c, "Transport"))
						vehicles = true;
				}
				if (vehicles)
				{
					s_Nominal.Set(ev, nominal);
					foreach (string t : children)
					{
						if (!s_EventOf.Contains(t))
							s_EventOf.Set(t, ev);
					}
				}
				ev = "";
			}
		}
		CloseFile(fh);
	}

	// value of name="..." in an XML line
	protected static string Attr(string line, string name)
	{
		int at = line.IndexOf(name + "=\"");
		if (at < 0)
			return "";
		string rest = line.Substring(at + name.Length() + 2, line.Length() - at - name.Length() - 2);
		int end = rest.IndexOf("\"");
		if (end < 0)
			return "";
		return rest.Substring(0, end);
	}

	// ---------------- tags ----------------

	protected static void Load()
	{
		if (s_File)
			return;
		string err;
		if (FileExist(FILE))
			JsonFileLoader<DL_CarLedgerFile>.LoadFile(FILE, s_File, err);
		if (!s_File)
			s_File = new DL_CarLedgerFile();
		if (!s_File.tags)
			s_File.tags = new array<ref DL_CarTag>;
		if (!s_File.left)
			s_File.left = new array<ref DL_CarTag>;
		if (!s_File.away)
			s_File.away = new array<ref DL_CarAway>;
		s_Away.Clear();
		foreach (DL_CarAway a : s_File.away)
			s_Away.Set(a.type, a.n);
	}

	protected static void Save()
	{
		string err;
		MakeDirectory(DL_Const.DIR);
		if (!JsonFileLoader<DL_CarLedgerFile>.SaveFile(FILE, s_File, err))
			DL_Log.Warn("WARNING could not write " + FILE + ": " + err);
	}

	static string PidOf(EntityAI e)
	{
		int b1;
		int b2;
		int b3;
		int b4;
		e.GetPersistentID(b1, b2, b3, b4);
		if (b1 == 0 && b2 == 0 && b3 == 0 && b4 == 0)
			return "";
		return b1.ToString() + "-" + b2.ToString() + "-" + b3.ToString() + "-" + b4.ToString();
	}

	// the car's tag in the file (null = never travelled)
	protected static DL_CarTag TagOf(Transport car)
	{
		if (!s_File || !car)
			return null;
		string dlid = DlidOf(car);
		if (dlid == "")
			return null;
		foreach (DL_CarTag t : s_File.tags)
		{
			if (t.dlid == dlid)
				return t;
		}
		return null;
	}

	// Transport.AfterStoreLoad: give a travelled car its tag back
	static void Loaded(Transport car)
	{
		if (!On() || !s_File)
			return;
		string pid = PidOf(car);
		if (pid == "")
			return;
		foreach (DL_CarTag t : s_File.tags)
		{
			if (t.pid == pid)
			{
				SetTag(car, t.dlid, t.home);
				return;
			}
		}
	}

	// DL_CarSnapshot.Capture: the car is about to travel. A car that never travelled gets its id and home now.
	static void Stamp(Transport car, DL_CarData d)
	{
		if (!On() || s_ServerId == "")
			return;
		if (DlidOf(car) == "")
		{
			// seeded once in Start; the counter keeps ids made in the same moment apart
			s_Made++;
			string dlid = s_ServerId + "-" + Math.RandomInt(0, int.MAX).ToString() + "-" + s_Made.ToString();
			SetTag(car, dlid, s_ServerId);
		}
		d.dlid = DlidOf(car);
		d.home = HomeOf(car);
	}

	// DL_CarSnapshot.Build: a car that arrived keeps its id and home
	static void Arrived(Transport car, DL_CarData d)
	{
		if (!car || !d || d.dlid == "")
			return;
		SetTag(car, d.dlid, d.home);
		if (!On())
			return;
		DL_CarTag t = TagOf(car);
		if (!t)
		{
			t = new DL_CarTag();
			s_File.tags.Insert(t);
		}
		t.dlid = d.dlid;
		t.home = d.home;
		t.type = car.GetType();
		t.pid = PidOf(car);
		Save();
		DL_Log.Info("vehicle economy: " + t.type + " " + t.dlid + " arrived (home " + t.home + ")");
	}

	// the hub took the trip and the car is about to be deleted here
	static void Left(Transport car)
	{
		if (!On() || !s_File || DlidOf(car) == "")
			return;
		DL_CarTag t = TagOf(car);
		if (t)
			s_File.tags.RemoveItem(t);
		DL_CarTag l = new DL_CarTag();
		l.dlid = DlidOf(car);
		l.home = HomeOf(car);
		l.type = car.GetType();
		s_File.left.Insert(l);
		if (l.home == s_ServerId)
			s_Away.Set(l.type, s_Away.Get(l.type) + 1); // counted at once, before the hub answers
		SetLeaving(car);
		Save();
		DL_Log.Info("vehicle economy: " + l.type + " " + l.dlid + " left (home " + l.home + ")");
	}

	// ---------------- live cars + the veto ----------------

	static string DlidOf(Transport car)
	{
		DL_CarState st = s_State.Get(car);
		if (!st)
			return "";
		return st.dlid;
	}

	static string HomeOf(Transport car)
	{
		DL_CarState st = s_State.Get(car);
		if (!st)
			return "";
		return st.home;
	}

	static bool IsLeaving(Transport car)
	{
		DL_CarState st = s_State.Get(car);
		return st && st.leaving;
	}

	protected static DL_CarState StateOf(Transport car)
	{
		DL_CarState st = s_State.Get(car);
		if (!st)
		{
			st = new DL_CarState();
			s_State.Set(car, st);
		}
		return st;
	}

	static void SetTag(Transport car, string dlid, string home)
	{
		DL_CarState st = StateOf(car);
		st.dlid = dlid;
		st.home = home;
	}

	static void SetLeaving(Transport car)
	{
		StateOf(car).leaving = true;
	}

	// CE made this car: veto it when it would replace one of ours that is away
	static void CEMade(Transport car)
	{
		if (Veto(car))
		{
			SetLeaving(car); // no longer one of ours
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DeleteLater, 100, false, car);
		}
	}

	static void DeleteLater(Transport car)
	{
		if (car)
			g_Game.ObjectDelete(car);
	}

	static void Born(Transport car)
	{
		if (s_Live.Find(car) < 0)
			s_Live.Insert(car);
	}

	static void Gone(Transport car)
	{
		s_Live.RemoveItem(car);
		s_State.Remove(car);
	}

	// Transport.EEOnCECreate: CE made this car (EEInit already ran). Returns true when it must go.
	static bool Veto(Transport car)
	{
		if (!On() || s_ServerId == "")
			return false;
		string type = car.GetType();
		if (!s_EventOf.Contains(type))
			return false;
		string ev = s_EventOf.Get(type);
		int nominal = s_Nominal.Get(ev);
		int own = 0;
		int ownTravelled = 0;
		foreach (Transport c : s_Live)
		{
			if (!c || c == car || IsLeaving(c) || s_EventOf.Get(c.GetType()) != ev)
				continue;
			string home = HomeOf(c);
			if (home == "" || home == s_ServerId)
				own++;
			if (home == s_ServerId)
				ownTravelled++; // came home by script: CE does not count it until the next restart
		}
		int away = 0;
		for (int i = 0; i < s_Away.Count(); i++)
		{
			if (s_EventOf.Get(s_Away.GetKey(i)) == ev)
				away += s_Away.GetElement(i);
		}
		if (own + away < nominal)
			return false;
		if (away == 0 && ownTravelled == 0)
			return false; // no travelled car of ours in this event: CE's own rules decide, not DriveLink
		s_Vetoes++;
		string what = "vetoed";
		if (s_Mode == "track")
			what = "would veto (track mode)";
		string dlMsg437 = "vehicle economy: ";
		dlMsg437 += what + " CE " + type;
		dlMsg437 += " at " + car.GetPosition().ToString() + " - ";
		dlMsg437 += ev + ": " + own.ToString();
		dlMsg437 += " here + " + away.ToString() + " away >= nominal ";
		dlMsg437 += nominal.ToString() + " (" + s_Vetoes.ToString();
		dlMsg437 += " so far)";
		DL_Log.Info(dlMsg437);
		return s_Mode == "veto";
	}

	static string EventOf(string type)
	{
		return s_EventOf.Get(type);
	}

	// one line: the event's cars here (own never travelled / own travelled / from other servers), away, nominal
	static string Status(string ev)
	{
		int untagged;
		int ownTravelled;
		int foreign;
		foreach (Transport c : s_Live)
		{
			if (!c || IsLeaving(c) || s_EventOf.Get(c.GetType()) != ev)
				continue;
			string home = HomeOf(c);
			if (home == "")
				untagged++;
			else if (home == s_ServerId)
				ownTravelled++;
			else
				foreign++;
		}
		int away;
		for (int i = 0; i < s_Away.Count(); i++)
		{
			if (s_EventOf.Get(s_Away.GetKey(i)) == ev)
				away += s_Away.GetElement(i);
		}
		string dlMsg470 = ev;
		dlMsg470 += ": here " + (untagged + ownTravelled + foreign).ToString() + " (own ";
		dlMsg470 += untagged.ToString() + " + own back home " + ownTravelled.ToString();
		dlMsg470 += " + from other servers " + foreign.ToString() + "), own away ";
		dlMsg470 += away.ToString() + ", nominal " + s_Nominal.Get(ev).ToString();
		dlMsg470 += ", vetoes " + s_Vetoes.ToString();
		return dlMsg470;
	}

	static array<Transport> Live()
	{
		return s_Live;
	}

	// ---------------- the report to the hub ----------------

	static string ReportJson(string key)
	{
		if (!On() || !s_File || s_ServerId == "")
			return "";
		DL_CarsReq q = new DL_CarsReq();
		q.serverId = s_ServerId;
		q.key = key;
		q.left = new array<ref DL_CarTag>;
		q.here = new array<ref DL_CarTag>;
		foreach (DL_CarTag l : s_File.left)
			q.left.Insert(l);
		s_Sent = s_File.left.Count();
		// every tagged car here; pids are filled in once the game has saved a car
		ref array<ref DL_CarTag> keep = new array<ref DL_CarTag>;
		foreach (Transport c : s_Live)
		{
			if (!c || IsLeaving(c) || DlidOf(c) == "")
				continue;
			DL_CarTag t = TagOf(c);
			if (!t)
			{
				t = new DL_CarTag();
				t.dlid = DlidOf(c);
				t.home = HomeOf(c);
			}
			t.type = c.GetType();
			string pid = PidOf(c);
			if (pid != "")
				t.pid = pid;
			keep.Insert(t);
			q.here.Insert(t);
		}
		s_File.tags = keep;
		Save();
		string json;
		string err;
		if (!JsonFileLoader<DL_CarsReq>.MakeData(q, json, err, false))
			DL_Log.Warn("vehicle economy: report json: " + err);
		return json;
	}

	static void OnReport(bool ok, string body)
	{
		if (!ok)
			return; // hub down: keep the left list and the last away counts
		DL_CarsResp r;
		string err;
		JsonSerializer js = new JsonSerializer();
		if (!js.ReadFromString(r, body, err) || !r || !r.ok)
		{
			DL_Log.Warn("vehicle economy: unreadable hub answer (" + err + ")");
			return;
		}
		for (int i = 0; i < s_Sent && s_File.left.Count() > 0; i++)
			s_File.left.RemoveOrdered(0);
		s_Sent = 0;
		s_Away.Clear();
		s_File.away = new array<ref DL_CarAway>;
		if (r.away)
		{
			foreach (DL_CarAway a : r.away)
			{
				s_Away.Set(a.type, a.n);
				s_File.away.Insert(a);
			}
		}
		Save();
		DL_Log.Detail("vehicle economy: hub took the report, " + s_Away.Count().ToString() + " classes with cars away");
	}
}

class DL_CarReportWaiter : Managed
{
	void OnAnswer(bool ok, string body) { DL_CarLedger.OnReport(ok, body); }
}

// Hooks. Transport itself can't be modded (transport.c declares it under #ifdef), so each vehicle script class gets
// them. Their vanilla EEOnCECreate (random fuel) does not call super either.
modded class CarScript
{
	override void EEInit()
	{
		super.EEInit();
		if (g_Game.IsDedicatedServer())
			DL_CarLedger.Born(this);
	}

	override void EEDelete(EntityAI parent)
	{
		if (g_Game.IsDedicatedServer())
			DL_CarLedger.Gone(this);
		super.EEDelete(parent);
	}

	override void EEOnCECreate()
	{
		super.EEOnCECreate();
		if (g_Game.IsDedicatedServer())
			DL_CarLedger.CEMade(this);
	}

	override void AfterStoreLoad()
	{
		super.AfterStoreLoad();
		if (g_Game.IsDedicatedServer())
			DL_CarLedger.Loaded(this);
	}
}

modded class BoatScript
{
	override void EEInit()
	{
		super.EEInit();
		if (g_Game.IsDedicatedServer())
			DL_CarLedger.Born(this);
	}

	override void EEDelete(EntityAI parent)
	{
		if (g_Game.IsDedicatedServer())
			DL_CarLedger.Gone(this);
		super.EEDelete(parent);
	}

	override void EEOnCECreate()
	{
		super.EEOnCECreate();
		if (g_Game.IsDedicatedServer())
			DL_CarLedger.CEMade(this);
	}

	override void AfterStoreLoad()
	{
		super.AfterStoreLoad();
		if (g_Game.IsDedicatedServer())
			DL_CarLedger.Loaded(this);
	}
}

modded class MotorbikeScript
{
	override void EEInit()
	{
		super.EEInit();
		if (g_Game.IsDedicatedServer())
			DL_CarLedger.Born(this);
	}

	override void EEDelete(EntityAI parent)
	{
		if (g_Game.IsDedicatedServer())
			DL_CarLedger.Gone(this);
		super.EEDelete(parent);
	}

	override void EEOnCECreate()
	{
		super.EEOnCECreate();
		if (g_Game.IsDedicatedServer())
			DL_CarLedger.CEMade(this);
	}

	override void AfterStoreLoad()
	{
		super.AfterStoreLoad();
		if (g_Game.IsDedicatedServer())
			DL_CarLedger.Loaded(this);
	}
}
