// Experimental client-session overlay. This is a plain travel scene, never a prediction of road outcomes.
class DL_TransitionFilm
{
	protected ref DL_Film m_Prepared;
	protected ref DL_FilmPlayer m_Player;
	protected Widget m_Root;
	protected TextWidget m_Status;
	protected bool m_Loading;
	protected bool m_Active; // session state must survive destruction of the mission's widgets
	protected float m_Age;
	protected float m_ArrivalAge = -1;
	protected float m_LogAge;
	protected string m_Target;
	protected int m_Id;
	protected int m_Got;
	protected bool m_Handoff;
	protected Widget m_LoginTimer;
	protected int m_LoginSeconds;
	protected ref DL_TransitionMotion m_Motion = new DL_TransitionMotion();
	protected ref array<string> m_Parts = new array<string>;

	// Opt-in menu smoke test, enabled only by the prototype launcher's command-line switch.
	void Demo()
	{
		m_Prepared = DL_FilmPlayer.Demo();
		m_Prepared.events.Clear();
		m_Prepared.Add("drive", 1);
		m_Prepared.biomeTo = m_Prepared.biomeFrom;
		m_Prepared.skip = "none";
		Start("prototype demonstration");
	}

	void Receive(ParamsReadContext ctx)
	{
		int id, part, count;
		string chunk;
		if (!ctx.Read(id) || !ctx.Read(part) || !ctx.Read(count) || !ctx.Read(chunk))
			return;
		if (count < 1 || count > 128 || part < 0 || part >= count || chunk.Length() > DL_Const.FILM_CHUNK)
			return;
		if (m_Id != id || m_Parts.Count() != count)
		{
			m_Id = id;
			m_Got = 0;
			m_Parts.Clear();
			for (int i = 0; i < count; i++)
				m_Parts.Insert("");
		}
		if (m_Parts[part] == "")
			m_Got++;
		m_Parts[part] = chunk;
		if (m_Got != count)
			return;
		string json;
		foreach (string piece : m_Parts)
			json += piece;
		string err;
		JsonSerializer js = new JsonSerializer();
		if (!js.ReadFromString(m_Prepared, json, err))
			m_Prepared = null;
		m_Parts.Clear();
		m_Got = 0;
		DL_Log.Info("transition prototype: prepared travel cast");
	}

	void Start(string target)
	{
		Stop("new transfer");
		if (!m_Prepared || m_Prepared.events.Count() == 0)
			return;
		m_Target = target;
		m_Age = 0;
		m_LogAge = 0;
		m_ArrivalAge = -1;
		m_Active = true;
		m_Handoff = false;
		m_Motion = new DL_TransitionMotion();
		// The system update queue runs through menu/loading phases even while GUI queues are suspended.
		g_Game.GetUpdateQueue(CALL_CATEGORY_SYSTEM).Insert(Update);
		BuildView();
		DL_Log.Info("transition prototype: continuous scene started for " + target);
	}

	protected bool WantsLoadingWorkspace()
	{
		// The engine can report IsLoading=true while the login timer is drawn on the NORMAL workspace.
		if (m_LoginTimer && m_LoginTimer.IsVisible())
			return false;
		return m_Handoff || GetDayZGame().IsLoading();
	}

	protected void BuildView()
	{
		if (!m_Active || !m_Prepared)
			return;
		m_Loading = WantsLoadingWorkspace();
		WorkspaceWidget ws = g_Game.GetWorkspace();
		if (m_Loading)
			ws = g_Game.GetLoadingWorkspace();
		if (!ws)
			return;
		int sw, sh;
		GetScreenSize(sw, sh);
		m_Root = ws.CreateWidget(FrameWidgetTypeID, 0, 0, sw, sh, DL_FilmSpike.Flags(), ARGB(255, 255, 255, 255), 40000);
		if (!m_Root)
			return;
		DL_FilmSpike.Image(m_Root, DL_FilmSpike.PLAIN, ARGB(255, 10, 12, 14), 0, 0, 0, sw, sh);
		m_Status = TextWidget.Cast(ws.CreateWidget(TextWidgetTypeID, sw * 0.05, sh * 0.88, sw * 0.9, sh * 0.08, DL_FilmSpike.Flags(), ARGB(255, 240, 236, 226), 990000, m_Root));
		if (m_Status)
		{
			m_Status.SetText("Travelling to " + m_Target + " - connecting");
			m_Status.SetTextProportion(0.035);
		}
		m_Player = new DL_FilmPlayer();
		m_Player.TransitionParent(m_Root);
		m_Player.Play(m_Prepared);
		m_Player.RestoreTransitionMotion(m_Motion);
		DL_Log.Info("transition prototype: view recovered at " + m_Motion.elapsed.ToString() + " s, loading workspace " + m_Loading.ToString());
	}

	protected void MoveView(bool loading)
	{
		if (!m_Root)
			return;
		Widget parent = g_Game.GetWorkspace();
		if (loading)
			parent = g_Game.GetLoadingWorkspace();
		if (!parent)
			return;
		if (m_Root.GetParent() == parent && m_Loading == loading)
		{
			m_Root.SetSort(40000);
			m_Root.Show(true);
			return;
		}
		parent.AddChild(m_Root);
		m_Root.SetSort(40000);
		m_Root.Show(true);
		m_Loading = loading;
		DL_Log.Info("transition prototype: moved existing scene, loading workspace " + loading.ToString());
	}

	void LoginTimer(Widget root, int seconds)
	{
		m_LoginTimer = root;
		if (seconds >= 0)
			m_LoginSeconds = seconds;
		if (m_Active && m_Root)
			MoveView(false);
	}

	void LoginTimerClosed(Widget root)
	{
		if (m_LoginTimer == root)
		{
			m_LoginTimer = null;
			m_LoginSeconds = 0;
		}
	}

	void MissionFinishing()
	{
		if (!m_Active)
			return;
		m_Handoff = true;
		// Keep the native tree on the loading workspace while the old mission tears its UI down.
		MoveView(true);
	}

	// Missions clean up their widgets during a map change; retain only the session data across that cleanup.
	void DetachView()
	{
		if (m_Player)
			m_Player.SaveTransitionMotion(m_Motion);
		if (m_Player)
			m_Player.Stop();
		m_Player = null;
		if (m_Root)
			m_Root.Unlink();
		m_Root = null;
		m_Status = null;
		if (m_Active)
			DL_Log.Info("transition prototype: native view lost, motion retained");
	}

	void MissionStarted(string phase)
	{
		if (!m_Active)
			return;
		m_Handoff = false;
		DL_Log.Info("transition prototype: mission started (" + phase + ")");
		if (m_Root && m_Player && m_Player.HasView())
			MoveView(WantsLoadingWorkspace());
		else
			BuildView();
	}

	DL_FilmPlayer TakeArrival(string json)
	{
		if (!m_Active || !m_Player)
			return null;
		DL_Film film;
		string err;
		JsonSerializer js = new JsonSerializer();
		if (!js.ReadFromString(film, json, err) || !m_Player.AdoptArrival(film, json))
			return null;
		DL_FilmPlayer player = m_Player;
		m_Player = null;
		Stop("arrival continues with the same sprites");
		return player;
	}

	void Arrived()
	{
		if (m_Active)
		{
			m_ArrivalAge = 0;
			DL_Log.Info("transition prototype: destination mission started");
		}
	}

	void Stop(string reason)
	{
		if (m_Active)
			DL_Log.Info("transition prototype: stopped (" + reason + ")");
		m_Active = false;
		g_Game.GetUpdateQueue(CALL_CATEGORY_SYSTEM).Remove(Update);
		DetachView();
	}

	void Update(float dt)
	{
		if (!m_Active)
			return;
		m_Age += dt;
		if (m_ArrivalAge >= 0)
			m_ArrivalAge += dt;
		if (m_Age > 120 || m_ArrivalAge > 12 || KeyState(KeyCode.KC_ESCAPE) != 0 || g_Game.GetUIManager().IsDialogVisible())
		{
			Stop("timeout, cancel or dialog");
			return;
		}
		bool loading = WantsLoadingWorkspace();
		if (!m_Root || !m_Player || !m_Player.HasView())
		{
			DetachView();
			BuildView();
		}
		else if (loading != m_Loading)
			MoveView(loading);
		// Loading can produce a very large frame delta. Hold the film instead of jumping past scenes.
		if (m_Player)
		{
			m_Player.Update(Math.Min(dt, 0.05));
			m_Player.SaveTransitionMotion(m_Motion);
		}
		if (m_Status)
		{
			string phase = "connecting";
			if (m_LoginTimer && m_LoginTimer.IsVisible())
				phase = "server login: " + m_LoginSeconds.ToString() + " s (Esc to cancel)";
			else if (loading)
				phase = "loading destination";
			else if (GetDayZGame().DL_HasPending())
				phase = "changing servers";
			else if (m_ArrivalAge >= 0)
				phase = "preparing arrival";
			m_Status.SetText("Travelling to " + m_Target + " - " + phase);
		}
		m_LogAge += dt;
		if (m_LogAge >= 5)
		{
			m_LogAge = 0;
			DL_Log.Info("transition prototype: updating at " + m_Age.ToString() + " s, loading " + loading.ToString());
		}
	}
}
