// DriveLink road animation - ENGINE SPIKE (Research/road-animation-research.md, build order 1). Client only.
// F9 opens/closes a window that plays a test scene, top-down like MiniDayZ (Derk 2026-10-20): the ground (road + trees)
// scrolls up while the UAZ drives down the middle of the road, a survivor walks down the verge. Proves: .paa textures packed in the mod, frame picking from a sprite
// sheet by CLIPPING (a CLIPCHILDREN frame shows one cell of the whole sheet moved behind it; ImageWidget.SetUV
// would not take a script float[4][2]: "Incompatible parameter uv", 2026-10-20), endless scrolling by sliding copies
// of a tiling strip inside a clipping band, per-frame updates from MissionGameplay.OnUpdate. Debug feature: remove or gate before
// publishing. Art: ground tile = DriveLink's own; UAZ + skin = MiniDayZ, prototype only.

class DL_FilmSpike
{
	static const string DIR = "DriveLink/gui/anim/";
	static const string PLAIN = "#(argb,8,8,3)color(1,1,1,1,CO)";
	protected Widget m_Root;
	protected ref DL_Strip m_Ground;
	protected ref DL_Sprite m_Car;
	protected ref DL_Sprite m_Walker;
	protected bool m_Open;
	protected bool m_KeyWasDown;
	protected float m_T;
	protected float m_W;
	protected float m_H;
	protected float m_Px;         // screen pixels per texture pixel (everything at the same pixel scale)

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
		ImageWidget img = ImageWidget.Cast(g_Game.GetWorkspace().CreateWidget(ImageWidgetTypeID, x, y, w, h, Flags() | WidgetFlags.STRETCH | WidgetFlags.NOFILTER | WidgetFlags.SOURCEALPHA | WidgetFlags.BLEND, color, sort, parent));
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
		m_H = sh * 0.7;
		m_W = m_H * 0.62;
		m_Root = ws.CreateWidget(FrameWidgetTypeID, (sw - m_W) / 2, sh * 0.12, m_W, m_H, Flags() | WidgetFlags.CLIPCHILDREN, ARGB(255, 255, 255, 255), 31000);
		if (!m_Root)
			return false;
		Image(m_Root, PLAIN, ARGB(255, 20, 20, 20), 0, 0, 0, m_W, m_H);                       // frame
		float top = m_H * 0.06;
		float gw = m_W - 8;
		m_Px = gw / 256.0;                                                                   // the 256-px ground tile fills the width
		m_Ground = new DL_Strip(m_Root, DIR + "ground_ca.paa", 1, 4, top, gw, m_H - top - 4, 256, 256, true);
		// UAZ seen from above, pointing down: cell (176,80) 68x74 of the 256x256 sheet (shadow included)
		m_Car = new DL_Sprite(m_Root, DIR + "uaz_ca.paa", 5, 68 * m_Px, 74 * m_Px, 256, 256);
		m_Car.Cell(176, 80, 68, 74);
		// survivor: 32x32 cells, row 1 of the 128x128 sheet walks towards the viewer (down)
		m_Walker = new DL_Sprite(m_Root, DIR + "skin_biker_ca.paa", 6, 32 * m_Px, 32 * m_Px, 128, 128);
		TextWidget cap = TextWidget.Cast(ws.CreateWidget(TextWidgetTypeID, 0, 2, m_W, top - 4, Flags() | WidgetFlags.CENTER | WidgetFlags.VCENTER, ARGB(255, 230, 210, 170), 7, m_Root));
		if (cap)
			cap.SetText("DriveLink road film - engine test (F9 closes)");
		return true;
	}

	protected void Animate(float dt)
	{
		m_T += dt;
		float top = m_H * 0.06;
		// the car drives down = the ground moves up
		m_Ground.Scroll(m_T * 140 * m_Px);
		float bob = Math.Sin(m_T * 18) * 0.6 * m_Px;
		m_Car.MoveTo(4 + (256 - 68) / 2.0 * m_Px + bob, top + m_H * 0.38);
		// the survivor walks down the left verge, slower than the car: on screen he drifts up and comes back from below
		int frame = Math.Floor(m_T * 8);
		frame = frame % 4;
		m_Walker.Cell(frame * 32, 32, 32, 32);
		float wh = 32 * m_Px;
		float span = m_H - top + wh;
		float wy = m_T * 80 * m_Px;
		wy = wy - Math.Floor(wy / span) * span;
		m_Walker.MoveTo(4 + 20 * m_Px, m_H - wy);
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

	protected bool m_Vertical;    // true: tiles stacked top to bottom, scrolling up (top-down view)

	void DL_Strip(Widget parent, string tex, int sort, float x, float y, float w, float h, float texW, float texH, bool vertical = false)
	{
		m_Vertical = vertical;
		m_Clip = DL_FilmSpike.Clip(parent, sort, x, y, w, h);
		int n;
		int i;
		if (vertical)
		{
			m_TileW = texH * (w / texW); // tile height, square texels
			n = Math.Ceil(h / m_TileW) + 1;
			for (i = 0; i < n; i++)
				m_Tiles.Insert(DL_FilmSpike.Image(m_Clip, tex, ARGB(255, 255, 255, 255), 0, 0, i * m_TileW, w, m_TileW + 1));
			return;
		}
		m_TileW = texW * (h / texH);
		n = Math.Ceil(w / m_TileW) + 1;
		for (i = 0; i < n; i++)
			m_Tiles.Insert(DL_FilmSpike.Image(m_Clip, tex, ARGB(255, 255, 255, 255), 0, i * m_TileW, 0, m_TileW + 1, h));
	}

	void Scroll(float offsetPx)
	{
		float o = offsetPx - Math.Floor(offsetPx / m_TileW) * m_TileW;
		for (int i = 0; i < m_Tiles.Count(); i++)
		{
			if (m_Vertical)
				m_Tiles[i].SetPos(0, i * m_TileW - o);
			else
				m_Tiles[i].SetPos(i * m_TileW - o, 0);
		}
	}
}
