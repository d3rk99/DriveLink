// DriveLink interactive trail - the choice card, the votes on it and the summary at the end, drawn over the road
// film (DL_Trail, Research/interactive-trail-design.md). Widgets made in code like DL_ShortMenu (the default font is
// about 3/4 of its box, so every text gets a box sized to the screen and long text is wrapped by hand). The client
// menu (DL_TrailMenu, 5_Mission) gives it the mouse; clicks and keys come back as Pick / Lock / Alone.
class DL_TrailButtons : ScriptedWidgetEventHandler
{
	protected DL_TrailView m_View;

	void DL_TrailButtons(DL_TrailView view)
	{
		m_View = view;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_View)
			return m_View.Clicked(w);
		return false;
	}
}

class DL_TrailView
{
	static const int SORT = 31600;     // over the road film (31500)
	static bool s_AutoInGame;          // dev -dlTrailAuto: he was in a server (back in the main menu = the test is over)
	protected Widget m_Root;
	protected ref DL_TrailButtons m_Handler;
	protected ref DL_TrailVoteCard m_Card;
	protected int m_You = -1;          // my place in the card's names (-1 = watching)
	protected ref array<ButtonWidget> m_Buttons = new array<ButtonWidget>;   // one per choice, then turn back
	protected ref array<ImageWidget> m_RowBox = new array<ImageWidget>;
	protected ref array<TextWidget> m_RowVotes = new array<TextWidget>;
	protected ButtonWidget m_LockBtn;
	protected ButtonWidget m_AloneBtn;
	protected TextWidget m_AloneText;
	protected TextWidget m_Timer;
	protected TextWidget m_Status;
	protected ref array<TextWidget> m_Outcome = new array<TextWidget>;
	protected float m_RowW;
	protected float m_Rh;
	protected float m_TextX;
	protected float m_TextW;
	protected int m_Mine = -1;         // what I picked (-1 none, -2 turn back)
	protected bool m_Locked;
	protected bool m_Decided;
	protected float m_AloneArmed;      // seconds the "are you sure" stays up
	protected int m_KeyHeld = -1;
	protected bool m_HasBack;          // the party's card has "turn back (all of us)"; his own question (ask) not
	// what the client sends (DL_TrailMenu / the dev test reads them once)
	int outPick = -9;
	bool outLock;
	bool outAlone;

	void DL_TrailView()
	{
		m_Handler = new DL_TrailButtons(this);
	}

	void Close()
	{
		if (m_Root)
			m_Root.Unlink();
		m_Root = null;
		m_Buttons.Clear();
		m_RowBox.Clear();
		m_RowVotes.Clear();
		m_Outcome.Clear();
		m_LockBtn = null;
		m_AloneBtn = null;
	}

	bool IsOpen()
	{
		return m_Root != null;
	}

	// ---------------- widgets ----------------

	protected Widget Frame(Widget parent)
	{
		int sw;
		int sh;
		GetScreenSize(sw, sh);
		int flags = WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE;
		return g_Game.GetWorkspace().CreateWidget(FrameWidgetTypeID, 0, 0, sw, sh, flags, ARGB(255, 255, 255, 255), SORT, parent);
	}

	// from a layout: only those get the scalable font (its letters follow the box's height); a text widget made in
	// script stays small at any size (DL_MainMenu). align 0 left, 1 centre, 2 right.
	protected TextWidget Text(float x, float y, float w, float h, string text, int color, int align = 0)
	{
		string lay = "DriveLink/gui/layouts/dl_trail_text_left.layout";
		if (align == 1)
			lay = "DriveLink/gui/layouts/dl_trail_text_center.layout";
		else if (align == 2)
			lay = "DriveLink/gui/layouts/dl_trail_text_right.layout";
		TextWidget t = TextWidget.Cast(g_Game.GetWorkspace().CreateWidgets(lay, m_Root));
		if (!t)
			return null;
		t.SetPos(x, y);
		t.SetSize(w, h);
		t.SetColor(color);
		t.SetSort(5);
		t.SetText(text);
		return t;
	}

	protected ImageWidget Box(float x, float y, float w, float h, int color, int sort)
	{
		int flags = WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE | WidgetFlags.IGNOREPOINTER | WidgetFlags.STRETCH;
		ImageWidget b = ImageWidget.Cast(g_Game.GetWorkspace().CreateWidget(ImageWidgetTypeID, x, y, w, h, flags, color, sort, m_Root));
		if (b)
		{
			b.LoadImageFile(0, "#(argb,8,8,3)color(1,1,1,1,CO)");
			b.SetColor(color);
		}
		return b;
	}

	protected ButtonWidget Hit(float x, float y, float w, float h)
	{
		int flags = WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE;
		ButtonWidget b = ButtonWidget.Cast(g_Game.GetWorkspace().CreateWidget(ButtonWidgetTypeID, x, y, w, h, flags, ARGB(0, 0, 0, 0), 8, m_Root));
		if (b)
			b.SetHandler(m_Handler);
		return b;
	}

	// text cut into lines of about chars characters (the default font has no wrapping in a plain TextWidget)
	static array<string> Wrap(string text, int chars)
	{
		array<string> lines = new array<string>;
		array<string> words = new array<string>;
		text.Split(" ", words);
		string line = "";
		foreach (string w : words)
		{
			if (line != "" && line.Length() + 1 + w.Length() > chars)
			{
				lines.Insert(line);
				line = "";
			}
			if (line != "")
				line += " ";
			line += w;
		}
		if (line != "")
			lines.Insert(line);
		return lines;
	}

	static int HintColor(string hint)
	{
		if (hint == "safe")
			return ARGB(255, 120, 210, 120);
		if (hint == "dangerous")
			return ARGB(255, 240, 95, 80);
		return ARGB(255, 235, 190, 80);
	}

	// ---------------- the choice card ----------------

	// you = my place among the voters (-1 = I only watch)
	void ShowCard(DL_TrailVoteCard card, int you, Widget parent = null, bool canAlone = true)
	{
		Close();
		m_Card = card;
		m_You = you;
		m_Mine = -1;
		m_Locked = false;
		m_Decided = false;
		m_AloneArmed = 0;
		outPick = -9;
		outLock = false;
		outAlone = false;
		m_Root = Frame(parent);
		int sw;
		int sh;
		GetScreenSize(sw, sh);
		float rh = sh * 0.026;
		m_Rh = rh;
		float pw = Math.Min(sw * 0.44, sh * 0.82);
		float pad = rh * 0.7;
		array<string> textLines = Wrap(card.text, Math.Floor((pw - pad * 2) / (rh * 0.75 * 0.5)));
		m_HasBack = !card.ask;
		int rows = card.choices.Count();
		if (m_HasBack)
			rows++;
		float rowH = rh * 1.8;
		float ph = pad * 2 + rh * 1.0 + rh * 1.5 + textLines.Count() * rh + rh * 0.6 + rows * (rowH + rh * 0.25) + rh * 0.4 + rh * 1.5 + rh * 2.2;
		float px = (sw - pw) / 2;
		float py = Math.Max(sh * 0.03, sh - ph - sh * 0.035);
		Box(px, py, pw, ph, ARGB(235, 14, 16, 19), 1);
		Box(px, py, pw, rh * 0.12, ARGB(255, 214, 170, 90), 2);
		float x = px + pad;
		float w = pw - pad * 2;
		m_TextX = x;
		m_TextW = w;
		float y = py + pad;
		string of = "";
		if (card.of > 0)
			of = " of " + card.of.ToString();
		string head = "On the road: choice " + card.n.ToString() + of;
		if (card.ask)
			head = "On the road: your choice";
		Text(x, y, w * 0.6, rh * 0.6, head, ARGB(255, 150, 150, 150));
		m_Timer = Text(x + w * 0.6, y, w * 0.4, rh * 0.8, "", ARGB(255, 214, 170, 90), 2);
		y += rh * 1.0;
		Text(x, y, w, rh * 1.2, card.title, ARGB(255, 240, 225, 190));
		y += rh * 1.5;
		foreach (string tl : textLines)
		{
			Text(x, y, w, rh * 0.75, tl, ARGB(255, 215, 215, 215));
			y += rh;
		}
		y += rh * 0.6;
		m_RowW = w;
		for (int i = 0; i < rows; i++)
		{
			bool back = m_HasBack && i == card.choices.Count();
			string label;
			string hint;
			string help = "";
			if (back)
			{
				label = "Turn back (all of us)";
				hint = "needs more than half";
			}
			else
			{
				label = card.choices[i].label;
				hint = card.choices[i].hint;
				help = card.choices[i].help;
			}
			ImageWidget rb = Box(x, y, w, rowH, ARGB(255, 32, 36, 42), 2);
			m_RowBox.Insert(rb);
			string key = "[" + (i + 1).ToString() + "] ";
			if (back)
				key = "[0] ";
			Text(x + rh * 0.4, y + rh * 0.15, w * 0.68, rh * 0.8, key + label, ARGB(255, 235, 235, 235));
			int hc = HintColor(hint);
			if (back)
				hc = ARGB(255, 160, 160, 160);
			Text(x + w * 0.66, y + rh * 0.2, w * 0.34 - rh * 0.4, rh * 0.65, hint, hc, 2);
			if (help != "")
				Text(x + rh * 1.4, y + rh * 1.05, w * 0.5, rh * 0.6, "with " + help, ARGB(255, 150, 190, 230));
			TextWidget votes = Text(x + w * 0.45, y + rh * 1.05, w * 0.55 - rh * 0.4, rh * 0.6, "", ARGB(255, 214, 170, 90), 2);
			m_RowVotes.Insert(votes);
			m_Buttons.Insert(Hit(x, y, w, rowH));
			y += rowH + rh * 0.25;
		}
		y += rh * 0.4;
		m_Status = Text(x, y, w, rh * 0.65, "", ARGB(255, 190, 190, 190), 1);
		y += rh * 1.5;
		if (m_You >= 0 && card.ask)
			Text(x, y, w, rh * 0.7, "Click your answer (or press its number).", ARGB(255, 190, 190, 190), 1);
		else if (m_You >= 0)
		{
			float bw = w * 0.36;
			Box(x, y, bw, rh * 1.6, ARGB(255, 60, 90, 60), 2);
			Text(x, y + rh * 0.4, bw, rh * 0.8, "[L] Lock in my vote", ARGB(255, 255, 255, 255), 1);
			m_LockBtn = Hit(x, y, bw, rh * 1.6);
			if (canAlone)
			{
				Box(x + w - bw, y, bw, rh * 1.6, ARGB(255, 110, 50, 45), 2);
				m_AloneText = Text(x + w - bw, y + rh * 0.4, bw, rh * 0.8, "[T] Turn back alone", ARGB(255, 255, 255, 255), 1);
				m_AloneBtn = Hit(x + w - bw, y, bw, rh * 1.6);
			}
			else
				Text(x + w - bw * 1.3, y + rh * 0.45, bw * 1.3, rh * 0.6, "Only the driver can turn the car back", ARGB(255, 150, 150, 150), 2);
		}
		else
			Text(x, y, w, rh * 0.7, "You are watching: the party decides.", ARGB(255, 150, 150, 150), 1);
		UpdateTally(null);
	}

	// the votes so far (t null = none yet)
	void UpdateTally(DL_TrailTally t)
	{
		if (!m_Root || !m_Card)
			return;
		int rows = m_RowVotes.Count();
		array<string> who = new array<string>;
		for (int r = 0; r < rows; r++)
			who.Insert("");
		int voted = 0;
		int lockedN = 0;
		if (t)
		{
			for (int i = 0; i < t.picks.Count() && i < m_Card.names.Count(); i++)
			{
				int p = t.picks[i];
				int row = p;
				if (p == DL_TrailGame.TURN_BACK && m_HasBack)
					row = rows - 1;
				if (row < 0 || row >= rows)
					continue;
				voted++;
				string n = m_Card.names[i];
				if (i == m_You)
					n = "You";
				if (i < t.locked.Count() && t.locked[i])
				{
					n += "*";
					lockedN++;
				}
				if (who[row] != "")
					who[row] = who[row] + ", ";
				who[row] = who[row] + n;
			}
			if (m_Timer && t.result == -9)
				m_Timer.SetText(Math.Ceil(t.left).ToString() + " s");
		}
		for (int k = 0; k < rows; k++)
		{
			m_RowVotes[k].SetText(who[k]);
			int col = ARGB(255, 32, 36, 42);
			int mineRow = m_Mine;
			if (m_Mine == DL_TrailGame.TURN_BACK && m_HasBack)
				mineRow = rows - 1;
			if (k == mineRow)
				col = ARGB(255, 52, 62, 78);
			if (t && t.result != -9)
			{
				int win = t.result;
				if (win == DL_TrailGame.TURN_BACK)
					win = rows - 1;
				col = ARGB(255, 26, 28, 32);
				if (k == win)
					col = ARGB(255, 70, 92, 60);
			}
			m_RowBox[k].SetColor(col);
		}
		if (!m_Status)
			return;
		if (t && t.result != -9)
		{
			m_Decided = true;
			if (m_Timer)
				m_Timer.SetText("");
			m_Status.SetText("");
			ShowOutcome(t.outcome);
			return;
		}
		string st = voted.ToString() + " of " + m_Card.names.Count().ToString() + " voted";
		if (lockedN > 0)
			st += ", " + lockedN.ToString() + " locked in (*)";
		if (m_Locked)
			st += ". You locked in.";
		else if (m_You >= 0 && m_Mine == -1)
			st += ". Click a choice (or press its number).";
		m_Status.SetText(st);
	}

	protected void ShowOutcome(string text)
	{
		foreach (TextWidget o : m_Outcome)
			o.Unlink();
		m_Outcome.Clear();
		if (text == "" || !m_Status)
			return;
		float sx;
		float sy;
		m_Status.GetPos(sx, sy);
		array<string> lines = Wrap(text, Math.Floor(m_TextW / (m_Rh * 0.8 * 0.5)));
		float y = sy - (lines.Count() - 1) * m_Rh * 0.5;
		foreach (string l : lines)
		{
			m_Outcome.Insert(Text(m_TextX, y, m_TextW, m_Rh * 0.8, l, ARGB(255, 240, 225, 190), 1));
			y += m_Rh;
		}
	}

	// ---------------- input ----------------

	bool Clicked(Widget w)
	{
		if (!w || m_Decided || m_You < 0)
			return false;
		for (int i = 0; i < m_Buttons.Count(); i++)
		{
			if (w == m_Buttons[i])
			{
				Pick(i);
				return true;
			}
		}
		if (w == m_LockBtn)
		{
			Lock();
			return true;
		}
		if (w == m_AloneBtn)
		{
			Alone();
			return true;
		}
		return false;
	}

	protected void Pick(int row)
	{
		if (m_Locked)
			return;
		int p = row;
		if (m_HasBack && row == m_Buttons.Count() - 1)
			p = DL_TrailGame.TURN_BACK;
		m_Mine = p;
		outPick = p;
	}

	protected void Lock()
	{
		if (m_Locked || m_Mine == -1)
			return;
		m_Locked = true;
		outLock = true;
	}

	// twice within 5 s: the first press asks "are you sure"
	protected void Alone()
	{
		if (m_AloneArmed <= 0)
		{
			m_AloneArmed = 5;
			if (m_AloneText)
				m_AloneText.SetText("Sure? You go back on your own");
			return;
		}
		m_AloneArmed = 0;
		outAlone = true;
	}

	// keys: 1..9 a choice, 0 turn back, L lock in, T turn back alone (read straight from the keyboard: the game keeps it)
	void Update(float dt)
	{
		if (m_AloneArmed > 0)
		{
			m_AloneArmed -= dt;
			if (m_AloneArmed <= 0 && m_AloneText)
				m_AloneText.SetText("[T] Turn back alone");
		}
		if (!m_Root || m_Decided || m_You < 0)
			return;
		int down = -1;
		array<int> keys = {KeyCode.KC_0, KeyCode.KC_1, KeyCode.KC_2, KeyCode.KC_3, KeyCode.KC_4, KeyCode.KC_5, KeyCode.KC_6, KeyCode.KC_7, KeyCode.KC_8, KeyCode.KC_9, KeyCode.KC_L, KeyCode.KC_T};
		for (int i = 0; i < keys.Count(); i++)
		{
			if (KeyState(keys[i]) != 0)
				down = i;
		}
		if (down == m_KeyHeld)
			return;
		m_KeyHeld = down;
		if (down < 0)
			return;
		int choices = m_Buttons.Count();
		if (m_HasBack)
			choices--;
		if (down == 0 && m_HasBack)
			Pick(choices);
		else if (down <= 9 && down <= choices)
			Pick(down - 1);
		else if (down == 10 && m_LockBtn)
			Lock();
		else if (down == 11 && m_AloneBtn)
			Alone();
	}

	// a vote to send now (once): pick (-9 none), lock, alone
	bool TakeOut(out int pick, out bool lock, out bool alone)
	{
		pick = outPick;
		lock = outLock;
		alone = outAlone;
		outPick = -9;
		outLock = false;
		outAlone = false;
		return pick != -9 || lock || alone;
	}

	// ---------------- the summary ----------------

	void ShowSummary(DL_TrailSummary s, Widget parent = null)
	{
		Close();
		m_Root = Frame(parent);
		int sw;
		int sh;
		GetScreenSize(sw, sh);
		float rh = sh * 0.03;
		float pw = Math.Min(sw * 0.4, sh * 0.75);
		float pad = rh * 0.8;
		float ph = pad * 2 + rh * 1.8 + s.lines.Count() * rh * 1.05 + rh * 0.4;
		float px = (sw - pw) / 2;
		float py = sh - ph - sh * 0.06;
		Box(px, py, pw, ph, ARGB(235, 14, 16, 19), 1);
		Box(px, py, pw, rh * 0.12, ARGB(255, 214, 170, 90), 2);
		float y = py + pad;
		Text(px + pad, y, pw - pad * 2, rh * 1.2, s.title, ARGB(255, 240, 225, 190), 1);
		y += rh * 1.8;
		foreach (string l : s.lines)
		{
			Text(px + pad, y, pw - pad * 2, rh * 0.75, l, ARGB(255, 215, 215, 215), 1);
			y += rh * 1.05;
		}
	}
}
