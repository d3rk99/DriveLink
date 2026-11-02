// DriveLink travel guide: pages on how DriveLink works and what to pack, opened from the pause menu (a button under
// Options) and from the ready-up table. An overlay drawn into whichever menu opened it (that menu keeps the mouse).
// The item pictures are the DayZ wiki's item images in one atlas (Tools/guide_icons.py -> gui/anim/guide_items_ca.paa).

class DL_GuideItem
{
	string type;                  // the item class shown, e.g. "Compass"
	string name;
	string text;

	void DL_GuideItem(string t, string n, string x)
	{
		type = t;
		name = n;
		text = x;
	}
}

class DL_GuidePage
{
	string tab;                   // short name in the list on the left
	string title;
	string body;
	ref array<ref DL_GuideItem> items = new array<ref DL_GuideItem>;

	void DL_GuidePage(string t, string ti, string b)
	{
		tab = t;
		title = ti;
		body = b;
	}

	DL_GuidePage Item(string type, string name, string text)
	{
		items.Insert(new DL_GuideItem(type, name, text));
		return this;
	}
}

// the guide's buttons: their clicks go straight to the guide (not through the menu that opened it)
class DL_GuideButtons : ScriptedWidgetEventHandler
{
	protected DL_Guide m_Guide;

	void DL_GuideButtons(DL_Guide guide)
	{
		m_Guide = guide;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_Guide)
			return m_Guide.OnClick(w, x, y, button);
		return false;
	}
}

class DL_Guide
{
	protected ref DL_GuideButtons m_Handler;
	protected ref array<ref DL_GuidePage> m_Pages;
	protected Widget m_Root;                   // the overlay, under the opening menu's root
	protected Widget m_Content;                // this page's widgets
	protected int m_Page;
	protected ButtonWidget m_Prev;
	protected ButtonWidget m_Next;
	protected ButtonWidget m_Close;
	protected ref array<ButtonWidget> m_Tabs = new array<ButtonWidget>;
	protected ref array<ref DL_Sprite> m_Icons = new array<ref DL_Sprite>;
	// the atlas' cells in order (Tools/guide_icons.py ITEMS), 8 to a row of 128 x 128
	static const ref array<string> ICONS = {"BakedBeansCan", "WaterBottle", "CanisterGasoline", "TireRepairKit", "BandageDressing", "HuntingKnife", "Compass", "GPSReceiver", "Rope", "EpoxyPutty", "DuctTape", "Shovel", "Mosin9130", "FishingRod", "Matchbox", "PetrolLighter", "Canteen", "Headtorch_Grey", "CharcoalTablets", "TetracyclineAntibiotics", "GasMask", "Heatpack", "FirstAidKit", "Flashlight"};
	protected float m_Sw;
	protected float m_Sh;
	protected float m_X;
	protected float m_Y;
	protected float m_W;
	protected float m_H;

	static int Gold()
	{
		return ARGB(255, 214, 170, 90);
	}

	void DL_Guide()
	{
		m_Handler = new DL_GuideButtons(this);
		m_Pages = Pages();
	}

	void ~DL_Guide()
	{
		Close();
	}

	bool IsOpen()
	{
		return m_Root != null;
	}

	int PageCount()
	{
		return m_Pages.Count();
	}

	void GoTo(int n)
	{
		if (m_Root)
			ShowPage(n);
	}

	void Open(Widget parent, int sort)
	{
		Close();
		int sw;
		int sh;
		GetScreenSize(sw, sh);
		m_Sw = sw;
		m_Sh = sh;
		int flags = WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE;
		m_Root = g_Game.GetWorkspace().CreateWidget(FrameWidgetTypeID, 0, 0, sw, sh, flags, ARGB(255, 255, 255, 255), sort, parent);
		Box(m_Root, 0, 0, sw, sh, ARGB(200, 4, 6, 8), 0, false);         // the dim behind; it takes the clicks too
		m_W = Math.Min(sw * 0.74, sh * 1.3);
		m_H = sh * 0.82;
		m_X = (sw - m_W) / 2;
		m_Y = (sh - m_H) / 2;
		Box(m_Root, m_X, m_Y, m_W, m_H, ARGB(245, 18, 21, 25), 1, true);
		Box(m_Root, m_X, m_Y, m_W, sh * 0.004, Gold(), 2, true);
		float rh = sh * 0.024;
		Text(m_Root, m_X + rh, m_Y + rh * 0.6, m_W * 0.6, rh * 1.2, "DRIVELINK TRAVEL GUIDE", Gold(), 0);
		m_Close = Button(m_Root, m_X + m_W - rh * 5.2, m_Y + rh * 0.5, rh * 4.6, rh * 1.4, "Close", ARGB(255, 70, 76, 86));
		// the pages on the left
		m_Tabs.Clear();
		float ty = m_Y + rh * 2.6;
		float tw = m_W * 0.24;
		for (int i = 0; i < m_Pages.Count(); i++)
		{
			m_Tabs.Insert(Button(m_Root, m_X + rh * 0.6, ty, tw, rh * 1.35, m_Pages[i].tab, ARGB(255, 34, 39, 46), true));
			ty += rh * 1.55;
		}
		m_Prev = Button(m_Root, m_X + m_W * 0.27, m_Y + m_H - rh * 2.1, rh * 6, rh * 1.5, "< Back", ARGB(255, 70, 76, 86));
		m_Next = Button(m_Root, m_X + m_W - rh * 6.6, m_Y + m_H - rh * 2.1, rh * 6, rh * 1.5, "Next >", ARGB(255, 60, 100, 60));
		ShowPage(m_Page);
	}

	void Close()
	{
		ClearItems();
		if (m_Root)
			m_Root.Unlink();
		m_Root = null;
		m_Content = null;
		m_Tabs.Clear();
	}

	// nothing moves for now; the menus call it every frame
	void Update(float dt)
	{
	}

	bool OnClick(Widget w, int x, int y, int button)
	{
		if (!m_Root)
			return false;
		if (w == m_Close)
		{
			Close();
			return true;
		}
		if (w == m_Prev && m_Page > 0)
		{
			ShowPage(m_Page - 1);
			return true;
		}
		if (w == m_Next && m_Page < m_Pages.Count() - 1)
		{
			ShowPage(m_Page + 1);
			return true;
		}
		int t = m_Tabs.Find(ButtonWidget.Cast(w));
		if (t >= 0)
		{
			ShowPage(t);
			return true;
		}
		return false;
	}

	protected void ClearItems()
	{
		m_Icons.Clear();
	}

	protected void ShowPage(int n)
	{
		m_Page = Math.Clamp(n, 0, m_Pages.Count() - 1);
		ClearItems();
		if (m_Content)
			m_Content.Unlink();
		int flags = WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE;
		m_Content = g_Game.GetWorkspace().CreateWidget(FrameWidgetTypeID, 0, 0, m_Sw, m_Sh, flags, ARGB(255, 255, 255, 255), 5, m_Root);
		DL_GuidePage p = m_Pages[m_Page];
		float rh = m_Sh * 0.024;
		// the open page's tab lit
		for (int i = 0; i < m_Tabs.Count(); i++)
		{
			if (i == m_Page)
			{
				float tx;
				float tyy;
				m_Tabs[i].GetPos(tx, tyy);
				Box(m_Content, tx - rh * 0.25, tyy, rh * 0.18, rh * 1.35, Gold(), 9, true);
			}
		}
		float cx = m_X + m_W * 0.27;
		float cw = m_W * 0.71;
		float cy = m_Y + rh * 2.6;
		Text(m_Content, cx, cy, cw, rh * 1.3, p.title, ARGB(255, 240, 225, 190), 0);
		cy += rh * 2.0;
		float bottom = m_Y + m_H - rh * 2.6;
		if (p.body != "")
		{
			float bh = bottom - cy;
			if (p.items.Count() > 0)
				bh = rh * 2.6;
			Body(m_Content, cx, cy, cw, bh, p.body, ARGB(255, 214, 214, 214), m_Sh * 0.0185);
			cy += bh + rh * 0.4;
		}
		// items: 2 columns, the item turning on the left of its text
		int cols = 2;
		float gap = rh * 0.6;
		float cellW = (cw - gap) / cols;
		int rows = Math.Ceil(p.items.Count() / 2.0);
		if (rows < 1)
			rows = 1;
		float cellH = (bottom - cy - gap * (rows - 1)) / rows;
		for (int k = 0; k < p.items.Count(); k++)
		{
			DL_GuideItem it = p.items[k];
			int row = k / cols;
			int col = k - row * cols;
			float ix = cx + col * (cellW + gap);
			float iy = cy + row * (cellH + gap);
			float pic = Math.Min(cellH - rh * 0.6, cellW * 0.34);
			Box(m_Content, ix, iy, cellW, cellH, ARGB(255, 28, 32, 38), 1, true);
			Box(m_Content, ix + rh * 0.3, iy + rh * 0.3, pic, pic, ARGB(255, 52, 58, 66), 2, true);
			Preview(m_Content, it.type, ix + rh * 0.3, iy + rh * 0.3, pic, pic);
			float tx2 = ix + pic + rh * 0.8;
			float tw2 = cellW - pic - rh * 1.2;
			Text(m_Content, tx2, iy + rh * 0.3, tw2, rh * 0.95, it.name, Gold(), 0);
			Body(m_Content, tx2, iy + rh * 1.4, tw2, cellH - rh * 1.6, it.text, ARGB(255, 205, 205, 205), m_Sh * 0.0158);
		}
		Text(m_Content, m_X + m_W * 0.27, m_Y + m_H - rh * 1.85, m_W * 0.71 - rh * 0.6, rh * 1.0, "Page " + (m_Page + 1).ToString() + " of " + m_Pages.Count().ToString(), ARGB(255, 150, 150, 150), 1);
		m_Prev.Show(m_Page > 0);
		m_Next.Show(m_Page < m_Pages.Count() - 1);
	}

	protected void Preview(Widget parent, string type, float x, float y, float w, float h)
	{
		int n = ICONS.Find(type);
		if (n < 0)
			return;
		int row = n / 8;
		int col = n - row * 8;
		DL_Sprite icon = new DL_Sprite(parent, "DriveLink/gui/anim/guide_items_ca.paa", 10, w, h, 1024, 512);
		icon.Cell(col * 128, row * 128, 128, 128);
		icon.MoveTo(x, y);
		m_Icons.Insert(icon);
	}

	protected void Box(Widget parent, float x, float y, float w, float h, int color, int sort, bool ignore)
	{
		int flags = WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE | WidgetFlags.STRETCH;
		if (ignore)
			flags = flags | WidgetFlags.IGNOREPOINTER;
		ImageWidget b = ImageWidget.Cast(g_Game.GetWorkspace().CreateWidget(ImageWidgetTypeID, x, y, w, h, flags, color, sort, parent));
		if (b)
		{
			b.LoadImageFile(0, "#(argb,8,8,3)color(1,1,1,1,CO)");
			b.SetColor(color);
		}
	}

	protected TextWidget Text(Widget parent, float x, float y, float w, float h, string text, int color, int align)
	{
		string lay = "DriveLink/gui/layouts/dl_trail_text_left.layout";
		if (align == 1)
			lay = "DriveLink/gui/layouts/dl_trail_text_center.layout";
		TextWidget t = TextWidget.Cast(g_Game.GetWorkspace().CreateWidgets(lay, parent));
		if (!t)
			return null;
		t.SetPos(x, y);
		t.SetSize(w, h);
		t.SetColor(color);
		t.SetSort(12);
		t.SetText(text);
		return t;
	}

	// a text block; paragraphs (split by a blank line) each get their own widget with a gap between them, since a
	// multiline text widget drops empty lines. Their heights are estimated from the text's length and the width.
	protected void Body(Widget parent, float x, float y, float w, float h, string text, int color, float size)
	{
		array<string> paras = new array<string>;
		text.Replace("\n\n", "|");
		text.Split("|", paras);
		float lineH = size * 1.32;
		float charW = size * 0.56;
		float bottom = y + h;
		for (int pi = 0; pi < paras.Count(); pi++)
		{
			string para = paras[pi];
			array<string> lines = new array<string>;
			para.Split("\n", lines);
			int count = 0;
			foreach (string ln : lines)
				count += Math.Max(1, Math.Ceil(ln.Length() * charW / w));
			float ph = (count + 0.4) * lineH;
			if (pi == paras.Count() - 1)
				ph = Math.Max(ph, bottom - y);       // the last one may use all the room left
			MultilineTextWidget t = MultilineTextWidget.Cast(g_Game.GetWorkspace().CreateWidgets("DriveLink/gui/layouts/dl_guide_body.layout", parent));
			if (!t)
				return;
			t.SetPos(x, y);
			t.SetSize(w, Math.Max(lineH, Math.Min(ph, bottom - y)));
			t.SetColor(color);
			t.SetSort(12);
			t.SetTextExactSize(Math.Round(size));
			t.SetText(para);
			y += ph + size * 0.7;
		}
	}

	protected ButtonWidget Button(Widget parent, float x, float y, float w, float h, string text, int color, bool left = false)
	{
		Box(parent, x, y, w, h, color, 2, true);
		int align = 1;
		float tx = x;
		if (left)
		{
			align = 0;
			tx = x + h * 0.4;
		}
		Text(parent, tx, y + h * 0.2, w - (tx - x), h * 0.6, text, ARGB(255, 235, 235, 235), align);
		int flags = WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE;
		ButtonWidget b = ButtonWidget.Cast(g_Game.GetWorkspace().CreateWidget(ButtonWidgetTypeID, x, y, w, h, flags, ARGB(0, 0, 0, 0), 20, parent));
		if (b)
			b.SetHandler(m_Handler);
		return b;
	}

	// ---------------- the pages ----------------

	static array<ref DL_GuidePage> Pages()
	{
		array<ref DL_GuidePage> ps = new array<ref DL_GuidePage>;
		ps.Insert(new DL_GuidePage("What is DriveLink", "What is DriveLink?",
			"DriveLink links DayZ servers, and their maps, into one big world.\n\nWalk, drive, ride or sail out through a leaving zone at the edge of a map and you arrive on the next server: with your character, everything you carry, your car and your crew.\n\nYour character is only ever on one server at a time. While you are on the road, the hub (the server network's travel office) holds you until you arrive.\n\nThe road between maps is not free. Depending on the server it costs food, water and fuel, it can take time, and things can happen on the way. This guide shows you how a trip works and what to bring."));
		ps.Insert(new DL_GuidePage("Leaving and arriving", "Leaving and arriving",
			"Leaving zones sit at the edges of the map. Your map and the server's info show where they lead.\n\nStep into one, alone, with your group on foot, or in a convoy of cars or boats. Everyone who enters the zone together travels together.\n\nA countdown starts. Walk or drive back out of the zone at any time to call the trip off.\n\nYou arrive at the next server's arrival point. A car that comes along is parked at its car spot, and a convoy lines up behind it. For a few seconds after arriving you can't be hurt."));
		ps.Insert(new DL_GuidePage("The ready-up table", "Before you go: the ready-up table",
			"In the zone, everyone sees the same table: each person's health, food and water, the car's fuel (tanks and canisters both count) and, on long trips, whether you carry a spare wheel or a tyre repair kit. Red means short.\n\nThe hub also works out your chance of surviving the trip, and each vehicle's chance of making it all the way.\n\nPress Ready (R) or Not ready (N). Everyone has to be ready before the clock runs out, or nobody goes: leave the zone and come back to try again.\n\nShort of something? Some servers refuse the trip. Others let you ready up anyway, and then the road decides."));
		ps.Insert(new DL_GuidePage("Road events", "On the road: events",
			"Once everyone is ready, the screen goes dark and the trip plays out as a film. You can't be hurt while it plays, and hunger and thirst stop.\n\nAlong the way, events come up as cards: a river with the bridge out, a roadblock, a horde on the road, a fork, a boiling radiator, a deer or a lake, a stranger, a wounded survivor, a helicopter crash, mud, fever, nightfall, a blizzard, a gas zone.\n\nEach card has a few choices, and each choice shows a hint: safe, risky or dangerous. The exact odds stay hidden. What you carry changes them: the card shows which items help, and who has them.\n\nSome choices only appear when somebody carries the right thing, like Follow the compass or Patch the leak. Longer trips bring more cards."));
		ps.Insert(new DL_GuidePage("Voting and turning back", "Voting and turning back",
			"Everyone votes on each card and can change their vote until they lock it in.\n\n- The choice with the most votes wins.\n- A tie goes to the lead driver's pick.\n- If nobody votes, the careful choice wins.\n\nTurn back (all of us) needs more than half of the party.\n\nTurn back alone can be pressed any time a card is up. A driver turns back with the car. Walking back on your own is riskier, and the further into the trip you are, the riskier it gets. When a driver turns back, each passenger chooses to go with them or stay with the convoy.\n\nBack at the zone you are as you are: anything used stays used, fuel burned is gone. Disconnecting during the trip counts as turning back."));
		ps.Insert(new DL_GuidePage("Time and delays", "Time, delays and costs",
			"Each server sets its own rules for trips that start there, from free and instant to real-world costs.\n\nCosts are counted per 10 km of the trip: food, water and fuel. A full DayZ stomach bar is about 5000 kcal; a can of beans is 800 kcal and a water bottle 1 L.\n\nOn servers with travel time you are on the road until you arrive, and you can't log in anywhere else meanwhile.\n\nEvents can add to it. A wrong turn adds kilometres and time. Waiting out a horde, resting a fever or making camp for the night all take hours. Some choices save time instead: a compass shortcut, or the survivor you helped telling you a quicker way.\n\nIf the car is lost on the way, the rest of the trip is on foot, at walking speed.\n\nOn instant-arrival servers there is no clock, so choices that would cost time carry more risk instead."));
		ps.Insert(new DL_GuidePage("What can go wrong", "What can go wrong",
			"The road can wound you, take some of your gear, take the car, or kill you.\n\nA death on the road is a real death. You respawn fresh on the starting server, and your gear is lost: it does not go to the rest of your group.\n\nEating raw meat or fish can make you sick, and a sick member slows the party down. A bandage, rags or a first aid kit turns a deadly wound into a bad one.\n\nAt the end everyone sees a summary: how long it took, who was hurt, and what was lost.\n\nRead the survival chance on the ready-up table. If it is low, top up first: eat, drink, fill the tank, heal up."));
		DL_GuidePage p = new DL_GuidePage("Pack: the basics", "Packing list: the basics",
			"The ready-up table checks these first. The trip is refused, or gets dangerous, without them.");
		p.Item("BakedBeansCan", "Food", "Every 10 km costs food. The ready-up table shows what you have and what you need. Closed cans need an opener or a knife.");
		p.Item("WaterBottle", "Water", "Every 10 km costs water too. Fill bottles and canteens before you leave.");
		p.Item("CanisterGasoline", "Fuel", "Cars and boats burn fuel per 10 km. Full jerry cans in the car count for the trip as well as the tank.");
		p.Item("TireRepairKit", "Tyre repair kit", "Long trips need a spare wheel of your car's type or a tyre repair kit, for flats on the way.");
		p.Item("BandageDressing", "Bandages", "A bandage, rags or a first aid kit turns a deadly wound on the road into a bad one. Patch up a wounded survivor and they may know a shortcut.");
		p.Item("HuntingKnife", "Knife", "Opens cans, and you'll want one for whatever you hunt or catch on the way.");
		ps.Insert(p);
		p = new DL_GuidePage("Pack: the road", "Packing list: the road",
			"Tools that open better choices on the road cards.");
		p.Item("Compass", "Compass", "At a fork, Follow the compass always finds the right way. It also helps if you push on through a blizzard.");
		p.Item("GPSReceiver", "GPS", "Does the same job as a compass: the sure way at a fork, and safer going in a blizzard.");
		p.Item("Rope", "Rope", "A safer river crossing, by car or on foot, and a tow out of the mud from a second car.");
		p.Item("EpoxyPutty", "Epoxy putty", "A boiling radiator: Patch the leak works with this or duct tape, and the car is good as new.");
		p.Item("DuctTape", "Duct tape", "Patches a leaking radiator just like epoxy putty does.");
		p.Item("Shovel", "Shovel", "Stuck to the axles: a shovel, field shovel or farming hoe digs the car out.");
		ps.Insert(p);
		p = new DL_GuidePage("Pack: food on the way", "Packing list: food on the way",
			"Hunting and fishing on the road feed the whole party, if you can cook what you catch.");
		p.Item("Mosin9130", "A rifle", "A deer only shows up if someone has a rifle. Rifles also make a fight at a roadblock or a look at a heli crash safer. Any gun lets you fight or rob.");
		p.Item("FishingRod", "Fishing rod", "Fish at a lake, safer but slower than hunting. Hooks work too.");
		p.Item("Matchbox", "Matches", "Cook the meat and fish, or eat them raw and risk getting sick. A fire also makes camp at night or shelter in a blizzard safer.");
		p.Item("PetrolLighter", "Lighter", "Starts a fire just like matches do. So does a hand drill kit.");
		p.Item("Canteen", "Canteen", "Water for you, and for the radiator: let it cool, then top it up.");
		p.Item("Headtorch_Grey", "Headtorch", "Keep going after dark with a headtorch, flashlight or spotlight. It's safer than going without.");
		ps.Insert(p);
		p = new DL_GuidePage("Pack: trouble", "Packing list: when it goes wrong",
			"Things you hope not to need.");
		p.Item("CharcoalTablets", "Charcoal tablets", "Fever on the road: give medicine instead of resting a day or pushing on sick. Tetracycline and painkillers work too.");
		p.Item("TetracyclineAntibiotics", "Tetracycline", "Medicine for a fever on the way, the strongest choice against it.");
		p.Item("GasMask", "Gas mask", "A toxic zone across the road: masks on and straight through, with no detour.");
		p.Item("Heatpack", "Heat pack", "In a blizzard, shelter and wait is safer with a fire or a heat pack.");
		p.Item("FirstAidKit", "First aid kit", "Counts as a bandage: patch a survivor, and it makes a deadly wound on the road a bad one.");
		p.Item("Flashlight", "Flashlight", "Lights the way at night if you have no headtorch.");
		ps.Insert(p);
		ps.Insert(new DL_GuidePage("Tips", "Tips for a good trip",
			"- Check the ready-up table before you press Ready. Red rows and a low survival chance mean top up first.\n\n- Travel as a group. More people means more hands for the cards, more gear between you, and someone to carry what you forgot.\n\n- Spread the useful items around the group. The card shows who has what, and anyone's item counts.\n\n- Fill up before the zone: a full tank plus a jerry can, food and water for everyone.\n\n- Bring a compass, rope and something to make fire. They come up again and again.\n\n- When in doubt, the careful choice is usually slower but safer. Time lost on the road is better than a life.\n\n- Walking back out of the zone, or turning back alone, is always an option."));
		return ps;
	}
}

// the pause menu: a button under Options opens the guide
modded class InGameMenu
{
	protected Widget m_DLGuideButton;
	protected ref DL_Guide m_DLGuide;

	override Widget Init()
	{
		Widget root = super.Init();
		SpacerBaseWidget top = SpacerBaseWidget.Cast(root.FindAnyWidget("top"));
		if (top && m_OptionsButton)
		{
			m_DLGuideButton = g_Game.GetWorkspace().CreateWidgets("DriveLink/gui/layouts/dl_guide_button.layout");
			if (m_DLGuideButton)
				top.AddChildAfter(m_DLGuideButton, m_OptionsButton);
		}
		return root;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_DLGuide && m_DLGuide.IsOpen())
			return true;                       // the guide's own buttons have their handler; the rest is the dim
		if (w && w == m_DLGuideButton)
		{
			if (!m_DLGuide)
				m_DLGuide = new DL_Guide();
			m_DLGuide.Open(layoutRoot, 100);
			return true;
		}
		return super.OnClick(w, x, y, button);
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);
		if (m_DLGuide)
			m_DLGuide.Update(timeslice);
	}
}

#ifdef DL_DEV
// -dlGuideTest (dev builds): the guide opens in the main menu, shows each page for 3 s, then the game quits
modded class MissionMainMenu
{
	protected ref DL_Guide m_DLGuideTest;
	protected float m_DLGuideT;
	protected int m_DLGuidePage;

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		string arg;
		if (!g_Game.CommandlineGetParam("dlGuideTest", arg))
			return;
		m_DLGuideT += timeslice;
		if (!m_DLGuideTest)
		{
			if (m_DLGuideT < 8)
				return;
			m_DLGuideTest = new DL_Guide();
			m_DLGuideTest.Open(null, 30000);
			m_DLGuideT = 0;
			DL_Log.Info("guide test: page 1");
			return;
		}
		m_DLGuideTest.Update(timeslice);
		if (m_DLGuideT < 3)
			return;
		m_DLGuideT = 0;
		m_DLGuidePage++;
		if (m_DLGuidePage >= m_DLGuideTest.PageCount())
		{
			DL_Log.Info("guide test: done");
			g_Game.RequestExit(0);
			return;
		}
		m_DLGuideTest.GoTo(m_DLGuidePage);
		DL_Log.Info("guide test: page " + (m_DLGuidePage + 1).ToString());
	}
}
#endif
