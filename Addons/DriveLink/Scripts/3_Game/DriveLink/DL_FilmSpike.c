// DriveLink road animation - ENGINE SPIKE (Research/road-animation-research.md, build order 1). Client only.
// F9 opens/closes a window that plays a test scene, top-down like MiniDayZ (Derk 2026-10-20): the ground (road + trees)
// scrolls up while the UAZ drives down the middle of the road, a survivor walks down the verge. Proves: .paa textures packed in the mod, frame picking from a sprite
// sheet by CLIPPING (a CLIPCHILDREN frame shows one cell of the whole sheet moved behind it; ImageWidget.SetUV
// would not take a script float[4][2]: "Incompatible parameter uv", 2026-10-20), endless scrolling by sliding copies
// of a tiling strip inside a clipping band, per-frame updates from MissionGameplay.OnUpdate. Debug feature: remove or gate before
// publishing. Art: ground tile = DriveLink's own; UAZ + skin = MiniDayZ, prototype only.

// shared widget helpers of the road film (the F9 spike scene itself is gone: F9 now plays DL_FilmPlayer.Demo())
class DL_FilmSpike
{
	static const string PLAIN = "#(argb,8,8,3)color(1,1,1,1,CO)";

	static int Flags()
	{
		return WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE | WidgetFlags.IGNOREPOINTER;
	}

	static ImageWidget Image(Widget parent, string tex, int color, int sort, float x, float y, float w, float h)
	{
		ImageWidget img = ImageWidget.Cast(g_Game.GetWorkspace().CreateWidget(ImageWidgetTypeID, x, y, w, h, Flags() | WidgetFlags.STRETCH | WidgetFlags.NOFILTER | WidgetFlags.SOURCEALPHA | WidgetFlags.BLEND, color, sort, parent));
		if (!img)
			return null;
		// Solid panels use the widget's color directly, like the existing arrival HUD.
		// The engine does not accept procedural material strings as image files.
		if (tex != PLAIN && !img.LoadImageFile(0, tex))
			DL_Log.Warn("road film: could not load " + tex);
		img.SetImage(0);
		img.SetColor(color);
		return img;
	}

	static Widget Clip(Widget parent, int sort, float x, float y, float w, float h)
	{
		return g_Game.GetWorkspace().CreateWidget(FrameWidgetTypeID, x, y, w, h, Flags() | WidgetFlags.CLIPCHILDREN, ARGB(255, 255, 255, 255), sort, parent);
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

	void Show(bool on)
	{
		m_Clip.Show(on);
	}

	bool IsShown()
	{
		return m_Clip.IsVisible();
	}

	void SetAlpha(float a)
	{
		m_Sheet.SetAlpha(a);
	}

	// draw order among the film's widgets (higher = on top), changed every frame by depth
	void SetSort(int s)
	{
		m_Clip.SetSort(s);
	}

	// show only the top part of the cell (frac 0..1 of its height): a rider on a motorbike, legs behind the tank
	void Cut(float frac)
	{
		m_Clip.SetSize(m_DW, m_DH * frac);
	}

	// multiplies the sheet's colours (tinted clothing)
	void SetColor(int argb)
	{
		m_Sheet.SetColor(argb);
	}

	// degrees (lying people in the film)
	void Turn(float deg)
	{
		m_Clip.SetRotation(0, 0, deg);
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

	void SetAlpha(float a)
	{
		foreach (ImageWidget t : m_Tiles)
			t.SetAlpha(a);
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
