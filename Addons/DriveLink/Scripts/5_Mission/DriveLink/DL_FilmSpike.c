// DriveLink road animation - ENGINE SPIKE (Research/road-animation-research.md, build order 1). Client only.
// F9 opens/closes a window that plays a test scene: the UAZ drives right, the road and the tree line scroll past
// (parallax), a survivor runs along the verge. Proves: .paa textures packed in the mod, frame picking from a sprite
// sheet with WidgetFlags.CUSTOMUV + ImageWidget.SetUV (1_core/proto/enwidgets.c:77,275), endless scrolling by sliding
// the UV of a tiling strip, per-frame updates from MissionGameplay.OnUpdate. Debug feature: remove or gate before
// publishing. Art: road/trees = DriveLink's own; UAZ + skin = MiniDayZ, prototype only.

class DL_FilmSpike
{
	static const string DIR = "DriveLink\\gui\\anim\\";
	protected Widget m_Root;
	protected ImageWidget m_Sky;
	protected ImageWidget m_Trees;
	protected ImageWidget m_Road;
	protected ImageWidget m_Car;
	protected ImageWidget m_Walker;
	protected TextWidget m_Caption;
	protected bool m_Open;
	protected bool m_KeyWasDown;
	protected float m_T;
	protected float m_X;          // window
	protected float m_Y;
	protected float m_W;
	protected float m_H;
	protected float m_TreesH;
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

	protected static ImageWidget Image(WorkspaceWidget ws, Widget parent, string tex, int color, int sort, bool uv)
	{
		int flags = WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE | WidgetFlags.IGNOREPOINTER | WidgetFlags.NOFILTER;
		if (uv)
			flags = flags | WidgetFlags.CUSTOMUV;
		else
			flags = flags | WidgetFlags.STRETCH;
		ImageWidget w = ImageWidget.Cast(ws.CreateWidget(ImageWidgetTypeID, 0, 0, 16, 16, flags, color, sort, parent));
		if (!w)
			return null;
		if (!w.LoadImageFile(0, tex))
			Print(DL_Const.LOG_TAG + "film spike: could not load " + tex);
		w.SetImage(0);
		return w;
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
		m_X = (sw - m_W) / 2;
		m_Y = sh * 0.12;
		int flags = WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE | WidgetFlags.IGNOREPOINTER;
		m_Root = ws.CreateWidget(FrameWidgetTypeID, m_X, m_Y, m_W, m_H, flags, ARGB(255, 255, 255, 255), 31000);
		if (!m_Root)
			return false;
		string plain = "#(argb,8,8,3)color(1,1,1,1,CO)";
		ImageWidget border = Image(ws, m_Root, plain, ARGB(255, 20, 20, 20), 0, false);
		border.SetPos(0, 0);
		border.SetSize(m_W, m_H);
		m_Sky = Image(ws, m_Root, plain, ARGB(255, 150, 172, 182), 1, false);
		m_TreesH = m_H * 0.22;
		m_RoadH = m_H * 0.36;
		float bandTop = m_H * 0.08;
		float skyH = m_H - bandTop - m_TreesH - m_RoadH - m_H * 0.06;
		m_Sky.SetPos(4, bandTop);
		m_Sky.SetSize(m_W - 8, skyH + m_TreesH * 0.5);
		m_Trees = Image(ws, m_Root, DIR + "trees_ca.paa", ARGB(255, 255, 255, 255), 2, true);
		m_Trees.SetPos(4, bandTop + skyH);
		m_Trees.SetSize(m_W - 8, m_TreesH);
		m_Road = Image(ws, m_Root, DIR + "road_ca.paa", ARGB(255, 255, 255, 255), 3, true);
		m_Road.SetPos(4, bandTop + skyH + m_TreesH);
		m_Road.SetSize(m_W - 8, m_RoadH);
		// UAZ 3/4 view facing right: pixels (0,73)-(100,146) of the 256x256 sheet
		m_Car = Image(ws, m_Root, DIR + "uaz_ca.paa", ARGB(255, 255, 255, 255), 5, true);
		float carW = m_RoadH * 1.6;
		float carH = carW * 73 / 100;
		m_Car.SetSize(carW, carH);
		SetFrame(m_Car, 0, 73, 100, 73, 256, 256, false);
		// survivor: 32x32 frames, row 3 of the 128x128 sheet runs right
		m_Walker = Image(ws, m_Root, DIR + "skin_biker_ca.paa", ARGB(255, 255, 255, 255), 6, true);
		float wh = m_RoadH * 0.7;
		m_Walker.SetSize(wh, wh);
		m_Caption = TextWidget.Cast(ws.CreateWidget(TextWidgetTypeID, 0, 0, m_W, m_H * 0.07, flags | WidgetFlags.CENTER | WidgetFlags.VCENTER, ARGB(255, 230, 210, 170), 7, m_Root));
		m_Caption.SetPos(0, 2);
		m_Caption.SetText("DriveLink road film - engine test (F9 closes)");
		return true;
	}

	// one rectangle of a sheet (pixels) onto the widget; flip = mirror left-right
	protected static void SetFrame(ImageWidget w, float px, float py, float pw, float ph, float texW, float texH, bool flip)
	{
		float u0 = px / texW;
		float u1 = (px + pw) / texW;
		float v0 = py / texH;
		float v1 = (py + ph) / texH;
		if (flip)
		{
			float t = u0;
			u0 = u1;
			u1 = t;
		}
		float uv[4][2];
		uv[0][0] = u0; uv[0][1] = v0;   // top left
		uv[1][0] = u1; uv[1][1] = v0;   // top right
		uv[2][0] = u1; uv[2][1] = v1;   // bottom right
		uv[3][0] = u0; uv[3][1] = v1;   // bottom left
		w.SetUV(uv);
	}

	// a tiling strip scrolled by "offset" texels; the visible span keeps the texels square
	protected static void Scroll(ImageWidget w, float offsetPx, float widgetW, float widgetH, float texW, float texH)
	{
		float span = widgetW / (widgetH / texH); // texels across the widget
		SetFrame(w, offsetPx, 0, span, texH, texW, texH, false);
	}

	protected void Animate(float dt)
	{
		m_T += dt;
		// the car drives right = the world moves left; the tree line is farther away, so slower
		Scroll(m_Road, m_T * 90, m_W - 8, m_RoadH, 256, 64);
		Scroll(m_Trees, m_T * 35, m_W - 8, m_TreesH, 256, 64);
		float roadTop = m_H * 0.08 + (m_H - m_H * 0.08 - m_TreesH - m_RoadH - m_H * 0.06) + m_TreesH;
		float carW = m_RoadH * 1.6;
		float carH = carW * 73 / 100;
		float bob = Math.Sin(m_T * 18) * 1.5;
		m_Car.SetPos((m_W - carW) / 2, roadTop + m_RoadH * 0.45 - carH + bob);
		// the survivor runs right along the near verge, slower than the scenery passes, and wraps around
		int frame = Math.Floor(m_T * 10);
		frame = frame % 4;
		SetFrame(m_Walker, frame * 32, 96, 32, 32, 128, 128, false);
		float wh = m_RoadH * 0.7;
		float span = m_W + wh;
		float wx = (m_T * 60) - wh;
		wx = wx - Math.Floor(wx / span) * span - wh;
		m_Walker.SetPos(wx, roadTop + m_RoadH - wh * 0.95);
	}
}
