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
	protected float m_DL_InputRecoveryRemaining;
	protected string m_DL_WaitMsg;
	protected float m_DL_WaitAge;      // seconds since the last update from the server
	protected bool m_DL_StartEngine;
	protected bool m_DL_ArrivalEngineHandled; // WAIT_DONE is resent; do not shift a driver twice
	protected vector m_DL_FlyVel;        // arriving in a plane: keep giving it this speed for a moment (RPC_FLY)
	protected float m_DL_FlyT;
	protected ref DL_FilmPlayer m_DL_Film = new DL_FilmPlayer(); // the road film (RPC_FILM); F9 = demo film
	protected int m_DL_FilmId;                                   // the film being received in parts (RPC_FILM)
	protected ref array<string> m_DL_FilmParts = new array<string>;
	protected int m_DL_FilmGot;
	protected string m_DL_PendingFilm; // receive while loading; begin event storytelling only when the client is ready
	protected bool m_DL_F9WasDown;
	protected TextWidget m_DL_Title;
	protected bool m_DL_TextLow;        // the black screen's texts moved under the film
	protected ref DL_ShortInfo m_DL_Short; // "not enough supplies" (RPC_SHORT): shown once no other menu is open
	protected ref DL_ShortMenu m_DL_ShortMenu; // held here: the UI manager does not keep a script menu alive

	override void OnMissionStart()
	{
		super.OnMissionStart();
		GetDayZGame().Event_OnRPC.Insert(DL_OnRpc);
		GetDayZGame().DL_Transition().MissionStarted("gameplay");
		GetDayZGame().DL_Transition().Arrived();
	}

	override void OnMissionFinish()
	{
		GetDayZGame().Event_OnRPC.Remove(DL_OnRpc);
		m_DL_PendingFilm = "";
		GetDayZGame().DL_Transition().MissionFinishing();
		if (!GetDayZGame().DL_HasPending())
			GetDayZGame().DL_ClearTransferLogin();
		if (m_DL_Film)
			m_DL_Film.Stop();
		DL_WaitHide();
		if (m_DL_WaitRoot)
			m_DL_WaitRoot.Unlink();
		super.OnMissionFinish();
	}

	protected void DL_OnRpc(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
	{
		if (rpc_type == DL_Const.RPC_FLY)
		{
			vector fv;
			if (ctx.Read(fv))
			{
				m_DL_FlyVel = fv;
				m_DL_FlyT = 1.5;
				m_DL_StartEngine = true;
				DL_Log.Detail("arriving in the air: flying on at " + Math.Round(fv.Length() * 3.6).ToString() + " km/h");
			}
			return;
		}
		if (rpc_type == DL_Const.RPC_FILM)
		{
			int fid;
			int part;
			int parts;
			string chunk;
			if (!ctx.Read(fid) || !ctx.Read(part) || !ctx.Read(parts) || !ctx.Read(chunk) || parts < 1 || part < 0 || part >= parts)
				return;
			if (fid != m_DL_FilmId || m_DL_FilmParts.Count() != parts)
			{
				m_DL_FilmId = fid;
				m_DL_FilmParts.Clear();
				for (int fp = 0; fp < parts; fp++)
					m_DL_FilmParts.Insert("");
				m_DL_FilmGot = 0;
			}
			if (m_DL_FilmParts[part] == "")
				m_DL_FilmGot++;
			m_DL_FilmParts[part] = chunk;
			if (m_DL_FilmGot < parts)
				return;
			string fj = "";
			foreach (string pc : m_DL_FilmParts)
				fj += pc;
			m_DL_FilmParts.Clear();
			m_DL_FilmId = 0;
			m_DL_PendingFilm = fj;
			return;
		}
		if (rpc_type == DL_Const.RPC_FILM_SKIP)
		{
			// the skip vote: everyone watching voted (skip) or how many have
			int votes;
			int total;
			bool skip;
			if (!ctx.Read(votes) || !ctx.Read(total) || !ctx.Read(skip) || !m_DL_Film)
				return;
			if (skip)
			{
				m_DL_PendingFilm = "";
				DL_Log.Detail("road film skipped: everyone voted");
				m_DL_Film.Stop();
			}
			else
				m_DL_Film.VoteStatus(votes, total);
			return;
		}
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
				GetDayZGame().DL_ClearTransferLogin();
				if (engine && !m_DL_ArrivalEngineHandled)
					m_DL_StartEngine = true;
			}
			return;
		}
		if (rpc_type == DL_Const.RPC_SHORT)
		{
			string sj;
			if (!ctx.Read(sj))
				return;
			DL_ShortInfo si;
			string serr;
			JsonSerializer sjs = new JsonSerializer();
			if (!sjs.ReadFromString(si, sj, serr) || !si || !si.people)
			{
				DL_Log.Warn("bad 'not enough supplies' message: " + serr);
				return;
			}
			m_DL_Short = si;
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

	protected void DL_PlayArrivalFilm(string json)
	{
		// Separate lifecycles: the cosmetic loading tree ends before the event story starts.
		GetDayZGame().DL_Transition().Stop("arrival story begins");
		if (m_DL_Film)
			m_DL_Film.PlayJson(json);
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if(m_DL_InputRecoveryRemaining>0)
		{
			m_DL_InputRecoveryRemaining-=timeslice;
			// Only the completed arrival may request this once. Never clear an inventory,
			// chat, pause menu or another mod's active control restriction.
			if(!m_DL_WaitWanted && !m_DL_WaitShown && g_Game.IsAppActive() && !g_Game.GetUIManager().GetMenu() && !IsControlDisabled())
			{
				bool focused=g_Game.GetInput().HasGameFocus();
				if(!focused)g_Game.GetInput().ResetGameFocus();
				GetUApi().UpdateControls();GetUApi().SupressNextFrame(false);
				GetDayZGame().RefreshMouseCursorVisibility();
				m_DL_InputRecoveryRemaining=0;
				DL_Log.Info("arrival input refreshed; previous game focus "+focused.ToString());
			}
		}
		if (m_DL_PendingFilm != "" && GetDayZGame().DL_Transition().ReadyForArrival())
		{
			string readyFilm = m_DL_PendingFilm;
			m_DL_PendingFilm = "";
			DL_PlayArrivalFilm(readyFilm);
		}
		if (m_DL_Film)
		{
#ifdef DL_DEV
			// F9: the demo film (every scene), for testing; F9 again or Space stops it - dev builds only
			bool f9 = KeyState(KeyCode.KC_F9) != 0;
			if (f9 && !m_DL_F9WasDown)
			{
				if (m_DL_Film.IsPlaying())
					m_DL_Film.Stop();
				else
					m_DL_Film.Play(DL_FilmPlayer.Demo());
			}
			m_DL_F9WasDown = f9;
#endif
			m_DL_Film.Update(timeslice);
			if (m_DL_Film.TakeVote())
				DL_SendFilmVote();
			if (m_DL_WaitShown)
				DL_TextLayout(m_DL_Film.IsPlaying());
		}
		if (m_DL_WaitWanted)
		{
			m_DL_WaitAge += timeslice;
			if (m_DL_WaitAge > 20)
			{
				// the server sends one every second; it went quiet, never leave the player stuck in the dark
				DL_Log.Warn("waiting room: no word from the server for 20 s, lifting the black screen");
				m_DL_WaitWanted = false;
			}
		}
		// the road film is the loading screen: the black screen stays up until it has played out (the server may be
		// done seating sooner; Space skips the film)
		bool filmOn = (m_DL_PendingFilm != "" || (m_DL_Film && m_DL_Film.IsPlaying())) && m_DL_WaitShown;
		if (!m_DL_WaitWanted && m_DL_WaitShown && !filmOn && GetDayZGame().DL_Transition().ReadyForArrival())
			GetDayZGame().DL_Transition().Stop("arrival ready without a story film");
		if (m_DL_WaitWanted || filmOn)
			DL_WaitShow(m_DL_WaitMsg);
		else if (m_DL_WaitShown)
			DL_WaitHide();
		if (m_DL_StartEngine && !m_DL_WaitWanted && !filmOn)
			DL_TryStartEngine();
		if (m_DL_Short && !m_DL_WaitShown && !g_Game.GetUIManager().GetMenu())
		{
			m_DL_ShortMenu = new DL_ShortMenu(m_DL_Short);
			m_DL_Short = null;
			g_Game.GetUIManager().ShowScriptedMenu(m_DL_ShortMenu, null);
		}
		if (m_DL_FlyT > 0)
		{
			// the pilot's client simulates the plane: hold its speed until its own flight model has it
			m_DL_FlyT -= timeslice;
			PlayerBase me = PlayerBase.Cast(g_Game.GetPlayer());
			if (me && me.GetCommand_Vehicle() && me.GetCommand_Vehicle().GetTransport())
			{
				Transport fly = me.GetCommand_Vehicle().GetTransport();
				SetVelocity(fly, m_DL_FlyVel);
				Car fc = Car.Cast(fly);
				if (fc && fc.GetGear() != CarGear.FIRST)
					fc.ShiftTo(CarGear.FIRST);
			}
		}
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
			m_DL_Title = TextWidget.Cast(ws.CreateWidget(TextWidgetTypeID, 0, top, w, titleH, flags | WidgetFlags.CENTER | WidgetFlags.VCENTER, ARGB(255, 200, 200, 200), 1, m_DL_WaitRoot));
			if (m_DL_Title)
				m_DL_Title.SetText("DriveLink");
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
			DL_Log.Detail("waiting room: black screen on (" + msg + ")");
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

	// with a road film on screen the title, status and story move under it (the film takes the top 2/3)
	// to the server through his own player (PlayerBase.OnRPC there, DL_FilmVotes)
	protected void DL_SendFilmVote()
	{
		PlayerBase me = PlayerBase.Cast(g_Game.GetPlayer());
		if (!me)
			return;
		ScriptRPC rpc = new ScriptRPC();
		rpc.Send(me, DL_Const.RPC_FILM_VOTE, true, null);
	}

	protected void DL_TextLayout(bool low)
	{
		if ((low == m_DL_TextLow && !low) || !m_DL_WaitRoot)
			return;
		m_DL_TextLow = low;
		int w;
		int h;
		GetScreenSize(w, h);
		float titleH = h * 0.035;
		float statusH = h * 0.045;
		float lineH = h * 0.034;
		float top = h * 0.30;
		if (low)
		{
			top = h * 0.67;
			if (m_DL_Film && m_DL_Film.BottomFrac() > 0)
				top = h * (m_DL_Film.BottomFrac() + 0.015); // under the film window (a convoy's is taller)
		}
		if (m_DL_Title)
			m_DL_Title.SetPos(0, top);
		if (m_DL_WaitText)
			m_DL_WaitText.SetPos(0, top + titleH * 1.1);
		float storyTop = top + titleH * 1.1 + statusH * 1.3;
		for (int i = 0; i < m_DL_StoryLines.Count(); i++)
		{
			m_DL_StoryLines[i].SetPos(0, storyTop + i * lineH * 1.15);
			m_DL_StoryLines[i].Show(!low || storyTop + (i + 1) * lineH * 1.15 < h * 0.99); // under the film: what fits
		}
	}

	protected void DL_WaitHide()
	{
		if (!m_DL_WaitShown)
			return;
		if (m_DL_Film && m_DL_Film.IsPlaying())
			m_DL_Film.Stop();
		m_DL_WaitShown = false;
		if (m_DL_WaitRoot)
			m_DL_WaitRoot.Show(false);
		PlayerControlEnable(false);
		m_DL_InputRecoveryRemaining=5;
		DL_Log.Detail("waiting room: black screen off");
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
		Transport car;
		if (vc)
			car = DL_Veh.VehicleOf(pb);
		if (car && car.CrewDriver() == pb)
		{
			if (!DL_Veh.EngineIsOn(car) && car.GetNetworkMoveStrategy() == NetworkMoveStrategy.PHYSICS)
			{
				DL_Veh.EngineStart(car);
				DL_Log.Detail("waiting room: engine started on this client, running " + DL_Veh.EngineIsOn(car).ToString());
			}
			if (DL_Veh.EngineIsOn(car))
			{
				Car roadCar = Car.Cast(car);
				if (roadCar && roadCar.GetGear() == roadCar.GetNeutralGear())
				{
					roadCar.ShiftUp(); // first forward gear; same gearbox operation as E
					DL_Log.Info("waiting room: arrival car shifted into first gear");
				}
				m_DL_StartEngine = false;
				m_DL_ArrivalEngineHandled = true;
				m_DL_EngineTries = 0;
				return;
			}
		}
		if (m_DL_EngineTries > 300)
		{
			m_DL_StartEngine = false;
			m_DL_EngineTries = 0;
			DL_Log.Detail("waiting room: not at the wheel, engine not started");
		}
	}
}
