// DriveLink road animation - ENGINE SPIKE (Research/road-animation-research.md, build order 1). Client only.
// F9 opens/closes a window that plays a test scene: the UAZ drives right, the road and the tree line scroll past
// (parallax), a survivor runs along the verge. Proves: .paa textures packed in the mod, frame picking from a sprite
// sheet by CLIPPING (a CLIPCHILDREN frame shows one cell of the whole sheet moved behind it; ImageWidget.SetUV
// would not take a script float[4][2]: "Incompatible parameter uv", 2026-10-20), endless scrolling by sliding copies
// of a tiling strip inside a clipping band, per-frame updates from MissionGameplay.OnUpdate. Debug feature: remove or gate before
// publishing. Art: road/trees = DriveLink's own; UAZ + skin = MiniDayZ, prototype only.

class DL_FilmSpike
{
	static const string DIR = "DriveLink/gui/anim/";
	static const string PLAIN = "#(argb,8,8,3)color(1,1,1,1,CO)";
	protected Widget m_Root;
	protected ref DL_Strip m_Trees;
	protected ref DL_Strip m_Road;
	protected ref DL_Sprite m_Car;
	protected ref DL_Sprite m_Walker;
	protected bool m_Open;
	protected bool m_KeyWasDown;
	protected float m_T;
	protected float m_W;
	protected float m_H;
	protected float m_RoadTop;
	protected float m_RoadH;

	void Update(float dt)
	{
		bool down = KeyState(KeyCode.KC_F9) != 0;
		if (down && !m_KeyWasDown)
			Toggle();
		m_KeyWasDown = down;
		if (m_Open)
			Animate(dt);
	}

	void Close()
	{
		if (m_Root)
			m_Root.Unlink();
		m_Root = null;
		m_Open = false;
	}

	protected void Toggle()
	{
		if (m_Open)
		{
			Close();
			Print(DL_Const.LOG_TAG + "film spike: closed");
			return;
		}
		if (!Build())
			return;
		m_Open = true;
		m_T = 0;
		Print(DL_Const.LOG_TAG + "film spike: open");
	}

	static int Flags()
	{
		return WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE | WidgetFlags.IGNOREPOINTER;
	}

	static ImageWidget Image(Widget parent, string tex, int color, int sort, float x, float y, float w, float h)
	{
		ImageWidget img = ImageWidget.Cast(g_Game.GetWorkspace().CreateWidget(ImageWidgetTypeID, x, y, w, h, Flags() | WidgetFlags.STRETCH | WidgetFlags.NOFILTER, color, sort, parent));
		if (!img)
			return null;
		if (!img.LoadImageFile(0, tex))
			Print(DL_Const.LOG_TAG + "film spike: could not load " + tex);
		img.SetImage(0);
		img.SetColor(color);
		return img;
	}

	static Widget Clip(Widget parent, int sort, float x, float y, float w, float h)
	{
		return g_Game.GetWorkspace().CreateWidget(FrameWidgetTypeID, x, y, w, h, Flags() | WidgetFlags.CLIPCHILDREN, ARGB(255, 255, 255, 255), sort, parent);
	}

	protected bool Build()
	{
		WorkspaceWidget ws = g_Game.GetWorkspace();
		if (!ws)
			return false;
		int sw;
		int sh;
		GetScreenSize(sw, sh);
		m_W = sw * 0.6;
		m_H = sh * 0.42;
		m_Root = ws.CreateWidget(FrameWidgetTypeID, (sw - m_W) / 2, sh * 0.12, m_W, m_H, Flags() | WidgetFlags.CLIPCHILDREN, ARGB(255, 255, 255, 255), 31000);
		if (!m_Root)
			return false;
		Image(m_Root, PLAIN, ARGB(255, 20, 20, 20), 0, 0, 0, m_W, m_H);                       // frame
		float bandTop = m_H * 0.09;
		float treesH = m_H * 0.22;
		m_RoadH = m_H * 0.36;
		float skyH = m_H - bandTop - treesH - m_RoadH - m_H * 0.05;
		Image(m_Root, PLAIN, ARGB(255, 150, 172, 182), 1, 4, bandTop, m_W - 8, skyH + treesH * 0.5); // sky
		m_Trees = new DL_Strip(m_Root, DIR + "trees_ca.paa", 2, 4, bandTop + skyH, m_W - 8, treesH, 256, 64);
		m_RoadTop = bandTop + skyH + treesH;
		m_Road = new DL_Strip(m_Root, DIR + "road_ca.paa", 3, 4, m_RoadTop, m_W - 8, m_RoadH, 256, 64);
		// UAZ 3/4 view facing right: cell (0,73) 100x73 of the 256x256 sheet
		float carW = m_RoadH * 1.6;
		m_Car = new DL_Sprite(m_Root, DIR + "uaz_ca.paa", 5, carW, carW * 73 / 100, 256, 256);
		m_Car.Cell(0, 73, 100, 73);
		// survivor: 32x32 cells, row 3 of the 128x128 sheet runs right
		float wh = m_RoadH * 0.7;
		m_Walker = new DL_Sprite(m_Root, DIR + "skin_biker_ca.paa", 6, wh, wh, 128, 128);
		TextWidget cap = TextWidget.Cast(ws.CreateWidget(TextWidgetTypeID, 0, 2, m_W, m_H * 0.07, Flags() | WidgetFlags.CENTER | WidgetFlags.VCENTER, ARGB(255, 230, 210, 170), 7, m_Root));
		if (cap)
			cap.SetText("DriveLink road film - engine test (F9 closes)");
		return true;
	}

	protected void Animate(float dt)
	{
		m_T += dt;
		// the car drives right = the world moves left; the tree line is farther away, so slower
		m_Road.Scroll(m_T * 260);
		m_Trees.Scroll(m_T * 100);
		float carW = m_RoadH * 1.6;
		float carH = carW * 73 / 100;
		float bob = Math.Sin(m_T * 18) * 1.5;
		m_Car.MoveTo((m_W - carW) / 2, m_RoadTop + m_RoadH * 0.5 - carH + bob);
		// the survivor runs right along the near verge, slower than the scenery, and wraps around
		int frame = Math.Floor(m_T * 10);
		frame = frame % 4;
		m_Walker.Cell(frame * 32, 96, 32, 32);
		float wh = m_RoadH * 0.7;
		float span = m_W + wh;
		float wx = m_T * 120;
		wx = wx - Math.Floor(wx / span) * span - wh;
		m_Walker.MoveTo(wx, m_RoadTop + m_RoadH - wh * 0.95);
	}
}

// one cell of a sprite sheet: a clipping frame of the cell's display size with the whole (scaled) sheet behind it
class DL_Sprite
{
	protected Widget m_Clip;
	protected ImageWidget m_Sheet;
	protected float m_DW;         // display size of one cell
	protected float m_DH;
	protected float m_TexW;
	protected float m_TexH;

	void DL_Sprite(Widget parent, string tex, int sort, float dispW, float dispH, float texW, float texH)
	{
		m_DW = dispW;
		m_DH = dispH;
		m_TexW = texW;
		m_TexH = texH;
		m_Clip = DL_FilmSpike.Clip(parent, sort, 0, 0, dispW, dispH);
		m_Sheet = DL_FilmSpike.Image(m_Clip, tex, ARGB(255, 255, 255, 255), 0, 0, 0, dispW, dispH);
	}

	// show sheet pixels (px,py)-(px+pw,py+ph) in the display box
	void Cell(float px, float py, float pw, float ph)
	{
		float sx = m_DW / pw;
		float sy = m_DH / ph;
		m_Sheet.SetSize(m_TexW * sx, m_TexH * sy);
		m_Sheet.SetPos(-px * sx, -py * sy);
	}

	void MoveTo(float x, float y)
	{
		m_Clip.SetPos(x, y);
	}
}

// a tiling strip that scrolls left forever: copies side by side inside a clipping band
class DL_Strip
{
	protected Widget m_Clip;
	protected ref array<ImageWidget> m_Tiles = new array<ImageWidget>;
	protected float m_TileW;

	void DL_Strip(Widget parent, string tex, int sort, float x, float y, float w, float h, float texW, float texH)
	{
		m_Clip = DL_FilmSpike.Clip(parent, sort, x, y, w, h);
		m_TileW = texW * (h / texH); // square texels
		int n = Math.Ceil(w / m_TileW) + 1;
		for (int i = 0; i < n; i++)
			m_Tiles.Insert(DL_FilmSpike.Image(m_Clip, tex, ARGB(255, 255, 255, 255), 0, i * m_TileW, 0, m_TileW + 1, h));
	}

	void Scroll(float offsetPx)
	{
		float o = offsetPx - Math.Floor(offsetPx / m_TileW) * m_TileW;
		for (int i = 0; i < m_Tiles.Count(); i++)
			m_Tiles[i].SetPos(i * m_TileW - o, 0);
	}
}
