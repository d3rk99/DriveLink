// DriveLink - client side: the server tells us where to go, we disconnect and connect there.
// Engine calls used (vanilla 1.29 scripts):
//   CGame.GetHostAddress(out address, out port)  "Gets the server address (from client)"  3_game/global/game.c:232
//   CGame.DisconnectSessionForce()                                                        3_game/global/game.c:265
//   CGame.Connect(UIScriptedMenu parent, string ip, int port, string password)            3_game/global/game.c:251
//   DayZGame.OnRPC(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext) 3_game/dayzgame.c:3104

class DL_LanMemory
{
	ref array<string> hosts = new array<string>;
}

class DL_Net
{
	// Private / local addresses (RFC 1918 + loopback). A player who joined us on one of these is on our network.
	static bool IsLocalAddress(string a)
	{
		if (a == "" || a == "localhost" || a.IndexOf("127.") == 0 || a.IndexOf("10.") == 0 || a.IndexOf("192.168.") == 0)
			return true;
		if (a.IndexOf("172.") == 0)
		{
			TStringArray parts = new TStringArray;
			a.Split(".", parts);
			if (parts.Count() >= 2)
			{
				int second = parts[1].ToInt();
				return second >= 16 && second <= 31;
			}
		}
		return false;
	}

	// LAN addresses this PC has joined a server on (kept in $profile:DriveLink_lan.json on the client). 2026-10-19:
	// Derk went Chernarus (LAN) -> Livonia on another network -> back: joined via a public address, so he was sent to
	// Chernarus's PUBLIC address, which his own router can't loop back to ("unreachable"). A LAN address this PC has
	// reached before is on its network, whatever way it came in this time.
	static const string LAN_FILE = "$profile:DriveLink_lan.json";
	static ref DL_LanMemory s_Lan;

	static DL_LanMemory Lan()
	{
		if (!s_Lan)
		{
			string err;
			if (!FileExist(LAN_FILE) || !JsonFileLoader<DL_LanMemory>.LoadFile(LAN_FILE, s_Lan, err) || !s_Lan)
				s_Lan = new DL_LanMemory();
		}
		return s_Lan;
	}

	static void RememberLan(string host)
	{
		if (!IsLocalAddress(host) || host == "" || host.IndexOf("127.") == 0 || Lan().hosts.Find(host) >= 0)
			return;
		Lan().hosts.Insert(host);
		string err;
		JsonFileLoader<DL_LanMemory>.SaveFile(LAN_FILE, Lan(), err);
		DL_Log.Detail("remembered " + host + " as a LAN server address of this PC");
	}

	// Which address of the target this client should use, decided by how it reached the current server.
	static string PickHost(DL_ServerInfo t, string joinedVia)
	{
		if (!t)
			return "";
		bool onLan = IsLocalAddress(joinedVia) || (t.lanHost != "" && joinedVia == t.lanHost) || (t.lanHost != "" && Lan().hosts.Find(t.lanHost) >= 0);
		if (onLan && t.lanHost != "")
			return t.lanHost;
		if (t.publicHost != "")
			return t.publicHost;
		return t.lanHost;
	}
}

modded class DayZGame
{
	protected ref DL_ServerInfo m_DL_Next;
	protected ref DL_TransitionFilm m_DL_Transition;
	protected bool m_DL_TransitionKey;

	DL_TransitionFilm DL_Transition()
	{
		if (!m_DL_Transition)
			m_DL_Transition = new DL_TransitionFilm();
		return m_DL_Transition;
	}

	void DL_LoginTimerClosed(Widget root)
	{
		if (m_DL_Transition)
			m_DL_Transition.LoginTimerClosed(root);
	}

	override void OnUpdate(bool doSim, float timeslice)
	{
		super.OnUpdate(doSim, timeslice);
		string prototype;
		if (!IsDedicatedServer() && CommandlineGetParam("dlTransitionDemo", prototype))
		{
			bool pressed = KeyState(KeyCode.KC_F8) != 0;
			if (pressed && !m_DL_TransitionKey)
				DL_Transition().Demo();
			m_DL_TransitionKey = pressed;
		}
	}
	protected string m_DL_NoticeCaption;
	protected string m_DL_Notice;    // shown as a dialog in the main menu after a RPC_KICK

	bool DL_HasPending()
	{
		return m_DL_Next != null;
	}

	bool DL_HasNotice()
	{
		return m_DL_Notice != "";
	}

	// Called from the main menu (5_Mission DL_MainMenu). UIManager.ShowDialog (3_game/tools/uimanager.c:45),
	// DBT_/DBB_/DMT_ constants are engine-defined (listed in 3_game/gameplay.c:617-633), used with a null handler
	// the same way as ingamemenuxbox.c:306.
	void DL_ShowNotice()
	{
		if (m_DL_Notice == "")
			return;
		GetUIManager().ShowDialog(m_DL_NoticeCaption, m_DL_Notice, 4810, DBT_OK, DBB_OK, DMT_INFO, null);
		m_DL_Notice = "";
	}

	override void OnRPC(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
	{
		if (rpc_type == DL_Const.RPC_TRANSITION && IsClient())
		{
			DL_Transition().Receive(ctx);
			return;
		}
		if (rpc_type == DL_Const.RPC_KICK && IsClient())
		{
			DL_Transition().Stop("server notice");
			string caption;
			string text;
			if (!ctx.Read(caption) || !ctx.Read(text))
				return;
			DL_Log.Detail("server sends us away: " + text);
			m_DL_NoticeCaption = caption;
			m_DL_Notice = text;
			GetCallQueue(CALL_CATEGORY_SYSTEM).Call(DisconnectSessionForce);
			return;
		}
		if (rpc_type == DL_Const.RPC_REDIRECT && IsClient())
		{
			string json;
			if (!ctx.Read(json))
				return;
			DL_Redirect r;
			string err;
			JsonSerializer js = new JsonSerializer();
			if (!js.ReadFromString(r, json, err) || !r || !r.target)
			{
				DL_Log.Warn("bad redirect: " + err);
				return;
			}
			string joinedVia;
			int joinedPort;
			GetHostAddress(joinedVia, joinedPort);
			DL_Net.RememberLan(joinedVia);
			string host = DL_Net.PickHost(r.target, joinedVia);
			string dlMsg134 = "redirect to " + r.target.name + " (" + r.reason;
			dlMsg134 += "): joined via " + joinedVia + " -> using " + host;
			dlMsg134 += ":" + r.target.port.ToString();
			DL_Log.Detail(dlMsg134);
			if (host == "")
			{
				DL_Transition().Stop("destination has no usable address");
				return;
			}
			m_DL_Next = r.target;
			m_DL_Next.lanHost = host; // the address we decided on; DL_Reconnect uses it
			if (r.reason == "zone")
				DL_Transition().Start(r.target.name);
			else
				DL_Transition().Stop("redirect without a travel preview");
			GetCallQueue(CALL_CATEGORY_SYSTEM).Call(DisconnectSessionForce);
			return;
		}
		super.OnRPC(sender, target, rpc_type, ctx);
	}

	// Called from the main menu once we are disconnected (5_Mission DL_MainMenu).
	void DL_Reconnect(UIScriptedMenu parent)
	{
		if (!m_DL_Next)
			return;
		DL_ServerInfo t = m_DL_Next;
		m_DL_Next = null;
		DL_Log.Detail("connecting to " + t.name + " " + t.lanHost + ":" + t.port.ToString());
		Connect(parent, t.lanHost, t.port, t.password);
	}
}
