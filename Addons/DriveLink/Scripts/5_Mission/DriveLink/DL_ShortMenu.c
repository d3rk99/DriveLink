// DriveLink - client: the "not enough supplies" dialog (RPC_SHORT, DL_ShortInfo as JSON). At the top the player's
// chance of surviving the trip (the hub works it out, Hub/src/odds.rs), then a table of what each one has and what the
// trip needs, then the buttons: the leader of the trip may travel anyway when the server allows it (Travel rules:
// allowAnyway), everyone else just closes it. Widgets made in code like the waiting room (DL_ClientHud), inside a
// UIScriptedMenu for the mouse cursor (3_game/tools/uiscriptedmenu.c LockControls / UnlockControls). It takes the
// mouse only: the keyboard stays with the game (UseKeyboard false, vanilla missiongameplay.c handles such a menu), so
// a driver keeps steering a convoy while it is open, and it sits small on the left with the road in view.
// The buttons get their own handler (ScriptedWidgetEventHandler.OnClick) so the clicks don't depend on how the engine
// routes events to a menu built without a layout file.
class DL_ShortButtons : ScriptedWidgetEventHandler
{
	protected DL_ShortMenu m_Menu;

	void DL_ShortButtons(DL_ShortMenu menu)
	{
		m_Menu = menu;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_Menu)
			return m_Menu.DL_Clicked(w);
		return false;
	}
}

class DL_ShortRow
{
	string label;
	string have;
	string need;
	bool bad;

	void DL_ShortRow(string l, string h, string n, bool b)
	{
		label = l;
		have = h;
		need = n;
		bad = b;
	}
}

class DL_ShortMenu extends UIScriptedMenu
{
	static const int MENU_ID = 4810013;
	protected ref DL_ShortInfo m_Info;
	protected ref DL_ShortButtons m_Buttons;
	protected ButtonWidget m_Go;
	protected ButtonWidget m_Stay;
	protected bool m_Answered;
	protected bool m_KeysHeld = true;         // Y / N count once both were up (a key held from before doesn't answer)

	void DL_ShortMenu(DL_ShortInfo info)
	{
		m_Info = info;
		m_Buttons = new DL_ShortButtons(this);
		SetID(MENU_ID);
	}

	static int ChanceColor(int pct)
	{
		if (pct < 0)
			return ARGB(255, 200, 200, 200);
		if (pct >= 75)
			return ARGB(255, 120, 220, 120);
		if (pct >= 40)
			return ARGB(255, 240, 200, 90);
		return ARGB(255, 240, 90, 80);
	}

	protected array<ref DL_ShortRow> Rows()
	{
		array<ref DL_ShortRow> rows = new array<ref DL_ShortRow>;
		bool crew = m_Info.people.Count() > 1;
		for (int i = 0; i < m_Info.people.Count(); i++)
		{
			DL_OddsPerson p = m_Info.people[i];
			string who = "";
			if (crew)
			{
				who = p.name + ": ";
				if (i == m_Info.you)
					who = "You: ";
			}
			string chance = "";
			if (crew && p.odds >= 0)
				chance = " (chance " + p.odds.ToString() + "%)";
			rows.Insert(new DL_ShortRow(who + "health" + chance, Math.Round(p.health).ToString() + "%", "-", p.health < 50));
			rows.Insert(new DL_ShortRow(who + "food", DL_TravelCost.Kcal(p.food), DL_TravelCost.Kcal(p.needFood), p.food < p.needFood));
			rows.Insert(new DL_ShortRow(who + "water", DL_TravelCost.Ml(p.water), DL_TravelCost.Ml(p.needWater), p.water < p.needWater));
		}
		if (m_Info.vehicle != "")
		{
			rows.Insert(new DL_ShortRow(m_Info.vehicle + ": fuel (tank + canisters)", Math.Floor(m_Info.fuel).ToString() + " L", Math.Ceil(m_Info.needFuel).ToString() + " L", m_Info.fuel < m_Info.needFuel));
			if (m_Info.needSpare)
			{
				string has = "no";
				if (m_Info.spareOk)
					has = "yes";
				rows.Insert(new DL_ShortRow("spare wheel or tyre repair kit", has, "1", !m_Info.spareOk));
			}
		}
		return rows;
	}

	protected TextWidget Text(Widget parent, float x, float y, float w, float h, string text, int color, bool center)
	{
		int flags = WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE | WidgetFlags.IGNOREPOINTER | WidgetFlags.VCENTER;
		if (center)
			flags |= WidgetFlags.CENTER;
		TextWidget t = TextWidget.Cast(g_Game.GetWorkspace().CreateWidget(TextWidgetTypeID, x, y, w, h, flags, color, 3, parent));
		if (t)
			t.SetText(text);
		return t;
	}

	protected void Box(Widget parent, float x, float y, float w, float h, int color, int sort)
	{
		int flags = WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE | WidgetFlags.IGNOREPOINTER | WidgetFlags.STRETCH;
		ImageWidget b = ImageWidget.Cast(g_Game.GetWorkspace().CreateWidget(ImageWidgetTypeID, x, y, w, h, flags, color, sort, parent));
		if (b)
		{
			b.LoadImageFile(0, "#(argb,8,8,3)color(1,1,1,1,CO)"); // plain texture, tinted (DL_ClientHud)
			b.SetColor(color);
		}
	}

	protected ButtonWidget Button(Widget parent, float x, float y, float w, float h, string text, int color)
	{
		Box(parent, x, y, w, h, color, 2);
		Text(parent, x, y, w, h, text, ARGB(255, 255, 255, 255), true);
		int flags = WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE;
		ButtonWidget b = ButtonWidget.Cast(g_Game.GetWorkspace().CreateWidget(ButtonWidgetTypeID, x, y, w, h, flags, ARGB(0, 0, 0, 0), 4, parent));
		if (b)
			b.SetHandler(m_Buttons);
		return b;
	}

	// the mouse for the buttons, the keyboard (and a gamepad) stays with the game: driving goes on
	override bool UseKeyboard()
	{
		return false;
	}

	override bool UseGamepad()
	{
		return false;
	}

	override Widget Init()
	{
		WorkspaceWidget ws = g_Game.GetWorkspace();
		int sw;
		int sh;
		GetScreenSize(sw, sh);
		int rootFlags = WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE;
		layoutRoot = ws.CreateWidget(FrameWidgetTypeID, 0, 0, sw, sh, rootFlags, ARGB(255, 255, 255, 255), 30100);
		if (!layoutRoot || !m_Info)
			return layoutRoot;
		array<ref DL_ShortRow> rows = Rows();
		float rh = sh * 0.026;                       // a table row (the default font is about 3/4 of its box)
		if (rows.Count() > 15)
			rh = sh * 0.4 / rows.Count();
		float pw = Math.Min(sw * 0.36, sh * 0.72);
		float pad = rh * 0.8;
		float ph = pad * 2 + rh * 1.2 + rh * 1.7 + rh * 1.0 + rh * 1.3 + rh * (rows.Count() + 1) + rh * 0.6 + rh * 1.0 + rh * 1.6 + rh * 1.2;
		if (m_Info.tail != "" || !m_Info.canGo)
			ph += rh * 1.1;
		int nVeh = 0;
		if (m_Info.vehicles)
			nVeh = m_Info.vehicles.Count();
		ph += rh * 1.15 * nVeh;
		float px = sw * 0.025;                       // on the left, the road in view (no dimming)
		float py = Math.Max(sh * 0.04, Math.Min(sh * 0.2, sh - ph - sh * 0.04));
		Box(layoutRoot, px, py, pw, ph, ARGB(225, 18, 21, 25), 1);      // the panel
		Box(layoutRoot, px, py, pw, rh * 0.12, ARGB(255, 240, 90, 80), 2);
		float y = py + pad;
		float iw = pw - pad * 2;
		float ix = px + pad;
		Text(layoutRoot, ix, y, iw, rh * 1.2, "Not enough supplies for " + m_Info.zone, ARGB(255, 230, 230, 230), true);
		y += rh * 1.4;
		int mine = -1;
		if (m_Info.you >= 0 && m_Info.you < m_Info.people.Count())
			mine = m_Info.people[m_Info.you].odds;
		string chance = "Your chance of surviving the trip: unknown (no answer from the hub)";
		if (mine >= 0)
			chance = "Your chance of surviving the trip: " + mine.ToString() + "%";
		Text(layoutRoot, ix, y, iw, rh * 1.5, chance, ChanceColor(mine), true);
		y += rh * 1.7;
		// each vehicle's chance to make it all the way (fuel, a flat without a spare, a worn-out engine)
		for (int vi = 0; vi < nVeh; vi++)
		{
			DL_OddsVehicle ov = m_Info.vehicles[vi];
			string vline = ov.name + " makes it all the way: unknown";
			if (ov.odds >= 0)
				vline = ov.name + " makes it all the way: " + ov.odds.ToString() + "%";
			Text(layoutRoot, ix, y, iw, rh * 1.05, vline, ChanceColor(ov.odds), true);
			y += rh * 1.15;
		}
		Text(layoutRoot, ix, y, iw, rh * 0.9, "Trip: " + m_Info.price, ARGB(255, 170, 170, 170), true);
		y += rh * 1.3;
		// the table: what you have | what the trip needs
		float c1 = iw * 0.46;
		float c2 = iw * 0.27;
		Box(layoutRoot, ix, y, iw, rh * 1.05, ARGB(255, 40, 45, 52), 2);
		Text(layoutRoot, ix + rh * 0.3, y, c1, rh, "", ARGB(255, 200, 200, 200), false);
		Text(layoutRoot, ix + c1, y, c2, rh, "You have", ARGB(255, 200, 200, 200), true);
		Text(layoutRoot, ix + c1 + c2, y, c2, rh, "The trip needs", ARGB(255, 200, 200, 200), true);
		y += rh * 1.1;
		foreach (DL_ShortRow r : rows)
		{
			int col = ARGB(255, 225, 225, 225);
			if (r.bad)
				col = ARGB(255, 240, 110, 100);
			Text(layoutRoot, ix + rh * 0.3, y, c1 - rh * 0.3, rh, r.label, ARGB(255, 210, 210, 210), false);
			Text(layoutRoot, ix + c1, y, c2, rh, r.have, col, true);
			Text(layoutRoot, ix + c1 + c2, y, c2, rh, r.need, ARGB(255, 210, 210, 210), true);
			y += rh;
		}
		y += rh * 0.6;
		string line = m_Info.tail;
		if (!m_Info.canGo && m_Info.people.Count() > 1 && m_Info.you != 0)
			line = m_Info.leader + " leads this trip. " + line;
		if (line != "")
		{
			Text(layoutRoot, ix, y, iw, rh * 0.9, line, ARGB(255, 170, 170, 170), true);
			y += rh * 1.1;
		}
		float bh = rh * 1.6;
		float bw = iw * 0.42;
		if (m_Info.canGo)
		{
			m_Go = Button(layoutRoot, ix + iw * 0.5 - bw - rh * 0.4, y, bw, bh, "[Y] Travel anyway", ARGB(255, 150, 60, 50));
			m_Stay = Button(layoutRoot, ix + iw * 0.5 + rh * 0.4, y, bw, bh, "[N] Don't go", ARGB(255, 60, 70, 80));
		}
		else
			m_Stay = Button(layoutRoot, ix + (iw - bw) / 2, y, bw, bh, "[N] OK", ARGB(255, 60, 70, 80));
		y += bh + rh * 0.4;
		Text(layoutRoot, ix, y, iw, rh * 0.8, "You keep control of your vehicle while this is open.", ARGB(255, 150, 150, 150), true);
		return layoutRoot;
	}

	// Y = travel anyway, N = don't go (the keyboard stays with the game, so they are read straight from it)
	override void Update(float timeslice)
	{
		super.Update(timeslice);
		if (m_Answered)
			return;
		bool y = KeyState(KeyCode.KC_Y) != 0;
		bool n = KeyState(KeyCode.KC_N) != 0;
		if (m_KeysHeld)
		{
			m_KeysHeld = y || n;
			return;
		}
		if (y && m_Info && m_Info.canGo)
			DL_Clicked(m_Go);
		else if (n)
			DL_Clicked(m_Stay);
	}

	// a click on our buttons, from their handler or the menu's own OnClick
	bool DL_Clicked(Widget w)
	{
		if (m_Answered || !w)
			return false;
		if (w == m_Go)
			Answer(true);
		else if (w == m_Stay)
			Answer(false);
		else
			return false;
		Close();
		return true;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (DL_Clicked(w))
			return true;
		return super.OnClick(w, x, y, button);
	}

	// to the server through his own player (DL_AnywayAnswers); only the leader of the trip was offered it
	protected void Answer(bool go)
	{
		m_Answered = true;
		if (!m_Info || !m_Info.canGo)
			return;
		PlayerBase me = PlayerBase.Cast(g_Game.GetPlayer());
		if (!me)
			return;
		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(go);
		rpc.Send(me, DL_Const.RPC_GO_ANYWAY, true, null);
		DL_Log.Detail("not enough supplies: travel anyway " + go.ToString());
	}

	override void OnHide()
	{
		super.OnHide();
		if (!m_Answered)
			Answer(false); // closed with Esc: he stays
	}
}
