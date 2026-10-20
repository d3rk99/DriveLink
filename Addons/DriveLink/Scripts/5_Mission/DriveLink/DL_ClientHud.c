// DriveLink - client: show the travel countdown the server sends (RPC_COUNTDOWN, int seconds + zone name).
// Uses vanilla notifications (NotificationSystem.AddNotificationExtended, 3_game/client/notifications/notificationsystem.c)
// so no layout files are needed in M1. 0 seconds = cancelled.
// M3 waiting room (RPC_CREW_WAIT): a black full-screen panel with "n/m connected" and no controls until the server says
// the crew is seated. Widgets made in code (WorkspaceWidget.CreateWidget, 1_core/proto/enwidgets.c:179), created in
// OnUpdate rather than in the RPC handler. Not vanilla ScreenFadeIn: MissionGameplay.OnUpdate fades that out again every
// frame while the player is alive (missiongameplay.c:483-497).
modded class MissionGameplay
{
	protected Widget m_DL_WaitRoot;
	protected TextWidget m_DL_WaitText;
	protected ref array<TextWidget> m_DL_StoryLines = new array<TextWidget>;
	protected int m_DL_LineChars = 60;      // how many characters fit on one story line at this screen size
	protected string m_DL_Story;
	protected bool m_DL_WaitWanted;
	protected bool m_DL_WaitShown;
	protected string m_DL_WaitMsg;
	protected float m_DL_WaitAge;      // seconds since the last update from the server
	protected bool m_DL_StartEngine;
	protected ref DL_FilmSpike m_DL_Film = new DL_FilmSpike(); // F9 debug window (road animation engine spike)

	override void OnMissionStart()
	{
		super.OnMissionStart();
		GetDayZGame().Event_OnRPC.Insert(DL_OnRpc);
	}

	override void OnMissionFinish()
	{
		GetDayZGame().Event_OnRPC.Remove(DL_OnRpc);
		if (m_DL_Film)
			m_DL_Film.Close();
		DL_WaitHide();
		if (m_DL_WaitRoot)
			m_DL_WaitRoot.Unlink();
		super.OnMissionFinish();
	}

	protected void DL_OnRpc(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
	{
		if (rpc_type == DL_Const.RPC_CREW_WAIT)
		{
			int phase;
			string msg;
			bool engine;
			if (!ctx.Read(phase) || !ctx.Read(msg) || !ctx.Read(engine))
				return;
			string story;
			if (!ctx.Read(story)) // an older server sends no story
				story = "";
			if (phase == DL_Const.WAIT_SHOW)
				m_DL_Story = story;
			m_DL_WaitAge = 0;
			if (phase == DL_Const.WAIT_SHOW)
			{
				m_DL_WaitWanted = true;
				m_DL_WaitMsg = msg;
			}
			else if (phase == DL_Const.WAIT_DONE)
			{
				m_DL_WaitWanted = false;
				if (engine)
					m_DL_StartEngine = true;
			}
			return;
		}
		if (rpc_type != DL_Const.RPC_COUNTDOWN)
			return;
		int seconds;
		string zone;
		if (!ctx.Read(seconds) || !ctx.Read(zone))
			return;
		string price;
		if (!ctx.Read(price)) // an older server sends no price
			price = "";
		string costLine = "";
		if (price != "")
			costLine = "\nTrip: " + price + ".";
		if (seconds > 0)
			NotificationSystem.AddNotificationExtended(seconds, "DriveLink: " + zone, "Leaving in " + seconds.ToString() + " s. Step out of the zone to cancel." + costLine, "");
		else
			NotificationSystem.AddNotificationExtended(3, "DriveLink", "Travel cancelled.", "");
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (m_DL_Film)
			m_DL_Film.Update(timeslice);
		if (m_DL_WaitWanted)
		{
			m_DL_WaitAge += timeslice;
			if (m_DL_WaitAge > 20)
			{
				// the server sends one every second; it went quiet, never leave the player stuck in the dark
				Print(DL_Const.LOG_TAG + "waiting room: no word from the server for 20 s, lifting the black screen");
				m_DL_WaitWanted = false;
			}
		}
		if (m_DL_WaitWanted)
			DL_WaitShow(m_DL_WaitMsg);
		else if (m_DL_WaitShown)
			DL_WaitHide();
		if (m_DL_StartEngine && !m_DL_WaitWanted)
			DL_TryStartEngine();
	}

	protected void DL_WaitShow(string msg)
	{
		if (!m_DL_WaitRoot)
		{
			WorkspaceWidget ws = g_Game.GetWorkspace();
			if (!ws)
				return;
			int w;
			int h;
			GetScreenSize(w, h);
			int flags = WidgetFlags.VISIBLE | WidgetFlags.EXACTPOS | WidgetFlags.EXACTSIZE | WidgetFlags.IGNOREPOINTER;
			m_DL_WaitRoot = ws.CreateWidget(FrameWidgetTypeID, 0, 0, w, h, flags, ARGB(255, 255, 255, 255), 30000);
			if (!m_DL_WaitRoot)
				return;
			ImageWidget bg = ImageWidget.Cast(ws.CreateWidget(ImageWidgetTypeID, 0, 0, w, h, flags | WidgetFlags.STRETCH, ARGB(255, 0, 0, 0), 0, m_DL_WaitRoot));
			if (bg)
			{
				bg.LoadImageFile(0, "#(argb,8,8,3)color(1,1,1,1,CO)"); // a plain texture, tinted black (no texture paints nothing)
				bg.SetColor(ARGB(255, 0, 0, 0));
			}
			// The default font takes its letter height from the widget's box (about 3/4 of it): every text gets a box
			// sized to the screen, one TextWidget per story line (2026-10-19: one big MultilineTextWidget box drew
			// giant letters off the screen, and SetTextExactSize left the title tiny).
			int titleH = h * 0.035;
			int statusH = h * 0.045;
			int lineH = h * 0.034;
			int top = h * 0.30;
			TextWidget title = TextWidget.Cast(ws.CreateWidget(TextWidgetTypeID, 0, top, w, titleH, flags | WidgetFlags.CENTER | WidgetFlags.VCENTER, ARGB(255, 200, 200, 200), 1, m_DL_WaitRoot));
			if (title)
				title.SetText("DriveLink");
			m_DL_WaitText = TextWidget.Cast(ws.CreateWidget(TextWidgetTypeID, 0, top + titleH * 1.6, w, statusH, flags | WidgetFlags.CENTER | WidgetFlags.VCENTER, ARGB(255, 255, 255, 255), 1, m_DL_WaitRoot));
			// the road's story (travel step 4): up to 10 lines under the status, wrapped by DL_StoryWrap
			int storyTop = top + titleH * 1.6 + statusH * 1.8;
			for (int i = 0; i < 10; i++)
			{
				TextWidget ln = TextWidget.Cast(ws.CreateWidget(TextWidgetTypeID, 0, storyTop + i * lineH * 1.25, w, lineH, flags | WidgetFlags.CENTER | WidgetFlags.VCENTER, ARGB(255, 230, 210, 170), 1, m_DL_WaitRoot));
				if (ln)
					m_DL_StoryLines.Insert(ln);
			}
			// a letter is about 0.55 x its height wide; keep a margin of a fifth of the screen
			m_DL_LineChars = Math.Max(20, (w * 0.8) / (lineH * 0.75 * 0.55));
		}
		if (m_DL_WaitText)
			m_DL_WaitText.SetText(msg);
		array<string> lines = DL_StoryWrap(m_DL_Story, m_DL_LineChars);
		for (int k = 0; k < m_DL_StoryLines.Count(); k++)
		{
			string t = "";
			if (k < lines.Count())
				t = lines[k];
			m_DL_StoryLines[k].SetText(t);
		}
		if (!m_DL_WaitShown)
		{
			m_DL_WaitShown = true;
			m_DL_WaitRoot.Show(true);
			PlayerControlDisable(INPUT_EXCLUDE_ALL);
			Print(DL_Const.LOG_TAG + "waiting room: black screen on (" + msg + ")");
		}
	}

	// the story's own lines ("\n"), each cut at word boundaries to fit maxChars
	static array<string> DL_StoryWrap(string story, int maxChars)
	{
		array<string> res = new array<string>;
		if (story == "")
			return res;
		array<string> parts = new array<string>;
		story.Split("\n", parts);
		foreach (string part : parts)
		{
			array<string> words = new array<string>;
			part.Split(" ", words);
			string cur = "";
			foreach (string word : words)
			{
				if (cur == "")
					cur = word;
				else if (cur.Length() + 1 + word.Length() <= maxChars)
					cur += " " + word;
				else
				{
					res.Insert(cur);
					cur = word;
				}
			}
			if (cur != "")
				res.Insert(cur);
		}
		return res;
	}

	protected void DL_WaitHide()
	{
		if (!m_DL_WaitShown)
			return;
		m_DL_WaitShown = false;
		if (m_DL_WaitRoot)
			m_DL_WaitRoot.Show(false);
		PlayerControlEnable(false);
		Print(DL_Const.LOG_TAG + "waiting room: black screen off");
	}

	// The driver's machine simulates a PHYSICS car, so the engine is started here (as vanilla ActionStartEngine does,
	// actionstartengine.c:51-65). Tried every frame until he is at the wheel, at most ~5 s.
	protected float m_DL_EngineTries;
	protected void DL_TryStartEngine()
	{
		m_DL_EngineTries += 1;
		PlayerBase pb = PlayerBase.Cast(g_Game.GetPlayer());
		HumanCommandVehicle vc;
		if (pb)
			vc = pb.GetCommand_Vehicle();
		CarScript car;
		if (vc)
			car = CarScript.Cast(vc.GetTransport());
		if (car && car.CrewDriver() == pb)
		{
			m_DL_StartEngine = false;
			m_DL_EngineTries = 0;
			if (car.EngineIsOn())
				return;
			if (car.GetNetworkMoveStrategy() == NetworkMoveStrategy.PHYSICS)
			{
				car.EngineStart();
				Print(DL_Const.LOG_TAG + "waiting room: engine started on this client, running " + car.EngineIsOn().ToString());
			}
			return;
		}
		if (m_DL_EngineTries > 300)
		{
			m_DL_StartEngine = false;
			m_DL_EngineTries = 0;
			Print(DL_Const.LOG_TAG + "waiting room: not at the wheel, engine not started");
		}
	}
}
