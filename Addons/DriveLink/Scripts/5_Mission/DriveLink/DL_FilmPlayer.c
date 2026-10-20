// DriveLink road film - the player (client). Plays a DL_Film (DL_FilmData.c) top-down like MiniDayZ: the ground of
// the source biome scrolls up and blends into the target's, roadside props and wrecks pass by, the car drives down
// the middle of the road; every road event is a scene (flat tyre, fuel, walking, infected, rationing, death).
// Engine: code-made widgets (spike results in Research/road-animation-research.md section 7): sprite frames by
// CLIPPING (DL_Sprite), scrolling by sliding tiles (DL_Strip), everything moved from MissionGameplay.OnUpdate.
// Art: ground + props = DriveLink's own; cars, wrecks, survivors, gear, infected = MiniDayZ (prototype only).

// a survivor (or an infected): one sprite per layer, all on the same 32x32 cell
class DL_FilmPerson
{
	static const string CHR = "DriveLink/gui/anim/chr/chr_";
	string name;
	ref array<ref DL_Sprite> layers = new array<ref DL_Sprite>;
	float x;                  // film pixels (ground tile pixels, 0..256 across the road)
	float y;
	int dir;                  // 0 down, 1 up, 2 right, 3 left
	bool running;
	bool attacking;           // infected only (rows 8-11)
	bool down;                // lying (dead / knocked down): no animation, turned 90 degrees
	bool shown;
	bool zed;
	float animT;

	void DL_FilmPerson(Widget parent, int sort, float px, DL_FilmActor a, string zedSkin)
	{
		float size = 32 * px;
		if (zedSkin != "")
		{
			zed = true;
			layers.Insert(new DL_Sprite(parent, CHR + zedSkin + "_ca.paa", sort, size, size, 128, 512));
			return;
		}
		name = a.name;
		layers.Insert(new DL_Sprite(parent, CHR + a.skin + "_ca.paa", sort, size, size, 128, 256));
		foreach (string l : a.layers)
			layers.Insert(new DL_Sprite(parent, CHR + l + "_ca.paa", sort, size, size, 128, 256));
	}

	void Show(bool on)
	{
		shown = on;
		foreach (DL_Sprite s : layers)
			s.Show(on);
	}

	void Update(float dt, float px, float ox, float oy)
	{
		if (!shown)
			return;
		animT += dt;
		int row = dir;
		if (running)
			row = dir + 4;
		if (attacking && zed)
			row = 8 + dir;
		int frame = 0;
		if (!down)
		{
			float fps = 4;
			if (running)
				fps = 10;
			frame = Math.Floor(animT * fps);
			frame = frame % 4;
		}
		if (down)
			row = 0;
		foreach (DL_Sprite s : layers)
		{
			s.Cell(frame * 32, row * 32, 32, 32);
			s.MoveTo(ox + (x - 16) * px, oy + (y - 16) * px);
			if (down)
				s.Turn(90);
			else
				s.Turn(0);
		}
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

	void DL_FilmThing(Widget parent, int sort, string tex, float px, float cw, float ch, float texW, float texH)
	{
		w = cw;
		h = ch;
		sprite = new DL_Sprite(parent, tex, sort, cw * px, ch * px, texW, texH);
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

	protected ref DL_Film m_Film;
	protected Widget m_Root;
	protected ref DL_Strip m_GroundA;
	protected ref DL_Strip m_GroundB;
	protected ref DL_Sprite m_Car;
	protected float m_CarX;
	protected float m_CarY;       // film pixels (car centre)
	protected bool m_CarInWorld;  // left behind: it scrolls away with the ground
	protected ref array<ref DL_FilmPerson> m_Crew = new array<ref DL_FilmPerson>;
	protected ref DL_FilmPerson m_Zed;
	protected ref array<ref DL_FilmThing> m_Props = new array<ref DL_FilmThing>;
	protected ref array<ref DL_FilmThing> m_Wrecks = new array<ref DL_FilmThing>;
	protected ref DL_FilmThing m_Tool;    // wheel or fuel tank in a scene
	protected ref DL_FilmThing m_Puff;
	protected ref DL_FilmThing m_Fuel;    // MiniDayZ fuel tank, carried back in the fuel scene
	protected TextWidget m_Caption;
	protected float m_Px;         // screen pixels per film pixel
	protected float m_Ox;         // window origin of the film area
	protected float m_Oy;
	protected float m_ViewH;      // film pixels visible top to bottom
	protected float m_T;          // time since start
	protected float m_SceneT;     // time in the current scene
	protected int m_Scene;
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
		m_Tool = null;
		m_Puff = null;
		m_Fuel = null;
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
		m_GroundA = new DL_Strip(m_Root, Ground(m_Film.biomeFrom), 1, m_Ox, m_Oy, gw, gh, 256, 256, true);
		if (m_Film.biomeTo != m_Film.biomeFrom && m_Film.biomeTo != "")
		{
			m_GroundB = new DL_Strip(m_Root, Ground(m_Film.biomeTo), 2, m_Ox, m_Oy, gw, gh, 256, 256, true);
			m_GroundB.SetAlpha(0);
		}
		// roadside props and wrecks: a pool that is recycled as they scroll off the top
		for (int i = 0; i < 16; i++)
			m_Props.Insert(new DL_FilmThing(m_Root, 3, DIR + "props_ca.paa", m_Px, 64, 64, 256, 256));
		array<string> wrecks = {"wreck_blue_ca", "wreck_gray_ca", "wreck_green_ca", "wreck_red_ca", "wreck_police_ca"};
		foreach (string wr : wrecks)
		{
			DL_FilmThing wt = new DL_FilmThing(m_Root, 4, DIR + wr + ".paa", m_Px, 60, 76, 128, 128);
			wt.sprite.Cell(4, 0, 60, 76);
			m_Wrecks.Insert(wt);
		}
		m_Tool = new DL_FilmThing(m_Root, 7, DIR + "props_ca.paa", m_Px, 64, 64, 256, 256);
		m_Puff = new DL_FilmThing(m_Root, 9, DIR + "smoke_ca.paa", m_Px, 28, 28, 16, 16);
		m_Puff.sprite.Cell(0, 0, 14, 14);
		m_Fuel = new DL_FilmThing(m_Root, 9, DIR + "fueltank_ca.paa", m_Px, 22, 22, 16, 16);
		m_Fuel.sprite.Cell(0, 0, 12, 12);
		// the car, in the middle of the road
		m_CarX = 128;
		m_CarY = m_ViewH * 0.36;
		m_CarInWorld = false;
		if (m_Film.car == "volga")
		{
			m_Car = new DL_Sprite(m_Root, DIR + "volga_ca.paa", 6, 62 * m_Px, 80 * m_Px, 256, 256);
			m_Car.Cell(178, 142, 62, 80);
		}
		else if (m_Film.car != "")
		{
			m_Car = new DL_Sprite(m_Root, DIR + "uaz_ca.paa", 6, 68 * m_Px, 74 * m_Px, 256, 256);
			m_Car.Cell(176, 80, 68, 74);
		}
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
		ImageWidget band = DL_FilmSpike.Image(m_Root, DL_FilmSpike.PLAIN, ARGB(150, 0, 0, 0), 10, 0, winH - winH * 0.075, winW, winH * 0.075);
		m_Caption = TextWidget.Cast(ws.CreateWidget(TextWidgetTypeID, 0, winH - winH * 0.07, winW, winH * 0.065, flags, ARGB(255, 240, 225, 190), 11, m_Root));
		// a few props from the start, so the road isn't empty
		m_NextProp = 0;
		m_NextWreck = 1.5;
		for (int k = 0; k < 8; k++)
			SpawnProp(Math.RandomFloat(0, m_ViewH));
		m_Speed = 0;
		return true;
	}

	// ---------------- scenes ----------------

	protected DL_FilmEvent Ev()
	{
		if (m_Scene < 0 || m_Scene >= m_Film.events.Count())
			return null;
		return m_Film.events[m_Scene];
	}

	protected void NextScene()
	{
		m_Scene++;
		m_SceneT = 0;
		DL_FilmEvent e = Ev();
		if (!e)
		{
			Caption("");
			m_Target = 0;
			return;
		}
		Caption(Line(e));
		if (e.kind == "drive")
		{
			m_Target = 150;
			HideCrew();
		}
		else if (e.kind == "walk")
		{
			m_Target = 28;
			WalkFormation();
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

	protected void Caption(string t)
	{
		if (m_Caption)
			m_Caption.SetText(t);
	}

	protected DL_FilmPerson Who(string name)
	{
		foreach (DL_FilmPerson p : m_Crew)
		{
			if (p.name == name && !p.down)
				return p;
		}
		foreach (DL_FilmPerson q : m_Crew)
		{
			if (!q.down)
				return q;
		}
		return null;
	}

	protected void HideCrew()
	{
		foreach (DL_FilmPerson p : m_Crew)
		{
			if (!p.down)
				p.Show(false);
		}
	}

	// out of the car: driver to the left side, the others to the right
	protected void GetOut()
	{
		int i = 0;
		foreach (DL_FilmPerson p : m_Crew)
		{
			if (p.down)
				continue;
			float side = -28;
			if (i > 0)
				side = 28;
			p.x = m_CarX + side;
			p.y = m_CarY - 10 + (i / 2) * 14;
			p.dir = 0;
			p.running = false;
			p.Show(true);
			i++;
		}
	}

	protected void WalkFormation()
	{
		int i = 0;
		foreach (DL_FilmPerson p : m_Crew)
		{
			if (p.down)
				continue;
			p.Show(true);
			int col = i % 2;
			int rowN = i / 2;
			p.x = 114 + col * 28;
			p.y = m_ViewH * 0.40 + rowN * 22;
			p.dir = 0;
			p.running = true;
			i++;
		}
		if (m_Car && !m_CarInWorld)
			m_CarInWorld = true;  // the car (if any) is left behind and scrolls away
	}

	protected void MoveTowards(DL_FilmPerson p, float tx, float ty, float speed, float dt)
	{
		float dx = tx - p.x;
		float dy = ty - p.y;
		float d = Math.Sqrt(dx * dx + dy * dy);
		if (d < 1)
		{
			p.running = false;
			return;
		}
		float step = Math.Min(d, speed * dt);
		p.x += dx / d * step;
		p.y += dy / d * step;
		p.running = true;
		if (Math.AbsFloat(dx) > Math.AbsFloat(dy))
		{
			if (dx > 0)
				p.dir = 2;
			else
				p.dir = 3;
		}
		else if (dy > 0)
			p.dir = 0;
		else
			p.dir = 1;
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

	// one step of the current scene's choreography
	protected void Scene(float dt)
	{
		DL_FilmEvent e = Ev();
		if (!e)
			return;
		float st = m_SceneT;
		float len = DL_Film.SceneSeconds(e);
		if (e.kind == "drive")
			m_Km += e.km / len * dt;
		else if (e.kind == "walk")
		{
			m_Km += e.km / len * dt;
			foreach (DL_FilmPerson w : m_Crew)
			{
				if (!w.down)
					w.running = true;
			}
		}
		else if (e.kind == "flat")
		{
			m_Target = 0;
			if (st < 0.1)
			{
				m_Puff.x = m_CarX + 24;
				m_Puff.y = m_CarY + 20;
				m_Puff.sprite.Show(true);
			}
			if (st > 1.2 && st < 1.3)
				GetOut();
			DL_FilmPerson fixer = Who(e.who);
			if (fixer && st > 1.3 && st < 5)
			{
				MoveTowards(fixer, m_CarX + 30, m_CarY + 22, 40, dt);
				if (!fixer.running)
				{
					fixer.dir = 3;
					if (e.detail == "spare")
						ShowTool(P_WHEEL, m_CarX + 44, m_CarY + 30);
					else if (e.detail == "kit")
						ShowTool(P_CRATE, m_CarX + 44, m_CarY + 30);
				}
			}
			if (st > 5)
			{
				m_Tool.sprite.Show(false);
				m_Puff.sprite.Show(false);
				if (e.detail == "none")
				{
					// stands around the dead car; the next scene (walk) leaves it behind
					foreach (DL_FilmPerson s : m_Crew)
					{
						if (!s.down)
							s.dir = 0;
					}
				}
				else if (st > 5.8)
				{
					HideCrew();
					m_Target = 150;
				}
			}
		}
		else if (e.kind == "fuel")
		{
			m_Target = 0;
			if (st > 1.0 && st < 1.1)
				GetOut();
			DL_FilmPerson seeker = Who(e.who);
			if (seeker)
			{
				if (st > 1.2 && st < 3.4)
					MoveTowards(seeker, 300, m_CarY - 40, 70, dt);  // off the right edge, searching
				else if (st >= 3.4 && st < 5.6)
				{
					MoveTowards(seeker, m_CarX + 30, m_CarY - 4, 70, dt);
					if (e.detail == "found")
					{
						m_Fuel.sprite.Show(true);
						m_Fuel.x = seeker.x + 10;
						m_Fuel.y = seeker.y + 6;
					}
				}
				else if (st >= 5.6)
				{
					seeker.dir = 3;
					seeker.running = false;
				}
			}
			if (st > 6.2 && e.detail == "found")
			{
				m_Fuel.sprite.Show(false);
				HideCrew();
				m_Target = 150;
			}
		}
		else if (e.kind == "attack")
		{
			DL_FilmPerson victim = Who(e.who);
			if (st < 0.1)
			{
				m_Zed.x = -20;
				m_Zed.y = m_ViewH * 0.42;
				m_Zed.down = false;
				m_Zed.Show(true);
			}
			if (victim)
			{
				if (st < 1.6)
				{
					MoveTowards(m_Zed, victim.x - 14, victim.y, 80, dt);
					m_Zed.attacking = false;
				}
				else if (st < 3.2)
				{
					m_Zed.running = false;
					m_Zed.attacking = true;
					m_Zed.dir = 2;
					victim.dir = 3;
					victim.running = false;
				}
				else
				{
					m_Zed.attacking = false;
					if (e.detail == "died")
					{
						victim.down = true;
						MoveTowards(m_Zed, -40, m_Zed.y - 20, 60, dt);   // it wanders off
					}
					else
						m_Zed.down = true;                               // fought off: it lies on the road
				}
			}
		}
		else if (e.kind == "died")
		{
			DL_FilmPerson weak = Who(e.who);
			if (weak && st > 1)
				weak.down = true;
		}
		if (m_SceneT >= len)
			NextScene();
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
			m_NextWreck -= dt * (m_Speed / 150);
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
			float bob = 0;
			if (m_Speed > 20 && !m_CarInWorld)
				bob = Math.Sin(m_T * 18) * 0.5;
			float cw = 68;
			float ch = 74;
			if (m_Film.car == "volga")
			{
				cw = 62;
				ch = 80;
			}
			m_Car.MoveTo(m_Ox + (m_CarX - cw / 2 + bob) * m_Px, m_Oy + (m_CarY - ch / 2) * m_Px);
		}
		foreach (DL_FilmPerson p : m_Crew)
		{
			if (p.down)
				p.y -= dy;   // left lying on the road, it scrolls away
			p.Update(dt, m_Px, m_Ox, m_Oy);
		}
		if (m_Zed)
		{
			if (m_Zed.down)
				m_Zed.y -= dy;
			m_Zed.Update(dt, m_Px, m_Ox, m_Oy);
		}
		if (m_Tool)
			m_Tool.Place(m_Px, m_Ox, m_Oy);
		if (m_Fuel)
			m_Fuel.Place(m_Px, m_Ox, m_Oy);
		if (m_Puff)
		{
			m_Puff.y -= dt * 8;
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
					t.x = 52;      // right at the edge of the shoulder
				else
					t.x = 204;
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

	static DL_Film Demo()
	{
		DL_Film f = new DL_Film();
		f.car = "uaz";
		f.km = 80;
		f.biomeFrom = "forest";
		f.biomeTo = "desert";
		array<string> names = {"Derk", "OrionKaelin", "Britney"};
		array<string> skins = {"def1", "redneck", "kate"};
		for (int i = 0; i < 3; i++)
		{
			DL_FilmActor a = new DL_FilmActor();
			a.name = names[i];
			a.skin = skins[i];
			f.crew.Insert(a);
		}
		f.crew[0].layers.Insert("gorkapants");
		f.crew[0].layers.Insert("gorkajacket");
		f.crew[0].layers.Insert("vest");
		f.crew[0].layers.Insert("mountainbackpack");
		f.crew[0].layers.Insert("helmetarmy");
		f.crew[0].layers.Insert("ak74rifle");
		f.crew[1].layers.Insert("jeans");
		f.crew[1].layers.Insert("hunterjacket");
		f.crew[1].layers.Insert("hunterbackpack");
		f.crew[1].layers.Insert("cowboyhat");
		f.crew[1].layers.Insert("mosinnagant");
		f.crew[2].layers.Insert("sportpants");
		f.crew[2].layers.Insert("hoodiered");
		f.crew[2].layers.Insert("schoolbackpack");
		f.crew[2].layers.Insert("fireaxe");
		f.Add("drive", 30);
		f.Add("flat", 30, "OrionKaelin", "spare");
		f.Add("drive", 20);
		f.Add("fuel", 50, "Britney", "lost");
		f.Add("walk", 30);
		f.Add("attack", 65, "Derk", "hurt");
		f.Add("ration", 80);
		return f;
	}
}
