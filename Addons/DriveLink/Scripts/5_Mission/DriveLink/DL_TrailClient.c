// DriveLink interactive trail - client (Research/interactive-trail-design.md). The zone's ready-up table
// (RPC_TRAIL_READY, DL_ReadyMenu), then the trail: a black screen, the road film one chapter at a time
// (RPC_TRAIL_START / RPC_TRAIL_ROAD), the choice cards and votes (RPC_TRAIL_CARD / RPC_TRAIL_TALLY, DL_TrailView in a
// DL_TrailMenu for the mouse) and the summary (RPC_TRAIL_END). What he presses goes back as RPC_TRAIL_IN through his
// own player. MissionGameplay (DL_ClientHud) hands it the RPCs and the frames.

class DL_ReadyButtons : ScriptedWidgetEventHandler
{
	protected DL_ReadyMenu m_Menu;

	void DL_ReadyButtons(DL_ReadyMenu menu)
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

// the ready-up table: what everyone has against what the trip needs, who is ready, the clock. The mouse only: the
// keyboard stays with the game, so anyone can still walk (or drive) out of the zone to call it off.
class DL_ReadyMenu extends UIScriptedMenu
{
	static const int MENU_ID = 4810016;
	protected ref DL_ReadyInfo m_Info;
	protected ref DL_ReadyButtons m_Buttons;
	protected Widget m_Panel;
	protected ButtonWidget m_Ready;
	protected ButtonWidget m_NotReady;
	protected ButtonWidget m_GuideButton;
	protected ref DL_Guide m_Guide;
	protected TextWidget m_Clock;
	protected float m_Left;
	protected int m_You = -1;
	protected bool m_KeysHeld = true;
	string outAnswer;             // "ready|1" / "ready|0" for the client to send

	void DL_ReadyMenu()
	{
		m_Buttons = new DL_ReadyButtons(this);
		SetID(MENU_ID);
	}

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
		int sw;
		int sh;
		GetScreenSize(sw, sh);
		int flags = WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE;
		layoutRoot = g_Game.GetWorkspace().CreateWidget(FrameWidgetTypeID, 0, 0, sw, sh, flags, ARGB(255, 255, 255, 255), 30100);
		if (m_Info)
			Build();
		return layoutRoot;
	}

	void SetInfo(DL_ReadyInfo info, string mySid)
	{
		m_Info = info;
		m_You = info.sids.Find(mySid);
		m_Left = info.seconds;
		if (layoutRoot)
			Build();
	}

	protected TextWidget Text(float x, float y, float w, float h, string text, int color, int align = 0)
	{
		string lay = "DriveLink/gui/layouts/dl_trail_text_left.layout";
		if (align == 1)
			lay = "DriveLink/gui/layouts/dl_trail_text_center.layout";
		else if (align == 2)
			lay = "DriveLink/gui/layouts/dl_trail_text_right.layout";
		TextWidget t = TextWidget.Cast(g_Game.GetWorkspace().CreateWidgets(lay, m_Panel));
		if (!t)
			return null;
		t.SetPos(x, y);
		t.SetSize(w, h);
		t.SetColor(color);
		t.SetSort(5);
		t.SetText(text);
		return t;
	}

	protected void Box(float x, float y, float w, float h, int color, int sort)
	{
		int flags = WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE | WidgetFlags.IGNOREPOINTER | WidgetFlags.STRETCH;
		ImageWidget b = ImageWidget.Cast(g_Game.GetWorkspace().CreateWidget(ImageWidgetTypeID, x, y, w, h, flags, color, sort, m_Panel));
		if (b)
		{
			b.LoadImageFile(0, "#(argb,8,8,3)color(1,1,1,1,CO)");
			b.SetColor(color);
		}
	}

	protected ButtonWidget Button(float x, float y, float w, float h, string text, int color)
	{
		Box(x, y, w, h, color, 2);
		Text(x, y + h * 0.22, w, h * 0.56, text, ARGB(255, 255, 255, 255), 1);
		int flags = WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE;
		ButtonWidget b = ButtonWidget.Cast(g_Game.GetWorkspace().CreateWidget(ButtonWidgetTypeID, x, y, w, h, flags, ARGB(0, 0, 0, 0), 8, m_Panel));
		if (b)
			b.SetHandler(m_Buttons);
		return b;
	}

	protected void Build()
	{
		if (m_Panel)
			m_Panel.Unlink();
		int sw;
		int sh;
		GetScreenSize(sw, sh);
		int flags = WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE;
		m_Panel = g_Game.GetWorkspace().CreateWidget(FrameWidgetTypeID, 0, 0, sw, sh, flags, ARGB(255, 255, 255, 255), 1, layoutRoot);
		DL_ShortInfo t = m_Info.table;
		float rh = sh * 0.024;
		float pw = Math.Min(sw * 0.36, sh * 0.72);
		float pad = rh * 0.8;
		int people = 0;
		if (t && t.people)
			people = t.people.Count();
		int rows = people * 3;
		if (t && t.vehicle != "")
			rows++;
		if (t && t.needSpare)
			rows++;
		float ph = pad * 2 + rh * 1.5 + rh * 1.2 + rh * 1.0 + (rows + 1) * rh + rh * 0.6 + rh * 1.2 * m_Info.names.Count() + rh * 0.4 + rh * 1.8 + rh * 1.2;
		if (m_Info.short)
			ph += rh * 2.2;
		ph += rh * 1.6;                        // the guide button
		int myOdds = -1;
		if (t && m_You >= 0 && m_You < people)
			myOdds = t.people[m_You].odds;
		int nVeh = 0;
		if (t && t.vehicles)
			nVeh = t.vehicles.Count();
		bool anyOdds = myOdds >= 0;
		for (int ov = 0; ov < nVeh; ov++)
		{
			if (t.vehicles[ov].odds >= 0)
				anyOdds = true;
		}
		if (anyOdds)
			ph += rh * 1.2 + nVeh * rh * 1.0;
		float px = sw * 0.025;
		float py = Math.Max(sh * 0.04, Math.Min(sh * 0.16, sh - ph - sh * 0.04));
		Box(px, py, pw, ph, ARGB(225, 18, 21, 25), 1);
		Box(px, py, pw, rh * 0.12, ARGB(255, 214, 170, 90), 2);
		float x = px + pad;
		float w = pw - pad * 2;
		float y = py + pad;
		Text(x, y, w * 0.75, rh * 1.1, "Travel to " + m_Info.to + "?", ARGB(255, 240, 225, 190));
		m_Clock = Text(x + w * 0.75, y, w * 0.25, rh * 0.9, "", ARGB(255, 214, 170, 90), 2);
		y += rh * 1.5;
		if (t)
			Text(x, y, w, rh * 0.7, "Trip: " + t.price, ARGB(255, 170, 170, 170));
		y += rh * 1.2;
		// the hub's odds: mine, then each vehicle's
		if (anyOdds)
		{
			string mine = "Your chance of surviving the trip: unknown";
			if (myOdds >= 0)
				mine = "Your chance of surviving the trip: " + myOdds.ToString() + "%";
			Text(x, y, w, rh * 0.8, mine, DL_ShortMenu.ChanceColor(myOdds), 1);
			y += rh * 1.2;
			for (int vi = 0; vi < nVeh; vi++)
			{
				DL_OddsVehicle v = t.vehicles[vi];
				string vl = v.name + " makes it all the way: unknown";
				if (v.odds >= 0)
					vl = v.name + " makes it all the way: " + v.odds.ToString() + "%";
				Text(x, y, w, rh * 0.7, vl, DL_ShortMenu.ChanceColor(v.odds), 1);
				y += rh;
			}
		}
		// what everyone has | what the trip needs
		float c1 = w * 0.5;
		float c2 = w * 0.25;
		Box(x, y, w, rh * 1.0, ARGB(255, 40, 45, 52), 2);
		Text(x + c1, y + rh * 0.2, c2, rh * 0.6, "Has", ARGB(255, 200, 200, 200), 1);
		Text(x + c1 + c2, y + rh * 0.2, c2, rh * 0.6, "Needs", ARGB(255, 200, 200, 200), 1);
		y += rh * 1.1;
		for (int i = 0; i < people; i++)
		{
			DL_OddsPerson p = t.people[i];
			string who = p.name + ": ";
			if (i == m_You)
				who = "You: ";
			string chance = "";
			if (people > 1 && p.odds >= 0)
				chance = " (chance " + p.odds.ToString() + "%)";
			Row(x, y, c1, c2, rh, who + "health" + chance, Math.Round(p.health).ToString() + "%", "-", p.health < 50);
			y += rh;
			Row(x, y, c1, c2, rh, who + "food", DL_TravelCost.Kcal(p.food), DL_TravelCost.Kcal(p.needFood), p.food < p.needFood);
			y += rh;
			Row(x, y, c1, c2, rh, who + "water", DL_TravelCost.Ml(p.water), DL_TravelCost.Ml(p.needWater), p.water < p.needWater);
			y += rh;
		}
		if (t && t.vehicle != "")
		{
			Row(x, y, c1, c2, rh, "fuel (tanks + canisters)", Math.Floor(t.fuel).ToString() + " L", Math.Ceil(t.needFuel).ToString() + " L", t.fuel < t.needFuel);
			y += rh;
		}
		if (t && t.needSpare)
		{
			string has = "no";
			if (t.spareOk)
				has = "yes";
			Row(x, y, c1, c2, rh, "spare wheel or repair kit", has, "1", !t.spareOk);
			y += rh;
		}
		y += rh * 0.6;
		if (m_Info.short)
		{
			Text(x, y, w, rh * 0.7, "Not enough: " + m_Info.why, ARGB(255, 240, 110, 100), 1);
			y += rh * 1.0;
			string go = "Ready anyway = the road decides.";
			if (!m_Info.canGo)
				go = "You can't travel like this.";
			Text(x, y, w, rh * 0.7, go, ARGB(255, 200, 200, 200), 1);
			y += rh * 1.2;
		}
		// who is ready
		for (int k = 0; k < m_Info.names.Count(); k++)
		{
			string n = m_Info.names[k];
			if (k == m_You)
				n = "You";
			bool r = k < m_Info.ready.Count() && m_Info.ready[k];
			int col = ARGB(255, 170, 170, 170);
			string st = "not ready";
			if (r)
			{
				col = ARGB(255, 120, 210, 120);
				st = "READY";
			}
			Text(x + rh * 0.4, y, w * 0.6, rh * 0.75, n, ARGB(255, 225, 225, 225));
			Text(x + w * 0.6, y, w * 0.4 - rh * 0.4, rh * 0.75, st, col, 2);
			y += rh * 1.2;
		}
		y += rh * 0.4;
		float bw = w * 0.46;
		m_Ready = null;
		m_NotReady = null;
		if (m_Info.canGo && m_You >= 0)
		{
			m_Ready = Button(x, y, bw, rh * 1.6, "[R] Ready", ARGB(255, 60, 100, 60));
			m_NotReady = Button(x + w - bw, y, bw, rh * 1.6, "[N] Not ready", ARGB(255, 70, 76, 86));
		}
		y += rh * 1.8;
		Text(x, y, w, rh * 0.6, "Everyone has to be ready in time, or nobody goes. Not ready calls it off.", ARGB(255, 150, 150, 150), 1);
		y += rh * 1.0;
		m_GuideButton = Button(x, y, w, rh * 1.4, "How does this work? What should I bring?", ARGB(255, 70, 62, 40));
		UpdateClock();
	}

	protected void Row(float x, float y, float c1, float c2, float rh, string label, string have, string need, bool bad)
	{
		int col = ARGB(255, 225, 225, 225);
		if (bad)
			col = ARGB(255, 240, 110, 100);
		Text(x + rh * 0.3, y + rh * 0.15, c1, rh * 0.7, label, ARGB(255, 210, 210, 210));
		Text(x + c1, y + rh * 0.15, c2, rh * 0.7, have, col, 1);
		Text(x + c1 + c2, y + rh * 0.15, c2, rh * 0.7, need, ARGB(255, 210, 210, 210), 1);
	}

	// the clock has been at 0 for a while and nothing came from the server
	bool Expired()
	{
		return m_Left < -5;
	}

	protected void UpdateClock()
	{
		if (m_Clock)
			m_Clock.SetText(Math.Ceil(Math.Max(0, m_Left)).ToString() + " s");
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);
#ifdef DL_DEV
		GuideTest(timeslice);
#endif
		if (m_Guide)
			m_Guide.Update(timeslice);
		m_Left -= timeslice;
		UpdateClock();
		bool r = KeyState(KeyCode.KC_R) != 0;
		bool n = KeyState(KeyCode.KC_N) != 0;
		if (m_KeysHeld)
		{
			m_KeysHeld = r || n;
			return;
		}
		if (r && m_Ready)
		{
			DL_Clicked(m_Ready);
			m_KeysHeld = true;
		}
		else if (n && m_NotReady)
		{
			DL_Clicked(m_NotReady);
			m_KeysHeld = true;
		}
	}

#ifdef DL_DEV
	// -dlGuideTest on a live server: the guide opens from the table and shows the item pages (8 to 11), 3 s each
	protected float m_GuideTestT;
	protected int m_GuideTestPage = -1;

	protected void GuideTest(float dt)
	{
		string ga;
		if (m_GuideTestPage > 10 || !g_Game.CommandlineGetParam("dlGuideTest", ga))
			return;
		m_GuideTestT += dt;
		if (m_GuideTestT < 3)
			return;
		m_GuideTestT = 0;
		if (m_GuideTestPage < 0)
		{
			DL_Clicked(m_GuideButton);
			m_GuideTestPage = 7;
		}
		else
			m_GuideTestPage++;
		if (m_GuideTestPage > 10)
		{
			m_Guide.Close();
			return;
		}
		m_Guide.GoTo(m_GuideTestPage);
		DL_Log.Info("guide test (table): page " + (m_GuideTestPage + 1).ToString());
	}
#endif

	bool DL_Clicked(Widget w)
	{
		if (!w)
			return false;
		if (w == m_GuideButton)
		{
			if (!m_Guide)
				m_Guide = new DL_Guide();
			if (!m_Guide.IsOpen())
				m_Guide.Open(layoutRoot, 100); // on the root: the table under it can rebuild without closing it
			return true;
		}
		if (w == m_Ready)
			outAnswer = "ready|1";
		else if (w == m_NotReady)
			outAnswer = "ready|0";
		else
			return false;
		return true;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_Guide && m_Guide.IsOpen())
			return true;
		if (DL_Clicked(w))
			return true;
		return super.OnClick(w, x, y, button);
	}
}

// holds the mouse (and the keyboard: they are frozen anyway) while the trail is on; the card is drawn into it
class DL_TrailMenu extends UIScriptedMenu
{
	static const int MENU_ID = 4810017;

	void DL_TrailMenu()
	{
		SetID(MENU_ID);
	}

	override Widget Init()
	{
		int sw;
		int sh;
		GetScreenSize(sw, sh);
		int flags = WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE;
		layoutRoot = g_Game.GetWorkspace().CreateWidget(FrameWidgetTypeID, 0, 0, sw, sh, flags, ARGB(255, 255, 255, 255), DL_TrailView.SORT);
		return layoutRoot;
	}

	Widget Root()
	{
		return layoutRoot;
	}
}

class DL_TrailClient
{
	protected ref DL_FilmPlayer m_Film = new DL_FilmPlayer();
	protected ref DL_TrailView m_View = new DL_TrailView();
	protected ref DL_TrailMenu m_Menu;
	protected ref DL_ReadyMenu m_Ready;
	protected ref DL_ReadyInfo m_ReadyInfo;    // waiting to be shown (no other menu open)
	protected Widget m_Black;
	protected bool m_On;                       // the trail is running for him
	protected ref DL_TrailVoteCard m_Card;
	protected ref DL_TrailVoteCard m_Stash;    // the party's card while he answers his own question (card.ask)
	protected ref DL_TrailTally m_Tally;
	protected float m_CardClose;               // seconds until the decided card goes
	protected float m_EndClose;                // seconds until the summary goes
	protected ref map<int, ref array<string>> m_Parts = new map<int, ref array<string>>;
	protected ref map<int, int> m_PartId = new map<int, int>;
#ifdef DL_DEV
	// -dlTrailAuto (unattended tests): ready up, vote at random and lock in by itself, quit the game after the summary
	protected float m_AutoT;
	protected int m_AutoStep;
	protected bool m_AutoQuit;
	protected float m_AutoQuitIn;
#endif
	static const string MISSING = "~dl-part-missing~";

	bool IsOn()
	{
		return m_On;
	}

	protected string MySid()
	{
		Man me = g_Game.GetPlayer();
		if (me && me.GetIdentity())
			return me.GetIdentity().GetPlainId();
		return "";
	}

	// the trail's RPCs, put back together from their parts (DL_SendFilmRpc); true = it was one of them
	bool OnRpc(int rpcType, ParamsReadContext ctx)
	{
		if (rpcType == DL_Const.RPC_TRAIL_GETOUT)
		{
			// he leaves the car on the trail (he stays, or he walks back): the get-out is his own command
			PlayerBase me = PlayerBase.Cast(g_Game.GetPlayer());
			if (me && me.GetCommand_Vehicle())
			{
				me.GetCommand_Vehicle().GetOutVehicle();
				DL_Log.Info("trail: getting out of the car");
			}
			return true;
		}
		if (rpcType < DL_Const.RPC_TRAIL_READY || rpcType > DL_Const.RPC_TRAIL_END)
			return false;
		int id;
		int part;
		int parts;
		string chunk;
		if (!ctx.Read(id) || !ctx.Read(part) || !ctx.Read(parts) || !ctx.Read(chunk) || parts < 1 || part < 0 || part >= parts)
			return true;
		array<string> got = m_Parts.Get(rpcType);
		if (!got || m_PartId.Get(rpcType) != id || got.Count() != parts)
		{
			got = new array<string>;
			for (int i = 0; i < parts; i++)
				got.Insert(MISSING);
			m_Parts.Set(rpcType, got);
			m_PartId.Set(rpcType, id);
		}
		got[part] = chunk;
		string json = "";
		foreach (string c : got)
		{
			if (c == MISSING)
				return true;                   // more to come
			json += c;
		}
		m_Parts.Remove(rpcType);
		Take(rpcType, json);
		return true;
	}

	protected void Take(int rpcType, string json)
	{
		JsonSerializer js = new JsonSerializer();
		string err;
		if (rpcType == DL_Const.RPC_TRAIL_READY)
		{
			if (json == "")
			{
				m_ReadyInfo = null;
				CloseReady();
				return;
			}
			if (m_On)
				return;                        // a late table while the trip runs: it would come back after it
			DL_ReadyInfo r;
			if (js.ReadFromString(r, json, err) && r)
				m_ReadyInfo = r;
			return;
		}
		if (rpcType == DL_Const.RPC_TRAIL_START)
		{
			DL_Film f;
			if (!js.ReadFromString(f, json, err) || !f)
			{
				DL_Log.Warn("trail film unreadable: " + err);
				return;
			}
			m_ReadyInfo = null;
			CloseReady();
			Start(f);
			return;
		}
		if (!m_On)
			return;
		if (rpcType == DL_Const.RPC_TRAIL_CARD)
		{
			DL_TrailVoteCard card;
			if (!js.ReadFromString(card, json, err) || !card)
				return;
			if (card.ask && m_Card && !m_Card.ask)
				m_Stash = m_Card;                 // his own question first, the party's card after
			m_Card = card;
			m_Tally = null;
			m_CardClose = 0;
			ShowCard();
		}
		else if (rpcType == DL_Const.RPC_TRAIL_TALLY)
		{
			DL_TrailTally t;
			if (!js.ReadFromString(t, json, err) || !t)
				return;
			m_Tally = t;
			m_View.UpdateTally(t);
			if (t.result != -9)
				m_CardClose = 6;
		}
		else if (rpcType == DL_Const.RPC_TRAIL_ROAD)
		{
			DL_TrailRoad road;
			if (!js.ReadFromString(road, json, err) || !road)
				return;
			m_Film.Append(road.events, road.km);
			if (road.last)
				m_Film.SetHold(false);
		}
		else if (rpcType == DL_Const.RPC_TRAIL_END)
		{
			DL_TrailSummary s;
			if (!js.ReadFromString(s, json, err) || !s)
				return;
			m_Card = null;
			m_Film.SetHold(false);
			OpenMenu();
			Widget root = null;
			if (m_Menu)
				root = m_Menu.Root();
			m_View.ShowSummary(s, root);
			m_EndClose = 8;
			DL_Log.Info("trail: " + s.title);
		}
	}

	protected void Start(DL_Film f)
	{
		Stop();
		m_On = true;
		int sw;
		int sh;
		GetScreenSize(sw, sh);
		int flags = WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE | WidgetFlags.IGNOREPOINTER | WidgetFlags.STRETCH;
		ImageWidget b = ImageWidget.Cast(g_Game.GetWorkspace().CreateWidget(ImageWidgetTypeID, 0, 0, sw, sh, flags, ARGB(255, 0, 0, 0), 31400));
		if (b)
		{
			b.LoadImageFile(0, "#(argb,8,8,3)color(1,1,1,1,CO)");
			b.SetColor(ARGB(255, 0, 0, 0));
		}
		m_Black = b;
		m_Film.Play(f);
		OpenMenu();
		DL_Log.Info("trail: the film starts (" + f.events.Count().ToString() + " scenes in the first chapter)");
	}

	void Stop()
	{
		m_On = false;
		m_Card = null;
		m_Tally = null;
		m_View.Close();
		if (m_Film.IsPlaying())
			m_Film.Stop();
		if (m_Black)
			m_Black.Unlink();
		m_Black = null;
		if (m_Menu)
			m_Menu.Close();
		m_Menu = null;
		m_ReadyInfo = null;
		CloseReady();
	}

	protected void OpenMenu()
	{
		if (m_Menu && g_Game.GetUIManager().GetMenu() == m_Menu)
			return;
		if (g_Game.GetUIManager().GetMenu() && g_Game.GetUIManager().GetMenu() != m_Menu)
			return;                            // something else is open (inventory, chat): next frame
		m_Menu = new DL_TrailMenu();
		g_Game.GetUIManager().ShowScriptedMenu(m_Menu, null);
		if (m_Card)
			ShowCard();
	}

	protected void ShowCard()
	{
		if (!m_Card)
			return;
		Widget root = null;
		if (m_Menu)
			root = m_Menu.Root();
		int you = m_Card.sids.Find(MySid());
		bool alone = you >= 0 && you < m_Card.alone.Count() && m_Card.alone[you];
		if (m_Card.watch)
			you = -1;
		m_View.ShowCard(m_Card, you, root, alone);
		if (m_Tally)
			m_View.UpdateTally(m_Tally);
	}

	protected void CloseReady()
	{
		if (m_Ready)
			m_Ready.Close();
		m_Ready = null;
	}

	protected void Send(string what)
	{
		PlayerBase me = PlayerBase.Cast(g_Game.GetPlayer());
		if (!me)
			return;
		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(what);
		rpc.Send(me, DL_Const.RPC_TRAIL_IN, true, null);
	}

#ifdef DL_DEV
	protected void Auto(float dt)
	{
		string arg;
		if (!g_Game.CommandlineGetParam("dlTrailAuto", arg))
			return;
		m_AutoT += dt;
		DL_TrailView.s_AutoInGame = true;
		if (!m_Ready && m_AutoStep == 0)
			m_AutoT = 0;                       // the table isn't up yet: count from when it is
		string gta;
		float readyAfter = 6;
		if (g_Game.CommandlineGetParam("dlGuideTest", gta))
			readyAfter = 18;                   // the guide test (DL_ReadyMenu.GuideTest) goes first
		if (m_Ready && m_AutoStep == 0 && m_AutoT > readyAfter)
		{
			Send("ready|1");
			DL_Log.Info("trail auto: ready");
			m_AutoStep = 1;
		}
		if (m_On && m_Card && !m_Tally && m_AutoStep != 2)
		{
			m_AutoStep = 2;
			m_AutoT = 0;
		}
		if (m_AutoStep == 2 && m_Card && m_AutoT > 5 && arg == "alone" && !m_Card.ask)
		{
			Send("alone|1");                  // -dlTrailAuto=alone: turn back alone on the first card
			DL_Log.Info("trail auto: turning back alone");
			m_AutoStep = 3;
		}
		if (m_AutoStep == 2 && m_Card && m_AutoT > 5)
		{
			int pick = Math.RandomInt(0, m_Card.choices.Count());
			Send("pick|" + pick.ToString());
			Send("lock|1");
			DL_Log.Info("trail auto: voted " + m_Card.choices[pick].label);
			m_AutoStep = 3;
		}
		if (m_AutoStep == 3 && !m_Card)
			m_AutoStep = 1;
		if (m_EndClose > 0 && !m_AutoQuit)
		{
			m_AutoQuit = true;
			m_AutoQuitIn = 15;
			DL_Log.Info("trail auto: the trail is over, quitting in 15 s");
		}
		if (m_AutoQuit)
		{
			m_AutoQuitIn -= dt;
			if (m_AutoQuitIn <= 0 && m_AutoQuitIn > -1)
			{
				m_AutoQuitIn = -5;
				g_Game.RequestExit(0);
			}
		}
	}
#endif

	void Update(float dt)
	{
#ifdef DL_DEV
		Auto(dt);
#endif
		// the ready-up table
		if (m_ReadyInfo && !m_On)
		{
			if (!m_Ready && !g_Game.GetUIManager().GetMenu())
			{
				m_Ready = new DL_ReadyMenu();
				m_Ready.SetInfo(m_ReadyInfo, MySid());
				g_Game.GetUIManager().ShowScriptedMenu(m_Ready, null);
				m_ReadyInfo = null;
			}
			else if (m_Ready)
			{
				m_Ready.SetInfo(m_ReadyInfo, MySid());
				m_ReadyInfo = null;
			}
		}
		if (m_Ready && g_Game.GetUIManager().GetMenu() != m_Ready)
			m_Ready = null;                    // closed with Esc: it comes back with the next update
		if (m_Ready && m_Ready.outAnswer != "")
		{
			string answer = m_Ready.outAnswer;
			Send(answer);
			m_Ready.outAnswer = "";
			if (answer == "ready|0")
			{
				m_ReadyInfo = null;            // Not ready calls the trip off (the server says so to everyone)
				CloseReady();
			}
		}
		if (m_Ready && m_Ready.Expired())
		{
			m_ReadyInfo = null;                // the clock ran out and the server never closed it: don't leave it stuck
			CloseReady();
		}
		if (!m_On)
			return;
		m_Film.Update(dt);
		m_View.Update(dt);
		if (g_Game.GetUIManager().GetMenu() != m_Menu)
			OpenMenu();                        // Esc closed it: the trail isn't over
		int pick;
		bool lock;
		bool alone;
		if (m_View.TakeOut(pick, lock, alone))
		{
			if (pick != -9 && m_Card && m_Card.ask)
			{
				Send("ask|" + pick.ToString());
				m_Card = m_Stash;
				m_Stash = null;
				m_Tally = null;
				m_View.Close();
				if (m_Card)
					ShowCard();
				return;
			}
			if (pick != -9)
				Send("pick|" + pick.ToString());
			if (lock)
				Send("lock|1");
			if (alone)
				Send("alone|1");
		}
		if (m_CardClose > 0)
		{
			m_CardClose -= dt;
			if (m_CardClose <= 0)
			{
				m_Card = null;
				m_View.Close();
			}
		}
		if (m_EndClose > 0)
		{
			m_EndClose -= dt;
			if (m_EndClose <= 0)
				Stop();
		}
	}
}
