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
	string hand;                                  // weapon in the hands (DL_FilmCatalog.WEAPON) or ""
	string back;                                  // weapon slung on the back or ""
}

// One thing that happened on the road, in order.
//   drive km | walk km | flat (detail spare|kit|none, who) | fuel (detail found|lost, who) | attack (who, detail
//   hurt|died) | ration | died (who, hunger)
class DL_FilmEvent
{
	string kind;
	float km;                                     // drive/walk: length; others: where it happened
	string who;
	string detail;
}

class DL_Film
{
	static const float MAX_SECONDS = 45;
	static const float END_SECONDS = 4;
	string car;                                   // "uaz" | "volga" | "" (on foot)
	float km;
	string biomeFrom;                             // forest | field | desert | snow
	string biomeTo;
	ref array<ref DL_FilmActor> crew = new array<ref DL_FilmActor>;
	ref array<ref DL_FilmEvent> events = new array<ref DL_FilmEvent>;

	void Add(string kind, float km, string who = "", string detail = "")
	{
		DL_FilmEvent e = new DL_FilmEvent();
		e.kind = kind;
		e.km = km;
		e.who = who;
		e.detail = detail;
		// consecutive drive (or walk) legs become one stretch
		if ((kind == "drive" || kind == "walk") && events.Count() > 0 && events[events.Count() - 1].kind == kind)
		{
			events[events.Count() - 1].km = events[events.Count() - 1].km + km;
			return;
		}
		events.Insert(e);
	}

	// how long each part plays, seconds - the server and the client use the same numbers
	static float SceneSeconds(DL_FilmEvent e)
	{
		if (e.kind == "drive" || e.kind == "walk")
			return Math.Clamp(e.km * 0.15, 3, 8);
		if (e.kind == "flat")
			return 10;
		if (e.kind == "fuel")
			return 11;
		if (e.kind == "attack")
			return 8;
		return 3;                                 // ration, died
	}

	float Duration()
	{
		float d = 2 + END_SECONDS;                // fade in + the crew walking off at the end
		foreach (DL_FilmEvent e : events)
			d += SceneSeconds(e);
		return Math.Min(d, MAX_SECONDS);
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
