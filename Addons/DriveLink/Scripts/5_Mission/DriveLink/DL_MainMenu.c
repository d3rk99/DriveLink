// DriveLink - after the redirect disconnect the client lands in the main menu; connect to the target from there.
// MissionMainMenu.m_NoCutscene skips the intro scene (missionmainmenu.c:8-14); m_mainmenu is private there, so we
// pass the menu that is open now (UIManager.GetMenu) as Connect's parent.
// We wait a few seconds before connecting: connecting 0.1 s after the menu appeared crashed the game natively
// (access violation at 0x30 while loading the next map, 5 times on Chernarus -> Livonia, 2026-10-19/20) because the
// previous world was still being unloaded. DL_Go retries until the menu is open and the settle time has passed.
// After a RPC_KICK (travel could not finish) the server's message is shown here instead.
modded class MissionMainMenu
{
	static const int DL_SETTLE_MS = 3000;   // wait at least this long in the menu
	static const int DL_GIVE_UP_MS = 15000; // connect anyway after this long
	protected int m_DL_Waited;
	// travel time: "On the road to X: about 1 h 12 min left" at the top while the trip lasts (DL_MenuScene)
	protected ref DL_MenuScene m_DL_Road;
	protected Widget m_DL_RoadBanner;         // gui/layouts/dl_road_banner.layout: a dark strip, big outlined text
	protected TextWidget m_DL_RoadText;
	static const float DL_TEXT_H = 0.72;       // the text's height in the strip (its font scales with it)
	protected float m_DL_TextH = DL_TEXT_H;
	protected float m_DL_RoadTick;
	protected Widget m_DL_SumRoot;            // the trail's summary under the banner (DL_MenuScene.title / lines)

	override void OnInit()
	{
		if (GetDayZGame().DL_HasPending())
			m_NoCutscene = true;
		super.OnInit();
	}

	override void OnMissionStart()
	{
		super.OnMissionStart();
		GetDayZGame().DL_Transition().MissionStarted("main menu");
		if (GetDayZGame().DL_HasPending())
		{
			m_DL_Waited = 0;
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_Go, 500, false);
		}
		else if (GetDayZGame().DL_HasNotice())
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_Notice, 500, false);
		if (!GetDayZGame().DL_HasPending())
			m_DL_Road = DL_MenuScene.Load();
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (!m_DL_Road)
			return;
		m_DL_RoadTick -= timeslice;
		if (m_DL_RoadTick > 0)
			return;
		m_DL_RoadTick = 1;
		if (!m_DL_RoadText)
		{
			// from a layout: a text widget made in script has no scalable font and stayed small at any box size
			m_DL_RoadBanner = g_Game.GetWorkspace().CreateWidgets("DriveLink/gui/layouts/dl_road_banner.layout");
			if (!m_DL_RoadBanner)
				return;
			m_DL_RoadText = TextWidget.Cast(m_DL_RoadBanner.FindAnyWidget("DL_RoadText"));
			if (!m_DL_RoadText)
				return;
			m_DL_RoadText.SetOutline(2, ARGB(255, 0, 0, 0));
			m_DL_RoadText.SetShadow(4, ARGB(255, 0, 0, 0), 0.6, 0, 2);
			DL_ShowSummary();
		}
		int left = m_DL_Road.Left();
		if (left > 0)
			m_DL_RoadText.SetText("On the road to " + m_DL_Road.target + ": about " + DL_TravelCost.Duration(left) + " left");
		else
			m_DL_RoadText.SetText("You have arrived: join " + m_DL_Road.target);
		DL_FitRoadText();
	}

	// A long server name ("DriveLink LAN Test 1 - Chernarus") ran out of the strip on both sides: the text gets
	// smaller until it fits (its font follows the widget's height), full size again for a short one.
	protected void DL_FitRoadText()
	{
		float bw;
		float bh;
		m_DL_RoadBanner.GetScreenSize(bw, bh);
		int tw;
		int th;
		m_DL_RoadText.GetTextSize(tw, th);
		if (bw <= 0 || tw <= 0 || m_DL_TextH <= 0)
			return;
		float fullW = tw * DL_TEXT_H / m_DL_TextH;   // its width at the full height
		float h = DL_TEXT_H * Math.Min(1, bw * 0.94 / fullW);
		if (Math.AbsFloat(h - m_DL_TextH) < 0.01)
			return;
		m_DL_TextH = h;
		m_DL_RoadText.SetSize(0.96, h);
	}

	override void OnMissionFinish()
	{
		if (m_DL_RoadBanner)
			m_DL_RoadBanner.Unlink();
		m_DL_RoadBanner = null;
		if (m_DL_SumRoot)
			m_DL_SumRoot.Unlink();
		m_DL_SumRoot = null;
		m_DL_RoadText = null;
		GetDayZGame().DL_Transition().MissionFinishing();
		super.OnMissionFinish();
	}

	// the trail's summary: a dark panel under the banner, the title and one line each (delays, hurt, lost)
	protected void DL_ShowSummary()
	{
		if (m_DL_SumRoot || !m_DL_Road || m_DL_Road.title == "")
			return;
		int sw;
		int sh;
		GetScreenSize(sw, sh);
		float rh = sh * 0.028;
		float w = sw * 0.47;
		float x = sw * 0.305;
		float y = sh * 0.125;
		float h = rh * 0.5 + rh * 1.4 + m_DL_Road.lines.Count() * rh * 1.05 + rh * 0.5;
		// from a layout like the banner's: see-through (a script-made image ignored its alpha, test 2026-11-01)
		Widget panel = g_Game.GetWorkspace().CreateWidgets("DriveLink/gui/layouts/dl_trail_panel.layout");
		if (!panel)
			return;
		panel.SetPos(x, y);
		panel.SetSize(w, h);
		m_DL_SumRoot = panel;
		DL_SumText(rh * 0.5, w, rh * 1.1, m_DL_Road.title, ARGB(255, 250, 220, 140));
		float ty = rh * 0.5 + rh * 1.4;
		foreach (string l : m_DL_Road.lines)
		{
			DL_SumText(ty, w, rh * 0.75, l, ARGB(255, 230, 230, 230));
			ty += rh * 1.05;
		}
	}

	protected void DL_SumText(float y, float w, float h, string text, int color)
	{
		TextWidget t = TextWidget.Cast(g_Game.GetWorkspace().CreateWidgets("DriveLink/gui/layouts/dl_trail_text_center.layout", m_DL_SumRoot));
		if (!t)
			return;
		t.SetPos(0, y);
		t.SetSize(w, h);
		t.SetColor(color);
		t.SetText(text);
		t.SetOutline(1, ARGB(255, 0, 0, 0));
	}

	protected void DL_Go()
	{
		m_DL_Waited += 500;
		UIScriptedMenu menu = g_Game.GetUIManager().GetMenu();
		bool ready = m_DL_Waited >= DL_SETTLE_MS && menu != null;
		if (!ready && m_DL_Waited < DL_GIVE_UP_MS)
		{
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_Go, 500, false);
			return;
		}
		GetDayZGame().DL_Reconnect(menu);
	}

	protected void DL_Notice()
	{
		GetDayZGame().DL_ShowNotice();
	}
}
