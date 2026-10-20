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
	float homeX;                   // its car door
	float homeY;
	float animT;
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

	void Update(float dt, float px, float ox, float oy)
	{
		if (!shown)
			return;
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

	protected static int s_DemoN;

	protected ref DL_Film m_Film;
	protected Widget m_Root;
	protected ref DL_Strip m_GroundA;
	protected ref DL_Strip m_GroundB;
	protected ref DL_Sprite m_Car;
	protected ref DL_Sprite m_CarGlass;   // untinted windows over the tinted body
	protected float m_CarW;       // shown size, film px (with its shadow)
	protected float m_CarH;
	protected bool m_DriveOff;    // the end: the car drives out of the bottom
	protected float m_CarV;
	protected bool m_Leaving;     // walking round the car they leave behind
	protected float m_WalkY;      // the walkers' first row
	protected float m_CarX;
	protected float m_CarY;       // film pixels (car centre)
	protected bool m_CarInWorld;  // left behind: it scrolls away with the ground
	protected bool m_InCar;       // the crew sits in the car (hidden)
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
	protected bool m_Ending;      // after the last scene: the crew walks off the bottom
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
		if (!Build())
			return;
		m_Playing = true;
		m_T = 0;
		m_Scene = -1;
		m_Ending = false;
		NextScene();
		Print(DL_Const.LOG_TAG + "road film: " + f.events.Count().ToString() + " scenes, " + f.crew.Count().ToString() + " in the crew, " + f.biomeFrom + " -> " + f.biomeTo + ", car '" + f.car + "'");
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
		m_Zed = null;
		m_Actor = null;
		m_Car = null;
		m_CarGlass = null;
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
		float winH = sh * 0.62;
		float winW = Math.Min(sw * 0.62, winH * 1.15);
		float winX = (sw - winW) / 2;
		float winY = sh * 0.035;
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
		m_GroundA = new DL_Strip(m_Root, Ground(m_Film.biomeFrom), 1, m_Ox, m_Oy, gw, gh, 256, 256, true);
		m_GroundB = null;
		if (m_Film.biomeTo != m_Film.biomeFrom && m_Film.biomeTo != "")
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
		// the car, in the middle of the road
		m_CarX = 128;
		m_CarY = m_ViewH * 0.36;
		m_CarInWorld = false;
		m_Car = null;
		m_CarGlass = null;
		m_DriveOff = false;
		m_Leaving = false;
		m_CarV = 0;
		m_WalkY = m_ViewH * 0.40;
		BuildCar();
		m_InCar = m_Car != null;
		// the crew: in the car (hidden) or walking from the start
		foreach (DL_FilmActor a : m_Film.crew)
		{
			DL_FilmPerson p = new DL_FilmPerson(m_Root, 8, m_Px, a, "");
			p.Show(false);
			m_Crew.Insert(p);
		}
		m_Zed = new DL_FilmPerson(m_Root, 8, m_Px, null, "zednormalskin1");
		m_Zed.Show(false);
		int flags = DL_FilmSpike.Flags() | WidgetFlags.CENTER | WidgetFlags.VCENTER;
		DL_FilmSpike.Image(m_Root, DL_FilmSpike.PLAIN, ARGB(150, 0, 0, 0), 10, 0, winH - winH * 0.075, winW, winH * 0.075);
		m_Caption = TextWidget.Cast(ws.CreateWidget(TextWidgetTypeID, 0, winH - winH * 0.07, winW, winH * 0.065, flags, ARGB(255, 240, 225, 190), 11, m_Root));
		// a few props from the start, so the road isn't empty
		m_NextProp = 0;
		m_NextWreck = 1.5;
		for (int k = 0; k < 8; k++)
			SpawnProp(Math.RandomFloat(0, m_ViewH));
		m_Speed = 0;
		return true;
	}

	// the car the crew drives: DL_FilmCast.CarSprite "look@colour" -> a grey body sheet tinted with the colour and the
	// glass sheet over it (Tools/build-anim-assets.py car_sprites); "uaz" / "volga" = the old MiniDayZ sprites as they are
	protected void BuildCar()
	{
		string look = m_Film.car;
		if (look == "")
			return;
		string colour = "";
		int at = look.IndexOf("@");
		if (at > 0)
		{
			colour = look.Substring(at + 1, look.Length() - at - 1);
			look = look.Substring(0, at);
		}
		if (look == "volga")
		{
			SetCar(DIR + "volga_ca.paa", "", 256, 256, 178, 142, 62, 80, 1, 1, "");
			return;
		}
		if (look == "uaz")
		{
			SetCar(DIR + "uaz_ca.paa", "", 256, 256, 176, 80, 68, 74, 1, 1, "");
			return;
		}
		if (look == "sedan")              // Olga 24 (Volga GAZ-24)
			SetCar(DIR + "car_sedanw_ca.paa", DIR + "car_sedang_ca.paa", 64, 128, 0, 0, 62, 80, 1, 1, colour);
		else if (look == "sarka")         // Sarka 120 (Skoda 120): a bit smaller
			SetCar(DIR + "car_sedanw_ca.paa", DIR + "car_sedang_ca.paa", 64, 128, 0, 0, 62, 80, 0.94, 0.92, colour);
		else if (look == "gunter")        // Gunter 2 (Golf): a short hatchback
			SetCar(DIR + "car_sedanw_ca.paa", DIR + "car_sedang_ca.paa", 64, 128, 0, 0, 62, 80, 0.94, 0.82, colour);
		else if (look == "humvee")        // M1025: wide and long
			SetCar(DIR + "car_4x4w_ca.paa", DIR + "car_4x4g_ca.paa", 128, 128, 0, 0, 68, 74, 1.22, 1.08, colour);
		else if (look == "truck")         // M3S
			SetCar(DIR + "car_truckw_ca.paa", DIR + "car_truckg_ca.paa", 128, 256, 0, 0, 72, 136, 1, 1, colour);
		else                              // Ada 4x4 (Niva) and anything else
			SetCar(DIR + "car_4x4w_ca.paa", DIR + "car_4x4g_ca.paa", 128, 128, 0, 0, 68, 74, 0.92, 0.92, colour);
	}

	protected void SetCar(string tex, string glass, float texW, float texH, float cx, float cy, float cw, float ch, float sx, float sy, string colour)
	{
		m_CarW = cw * sx;
		m_CarH = ch * sy;
		m_Car = new DL_Sprite(m_Root, tex, 6, m_CarW * m_Px, m_CarH * m_Px, texW, texH);
		m_Car.Cell(cx, cy, cw, ch);
		if (colour != "")
			m_Car.SetColor(CarTint(colour));
		if (glass != "")
		{
			m_CarGlass = new DL_Sprite(m_Root, glass, 6, m_CarW * m_Px, m_CarH * m_Px, texW, texH);
			m_CarGlass.Cell(cx, cy, cw, ch);
		}
	}

	// DayZ car colours (class name suffixes) as paint over the grey body
	static int CarTint(string c)
	{
		if (c == "green") return ARGB(255, 92, 124, 74);
		if (c == "olive") return ARGB(255, 104, 114, 72);
		if (c == "blue") return ARGB(255, 62, 92, 156);
		if (c == "white") return ARGB(255, 236, 236, 230);
		if (c == "black") return ARGB(255, 58, 58, 64);
		if (c == "wine") return ARGB(255, 124, 32, 46);
		if (c == "yellow") return ARGB(255, 226, 186, 52);
		if (c == "grey" || c == "gray") return ARGB(255, 142, 142, 142);
		if (c == "red") return ARGB(255, 178, 46, 40);
		if (c == "orange") return ARGB(255, 212, 112, 42);
		if (c == "beige") return ARGB(255, 192, 172, 128);
		return DL_FilmPerson.Tint(c);
	}

	// ---------------- helpers ----------------

	// half the car body (without its shadow strip on the right)
	protected float CarHalfW()
	{
		return m_CarW / 2 - 3;
	}

	protected float CarHalfH()
	{
		return m_CarH / 2 - 2;
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

	protected void HideCrew()
	{
		foreach (DL_FilmPerson p : m_Crew)
		{
			p.path.Clear();
			p.pose = DL_FilmPerson.NORMAL;
			if (!p.down)
				p.Show(false);
		}
		m_InCar = m_Car != null && !m_CarInWorld;
	}

	// out of the car: each by his door (driver front left, then front right, rear left, rear right)
	protected void GetOut()
	{
		int i = 0;
		foreach (DL_FilmPerson p : m_Crew)
		{
			if (p.down)
				continue;
			int side = -1;
			if (i % 2 == 1)
				side = 1;
			int rowN = i / 2;
			p.homeX = m_CarX + side * (CarHalfW() + 9);
			p.homeY = m_CarY + 8 - rowN * 20;
			p.x = p.homeX;
			p.y = p.homeY;
			p.dir = 0;
			p.path.Clear();
			p.marching = false;
			p.pose = DL_FilmPerson.NORMAL;
			p.Show(true);
			i++;
		}
		m_InCar = false;
	}

	// walk to (side of the car, y) without walking through the car: round its front or back when crossing
	protected void RouteAround(DL_FilmPerson p, int toSide, float toY)
	{
		float sideX = m_CarX + toSide * (CarHalfW() + 9);
		int fromSide = -1;
		if (p.x > m_CarX)
			fromSide = 1;
		if (fromSide != toSide)
		{
			bool clear = p.y < m_CarY - CarHalfH() - 4 || p.y > m_CarY + CarHalfH() + 4;
			if (clear)
				p.Go(sideX, p.y);
			else
			{
				float endY = m_CarY - CarHalfH() - 10;
				if (p.y + toY > 2 * m_CarY)
					endY = m_CarY + CarHalfH() + 10;
				p.Go(p.x, endY);
				p.Go(sideX, endY);
			}
		}
		p.Go(sideX, toY);
	}

	// everyone back to his door
	protected void Board()
	{
		foreach (DL_FilmPerson p : m_Crew)
		{
			if (p.down || !p.shown)
				continue;
			p.pose = DL_FilmPerson.NORMAL;
			int side = -1;
			if (p.homeX > m_CarX)
				side = 1;
			RouteAround(p, side, p.homeY);
		}
	}

	// all at their doors: in they get
	protected bool Boarded()
	{
		foreach (DL_FilmPerson p : m_Crew)
		{
			if (!p.down && p.shown && p.Busy())
				return false;
		}
		HideCrew();
		return true;
	}

	protected void Formation(bool jump)
	{
		int i = 0;
		foreach (DL_FilmPerson p : m_Crew)
		{
			if (p.down)
				continue;
			p.Show(true);
			p.pose = DL_FilmPerson.NORMAL;
			int col = i % 2;
			int rowN = i / 2;
			float fx = 114 + col * 28;
			float fy = m_WalkY + rowN * 22;
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
		m_InCar = false;
		if (m_Car && !m_CarInWorld)
			m_CarInWorld = true;  // the car (if any) is left behind and scrolls away
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
		m_Actor = null;
		foreach (DL_FilmPerson c : m_Crew)
			c.pose = DL_FilmPerson.NORMAL;
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
		// who the scene is about (a random living survivor if the name is unknown)
		if (e.kind == "flat" || e.kind == "fuel" || e.kind == "attack" || e.kind == "died")
		{
			m_Actor = Who(e.who);
			if (m_Actor)
				e.who = m_Actor.name;
		}
		Caption(Line(e));
		if (e.kind == "drive")
		{
			m_Target = DRIVE;
			HideCrew();
		}
		else if (e.kind == "walk")
		{
			if (m_Car && !m_CarInWorld)
				LeaveCar();
			else
			{
				m_Target = WALK;
				Formation(true);
			}
		}
	}

	// the car stays (flat, no fuel): they walk down its side and form up in front of it, then set off on foot
	protected void LeaveCar()
	{
		if (m_InCar)
			GetOut();
		m_Target = 0;
		m_Leaving = true;
		m_WalkY = m_CarY + CarHalfH() + 16;
		int i = 0;
		foreach (DL_FilmPerson p : m_Crew)
		{
			if (p.down)
				continue;
			p.pose = DL_FilmPerson.NORMAL;
			p.marching = false;
			p.path.Clear();
			int side = -1;
			if (p.x > m_CarX)
				side = 1;
			float sideX = m_CarX + side * (CarHalfW() + 9);
			if (Math.AbsFloat(p.x - sideX) > 2 && p.y > m_CarY - CarHalfH() - 4 && p.y < m_CarY + CarHalfH() + 4)
				RouteAround(p, side, p.y);       // somewhere odd next to the car: back to its side first
			p.Go(sideX, m_WalkY - 4);
			int col = i % 2;
			int rowN = i / 2;
			p.Go(114 + col * 28, m_WalkY + rowN * 22);
			i++;
		}
	}

	protected string Line(DL_FilmEvent e)
	{
		string who = e.who;
		if (m_Film.crew.Count() == 1)
			who = "You";
		string km = Math.Round(e.km).ToString() + " km";
		if (e.kind == "drive")
			return "On the road: " + km;
		if (e.kind == "walk")
			return "On foot: " + km;
		if (e.kind == "flat" && e.detail == "spare")
			return "Blew a tyre. " + who + " put on the spare.";
		if (e.kind == "flat" && e.detail == "kit")
			return "Blew a tyre. " + who + " patched it.";
		if (e.kind == "flat")
			return "Blew a tyre - no spare. The car stays here.";
		if (e.kind == "fuel" && e.detail == "found")
			return "Out of gas. " + who + " found some fuel!";
		if (e.kind == "fuel")
			return "Out of gas. Nothing found: on foot from here.";
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
				m_CarInWorld = true;              // left behind: it scrolls away
				m_Target = WALK;
			}
		}
		else if (e.kind == "drive" || e.kind == "walk")
			m_Km += e.km / len * dt;
		else if (e.kind == "flat")
			SceneFlat(e);
		else if (e.kind == "fuel")
			SceneFuel(e);
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

	// stopped: true once the car (or the walkers) stand still
	protected bool Halted()
	{
		return m_Speed < 1 || m_StepT > 2;
	}

	// flat tyre: a random wheel blows, the car stops, the fixer walks round the car to it, works, they get back in
	protected void SceneFlat(DL_FilmEvent e)
	{
		float hw = CarHalfW();
		float hh = CarHalfH();
		if (m_Step == 0)
		{
			m_Target = 0;
			int wheel = Math.RandomInt(0, 4);   // 0 front left, 1 front right, 2 rear left, 3 rear right (screen)
			m_WheelSide = -1;
			if (wheel % 2 == 1)
				m_WheelSide = 1;
			m_WheelY = m_CarY + hh - 15;     // the car drives down: its front is at the bottom
			if (wheel >= 2)
				m_WheelY = m_CarY - hh + 15;
			m_Puff.x = m_CarX + m_WheelSide * (hw - 3);
			m_Puff.y = m_WheelY;
			m_Puff.sprite.Show(true);
			NextStep();
		}
		else if (m_Step == 1 && m_StepT > 0.8 && Halted())
		{
			GetOut();
			if (m_Actor)
				RouteAround(m_Actor, m_WheelSide, m_WheelY);
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
					if (e.detail == "spare")
						ShowTool(P_WHEEL, m_Actor.x + m_WheelSide * 9, m_Actor.y + 11);
					else if (e.detail == "kit")
						ShowTool(P_CRATE, m_Actor.x + m_WheelSide * 9, m_Actor.y + 11);
				}
				foreach (DL_FilmPerson o : m_Crew)
				{
					if (o != m_Actor && !o.down)
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
			if (m_StepT > work)
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
					Board();
				}
				NextStep();
			}
		}
		else if (m_Step == 4 && e.detail != "none")
		{
			if (Boarded())
			{
				m_Target = DRIVE;
				NextStep();
			}
		}
	}

	// out of gas: one goes off to search, comes back (with a can: he fills up, they drive on)
	protected void SceneFuel(DL_FilmEvent e)
	{
		float hw = CarHalfW();
		float hh = CarHalfH();
		float topY = m_CarY - hh - 14;
		if (m_Step == 0)
		{
			m_Target = 0;
			NextStep();
		}
		else if (m_Step == 1 && m_StepT > 0.8 && Halted())
		{
			GetOut();
			if (m_Actor)
			{
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
				m_Actor.Go(m_CarX + hw + 9, topY);
				m_Actor.Go(m_CarX + hw + 9, m_CarY - 6); // the filler cap
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
			Board();
			NextStep();
		}
		else if (m_Step == 6)
		{
			if (Boarded())
			{
				m_Target = DRIVE;
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
			if (m_InCar)
				GetOut();
			if (!v)
			{
				NextStep();
				return;
			}
			m_ZedSide = -1;
			if (Math.RandomFloat01() < 0.5)
				m_ZedSide = 1;
			m_Zed.down = false;
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
				if (o != v && !o.down)
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
			if (m_Car && !m_CarInWorld)
				Board();
			else
				Formation(false);
			NextStep();
		}
		else if (m_Step == 5)
		{
			if (m_Car && !m_CarInWorld)
			{
				if (Boarded())
				{
					m_Target = DRIVE;
					NextStep();
				}
				return;
			}
			foreach (DL_FilmPerson w : m_Crew)
			{
				if (!w.down && w.Busy())
					return;
			}
			foreach (DL_FilmPerson m : m_Crew)
			{
				if (!m.down)
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

	// after the last scene: the road stops, they get out and walk off the bottom
	protected void SceneEnd()
	{
		if (m_Step == 0)
		{
			m_Target = 0;
			foreach (DL_FilmPerson p : m_Crew)
				p.marching = false;
			NextStep();
		}
		else if (m_Step == 1 && m_InCar && m_Car && !m_CarInWorld)
		{
			// still in the car: it drives on, out of the bottom of the window (the road stands still)
			m_DriveOff = true;
			m_Step = 3;
			m_StepT = 0;
		}
		else if (m_Step == 1 && Halted())
		{
			if (m_InCar)
				GetOut();
			foreach (DL_FilmPerson w : m_Crew)
			{
				if (w.down)
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
			if (gone || m_StepT > DL_Film.END_SECONDS + 1)
			{
				Print(DL_Const.LOG_TAG + "road film ended");
				Stop();
			}
		}
		else if (m_Step == 3)
		{
			if (m_CarY - m_CarH / 2 > m_ViewH || m_StepT > DL_Film.END_SECONDS + 1)
			{
				Print(DL_Const.LOG_TAG + "road film ended (drove off)");
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
			Print(DL_Const.LOG_TAG + "road film skipped");
			Stop();
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
		if (m_GroundB)
		{
			m_GroundB.Scroll(m_Scroll * m_Px);
			// the biome blend: from 30 % to 70 % of the way
			float prog = 0;
			if (m_Film.km > 0)
				prog = m_Km / m_Film.km;
			m_GroundB.SetAlpha(Math.Clamp((prog - 0.3) / 0.4, 0, 1));
		}
		// props and wrecks come up from the bottom
		if (m_Speed > 1)
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
		// the car, the people, the bits of a scene
		if (m_Car)
		{
			if (m_CarInWorld)
				m_CarY -= dy;
			if (m_DriveOff)
			{
				m_CarV = Math.Min(m_CarV + 110 * dt, 170);
				m_CarY += m_CarV * dt;
			}
			float bob = 0;
			if ((m_Speed > 20 || m_DriveOff) && !m_CarInWorld)
				bob = Math.Sin(m_T * 18) * 0.5;
			float carSx = m_Ox + (m_CarX - m_CarW / 2 + bob) * m_Px;
			float carSy = m_Oy + (m_CarY - m_CarH / 2) * m_Px;
			m_Car.MoveTo(carSx, carSy);
			if (m_CarGlass)
				m_CarGlass.MoveTo(carSx, carSy);
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

	protected string BiomeHere()
	{
		if (!m_GroundB)
			return m_Film.biomeFrom;
		float prog = 0;
		if (m_Film.km > 0)
			prog = m_Km / m_Film.km;
		if (Math.RandomFloat01() < Math.Clamp((prog - 0.3) / 0.4, 0, 1))
			return m_Film.biomeTo;
		return m_Film.biomeFrom;
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

	protected static DL_FilmActor DemoActor(string name, string skin, string gear, string hand, string back)
	{
		DL_FilmActor a = new DL_FilmActor();
		a.name = name;
		a.skin = skin;
		gear.Split(",", a.layers);
		a.hand = hand;
		a.back = back;
		return a;
	}

	// F9 alternates: A = a crew of three through every scene the road has (2 flats, fuel found, fuel lost, walking,
	// infected fought off, infected kill, rationing, hunger), B = a solo driver whose car is lost to a flat
	static DL_Film Demo()
	{
		s_DemoN++;
		DL_Film f = new DL_Film();
		f.car = "4x4@green";
		f.biomeFrom = "forest";
		if (s_DemoN % 2 == 1)
		{
			f.km = 160;
			f.biomeTo = "desert";
			f.crew.Insert(DemoActor("Derk", "def1", "gorkapants,gorkajacket,vest,mountainbackpack,helmetmoto@blue", "ak74rifle", "baseballbat"));
			f.crew.Insert(DemoActor("OrionKaelin", "redneck", "jeans@blue,hoodiegray@red,hunterbackpack@green,cowboyhat", "", "mosinnagant"));
			f.crew.Insert(DemoActor("Britney", "kate", "sportpants@black,hoodiegray@pink,schoolbackpack@yellow,warmhat@lightblue", "fireaxe", ""));
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
		f.crew.Insert(DemoActor("Derk", "survivalist", "workpants@brown,tshirt@orange,taloonbackpack@green,baseballcap@red", "glockpistol", "crowbar"));
		f.Add("drive", 25);
		f.Add("flat", 25, "Derk", "none");
		f.Add("walk", 15);
		f.Add("attack", 40, "Derk", "hurt");
		f.Add("walk", 20);
		return f;
	}
}
