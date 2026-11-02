// DriveLink road film - what a trip's film shows (Research/road-animation-research.md). Built by the source server
// (DL_Journey / DL_FilmCast), carried in the traveller's snapshot through the hub, completed by the target (biomeTo)
// and played by the client on the arrival black screen (DL_FilmPlayer). Plain data, JSON-serialised.

// One survivor in the film: a base skin + gear layers, in draw order, from gui/anim/chr (DL_FilmCatalog).
class DL_FilmActor
{
	string name;
	string skin;                                  // DL_FilmCatalog.SKIN
	ref array<string> layers = new array<string>; // legs, body, vest, back, head, mask (any may be missing);
	                                              // "layer@colour" = the grey copy tinted (DL_FilmCast.COLOURS)
	int car = -1;                                 // which of the film's cars he rides in at the start (-1 = on foot)
	string hand;                                  // weapon in the hands (DL_FilmCatalog.WEAPON) or ""
	string back;                                  // weapon slung on the back or ""
}

// One thing that happened on the road, in order.
//   drive km | walk km | flat (detail spare|kit|none, who) | fuel (detail found|lost|shared, who) | attack (who,
//   detail hurt|died) | ration | died (who, hunger) | move (who changes car: car = the one he gets into, -1 = walks on)
// car = the car the event is about (convoys; -1 = the only car / none), donor = the car a spare or fuel came from
class DL_FilmEvent
{
	string kind;
	float km;                                     // drive/walk: length; others: where it happened
	string who;
	string detail;
	int car = -1;
	int donor = -1;
	int times = 1;                                // find: how many of them this one scene stands for (Condense)
}

class DL_Film
{
	static const float MAX_SECONDS = 45;
	static const float END_SECONDS = 4;
	string car;                                   // one car: its look (DL_FilmCast.CarSprite), "" = on foot
	ref array<string> cars = new array<string>;   // a convoy: every car's look, front to back (crew: DL_FilmActor.car)
	float km;
	string biomeFrom;                             // forest | field | desert | snow
	string biomeTo;
	string biomeVia;                              // the look on the way (the zone's), "" = straight from -> to
	string skip;                                  // "vote" = everyone watching presses Space, "none", "" = Space skips (F9)
	ref array<ref DL_FilmActor> crew = new array<ref DL_FilmActor>;
	ref array<ref DL_FilmEvent> events = new array<ref DL_FilmEvent>;

	void Add(string kind, float km, string who = "", string detail = "", int car = -1, int donor = -1)
	{
		DL_FilmEvent e = new DL_FilmEvent();
		e.kind = kind;
		e.km = km;
		e.who = who;
		e.detail = detail;
		e.car = car;
		e.donor = donor;
		// consecutive drive (or walk) legs become one stretch
		if ((kind == "drive" || kind == "walk" || kind == "swim") && events.Count() > 0 && events[events.Count() - 1].kind == kind)
		{
			events[events.Count() - 1].km = events[events.Count() - 1].km + km;
			return;
		}
		events.Insert(e);
	}

	// how long each part plays, seconds - the server and the client use the same numbers
	static float SceneSeconds(DL_FilmEvent e)
	{
		if (e.kind == "drive" || e.kind == "walk" || e.kind == "swim")
			return Math.Clamp(e.km * 0.15, 3, 8);
		if (e.kind == "flat")
			return 10;
		if (e.kind == "fuel")
			return 11;
		if (e.kind == "attack")
			return 8;
		if (e.kind == "move")
			return 4;
		if (e.kind == "note")
			return 5;                                 // the trail: a choice's outcome, read while they roll on
		if (e.kind == "act")
			return 9;                                 // the trail: the outcome played out (DL_FilmPlayer.SceneAct)
		if (e.kind == "card")
			return 0;                                 // the trail: stands while the party votes
		if (e.kind == "find" && e.detail == "gas")
		{
			// it comes up the road, a can from the pumps into every car (donor = how many cars), back in
			int fillN = Math.Max(1, e.donor);
			return 13 + 3.5 * (fillN - 1);
		}
		if (e.kind == "find" && e.detail == "water")
			return 15;                             // the pump comes up the road, they stop, one walks over and back
		if (e.kind == "find")
			return 14;                            // food: the same, to a wreck, crate, bush or tree
		return 3;                                 // ration, died
	}

	float Duration()
	{
		return Math.Min(Length(), MAX_SECONDS);
	}

	// every scene played in full, seconds
	float Length()
	{
		float d = 2 + END_SECONDS;                // fade in + the crew walking off at the end
		foreach (DL_FilmEvent e : events)
			d += SceneSeconds(e);
		return d;
	}

	// A long road finds the same things over and over. Between two events that matter (a flat, fuel, an attack, a
	// death, someone changing car, rationing) the finds of each kind become one scene that stands for all of them
	// (times). Still longer than MAX_SECONDS: the finds that stand for the fewest go, later ones first, but one of
	// each kind (gas, water, food) stays with the count of all of them; the events that matter always stay. The km of the road stay the same (the legs around a removed find join up).
	void Condense(float maxSeconds = MAX_SECONDS)
	{
		array<ref DL_FilmEvent> kept = new array<ref DL_FilmEvent>;
		map<string, ref DL_FilmEvent> stretch = new map<string, ref DL_FilmEvent>;  // this stretch's find of each kind
		foreach (DL_FilmEvent e : events)
		{
			if (e.kind == "find")
			{
				DL_FilmEvent had;
				if (stretch.Find(e.detail, had))
				{
					had.times = had.times + e.times;
					had.donor = Math.Max(had.donor, e.donor);   // gas: the most cars filled at one
					continue;
				}
				stretch.Set(e.detail, e);
			}
			else if (!IsLeg(e))
				stretch.Clear();
			Keep(kept, e);
		}
		while (true)
		{
			float len = 2 + END_SECONDS;
			map<string, int> kinds = new map<string, int>;   // finds left of each kind: the last one of a kind stays
			foreach (DL_FilmEvent c : kept)
			{
				if (c.kind == "find")
					kinds.Set(c.detail, kinds.Get(c.detail) + 1);
			}
			int drop = -1;
			for (int i = 0; i < kept.Count(); i++)
			{
				len += SceneSeconds(kept[i]);
				if (kept[i].kind == "find" && kinds.Get(kept[i].detail) > 1 && (drop < 0 || kept[i].times <= kept[drop].times))
					drop = i;
			}
			if (len <= maxSeconds || drop < 0)
				break;
			DL_FilmEvent gone = kept[drop];
			kept.RemoveOrdered(drop);
			DL_FilmEvent heir;                            // the biggest one left of its kind counts it in
			foreach (DL_FilmEvent h : kept)
			{
				if (h.kind == "find" && h.detail == gone.detail && (!heir || h.times > heir.times))
					heir = h;
			}
			if (heir)
			{
				heir.times = heir.times + gone.times;
				heir.donor = Math.Max(heir.donor, gone.donor);
			}
			array<ref DL_FilmEvent> joined = new array<ref DL_FilmEvent>;
			foreach (DL_FilmEvent k : kept)
				Keep(joined, k);
			kept = joined;
		}
		events = kept;
	}

	protected static bool IsLeg(DL_FilmEvent e)
	{
		return e.kind == "drive" || e.kind == "walk" || e.kind == "swim";
	}

	// appended, a leg joining the one before it when they are the same kind (as Add does)
	protected static void Keep(array<ref DL_FilmEvent> list, DL_FilmEvent e)
	{
		if (IsLeg(e) && list.Count() > 0 && list[list.Count() - 1].kind == e.kind)
		{
			list[list.Count() - 1].km = list[list.Count() - 1].km + e.km;
			return;
		}
		list.Insert(e);
	}

	static string BiomeOfWorld(string world)
	{
		string w = world;
		w.ToLower();
		if (w.Contains("sakhal") || w.Contains("namalsk") || w.Contains("winter") || w.Contains("snow"))
			return "snow";
		if (w.Contains("takistan") || w.Contains("desert") || w.Contains("zagoria") || w.Contains("arid"))
			return "desert";
		if (w.Contains("banov") || w.Contains("field") || w.Contains("steppe"))
			return "field";
		return "forest";                          // chernarusplus, enoch (Livonia), deerisle...
	}
}
