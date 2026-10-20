// DriveLink road film - the player (client). Plays a DL_Film (DL_FilmData.c) top-down like MiniDayZ: the ground of
// the source biome scrolls up and blends into the target's, roadside props and wrecks pass by, the car drives down
// the middle of the road; every road event is a scene (flat tyre, fuel, walking, infected, rationing, death), and at
// the end the road stops and the crew walks off the bottom of the window.
// Engine: code-made widgets (spike results in Research/road-animation-research.md section 7): sprite frames by
// CLIPPING (DL_Sprite), scrolling by sliding tiles (DL_Strip), everything moved from MissionGameplay.OnUpdate.
// Art: ground + props = DriveLink's own; cars, wrecks, survivors, gear, infected = MiniDayZ (prototype only).

// a survivor (or an infected): one sprite per layer, all on the same 32x32 cell (rows: Tools/build-anim-characters.py)
class DL_FilmPerson
{
	static const string CHR = "DriveLink/gui/anim/chr/chr_";
	static const int NORMAL = 0;
	static const int AIM = 1;      // firearm raised (shooting)
	static const int SWING = 2;    // melee swing, or a punch
	static const int USE = 3;      // crouched, working (a wheel, a fuel can)
	string name;
	ref array<ref DL_Sprite> layers = new array<ref DL_Sprite>;
	ref DL_Sprite hand;            // weapon in the hands
	ref DL_Sprite back;            // weapon slung on the back
	bool handMelee;
	bool backMelee;
	float x;                       // film pixels (ground tile pixels, 0..256 across the road)
	float y;
	int dir;                       // 0 down, 1 up, 2 right, 3 left
	int pose;
	bool marching;                 // walking on the spot while the ground scrolls
	bool moving;                   // walking its path
	bool attacking;                // infected only
	bool down;                     // dead / killed: plays its dying frames once, then lies there
	float downT;
	bool shown;
	bool zed;
	float speed = 60;              // film px/s along the path
	int car = -1;                  // the film car he rides in (DL_FilmCar.idx), -1 = on foot
	float homeX;                   // its car door
	float homeY;
	float animT;
	bool rideShown;                // drawn in a seat of an open vehicle (bike, boat) while "in" it
	ref array<vector> path = new array<vector>;

	void DL_FilmPerson(Widget parent, int sort, float px, DL_FilmActor a, string zedSkin)
	{
		float size = 32 * px;
		if (zedSkin != "")
		{
			zed = true;
			speed = 42;
			layers.Insert(new DL_Sprite(parent, CHR + zedSkin + "_ca.paa", sort, size, size, 128, 512));
			return;
		}
		name = a.name;
		layers.Insert(new DL_Sprite(parent, CHR + a.skin + "_ca.paa", sort, size, size, 128, 1024));
		foreach (string l : a.layers)
		{
			string tex = l;
			int tint = ARGB(255, 255, 255, 255);
			int at = l.IndexOf("@");
			if (at > 0)
			{
				tex = l.Substring(0, at) + "w";
				tint = Tint(l.Substring(at + 1, l.Length() - at - 1));
			}
			DL_Sprite s = new DL_Sprite(parent, CHR + tex + "_ca.paa", sort, size, size, 128, 1024);
			s.SetColor(tint);
			layers.Insert(s);
		}
		if (a.back != "")
		{
			back = new DL_Sprite(parent, CHR + a.back + "_ca.paa", sort, size, size, 128, 1024);
			backMelee = IsMelee(a.back);
		}
		if (a.hand != "")
		{
			hand = new DL_Sprite(parent, CHR + a.hand + "_ca.paa", sort, size, size, 128, 1024);
			handMelee = IsMelee(a.hand);
		}
	}

	static bool IsMelee(string weapon)
	{
		return ("," + DL_FilmCatalog.MELEE + ",").Contains("," + weapon + ",");
	}

	// DL_FilmCast colour words -> the tint multiplied over the grey clothing copy
	static int Tint(string c)
	{
		if (c == "black") return ARGB(255, 72, 72, 78);
		if (c == "white") return ARGB(255, 250, 250, 250);
		if (c == "gray" || c == "grey") return ARGB(255, 165, 165, 168);
		if (c == "red" || c == "wine" || c == "maroon") return ARGB(255, 215, 55, 50);
		if (c == "blue") return ARGB(255, 70, 115, 230);
		if (c == "lightblue") return ARGB(255, 135, 190, 245);
		if (c == "darkblue" || c == "bluedark" || c == "navy") return ARGB(255, 45, 65, 150);
		if (c == "green") return ARGB(255, 85, 165, 75);
		if (c == "darkgreen" || c == "greendark") return ARGB(255, 55, 100, 55);
		if (c == "olive") return ARGB(255, 130, 140, 75);
		if (c == "lime") return ARGB(255, 160, 235, 65);
		if (c == "yellow") return ARGB(255, 250, 215, 60);
		if (c == "orange") return ARGB(255, 250, 140, 40);
		if (c == "pink") return ARGB(255, 250, 140, 195);
		if (c == "purple" || c == "violet") return ARGB(255, 150, 85, 205);
		if (c == "brown") return ARGB(255, 145, 100, 60);
		if (c == "beige" || c == "tan" || c == "khaki") return ARGB(255, 220, 195, 145);
		return ARGB(255, 255, 255, 255);
	}

	void Show(bool on)
	{
		shown = on;
		foreach (DL_Sprite s : layers)
			s.Show(on);
		if (hand)
			hand.Show(on);
		if (back)
			back.Show(on);
	}

	// the back weapon into the hands (and the hand one, if any, onto the back)
	void Draw()
	{
		DL_Sprite t = hand;
		hand = back;
		back = t;
		bool m = handMelee;
		handMelee = backMelee;
		backMelee = m;
	}

	bool Shoots()
	{
		return hand && !handMelee;
	}

	void Go(float tx, float ty)
	{
		path.Insert(Vector(tx, ty, 0));
	}

	bool Busy()
	{
		return path.Count() > 0;
	}

	void Face(float dx, float dy)
	{
		if (Math.AbsFloat(dx) > Math.AbsFloat(dy))
		{
			if (dx > 0)
				dir = 2;
			else
				dir = 3;
		}
		else if (dy > 0)
			dir = 0;
		else
			dir = 1;
	}

	void FaceTo(DL_FilmPerson o)
	{
		if (o)
			Face(o.x - x, o.y - y);
	}

	// along the path
	void Step(float dt)
	{
		moving = false;
		if (path.Count() == 0 || down)
			return;
		vector t = path[0];
		float dx = t[0] - x;
		float dy = t[1] - y;
		float d = Math.Sqrt(dx * dx + dy * dy);
		float st = speed * dt;
		if (d > 0.01)
			Face(dx, dy);
		if (d <= st)
		{
			x = t[0];
			y = t[1];
			path.RemoveOrdered(0);
			moving = path.Count() > 0;
			return;
		}
		x += dx / d * st;
		y += dy / d * st;
		moving = true;
	}

	// in a seat of a motorbike or a boat: drawn there (centre x, y), cut to its top part (a bike rider's legs are
	// behind the tank), not walking
	void RideAt(float px, float ox, float oy, float rx, float ry, int row, int frame, float cut, int backRow)
	{
		rideShown = true;
		float sx = ox + (rx - 16) * px;
		float sy = oy + (ry - 16) * px;
		foreach (DL_Sprite s : layers)
			Seat(s, frame, row, cut, sx, sy);
		if (back)
			Seat(back, 0, backRow, cut, sx, sy);
		if (hand)
			Seat(hand, frame, row, cut, sx, sy);
	}

	protected void Seat(DL_Sprite s, int frame, int row, float cut, float sx, float sy)
	{
		s.Show(true);
		s.Cut(cut);
		s.Cell(frame * 32, row * 32, 32, 32);
		s.MoveTo(sx, sy);
	}

	void HideRide()
	{
		rideShown = false;
		foreach (DL_Sprite s : layers)
		{
			s.Cut(1);
			s.Show(false);
		}
		if (back)
		{
			back.Cut(1);
			back.Show(false);
		}
		if (hand)
		{
			hand.Cut(1);
			hand.Show(false);
		}
	}

	void Update(float dt, float px, float ox, float oy)
	{
		if (!shown)
			return;
		if (rideShown)
		{
			// out of the seat: whole again
			rideShown = false;
			foreach (DL_Sprite rs : layers)
				rs.Cut(1);
			if (back)
				back.Cut(1);
			if (hand)
				hand.Cut(1);
		}
		animT += dt;
		bool run = moving || marching;
		int row = dir;
		int frame = 0;
		int backRow = 17 + dir;
		if (zed)
		{
			if (attacking)
			{
				row = 8 + dir;
				frame = Math.Floor(animT * 8);
				frame = frame % 4;
			}
			else if (run)
			{
				row = 4 + dir;
				frame = Math.Floor(animT * 8);
				frame = frame % 4;
			}
		}
		else if (pose == AIM)
		{
			row = 8 + dir;
			backRow = 25 + dir;
		}
		else if (pose == SWING)
		{
			row = 12 + dir;
			frame = Math.Floor(animT * 9);
			frame = frame % 3;
			backRow = 25 + dir;
		}
		else if (pose == USE)
		{
			row = 16;
			frame = Math.Floor(animT * 6);
			frame = frame % 4;
		}
		else if (run)
		{
			row = 4 + dir;
			frame = Math.Floor(animT * 10);
			frame = frame % 4;
			backRow = 21 + dir;
		}
		float sx = ox + (x - 16) * px;
		float sy = oy + (y - 16) * px;
		if (down)
		{
			// the dying frames (skin rows 17-18, infected 12-13), then the last one; MiniDayZ has them for the bare
			// skin only, so the gear and weapons drop away (as in MiniDayZ)
			downT += dt;
			int i = Math.Floor(downT * 9);
			if (i > 7)
				i = 7;
			int first = 17;
			if (zed)
				first = 12;
			Place(layers[0], i % 4, first + i / 4, sx, sy);
			for (int k = 1; k < layers.Count(); k++)
				layers[k].Show(false);
			if (hand)
				hand.Show(false);
			if (back)
				back.Show(false);
			return;
		}
		foreach (DL_Sprite s : layers)
			Place(s, frame, row, sx, sy);
		if (back)
			Place(back, frame, backRow, sx, sy);
		if (hand)
			Place(hand, frame, row, sx, sy);
	}

	protected void Place(DL_Sprite s, int frame, int row, float sx, float sy)
	{
		s.Cell(frame * 32, row * 32, 32, 32);
		s.MoveTo(sx, sy);
	}
}

// a roadside thing (prop from the atlas, or a wreck) that scrolls with the ground
class DL_FilmThing
{
	ref DL_Sprite sprite;
	float x;
	float y;                  // film pixels, centre
	float w;
	float h;
	bool used;

	// cw x ch = the cell in the texture; scale = how big it shows (1 = one texel per film pixel)
	void DL_FilmThing(Widget parent, int sort, string tex, float px, float cw, float ch, float texW, float texH, float scale = 1)
	{
		w = cw * scale;
		h = ch * scale;
		sprite = new DL_Sprite(parent, tex, sort, w * px, h * px, texW, texH);
		sprite.Show(false);
	}

	void Place(float px, float ox, float oy)
	{
		sprite.MoveTo(ox + (x - w / 2) * px, oy + (y - h / 2) * px);
	}
}

// one car of the film; a convoy has up to 4, front of the line first
class DL_FilmCar
{
	int idx;
	ref DL_Sprite sprite;
	bool truck;               // the M3S: 2 seats (cab doors), 6 wheels
	string kind = "car";      // "car" | "bike" (2 wheels on the middle line) | "boat" (on the water: nobody gets out)
	ref DL_Sprite over;       // open vehicles: drawn over the riders (a bike's bars + front, a boat's motor)
	ref DL_Sprite wake;       // a boat's wake, under it
	string look;              // the sprite: "bike" (dirt bike) | "moped" | "boat" | ...
	float drawX;              // where it is drawn this frame (with its bob), film px
	float w;                  // shown size, film px (with its shadow)
	float h;
	float x;                  // centre, film px
	float y;
	float slotY;              // where it belongs in the line (it closes up when a car is left behind)
	bool inWorld;             // left behind: it scrolls away with the ground
	bool driveOff;            // the end: it drives out of the bottom
	float v;

	// half the body (without its shadow strip on the right)
	float HalfW()
	{
		return w / 2 - 3;
	}

	float HalfH()
	{
		return h / 2 - 2;
	}

	float Top()
	{
		return y - h / 2;
	}
}

class DL_FilmPlayer
{
	static const string DIR = "DriveLink/gui/anim/";
	// props atlas order = Tools/build-anim-assets.py PROPS
	static const int P_PINE = 0;
	static const int P_BROADLEAF = 1;
	static const int P_BUSH = 2;
	static const int P_ROCK = 3;
	static const int P_FENCE = 4;
	static const int P_SIGN = 5;
	static const int P_POLE = 6;
	static const int P_BARREL = 7;
	static const int P_HAY = 8;
	static const int P_DRYBUSH = 9;
	static const int P_SANDROCK = 10;
	static const int P_SNOWPINE = 11;
	static const int P_DRIFT = 12;
	static const int P_TIRES = 13;
	static const int P_WHEEL = 14;
	static const int P_CRATE = 15;
	static const float DRIVE = 150;   // ground speeds, film px/s
	static const float WALK = 28;
	static const float CAR_GAP = 12;  // between cars in a convoy, film px
	static const float SIDE_LANE = 84; // where people walk beside a convoy

	protected static int s_DemoN;

	protected ref DL_Film m_Film;
	protected Widget m_Root;
	protected ref DL_Strip m_GroundA;
	protected ref DL_Strip m_GroundB;
	protected ref DL_Strip m_GroundV;      // the look on the way (zone viaBiome), between from and to
	protected TextWidget m_SkipText;       // "Space: vote to skip" / how many voted
	protected bool m_Voted;
	protected bool m_VoteOut;              // a vote for the HUD to send (DL_ClientHud, RPC_FILM_VOTE)
	protected ref array<ref DL_FilmCar> m_Cars = new array<ref DL_FilmCar>;
	protected DL_FilmCar m_V;          // the car the scene is about (null = on foot)
	protected DL_FilmCar m_Donor;      // convoy: the car a spare wheel or a jerry can comes from
	protected DL_FilmPerson m_Helper;  // convoy: who brings it
	protected bool m_HelperCarry;
	protected bool m_Convoy;           // more than one car: the tall window, cars in a line
	protected float m_LeadY;           // the lead car's place
	protected float m_Bottom;          // the window's bottom edge, fraction of the screen height (text goes below)
	protected bool m_Leaving;          // walking round the car they leave behind
	protected bool m_Water;            // a boat trip: open water instead of the road (until the boats are left)
	protected ref DL_Strip m_GroundLand; // a boat trip: the land they walk on when the boats are left
	protected float m_WalkY;           // the walkers' first row
	protected ref array<ref DL_FilmPerson> m_Crew = new array<ref DL_FilmPerson>;
	protected ref DL_FilmPerson m_Zed;
	protected DL_FilmPerson m_Actor;   // who the scene is about
	protected ref array<ref DL_FilmThing> m_Props = new array<ref DL_FilmThing>;
	protected ref array<ref DL_FilmThing> m_Wrecks = new array<ref DL_FilmThing>;
	protected ref DL_FilmThing m_Tool;    // wheel or repair kit in a scene
	protected ref DL_FilmThing m_Puff;
	protected ref DL_FilmThing m_Fuel;    // MiniDayZ fuel tank, carried back in the fuel scene
	protected ImageWidget m_Flash;        // muzzle flash
	protected TextWidget m_Caption;
	protected float m_Px;         // screen pixels per film pixel
	protected float m_Ox;         // window origin of the film area
	protected float m_Oy;
	protected float m_ViewH;      // film pixels visible top to bottom
	protected float m_T;          // time since start
	protected float m_SceneT;     // time in the current scene
	protected int m_Scene;
	protected int m_Step;         // choreography step inside a scene
	protected float m_StepT;
	protected bool m_Ending;      // after the last scene: the cars drive off, walkers walk off the bottom
	protected int m_WheelSide;    // flat: -1 left / 1 right
	protected float m_WheelY;
	protected int m_ZedSide;
	protected bool m_Drawn;       // the victim took the back weapon into his hands for the fight
	protected bool m_Carrying;    // the fuel seeker carries a can
	protected float m_Speed;      // ground speed now, film px/s (up = the car moves down)
	protected float m_Target;     // speed the scene wants
	protected float m_Scroll;     // total ground scrolled, film px
	protected float m_Km;         // km travelled so far in the film (drives the biome blend)
	protected float m_NextProp;
	protected float m_NextWreck;
	protected bool m_Playing;
	protected bool m_KeyWasDown;
	protected string m_LastJson;

	bool IsPlaying()
	{
		return m_Playing;
	}

	// where the window ends (fraction of the screen height): the black screen's text goes below it
	float BottomFrac()
	{
		return m_Bottom;
	}

	// from RPC_FILM; the same film arriving twice plays once
	void PlayJson(string json)
	{
		if (json == m_LastJson && m_Playing)
			return;
		DL_Film f;
		string err;
		JsonSerializer js = new JsonSerializer();
		if (!js.ReadFromString(f, json, err) || !f)
		{
			Print(DL_Const.LOG_TAG + "road film unreadable: " + err);
			return;
		}
		m_LastJson = json;
		Play(f);
	}

	void Play(DL_Film f)
	{
		Stop();
		if (!f || f.events.Count() == 0)
			return;
		m_Film = f;
		m_Voted = false;
		m_VoteOut = false;
		if (!Build())
			return;
		m_Playing = true;
		m_T = 0;
		m_Scene = -1;
		m_Ending = false;
		NextScene();
		Print(DL_Const.LOG_TAG + "road film: " + f.events.Count().ToString() + " scenes, " + f.crew.Count().ToString() + " in the crew, " + m_Cars.Count().ToString() + " car(s), " + f.biomeFrom + " -> " + f.biomeTo);
	}

	void Stop()
	{
		if (m_Root)
			m_Root.Unlink();
		m_Root = null;
		m_Playing = false;
		m_Crew.Clear();
		m_Props.Clear();
		m_Wrecks.Clear();
		m_Cars.Clear();
		m_GroundLand = null;
		m_V = null;
		m_Donor = null;
		m_Helper = null;
		m_Zed = null;
		m_Actor = null;
		m_Tool = null;
		m_Puff = null;
		m_Fuel = null;
		m_Flash = null;
	}

	// ---------------- build ----------------

	protected static string Ground(string biome)
	{
		if (biome == "field" || biome == "desert" || biome == "snow")
			return DIR + "ground_" + biome + "_ca.paa";
		return DIR + "ground_forest_ca.paa";
	}

	protected bool Build()
	{
		WorkspaceWidget ws = g_Game.GetWorkspace();
		if (!ws)
			return false;
		int sw;
		int sh;
		GetScreenSize(sw, sh);
		array<string> looks = new array<string>;
		foreach (string fl : m_Film.cars)
			looks.Insert(fl);
		if (looks.Count() == 0 && m_Film.car != "")
			looks.Insert(m_Film.car);  // an older film: one car, everyone in it
		m_Convoy = looks.Count() > 1;
		// a convoy gets a tall window (the cars in a line), else the wide one
		float winH = sh * 0.62;
		float winW = Math.Min(sw * 0.62, winH * 1.15);
		float winY = sh * 0.035;
		if (m_Convoy)
		{
			winH = sh * 0.80;
			winW = Math.Min(sw * 0.62, winH * 0.62);
			winY = sh * 0.02;
		}
		float winX = (sw - winW) / 2;
		m_Bottom = (winY + winH) / sh;
		m_Root = ws.CreateWidget(FrameWidgetTypeID, winX, winY, winW, winH, DL_FilmSpike.Flags() | WidgetFlags.CLIPCHILDREN, ARGB(255, 255, 255, 255), 31500);
		if (!m_Root)
			return false;
		DL_FilmSpike.Image(m_Root, DL_FilmSpike.PLAIN, ARGB(255, 16, 16, 16), 0, 0, 0, winW, winH);
		float gw = winW - 6;
		m_Px = gw / 256.0;                    // the 256-px ground tile (2-lane road) fills the width
		m_Ox = 3;
		m_Oy = 3;
		float gh = winH - 6;
		m_ViewH = gh / m_Px;
		m_Scroll = 0;
		m_Km = 0;
		m_Water = false;
		foreach (string wl : looks)
		{
			if (wl.IndexOf("boat") == 0)
				m_Water = true;
		}
		m_GroundLand = null;
		if (m_Water)
		{
			// open water; the biome's land is ready underneath for when the boats are left
			m_GroundLand = new DL_Strip(m_Root, Ground(m_Film.biomeTo), 1, m_Ox, m_Oy, gw, gh, 256, 256, true);
			m_GroundLand.SetAlpha(0);
			m_GroundA = new DL_Strip(m_Root, DIR + "ground_water_ca.paa", 1, m_Ox, m_Oy, gw, gh, 256, 256, true);
		}
		else
			m_GroundA = new DL_Strip(m_Root, Ground(m_Film.biomeFrom), 1, m_Ox, m_Oy, gw, gh, 256, 256, true);
		m_GroundB = null;
		m_GroundV = null;
		if (!m_Water && m_Film.biomeVia != "" && m_Film.biomeVia != m_Film.biomeFrom && m_Film.biomeVia != m_Film.biomeTo)
		{
			m_GroundV = new DL_Strip(m_Root, Ground(m_Film.biomeVia), 2, m_Ox, m_Oy, gw, gh, 256, 256, true);
			m_GroundV.SetAlpha(0);
		}
		if (!m_Water && m_Film.biomeTo != m_Film.biomeFrom && m_Film.biomeTo != "")
		{
			m_GroundB = new DL_Strip(m_Root, Ground(m_Film.biomeTo), 2, m_Ox, m_Oy, gw, gh, 256, 256, true);
			m_GroundB.SetAlpha(0);
		}
		// roadside props and wrecks: a pool that is recycled as they scroll off the top
		for (int i = 0; i < 16; i++)
			m_Props.Insert(new DL_FilmThing(m_Root, 3, DIR + "props_ca.paa", m_Px, 64, 64, 256, 256, 0.7));
		array<string> wrecks = {"wreck_blue_ca", "wreck_gray_ca", "wreck_green_ca", "wreck_red_ca", "wreck_police_ca"};
		foreach (string wr : wrecks)
		{
			DL_FilmThing wt = new DL_FilmThing(m_Root, 4, DIR + wr + ".paa", m_Px, 60, 76, 128, 128);
			wt.sprite.Cell(4, 0, 60, 76);
			m_Wrecks.Insert(wt);
		}
		m_Tool = new DL_FilmThing(m_Root, 9, DIR + "props_ca.paa", m_Px, 64, 64, 256, 256, 0.4);
		m_Puff = new DL_FilmThing(m_Root, 9, DIR + "smoke_ca.paa", m_Px, 14, 14, 16, 16, 1.8);
		m_Puff.sprite.Cell(0, 0, 14, 14);
		m_Fuel = new DL_FilmThing(m_Root, 9, DIR + "fueltank_ca.paa", m_Px, 12, 12, 16, 16, 1.4);
		m_Fuel.sprite.Cell(0, 0, 12, 12);
		m_Flash = DL_FilmSpike.Image(m_Root, DL_FilmSpike.PLAIN, ARGB(255, 255, 235, 130), 9, 0, 0, 3 * m_Px, 3 * m_Px);
		m_Flash.Show(false);
		// the cars, in the middle of the road, one behind the other
		m_Leaving = false;
		m_WalkY = m_ViewH * 0.40;
		m_LeadY = m_ViewH * 0.36;
		if (m_Convoy)
			m_LeadY = m_ViewH * 0.62;
		foreach (string look : looks)
		{
			DL_FilmCar c = BuildCar(look);
			if (c)
			{
				c.idx = m_Cars.Count();
				m_Cars.Insert(c);
			}
		}
		Relayout(true);
		// the crew: in their car (hidden) or walking from the start
		bool legacy = m_Film.cars.Count() == 0 && m_Film.car != "";
		foreach (DL_FilmActor a : m_Film.crew)
		{
			DL_FilmPerson p = new DL_FilmPerson(m_Root, 8, m_Px, a, "");
			p.car = a.car;
			if (legacy)
				p.car = 0;
			if (p.car >= m_Cars.Count())
				p.car = -1;
			p.Show(false);
			m_Crew.Insert(p);
		}
		m_Zed = new DL_FilmPerson(m_Root, 8, m_Px, null, "zednormalskin1");
		m_Zed.Show(false);
		int flags = DL_FilmSpike.Flags() | WidgetFlags.CENTER | WidgetFlags.VCENTER;
		float band = winH * 0.075;
		if (m_Convoy)
			band = winH * 0.058;
		DL_FilmSpike.Image(m_Root, DL_FilmSpike.PLAIN, ARGB(150, 0, 0, 0), 10, 0, winH - band, winW, band);
		m_Caption = TextWidget.Cast(ws.CreateWidget(TextWidgetTypeID, 0, winH - band * 0.93, winW, band * 0.87, flags, ARGB(255, 240, 225, 190), 11, m_Root));
		// how to skip, top of the window: everyone must vote on a trip, Space alone for the F9 demo
		m_SkipText = null;
		if (m_Film.skip != "none")
		{
			float sh2 = winH * 0.045;
			DL_FilmSpike.Image(m_Root, DL_FilmSpike.PLAIN, ARGB(120, 0, 0, 0), 10, 0, 0, winW, sh2 * 1.2);
			m_SkipText = TextWidget.Cast(ws.CreateWidget(TextWidgetTypeID, 0, sh2 * 0.1, winW, sh2, flags, ARGB(255, 200, 200, 200), 11, m_Root));
			if (m_Film.skip == "vote")
				m_SkipText.SetText("Space: vote to skip (everyone has to)");
			else
				m_SkipText.SetText("Space: skip");
		}
		// a few props from the start, so the road isn't empty
		m_NextProp = 0;
		m_NextWreck = 1.5;
		for (int k = 0; k < 8; k++)
		{
			if (!m_Water)
				SpawnProp(Math.RandomFloat(0, m_ViewH));
		}
		m_Speed = 0;
		return true;
	}

	// a car of the film: DL_FilmCast.CarSprite "look@colour" -> that body pre-painted in that colour
	// (Tools/build-anim-assets.py car_sprites); "uaz" / "volga" = the old MiniDayZ sprites as they are
	protected DL_FilmCar BuildCar(string carLook)
	{
		string look = carLook;
		if (look == "")
			return null;
		string colour = "";
		int at = look.IndexOf("@");
		if (at > 0)
		{
			colour = look.Substring(at + 1, look.Length() - at - 1);
			look = look.Substring(0, at);
		}
		DL_FilmCar c = new DL_FilmCar();
		c.x = 128;
		if (look == "volga")
			SetOldCar(c, DIR + "volga_ca.paa", 178, 142, 62, 80);
		else if (look == "uaz")
			SetOldCar(c, DIR + "uaz_ca.paa", 176, 80, 68, 74);
		// pre-painted in every paint the car comes in (Tools/build-anim-assets.py CAR_PAINTS); sizes with the shadow
		else if (look == "sedan")         // Olga 24 (Volga GAZ-24)
			SetCar(c, CarTex("sedan", colour, "white"), 64, 128, 50, 78, 1, 1);
		else if (look == "sarka")         // Sarka 120 (Skoda 120): a bit smaller
			SetCar(c, CarTex("sedan", colour, "yellow"), 64, 128, 50, 78, 0.95, 0.92);
		else if (look == "gunter")        // Gunter 2 (Golf): a short hatchback
			SetCar(c, CarTex("sedan", colour, "red"), 64, 128, 50, 78, 0.95, 0.82);
		else if (look == "humvee")        // M1025: wide and long
			SetCar(c, CarTex("4x4", colour, "olive"), 64, 128, 47, 76, 1.3, 1.06);
		else if (look == "bike")          // Motorbike_02, the dirt bike (Tools/anim_bikes.py)
		{
			c.kind = "bike";
			c.look = "bike";
			SetCar(c, CarTex("bike", colour, "red"), 32, 64, 29, 58, 1, 1);
			SetOver(c, OverTex("bike", "front", colour, "red"), 32, 64, 29, 58);
		}
		else if (look == "moped")         // Motorbike_01, the moped
		{
			c.kind = "bike";
			c.look = "moped";
			SetCar(c, CarTex("moped", colour, "red"), 32, 64, 30, 58, 1, 1);
			SetOver(c, OverTex("moped", "front", colour, "red"), 32, 64, 30, 58);
		}
		else if (look == "boat")          // Boat_01 (placeholder art)
		{
			c.kind = "boat";
			c.look = "boat";
			// its wake first (under it): 4 frames of 96 x 128 (Tools/anim_bikes.py wake_frames)
			c.wake = new DL_Sprite(m_Root, DIR + "wake_ca.paa", 5, 96 * m_Px, 128 * m_Px, 512, 128);
			SetCar(c, CarTex("boat", colour, "blue"), 64, 128, 48, 92, 1, 1);
			SetOver(c, OverTex("boat", "rim", colour, "blue"), 64, 128, 48, 92);
		}
		else if (look == "truck")         // M3S
		{
			c.truck = true;
			SetCar(c, CarTex("truck", colour, "green"), 64, 128, 58, 111, 1, 1);
		}
		else                              // Ada 4x4 (Niva) and anything else
			SetCar(c, CarTex("4x4", colour, "green"), 64, 128, 47, 76, 1, 0.95);
		return c;
	}

	static const string PAINTS_SEDAN = ",white,black,wine,yellow,grey,red,blue,";
	static const string PAINTS_4X4 = ",green,blue,white,olive,beige,grey,";
	static const string PAINTS_TRUCK = ",green,blue,orange,grey,";
	static const string PAINTS_BIKE = ",red,green,blue,yellow,grey,";
	static const string PAINTS_MOPED = ",red,yellow,blue,grey,";
	static const string PAINTS_BOAT = ",blue,black,orange,camo,grey,";

	// the texture of a car body in a paint it comes in (else its default paint)
	protected static string CarTex(string base, string colour, string fallback)
	{
		string c = colour;
		if (c == "gray")
			c = "grey";
		string have = PAINTS_4X4;
		if (base == "sedan")
			have = PAINTS_SEDAN;
		else if (base == "truck")
			have = PAINTS_TRUCK;
		else if (base == "bike")
			have = PAINTS_BIKE;
		else if (base == "moped")
			have = PAINTS_MOPED;
		else if (base == "boat")
			have = PAINTS_BOAT;
		if (c == "" || !have.Contains("," + c + ","))
			c = fallback;
		return DIR + "car_" + base + "_" + c + "_ca.paa";
	}

	protected void SetCar(DL_FilmCar c, string tex, float texW, float texH, float cw, float ch, float sx, float sy)
	{
		c.w = cw * sx;
		c.h = ch * sy;
		c.sprite = new DL_Sprite(m_Root, tex, 6, c.w * m_Px, c.h * m_Px, texW, texH);
		c.sprite.Cell(0, 0, cw, ch);
	}

	// the layer over the riders (Tools/build-anim-assets.py bike_front / boat_rim), same size and place as the body
	protected void SetOver(DL_FilmCar c, string tex, float texW, float texH, float cw, float ch)
	{
		c.over = new DL_Sprite(m_Root, tex, 9, c.w * m_Px, c.h * m_Px, texW, texH);
		c.over.Cell(0, 0, cw, ch);
	}

	protected static string OverTex(string base, string part, string colour, string fallback)
	{
		string t = CarTex(base, colour, fallback);
		t.Replace("car_" + base + "_", "car_" + base + part + "_");
		return t;
	}

	// where the i-th rider of an open vehicle sits (centre of his 32 px cell, film px) and how much of him shows
	protected void SeatPos(DL_FilmCar c, int i, out float sx, out float sy, out float cut, out int row, out int backRow)
	{
		backRow = 17;                              // a slung weapon, facing down
		if (c.kind == "bike")
		{
			// driver on the seat, passenger behind him; head and body only (19 of 32 rows), legs behind the tank
			// (Tools/anim_bikes.py DIRT_SEATS / MOPED_SEATS, in the cropped sprite's pixels)
			float bleft = c.drawX - c.w / 2;
			float bx = 11;
			float by = 26;
			if (i > 0)
				by = 17;
			if (c.look == "moped")
			{
				bx = 10;
				by = 22;
				if (i > 0)
					by = 13;
			}
			cut = 19.0 / 32.0;
			sx = bleft + bx;
			sy = c.Top() + by - 19 + 16;
			row = 0;                               // idle, facing the way it goes
			return;
		}
		// the inflatable (Tools/anim_boat.py SEATS, from the in-game seats): the driver on the left tube by the motor,
		// one on the right tube, one on the left, one at the bow facing back
		array<float> seatX = {11.0, 31.0, 11.0, 21.0};
		array<float> seatBottom = {24.0, 40.0, 54.0, 82.0};
		int j = Math.Min(i, 3);
		float left = c.drawX - c.w / 2;
		sx = left + seatX[j];
		if (j == 3)
		{
			cut = 0.62;                            // seen from behind: head and back
			row = 1;                               // idle, facing up (back to the bow)
			backRow = 18;
		}
		else
		{
			cut = 1;
			row = 16;                              // crouched on the tube (MiniDayZ "using_item")
		}
		sy = c.Top() + seatBottom[j] - 32 * cut + 16;
	}

	// the old MiniDayZ sprites as they are (films from an older server)
	protected void SetOldCar(DL_FilmCar c, string tex, float cx, float cy, float cw, float ch)
	{
		c.w = cw;
		c.h = ch;
		c.sprite = new DL_Sprite(m_Root, tex, 6, c.w * m_Px, c.h * m_Px, 256, 256);
		c.sprite.Cell(cx, cy, cw, ch);
	}

	// the line: the lead car at m_LeadY, each next one right behind (above) it; cars left behind don't count
	protected void Relayout(bool jump)
	{
		float y = m_LeadY;
		bool first = true;
		float prevH = 0;
		foreach (DL_FilmCar c : m_Cars)
		{
			if (c.inWorld)
				continue;
			if (!first)
				y -= prevH / 2 + CAR_GAP + c.h / 2;
			c.slotY = y;
			if (jump)
				c.y = y;
			prevH = c.h;
			first = false;
		}
	}

	// ---------------- helpers ----------------

	// the truck sprite's own rows (Tools/build-anim-assets.py top_truck), as film y: top of the sheet + row
	static const float TRUCK_DOOR_ROW = 69;
	static const float TRUCK_FRONT_AXLE = 91;
	static const float TRUCK_REAR_AXLE_1 = 31;
	static const float TRUCK_REAR_AXLE_2 = 16;

	protected DL_FilmCar FilmCar(int idx)
	{
		if (idx < 0 || idx >= m_Cars.Count())
			return null;
		return m_Cars[idx];
	}

	// the front car still on the road with them
	protected DL_FilmCar FirstCar()
	{
		foreach (DL_FilmCar c : m_Cars)
		{
			if (!c.inWorld)
				return c;
		}
		return null;
	}

	// the living who ride in car c
	protected array<DL_FilmPerson> Riders(DL_FilmCar c)
	{
		array<DL_FilmPerson> a = new array<DL_FilmPerson>;
		if (!c)
			return a;
		foreach (DL_FilmPerson p : m_Crew)
		{
			if (!p.down && p.car == c.idx)
				a.Insert(p);
		}
		return a;
	}

	// someone of car c sits inside it (hidden)
	protected bool Inside(DL_FilmCar c)
	{
		array<DL_FilmPerson> rl = Riders(c);
		foreach (DL_FilmPerson p : rl)
		{
			if (!p.shown)
				return true;
		}
		return false;
	}

	protected DL_FilmEvent Ev()
	{
		if (m_Scene < 0 || m_Scene >= m_Film.events.Count())
			return null;
		return m_Film.events[m_Scene];
	}

	protected void NextStep()
	{
		m_Step++;
		m_StepT = 0;
	}

	protected void Caption(string t)
	{
		if (m_Caption)
			m_Caption.SetText(t);
	}

	protected array<DL_FilmPerson> Alive()
	{
		array<DL_FilmPerson> a = new array<DL_FilmPerson>;
		foreach (DL_FilmPerson p : m_Crew)
		{
			if (!p.down)
				a.Insert(p);
		}
		return a;
	}

	// the named living survivor, else any living one at random
	protected DL_FilmPerson Who(string name)
	{
		foreach (DL_FilmPerson p : m_Crew)
		{
			if (p.name == name && !p.down)
				return p;
		}
		array<DL_FilmPerson> a = Alive();
		if (a.Count() == 0)
			return null;
		return a[Math.RandomInt(0, a.Count())];
	}

	// everyone who rides gets back in (walkers keep walking)
	protected void HideCrew()
	{
		foreach (DL_FilmPerson p : m_Crew)
		{
			if (p.down || p.car < 0)
				continue;
			p.path.Clear();
			p.pose = DL_FilmPerson.NORMAL;
			p.marching = false;
			p.Show(false);
		}
	}

	// car c's door for its i-th rider: driver front left, then front right, rear left, rear right; the truck has its
	// two cab doors only (anyone more climbs down off the back of the bed)
	protected void DoorOf(DL_FilmCar c, int i, out float hx, out float hy)
	{
		int side = -1;
		if (i % 2 == 1)
			side = 1;
		int rowN = i / 2;
		hx = c.x + side * (c.HalfW() + 9);
		hy = c.y + 8 - rowN * 20;
		if (c.truck)
		{
			hy = c.Top() + TRUCK_DOOR_ROW;
			if (rowN > 0)
			{
				hx = c.x + side * 10;
				hy = c.Top() - 8;
			}
		}
	}

	// out of car c: each of its riders by his door
	protected void GetOut(DL_FilmCar c)
	{
		if (!c)
			return;
		int i = 0;
		array<DL_FilmPerson> rl = Riders(c);
		foreach (DL_FilmPerson p : rl)
		{
			float hx;
			float hy;
			DoorOf(c, i, hx, hy);
			p.homeX = hx;
			p.homeY = hy;
			p.x = hx;
			p.y = hy;
			p.dir = 0;
			p.path.Clear();
			p.marching = false;
			p.pose = DL_FilmPerson.NORMAL;
			p.Show(true);
			i++;
		}
	}

	// one rider out of car c, at a door on side (-1 left / 1 right)
	protected void GetOutOne(DL_FilmPerson p, DL_FilmCar c, int side)
	{
		if (!p || !c)
			return;
		p.homeX = c.x + side * (c.HalfW() + 9);
		p.homeY = c.y - 4;
		if (c.truck)
			p.homeY = c.Top() + TRUCK_DOOR_ROW;
		if (!p.shown)
		{
			p.x = p.homeX;
			p.y = p.homeY;
			p.dir = 0;
		}
		p.path.Clear();
		p.marching = false;
		p.pose = DL_FilmPerson.NORMAL;
		p.Show(true);
	}

	// walk to (side of car c, y) without walking through it: round its front or back when crossing
	protected void RouteAround(DL_FilmPerson p, DL_FilmCar c, int toSide, float toY)
	{
		float sideX = c.x + toSide * (c.HalfW() + 9);
		int fromSide = -1;
		if (p.x > c.x)
			fromSide = 1;
		if (fromSide != toSide)
		{
			bool clear = p.y < c.y - c.HalfH() - 4 || p.y > c.y + c.HalfH() + 4;
			if (clear)
				p.Go(sideX, p.y);
			else
			{
				float endY = c.y - c.HalfH() - 10;
				if (p.y + toY > 2 * c.y)
					endY = c.y + c.HalfH() + 10;
				p.Go(p.x, endY);
				p.Go(sideX, endY);
			}
		}
		p.Go(sideX, toY);
	}

	// car c's riders who are out walk back to their doors
	protected void Board(DL_FilmCar c)
	{
		array<DL_FilmPerson> rl = Riders(c);
		foreach (DL_FilmPerson p : rl)
		{
			if (!p.shown)
				continue;
			p.pose = DL_FilmPerson.NORMAL;
			int side = -1;
			if (p.homeX > c.x)
				side = 1;
			RouteAround(p, c, side, p.homeY);
		}
	}

	// all of car c's riders at their doors: in they get
	protected bool Boarded(DL_FilmCar c)
	{
		array<DL_FilmPerson> rs = Riders(c);
		foreach (DL_FilmPerson p : rs)
		{
			if (p.shown && p.Busy())
				return false;
		}
		foreach (DL_FilmPerson q : rs)
		{
			q.path.Clear();
			q.pose = DL_FilmPerson.NORMAL;
			q.Show(false);
		}
		return true;
	}

	// the people on foot form up: beside the convoy (its side lane), or (no car with them) in the middle of the road
	protected void Formation(bool jump)
	{
		DL_FilmCar lead = FirstCar();
		int i = 0;
		foreach (DL_FilmPerson p : m_Crew)
		{
			if (p.down || p.car >= 0)
				continue;
			p.Show(true);
			p.pose = DL_FilmPerson.NORMAL;
			float fx;
			float fy;
			if (lead)
			{
				fx = SIDE_LANE;
				fy = lead.y - lead.h / 2 + 10 + i * 24;
			}
			else
			{
				int col = i % 2;
				int rowN = i / 2;
				fx = 114 + col * 28;
				fy = m_WalkY + rowN * 22;
			}
			p.path.Clear();
			if (jump)
			{
				p.x = fx;
				p.y = fy;
				p.dir = 0;
				p.marching = true;
			}
			else
				p.Go(fx, fy);
			i++;
		}
	}

	// cars nobody rides any more are left behind (they scroll away as the road moves on); the line closes up
	protected void LeaveEmptyCars()
	{
		bool changed = false;
		foreach (DL_FilmCar c : m_Cars)
		{
			if (!c.inWorld && Riders(c).Count() == 0)
			{
				c.inWorld = true;
				changed = true;
			}
		}
		if (changed)
			Relayout(false);
	}

	protected void ShowTool(int prop, float x, float y)
	{
		int cx = prop % 4;
		int cy = prop / 4;
		m_Tool.sprite.Cell(cx * 64, cy * 64, 64, 64);
		m_Tool.x = x;
		m_Tool.y = y;
		m_Tool.sprite.Show(true);
	}

	// ---------------- scenes ----------------

	protected void NextScene()
	{
		m_Scene++;
		m_SceneT = 0;
		m_Step = 0;
		m_StepT = 0;
		if (m_Drawn && m_Actor)
			m_Actor.Draw();
		m_Drawn = false;
		m_Carrying = false;
		m_HelperCarry = false;
		m_Actor = null;
		m_Helper = null;
		m_Donor = null;
		foreach (DL_FilmPerson c : m_Crew)
		{
			c.pose = DL_FilmPerson.NORMAL;
			c.speed = 60;
		}
		if (m_Tool)
			m_Tool.sprite.Show(false);
		if (m_Fuel)
			m_Fuel.sprite.Show(false);
		if (m_Puff)
			m_Puff.sprite.Show(false);
		if (m_Flash)
			m_Flash.Show(false);
		if (m_Zed)
		{
			m_Zed.attacking = false;
			m_Zed.path.Clear();
			if (!m_Zed.down)
				m_Zed.Show(false);
		}
		DL_FilmEvent e = Ev();
		if (!e)
		{
			m_Ending = true;
			Caption("");
			m_Target = 0;
			return;
		}
		// the car the scene is about (the event's, else the front one), and who it is about (a random living
		// survivor if the name is unknown)
		m_V = FilmCar(e.car);
		if (!m_V || m_V.inWorld)
			m_V = FirstCar();
		m_Donor = FilmCar(e.donor);
		if (e.kind == "flat" || e.kind == "fuel" || e.kind == "attack" || e.kind == "died" || e.kind == "move")
		{
			m_Actor = Who(e.who);
			if (m_Actor)
				e.who = m_Actor.name;
		}
		Caption(Line(e));
		if (e.kind == "drive")
		{
			LeaveEmptyCars();
			m_Target = DRIVE;
			HideCrew();
		}
		else if (e.kind == "walk" && m_Water)
		{
			ToLand();
			m_Target = WALK;
			Formation(true);
		}
		else if (e.kind == "walk")
		{
			if (m_Cars.Count() == 1 && m_V && !m_V.inWorld && Riders(m_V).Count() > 0)
				LeaveCar();                       // the only car stays: everyone on foot from here
			else
			{
				LeaveEmptyCars();
				HideCrew();
				m_Target = WALK;
				Formation(true);
			}
		}
	}

	// a boat trip goes on foot (no boat left that takes them): the water gives way to the land, the boats stay behind
	protected void ToLand()
	{
		m_Water = false;
		if (m_GroundLand)
			m_GroundLand.SetAlpha(1);
		if (m_GroundA)
			m_GroundA.SetAlpha(0);
		foreach (DL_FilmCar c : m_Cars)
			c.inWorld = true;
		foreach (DL_FilmPerson p : m_Crew)
		{
			if (!p.down)
				p.car = -1;
		}
	}

	// a boat's event (fuel, a seat in another boat): the boats stop for a moment, nobody gets out on the water
	protected void SceneBoat(DL_FilmEvent e)
	{
		if (m_Step == 0)
		{
			m_Target = 0;
			NextStep();
		}
		else if (m_Step == 1 && m_StepT > 1.6)
		{
			if (e.kind == "move" && m_Actor)
			{
				if (e.detail == "walk")
					m_Actor.car = -1;
				else
					m_Actor.car = e.car;
				m_Actor.Show(false);
			}
			m_Target = DRIVE;
			NextStep();
		}
	}

	// the only car stays (flat, no fuel): they walk down its side and form up in front of it, then set off on foot
	protected void LeaveCar()
	{
		DL_FilmCar c = m_V;
		if (Inside(c))
			GetOut(c);
		m_Target = 0;
		m_Leaving = true;
		m_WalkY = c.y + c.HalfH() + 16;
		int i = 0;
		array<DL_FilmPerson> rl = Riders(c);
		foreach (DL_FilmPerson p : rl)
		{
			p.pose = DL_FilmPerson.NORMAL;
			p.marching = false;
			p.path.Clear();
			int side = -1;
			if (p.x > c.x)
				side = 1;
			float sideX = c.x + side * (c.HalfW() + 9);
			if (Math.AbsFloat(p.x - sideX) > 2 && p.y > c.y - c.HalfH() - 4 && p.y < c.y + c.HalfH() + 4)
				RouteAround(p, c, side, p.y);    // somewhere odd next to the car: back to its side first
			p.Go(sideX, m_WalkY - 4);
			int col = i % 2;
			int rowN = i / 2;
			p.Go(114 + col * 28, m_WalkY + rowN * 22);
			i++;
		}
		foreach (DL_FilmPerson w : m_Crew)
		{
			if (!w.down)
				w.car = -1;
		}
	}

	protected string Line(DL_FilmEvent e)
	{
		string who = e.who;
		if (m_Film.crew.Count() == 1)
			who = "You";
		string km = Math.Round(e.km).ToString() + " km";
		string donorName = "";
		DL_FilmPerson dp = DriverOf(m_Donor);
		if (dp)
			donorName = dp.name;
		if (e.kind == "drive")
		{
			if (m_Convoy)
				return "The convoy rolls on: " + km;
			return "On the road: " + km;
		}
		if (e.kind == "walk")
		{
			if (m_Convoy && FirstCar())
				return "The convoy keeps to walking pace: " + km;
			return "On foot: " + km;
		}
		if (e.kind == "flat" && e.detail == "spare" && donorName != "")
			return "Blew a tyre. " + donorName + "'s spare fits: " + who + " puts it on.";
		if (e.kind == "flat" && e.detail == "spare")
			return "Blew a tyre. " + who + " put on the spare.";
		if (e.kind == "flat" && e.detail == "kit")
			return "Blew a tyre. " + who + " patched it.";
		if (e.kind == "flat")
			return "Blew a tyre - no spare. The car stays here.";
		if (e.kind == "fuel" && e.detail == "found")
			return "Out of gas. " + who + " found some fuel!";
		if (e.kind == "fuel" && e.detail == "shared")
			return "Running dry. " + who + " brings a jerry can.";
		if (e.kind == "fuel")
		{
			if (m_Convoy)
				return "Out of gas. Nothing found: the car stays here.";
			return "Out of gas. Nothing found: on foot from here.";
		}
		if (e.kind == "move" && e.detail == "walk")
			return who + " walks on: no seat left.";
		if (e.kind == "move")
			return who + " climbs into another car.";
		if (e.kind == "attack" && e.detail == "died")
			return who + " didn't survive the attack.";
		if (e.kind == "attack")
			return "Infected! " + who + " fought it off.";
		if (e.kind == "ration")
			return "Food and water ran low: everyone rations.";
		if (e.kind == "died")
			return who + " didn't make it.";
		return "";
	}

	protected DL_FilmPerson DriverOf(DL_FilmCar c)
	{
		array<DL_FilmPerson> rs = Riders(c);
		if (rs.Count() == 0)
			return null;
		return rs[0];
	}

	// one step of the current scene's choreography
	protected void Scene(float dt)
	{
		if (m_Ending)
		{
			SceneEnd();
			return;
		}
		DL_FilmEvent e = Ev();
		if (!e)
			return;
		float len = DL_Film.SceneSeconds(e);
		if (e.kind == "walk" && m_Leaving)
		{
			bool ready = true;
			foreach (DL_FilmPerson lp : m_Crew)
			{
				if (!lp.down && lp.Busy())
					ready = false;
			}
			if (ready)
			{
				m_Leaving = false;
				foreach (DL_FilmPerson mp : m_Crew)
				{
					if (!mp.down)
					{
						mp.dir = 0;
						mp.marching = true;
					}
				}
				if (m_V)
					m_V.inWorld = true;           // left behind: it scrolls away
				m_Target = WALK;
			}
		}
		else if (e.kind == "drive" || e.kind == "walk")
			m_Km += e.km / len * dt;
		else if (m_V && m_V.kind == "boat" && (e.kind == "flat" || e.kind == "fuel" || e.kind == "move"))
			SceneBoat(e);
		else if (e.kind == "flat")
			SceneFlat(e);
		else if (e.kind == "fuel" && e.detail == "shared")
			SceneShareFuel(e);
		else if (e.kind == "fuel")
			SceneFuel(e);
		else if (e.kind == "move")
			SceneMove(e);
		else if (e.kind == "attack")
			SceneAttack(e);
		else if (e.kind == "died")
		{
			if (m_Actor && m_SceneT > 1)
				m_Actor.down = true;
		}
		if (m_SceneT >= len)
			NextScene();
	}

	// stopped: true once the cars (or the walkers) stand still
	protected bool Halted()
	{
		return m_Speed < 1 || m_StepT > 2;
	}

	// flat tyre: a random wheel blows, the car stops, the fixer walks round the car to it, works, they get back in.
	// In a convoy the spare may come from another car: someone of that car carries it over.
	protected void SceneFlat(DL_FilmEvent e)
	{
		DL_FilmCar c = m_V;
		if (!c)
			return;
		float hw = c.HalfW();
		float hh = c.HalfH();
		if (m_Step == 0)
		{
			m_Target = 0;
			int wheels = 4;
			if (c.truck)
				wheels = 6;
			else if (c.kind == "bike")
				wheels = 2;
			int wheel = Math.RandomInt(0, wheels);   // even = left, odd = right (screen); wheel / 2 = the axle
			m_WheelSide = -1;
			if (wheel % 2 == 1)
				m_WheelSide = 1;
			int axle = wheel / 2;
			m_WheelY = c.y + hh - 15;     // the car drives down: its front is at the bottom
			if (axle >= 1)
				m_WheelY = c.y - hh + 15;
			if (c.truck)
			{
				m_WheelY = c.Top() + TRUCK_FRONT_AXLE;
				if (axle == 1)
					m_WheelY = c.Top() + TRUCK_REAR_AXLE_1;
				else if (axle == 2)
					m_WheelY = c.Top() + TRUCK_REAR_AXLE_2;
			}
			m_Puff.x = c.x + m_WheelSide * (hw - 3);
			if (c.kind == "bike")
			{
				m_WheelY = c.y + hh - 8;          // front wheel (the bike drives down)
				if (axle >= 1)
					m_WheelY = c.y - hh + 8;      // rear wheel
				m_Puff.x = c.x;
			}
			m_Puff.y = m_WheelY;
			m_Puff.sprite.Show(true);
			NextStep();
		}
		else if (m_Step == 1 && m_StepT > 0.8 && Halted())
		{
			GetOut(c);
			if (m_Actor)
				RouteAround(m_Actor, c, m_WheelSide, m_WheelY);
			if (m_Donor && m_Donor != c && e.detail != "none")
			{
				// the spare (or kit) comes from another car of the convoy: its driver carries it over, along the side
				m_Helper = DriverOf(m_Donor);
				if (m_Helper)
				{
					GetOutOne(m_Helper, m_Donor, m_WheelSide);
					m_Helper.Go(c.x + m_WheelSide * (hw + 22), m_WheelY + 6);
					m_HelperCarry = true;
				}
			}
			NextStep();
		}
		else if (m_Step == 2)
		{
			if (!m_Actor || !m_Actor.Busy())
			{
				if (m_Actor)
				{
					m_Actor.dir = 2;               // left of the car: faces right, at the wheel
					if (m_WheelSide > 0)
						m_Actor.dir = 3;
					m_Actor.pose = DL_FilmPerson.USE;
					if (!m_HelperCarry)
					{
						if (e.detail == "spare")
							ShowTool(P_WHEEL, m_Actor.x + m_WheelSide * 9, m_Actor.y + 11);
						else if (e.detail == "kit")
							ShowTool(P_CRATE, m_Actor.x + m_WheelSide * 9, m_Actor.y + 11);
					}
				}
				array<DL_FilmPerson> rl = Riders(c);
				foreach (DL_FilmPerson o : rl)
				{
					if (o != m_Actor)
						o.FaceTo(m_Actor);
				}
				NextStep();
			}
		}
		else if (m_Step == 3)
		{
			float work = 2.6;
			if (e.detail == "none")
				work = 1.4;                       // nothing to fix it with
			if (m_StepT > work && !m_HelperCarry)
			{
				m_Tool.sprite.Show(false);
				if (m_Actor)
				{
					m_Actor.pose = DL_FilmPerson.NORMAL;
					m_Actor.dir = 0;
				}
				if (e.detail != "none")
				{
					m_Puff.sprite.Show(false);
					Board(c);
				}
				NextStep();
			}
		}
		else if (m_Step == 4 && e.detail != "none")
		{
			bool helperIn = true;
			if (m_Helper)
				helperIn = Boarded(m_Donor);
			if (Boarded(c) && helperIn)
			{
				m_Target = DRIVE;
				if (FirstCar() && Walkers() > 0)
					m_Target = WALK;
				NextStep();
			}
		}
		// the helper carries the spare over, leaves it at the wheel, goes back to his car
		if (m_Helper && m_HelperCarry)
		{
			int prop = P_WHEEL;
			if (e.detail == "kit")
				prop = P_CRATE;
			ShowTool(prop, m_Helper.x + 6, m_Helper.y + 8);
			if (!m_Helper.Busy())
			{
				m_HelperCarry = false;
				ShowTool(prop, c.x + m_WheelSide * (hw + 18), m_WheelY + 12);
				m_Helper.FaceTo(m_Actor);
				Board(m_Donor);
			}
		}
	}

	protected int Walkers()
	{
		int n = 0;
		foreach (DL_FilmPerson p : m_Crew)
		{
			if (!p.down && p.car < 0)
				n++;
		}
		return n;
	}

	// out of gas: one goes off to search, comes back (with a can: he fills up, they drive on)
	protected void SceneFuel(DL_FilmEvent e)
	{
		DL_FilmCar c = m_V;
		if (!c)
			return;
		float hw = c.HalfW();
		float hh = c.HalfH();
		float topY = c.y - hh - 14;
		if (m_Step == 0)
		{
			m_Target = 0;
			NextStep();
		}
		else if (m_Step == 1 && m_StepT > 0.8 && Halted())
		{
			GetOut(c);
			if (m_Actor)
			{
				if (!m_Actor.shown)
					GetOutOne(m_Actor, c, 1);
				m_Actor.speed = 80;
				m_Actor.Go(m_Actor.x, topY);
				m_Actor.Go(300, topY);           // off the right edge, searching
			}
			NextStep();
		}
		else if (m_Step == 2 && (!m_Actor || !m_Actor.Busy()))
			NextStep();
		else if (m_Step == 3 && m_StepT > 0.6)
		{
			if (m_Actor)
			{
				m_Actor.Go(c.x + hw + 9, topY);
				m_Actor.Go(c.x + hw + 9, c.y - 6); // the filler cap
			}
			m_Carrying = e.detail == "found";
			NextStep();
		}
		else if (m_Step == 4)
		{
			if (!m_Actor || !m_Actor.Busy())
			{
				if (m_Actor)
				{
					m_Actor.speed = 60;
					m_Actor.dir = 3;
					if (m_Carrying)
						m_Actor.pose = DL_FilmPerson.USE;
					else
						m_Actor.dir = 0;
				}
				NextStep();
			}
		}
		else if (m_Step == 5 && m_Carrying && m_StepT > 1.2)
		{
			m_Carrying = false;
			m_Fuel.sprite.Show(false);
			Board(c);
			NextStep();
		}
		else if (m_Step == 6)
		{
			if (Boarded(c))
			{
				m_Target = DRIVE;
				if (FirstCar() && Walkers() > 0)
					m_Target = WALK;
				NextStep();
			}
		}
		if (m_Carrying && m_Actor)
		{
			m_Fuel.sprite.Show(true);
			m_Fuel.x = m_Actor.x - 8;
			m_Fuel.y = m_Actor.y + 6;
		}
	}

	// a convoy: someone from another car brings a jerry can over and pours it in, then goes back to his car
	protected void SceneShareFuel(DL_FilmEvent e)
	{
		DL_FilmCar c = m_V;
		if (!c)
			return;
		DL_FilmCar home = m_Donor;
		if (!home)
			home = c;
		if (m_Step == 0)
		{
			m_Target = 0;
			NextStep();
		}
		else if (m_Step == 1 && m_StepT > 0.6 && Halted())
		{
			if (m_Actor)
			{
				if (m_Actor.car != home.idx)
					m_Actor = DriverOf(home);
			}
			if (m_Actor)
			{
				GetOutOne(m_Actor, home, 1);
				m_Actor.Go(c.x + c.HalfW() + 9, c.y - 6); // the filler cap, on the right
				m_Carrying = true;
			}
			NextStep();
		}
		else if (m_Step == 2 && (!m_Actor || !m_Actor.Busy()))
		{
			if (m_Actor)
			{
				m_Actor.dir = 3;
				m_Actor.pose = DL_FilmPerson.USE;
			}
			NextStep();
		}
		else if (m_Step == 3 && m_StepT > 1.4)
		{
			m_Carrying = false;
			m_Fuel.sprite.Show(false);
			if (m_Actor)
			{
				m_Actor.pose = DL_FilmPerson.NORMAL;
				Board(home);
			}
			NextStep();
		}
		else if (m_Step == 4)
		{
			if (Boarded(home))
			{
				m_Target = DRIVE;
				if (Walkers() > 0)
					m_Target = WALK;
				NextStep();
			}
		}
		if (m_Carrying && m_Actor)
		{
			m_Fuel.sprite.Show(true);
			m_Fuel.x = m_Actor.x - 8;
			m_Fuel.y = m_Actor.y + 6;
		}
	}

	// a convoy car was left: one of its crew walks to a free seat in another car, or out to the side lane
	protected void SceneMove(DL_FilmEvent e)
	{
		DL_FilmPerson p = m_Actor;
		if (!p)
			return;
		if (m_Step == 0)
		{
			m_Target = 0;
			NextStep();
		}
		else if (m_Step == 1 && Halted())
		{
			DL_FilmCar src = FilmCar(p.car);
			if (!p.shown && src)
				GetOutOne(p, src, 1);
			DL_FilmCar dst = FilmCar(e.car);
			if (dst && e.detail != "walk")
			{
				float hx;
				float hy;
				DoorOf(dst, 3, hx, hy);           // a rear door on the right
				p.homeX = hx;
				p.homeY = hy;
				RouteAround(p, dst, 1, hy);
			}
			else
			{
				DL_FilmCar lead = FirstCar();
				float ly = m_WalkY;
				if (lead)
					ly = lead.y - lead.h / 2 + 10 + Walkers() * 24;
				if (p.x > 128)
					p.Go(p.x, ly - 20);           // round the front of the cars to the left lane
				p.Go(SIDE_LANE, ly);
			}
			NextStep();
		}
		else if (m_Step == 2 && !p.Busy())
		{
			if (e.car >= 0 && e.detail != "walk")
			{
				p.car = e.car;
				p.Show(false);
			}
			else
			{
				p.car = -1;
				p.dir = 0;
			}
			NextStep();
		}
	}

	// infected: the road stops, it comes for one of them, he fights it (shoots, swings, or punches), they go on
	protected void SceneAttack(DL_FilmEvent e)
	{
		DL_FilmPerson v = m_Actor;
		if (m_Step == 0)
		{
			m_Target = 0;
			foreach (DL_FilmPerson p : m_Crew)
				p.marching = false;
			NextStep();
		}
		else if (m_Step == 1 && m_StepT > 0.4 && Halted())
		{
			if (!v)
			{
				NextStep();
				return;
			}
			if (!v.shown)
				GetOut(FilmCar(v.car));
			m_ZedSide = -1;
			if (Math.RandomFloat01() < 0.5)
				m_ZedSide = 1;
			if (FirstCar() && v.car < 0)
				m_ZedSide = -1;                    // beside a convoy: from the verge
			m_Zed.down = false;
			m_Zed.downT = 0;
			m_Zed.attacking = false;
			m_Zed.x = 128 + m_ZedSide * 150;
			m_Zed.y = v.y - 10;
			m_Zed.path.Clear();
			m_Zed.Go(v.x + m_ZedSide * 15, v.y);
			m_Zed.Show(true);
			v.Face(m_ZedSide, 0);
			NextStep();
		}
		else if (m_Step == 2 && v && (!m_Zed.Busy() || m_StepT > 3))
		{
			m_Zed.path.Clear();
			m_Zed.attacking = true;
			m_Zed.FaceTo(v);
			v.FaceTo(m_Zed);
			if (!v.hand && v.back)
			{
				v.Draw();                          // takes the slung weapon
				m_Drawn = true;
			}
			if (v.Shoots())
				v.pose = DL_FilmPerson.AIM;
			else
				v.pose = DL_FilmPerson.SWING;
			foreach (DL_FilmPerson o : m_Crew)
			{
				if (o != v && !o.down && o.shown)
					o.FaceTo(m_Zed);
			}
			NextStep();
		}
		else if (m_Step == 3)
		{
			if (v.pose == DL_FilmPerson.AIM)
				Shot(v, m_StepT);
			if (m_StepT > 2.2)
			{
				m_Flash.Show(false);
				m_Zed.attacking = false;
				v.pose = DL_FilmPerson.NORMAL;
				if (e.detail == "died")
					v.down = true;
				else
					m_Zed.down = true;
				NextStep();
			}
		}
		else if (m_Step == 4 && m_StepT > 0.7)
		{
			// the others finish it, or (nobody left) it shambles off
			if (!m_Zed.down)
			{
				if (Alive().Count() > 0)
					m_Zed.down = true;
				else
					m_Zed.Go(128 + m_ZedSide * 170, m_Zed.y - 30);
			}
			if (m_Drawn && !v.down)
				v.Draw();                          // slings it again
			m_Drawn = false;
			if (!v.down && v.car >= 0)
				Board(FilmCar(v.car));
			else
				Formation(false);
			NextStep();
		}
		else if (m_Step == 5)
		{
			if (!v.down && v.car >= 0)
			{
				if (Boarded(FilmCar(v.car)))
				{
					m_Target = DRIVE;
					if (Walkers() > 0)
						m_Target = WALK;
					NextStep();
				}
				return;
			}
			foreach (DL_FilmPerson w : m_Crew)
			{
				if (!w.down && w.car < 0 && w.Busy())
					return;
			}
			foreach (DL_FilmPerson m : m_Crew)
			{
				if (!m.down && m.car < 0)
				{
					m.dir = 0;
					m.marching = true;
				}
			}
			m_Target = WALK;
			NextStep();
		}
	}

	// muzzle flash every 0.45 s
	protected void Shot(DL_FilmPerson p, float t)
	{
		float ph = t - Math.Floor(t / 0.45) * 0.45;
		bool on = ph < 0.08;
		m_Flash.Show(on);
		if (!on)
			return;
		float fx = p.x;
		float fy = p.y;
		if (p.dir == 2)
		{
			fx += 15;
			fy -= 2;
		}
		else if (p.dir == 3)
		{
			fx -= 15;
			fy -= 2;
		}
		else if (p.dir == 0)
		{
			fx += 3;
			fy += 12;
		}
		else
		{
			fx -= 3;
			fy -= 14;
		}
		m_Flash.SetPos(m_Ox + (fx - 1.5) * m_Px, m_Oy + (fy - 1.5) * m_Px);
	}

	// after the last scene: the road stops; every car with its crew inside drives out of the bottom, whoever is on
	// foot walks out
	protected void SceneEnd()
	{
		if (m_Step == 0)
		{
			m_Target = 0;
			foreach (DL_FilmPerson p : m_Crew)
				p.marching = false;
			NextStep();
		}
		else if (m_Step == 1 && (Halted() || !FirstCar() || Walkers() == 0))
		{
			foreach (DL_FilmCar c : m_Cars)
			{
				if (!c.inWorld && Inside(c))
					c.driveOff = true;
			}
			foreach (DL_FilmPerson w : m_Crew)
			{
				if (w.down || !w.shown)
					continue;
				w.path.Clear();
				w.speed = 55;
				w.Go(w.x, m_ViewH + 40);
			}
			NextStep();
		}
		else if (m_Step == 2)
		{
			bool gone = true;
			foreach (DL_FilmPerson g : m_Crew)
			{
				if (!g.down && g.shown && g.y < m_ViewH + 30)
					gone = false;
			}
			foreach (DL_FilmCar dc : m_Cars)
			{
				if (dc.driveOff && dc.Top() < m_ViewH)
					gone = false;
			}
			if (gone || m_StepT > DL_Film.END_SECONDS + 2)
			{
				Print(DL_Const.LOG_TAG + "road film ended");
				if (m_Film.skip == "vote" && !m_Voted)
				{
					m_Voted = true;
					m_VoteOut = true;                 // done watching: he doesn't hold the others' skip up
				}
				Stop();
			}
		}
	}

	// ---------------- per frame ----------------

	void Update(float dt)
	{
		bool skip = KeyState(KeyCode.KC_SPACE) != 0;
		if (m_Playing && skip && !m_KeyWasDown)
		{
			if (m_Film.skip == "vote")
			{
				if (!m_Voted)
				{
					m_Voted = true;
					m_VoteOut = true;
					if (m_SkipText)
						m_SkipText.SetText("You voted to skip - waiting for the others");
					Print(DL_Const.LOG_TAG + "road film: voted to skip");
				}
			}
			else if (m_Film.skip != "none")
			{
				Print(DL_Const.LOG_TAG + "road film skipped");
				Stop();
			}
		}
		m_KeyWasDown = skip;
		if (!m_Playing || !m_Root)
			return;
		m_T += dt;
		m_SceneT += dt;
		m_StepT += dt;
		Scene(dt);
		if (!m_Playing)
			return;
		// ease the ground speed
		float a = 120 * dt;
		if (m_Speed < m_Target)
			m_Speed = Math.Min(m_Target, m_Speed + a);
		else
			m_Speed = Math.Max(m_Target, m_Speed - a * 1.4);
		float dy = m_Speed * dt;
		m_Scroll += dy;
		m_GroundA.Scroll(m_Scroll * m_Px);
		if (m_GroundLand)
			m_GroundLand.Scroll(m_Scroll * m_Px);
		if (m_GroundV)
		{
			m_GroundV.Scroll(m_Scroll * m_Px);
			m_GroundV.SetAlpha(ViaAlpha());
		}
		if (m_GroundB)
		{
			m_GroundB.Scroll(m_Scroll * m_Px);
			m_GroundB.SetAlpha(ToAlpha());
		}
		// props and wrecks come up from the bottom
		if (m_Speed > 1 && !m_Water)
		{
			m_NextProp -= dy;
			if (m_NextProp <= 0)
			{
				SpawnProp(m_ViewH + 40);
				m_NextProp = Math.RandomFloat(18, 60);
			}
			m_NextWreck -= dt * (m_Speed / DRIVE);
			if (m_NextWreck <= 0)
			{
				SpawnWreck();
				m_NextWreck = Math.RandomFloat(5, 11);
			}
		}
		foreach (DL_FilmThing t : m_Props)
			ScrollThing(t, dy);
		foreach (DL_FilmThing w : m_Wrecks)
			ScrollThing(w, dy);
		// the cars: left behind (scroll away), driving off, or in the line (closing up)
		foreach (DL_FilmCar c : m_Cars)
		{
			if (c.inWorld)
				c.y -= dy;
			else if (c.driveOff)
			{
				c.v = Math.Min(c.v + 110 * dt, 170);
				c.y += c.v * dt;
			}
			else if (Math.AbsFloat(c.slotY - c.y) > 0.1)
				c.y += (c.slotY - c.y) * Math.Min(1, dt * 1.5);
			float bob = 0;
			if ((m_Speed > 20 || c.driveOff) && !c.inWorld)
				bob = Math.Sin(m_T * 18 + c.idx * 1.7) * 0.5;
			c.drawX = c.x + bob;
			c.sprite.MoveTo(m_Ox + (c.x - c.w / 2 + bob) * m_Px, m_Oy + (c.y - c.h / 2) * m_Px);
			if (c.over)
				c.over.MoveTo(m_Ox + (c.x - c.w / 2 + bob) * m_Px, m_Oy + (c.y - c.h / 2) * m_Px);
			if (c.wake)
			{
				// the wake: its bow at the boat's bow (y 118 of 128), its centre on the hull's (x 48 of 96);
				// faint while it stands, full at speed, none once it is left behind
				int wf = Math.Floor(m_T * 8);
				wf = wf % 4;
				c.wake.Cell(wf * 96, 0, 96, 128);
				float hull = c.x - c.w / 2 + bob + 21;
				c.wake.MoveTo(m_Ox + (hull - 48) * m_Px, m_Oy + (c.Top() + 88 - 118) * m_Px);
				float wa = 0.2 + 0.8 * Math.Clamp(m_Speed / DRIVE, 0, 1);
				if (c.driveOff)
					wa = 1;
				if (c.inWorld)
					wa = 0;
				c.wake.SetAlpha(wa);
			}
		}
		// riders of open vehicles: seen in their seats (seat order = the order they ride in)
		array<int> seatN = new array<int>;
		for (int sn = 0; sn < m_Cars.Count(); sn++)
			seatN.Insert(0);
		foreach (DL_FilmPerson rp : m_Crew)
		{
			DL_FilmCar rc = FilmCar(rp.car);
			bool riding = !rp.down && !rp.shown && rc && rc.over && !rc.inWorld;
			if (!riding)
			{
				if (rp.rideShown && !rp.shown)
					rp.HideRide();
				continue;
			}
			float rx;
			float ry;
			float cut;
			int row;
			int backRow;
			SeatPos(rc, seatN[rc.idx], rx, ry, cut, row, backRow);
			seatN[rc.idx] = seatN[rc.idx] + 1;
			rp.RideAt(m_Px, m_Ox, m_Oy, rx, ry, row, 0, cut, backRow);
		}
		foreach (DL_FilmPerson p : m_Crew)
		{
			if (p.down)
				p.y -= dy;   // left lying on the road, it scrolls away
			p.Step(dt);
			p.Update(dt, m_Px, m_Ox, m_Oy);
		}
		if (m_Zed)
		{
			if (m_Zed.down)
				m_Zed.y -= dy;
			m_Zed.Step(dt);
			m_Zed.Update(dt, m_Px, m_Ox, m_Oy);
		}
		if (m_Tool)
			m_Tool.Place(m_Px, m_Ox, m_Oy);
		if (m_Fuel)
			m_Fuel.Place(m_Px, m_Ox, m_Oy);
		if (m_Puff)
		{
			m_Puff.y -= dt * 3;
			m_Puff.Place(m_Px, m_Ox, m_Oy);
		}
	}

	protected void ScrollThing(DL_FilmThing t, float dy)
	{
		if (!t.used)
			return;
		t.y -= dy;
		if (t.y < -t.h)
		{
			t.used = false;
			t.sprite.Show(false);
			return;
		}
		t.Place(m_Px, m_Ox, m_Oy);
	}

	// ---------------- what grows by the road ----------------

	protected float Progress()
	{
		if (m_Film.km > 0)
			return m_Km / m_Film.km;
		return 0;
	}

	// the biome blend: straight from -> to between 30 % and 70 % of the way; with a look on the way, from -> via
	// between 15 % and 35 %, via -> to between 65 % and 85 %
	protected float ViaAlpha()
	{
		if (!m_GroundV)
			return 0;
		return Math.Clamp((Progress() - 0.15) / 0.2, 0, 1);
	}

	protected float ToAlpha()
	{
		if (!m_GroundB)
			return 0;
		if (m_GroundV)
			return Math.Clamp((Progress() - 0.65) / 0.2, 0, 1);
		return Math.Clamp((Progress() - 0.3) / 0.4, 0, 1);
	}

	protected string BiomeHere()
	{
		float r = Math.RandomFloat01();
		if (m_GroundB && r < ToAlpha())
			return m_Film.biomeTo;
		if (m_GroundV && r < ViaAlpha())
			return m_Film.biomeVia;
		return m_Film.biomeFrom;
	}

	// ---------------- the skip vote (DL_ClientHud sends it, the server counts) ----------------

	// a vote to send now (once)
	bool TakeVote()
	{
		bool v = m_VoteOut;
		m_VoteOut = false;
		return v;
	}

	void VoteStatus(int votes, int total)
	{
		if (!m_SkipText)
			return;
		string mine = "";
		if (m_Voted)
			mine = "You voted. ";
		m_SkipText.SetText(mine + "Skip: " + votes.ToString() + " of " + total.ToString() + " - everyone has to press Space");
	}

	protected int PickProp(string biome)
	{
		float r = Math.RandomFloat01();
		if (biome == "desert")
		{
			if (r < 0.35) return P_DRYBUSH;
			if (r < 0.65) return P_SANDROCK;
			if (r < 0.75) return P_POLE;
			if (r < 0.83) return P_SIGN;
			if (r < 0.92) return P_TIRES;
			return P_BARREL;
		}
		if (biome == "snow")
		{
			if (r < 0.45) return P_SNOWPINE;
			if (r < 0.70) return P_DRIFT;
			if (r < 0.80) return P_ROCK;
			if (r < 0.88) return P_POLE;
			if (r < 0.94) return P_FENCE;
			return P_SIGN;
		}
		if (biome == "field")
		{
			if (r < 0.20) return P_HAY;
			if (r < 0.40) return P_FENCE;
			if (r < 0.55) return P_BUSH;
			if (r < 0.68) return P_POLE;
			if (r < 0.78) return P_BROADLEAF;
			if (r < 0.85) return P_SIGN;
			if (r < 0.93) return P_ROCK;
			return P_CRATE;
		}
		if (r < 0.42) return P_PINE;
		if (r < 0.60) return P_BROADLEAF;
		if (r < 0.72) return P_BUSH;
		if (r < 0.78) return P_ROCK;
		if (r < 0.84) return P_FENCE;
		if (r < 0.89) return P_SIGN;
		if (r < 0.95) return P_POLE;
		return P_BARREL;
	}

	protected void SpawnProp(float y)
	{
		foreach (DL_FilmThing t : m_Props)
		{
			if (t.used)
				continue;
			int p = PickProp(BiomeHere());
			int pcx = p % 4;
			int pcy = p / 4;
			t.sprite.Cell(pcx * 64, pcy * 64, 64, 64);
			// verges: 0..64 left, 192..256 right (the road + shoulders are 66..190)
			if (Math.RandomFloat01() < 0.5)
				t.x = Math.RandomFloat(6, 50);
			else
				t.x = Math.RandomFloat(206, 250);
			if (p == P_SIGN || p == P_POLE || p == P_FENCE)
			{
				if (t.x < 128)
					t.x = 54;      // right at the edge of the shoulder
				else
					t.x = 202;
			}
			t.y = y;
			t.used = true;
			t.sprite.Show(true);
			return;
		}
	}

	protected void SpawnWreck()
	{
		array<DL_FilmThing> free = new array<DL_FilmThing>;
		foreach (DL_FilmThing w : m_Wrecks)
		{
			if (!w.used)
				free.Insert(w);
		}
		if (free.Count() == 0)
			return;
		DL_FilmThing t = free[Math.RandomInt(0, free.Count())];
		// on the far shoulder or the opposite lane, never in the car's way
		if (Math.RandomFloat01() < 0.5)
			t.x = 46;
		else
			t.x = 212;
		t.y = m_ViewH + 60;
		t.used = true;
		t.sprite.Show(true);
	}

	// ---------------- demo (F9) ----------------

	protected static DL_FilmActor DemoActor(string name, string skin, string gear, string hand, string back, int car)
	{
		DL_FilmActor a = new DL_FilmActor();
		a.car = car;
		a.name = name;
		a.skin = skin;
		gear.Split(",", a.layers);
		a.hand = hand;
		a.back = back;
		return a;
	}

	// F9 cycles: A = a crew of three through every scene the road has (2 flats, fuel found, fuel lost, walking,
	// infected fought off, infected kill, rationing, hunger), B = a solo driver whose car is lost to a flat,
	// C = two in the M3S (cab doors, any of its 6 wheels), D = a convoy of three (a spare from another car, a shared
	// jerry can, a car left behind: one climbs into a free seat, one walks beside the convoy), E = a group on foot,
	// F = a car, a dirt bike and a moped with 2 (the bike's spare rides in the car), G = two boats on open water (a
	// full one: all 4 seats)
	static DL_Film Demo()
	{
		s_DemoN++;
		DL_Film f = new DL_Film();
		f.car = "4x4@green";
		f.biomeFrom = "forest";
		int pick = s_DemoN % 7;
		if (pick == 5)
		{
			f.car = "";
			f.cars.Insert("4x4@green");
			f.cars.Insert("bike@red");
			f.cars.Insert("moped@blue");
			f.km = 60;
			f.biomeTo = "field";
			f.crew.Insert(DemoActor("Derk", "def1", "gorkapants,gorkajacket,vest,mountainbackpack", "ak74rifle", "", 0));
			f.crew.Insert(DemoActor("Britney", "kate", "sportpants@black,hoodiegray@pink,schoolbackpack@yellow", "", "", 0));
			f.crew.Insert(DemoActor("OrionKaelin", "biker", "jeans@black,downjacket@red,helmetmoto@red", "", "sksrifle", 1));
			f.crew.Insert(DemoActor("Mika", "def2", "workpants@brown,tshirt@orange,baseballcap@red", "", "", 2));
			f.crew.Insert(DemoActor("Jonas", "redneck", "jeans@blue,hoodiegray@green,cowboyhat", "", "mosinnagant", 2));
			f.Add("drive", 20);
			f.Add("flat", 20, "OrionKaelin", "spare", 1, 0);  // the bike's spare wheel rides in the Ada
			f.Add("drive", 20);
			f.Add("flat", 40, "OrionKaelin", "kit", 1);
			f.Add("drive", 20);
			return f;
		}
		if (pick == 6)
		{
			f.car = "";
			f.cars.Insert("boat@orange");
			f.cars.Insert("boat@blue");
			f.km = 40;
			f.biomeTo = "forest";
			f.crew.Insert(DemoActor("Derk", "veteran", "orelpants,oreljacket,razgruz,helmetarmy", "akmrifle", "", 0));
			f.crew.Insert(DemoActor("Britney", "kate", "sportpants@black,raincoatblue,warmhat@lightblue", "", "", 0));
			f.crew.Insert(DemoActor("OrionKaelin", "biker", "jeans@black,downjacket@red,helmetmoto@red", "", "sksrifle", 0));
			f.crew.Insert(DemoActor("Jonas", "redneck", "jeans@blue,hoodiegray@green,cowboyhat", "", "mosinnagant", 0));
			f.crew.Insert(DemoActor("Mika", "def2", "workpants@brown,tshirt@orange,baseballcap@red", "glockpistol", "", 1));
			f.Add("drive", 15);
			f.Add("fuel", 15, "Derk", "shared", 1, 0);        // the second boat runs dry: a can from the first
			f.Add("drive", 25);
			return f;
		}
		if (pick == 4)
		{
			f.car = "";
			f.cars.Insert("truck@green");
			f.cars.Insert("4x4@blue");
			f.cars.Insert("sedan@white");
			f.km = 90;
			f.biomeTo = "field";
			f.crew.Insert(DemoActor("Derk", "veteran", "orelpants,oreljacket,razgruz,hunterbackpack,helmetarmy", "akmrifle", "", 0));
			f.crew.Insert(DemoActor("OrionKaelin", "biker", "jeans@black,downjacket@red,taloonbackpack@orange,warmhat@black", "", "sksrifle", 0));
			f.crew.Insert(DemoActor("Britney", "kate", "sportpants@black,hoodiegray@pink,schoolbackpack@yellow,warmhat@lightblue", "fireaxe", "", 1));
			f.crew.Insert(DemoActor("Jonas", "redneck", "jeans@blue,hoodiegray@green,mountainbackpack,cowboyhat", "", "mosinnagant", 1));
			f.crew.Insert(DemoActor("Mika", "def2", "workpants@brown,tshirt@orange,baseballcap@red", "glockpistol", "crowbar", 2));
			f.Add("drive", 15);
			f.Add("flat", 15, "Britney", "spare", 1, 0);   // the Ada's spare comes from the truck
			f.Add("drive", 15);
			f.Add("fuel", 30, "Derk", "shared", 2, 0);     // the Olga runs dry: a jerry can from the truck
			f.Add("drive", 15);
			f.Add("flat", 45, "Jonas", "none", 1);         // the Ada is lost
			f.Add("move", 45, "Britney", "", 2);           // she climbs into the Olga
			f.Add("move", 45, "Jonas", "walk", -1);        // the truck and the Olga are full: he walks
			f.Add("walk", 15);
			f.Add("attack", 60, "Jonas", "hurt");
			f.Add("walk", 15);
			return f;
		}
		if (pick == 0)
		{
			f.car = "";
			f.km = 50;
			f.biomeTo = "snow";
			f.crew.Insert(DemoActor("Derk", "survivalist", "hunterpants,hunterjacket,hunterbackpack,warmhat@black", "sksrifle", "", -1));
			f.crew.Insert(DemoActor("OrionKaelin", "bandit", "gorkapants,gorkajacket,vest,gorkahelmet", "", "akmrifle", -1));
			f.crew.Insert(DemoActor("Britney", "jasmine", "jeans@blue,raincoatblue,schoolbackpack@red,beretred", "", "baseballbat", -1));
			f.crew.Insert(DemoActor("Mika", "def3", "workpants,sweater@green,improvisedbag,ushanka", "glockpistol", "", -1));
			f.Add("walk", 20);
			f.Add("attack", 20, "", "hurt");
			f.Add("walk", 15);
			f.Add("ration", 35);
			f.Add("walk", 15);
			return f;
		}
		if (pick == 3)
		{
			f.car = "truck@blue";
			f.km = 70;
			f.biomeTo = "field";
			f.crew.Insert(DemoActor("Derk", "veteran", "orelpants,oreljacket,razgruz,hunterbackpack,helmetarmy", "akmrifle", "", 0));
			f.crew.Insert(DemoActor("OrionKaelin", "biker", "jeans@black,downjacket@red,taloonbackpack@orange,warmhat@black", "", "sksrifle", 0));
			f.Add("drive", 20);
			f.Add("flat", 20, "OrionKaelin", "spare");
			f.Add("drive", 15);
			f.Add("flat", 35, "Derk", "kit");
			f.Add("drive", 35);
			return f;
		}
		if (pick == 1)
		{
			f.km = 160;
			f.biomeTo = "desert";
			f.crew.Insert(DemoActor("Derk", "def1", "gorkapants,gorkajacket,vest,mountainbackpack,helmetmoto@blue", "ak74rifle", "baseballbat", 0));
			f.crew.Insert(DemoActor("OrionKaelin", "redneck", "jeans@blue,hoodiegray@red,hunterbackpack@green,cowboyhat", "", "mosinnagant", 0));
			f.crew.Insert(DemoActor("Britney", "kate", "sportpants@black,hoodiegray@pink,schoolbackpack@yellow,warmhat@lightblue", "fireaxe", "", 0));
			f.Add("drive", 20);
			f.Add("flat", 20, "OrionKaelin", "spare");
			f.Add("drive", 15);
			f.Add("flat", 35, "Britney", "kit");
			f.Add("drive", 15);
			f.Add("fuel", 50, "Derk", "found");
			f.Add("drive", 15);
			f.Add("fuel", 65, "OrionKaelin", "lost");
			f.Add("walk", 15);
			f.Add("attack", 80, "", "hurt");
			f.Add("walk", 15);
			f.Add("ration", 95);
			f.Add("walk", 15);
			f.Add("attack", 110, "", "died");
			f.Add("walk", 15);
			f.Add("died", 125, "", "hunger");
			f.Add("walk", 15);
			return f;
		}
		f.car = "sedan@wine";
		f.km = 60;
		f.biomeTo = "snow";
		f.crew.Insert(DemoActor("Derk", "survivalist", "workpants@brown,tshirt@orange,taloonbackpack@green,baseballcap@red", "glockpistol", "crowbar", 0));
		f.Add("drive", 25);
		f.Add("flat", 25, "Derk", "none");
		f.Add("walk", 15);
		f.Add("attack", 40, "Derk", "hurt");
		f.Add("walk", 20);
		return f;
	}
}
