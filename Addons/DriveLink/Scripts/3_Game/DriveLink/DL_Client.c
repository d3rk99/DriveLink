// DriveLink - client side: the server tells us where to go, we disconnect and connect there.
// Engine calls used (vanilla 1.29 scripts):
//   CGame.GetHostAddress(out address, out port)  "Gets the server address (from client)"  3_game/global/game.c:232
//   CGame.DisconnectSessionForce()                                                        3_game/global/game.c:265
//   CGame.Connect(UIScriptedMenu parent, string ip, int port, string password)            3_game/global/game.c:251
//   DayZGame.OnRPC(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext) 3_game/dayzgame.c:3104

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

	// Which address of the target this client should use, decided by how it reached the current server.
	static string PickHost(DL_ServerInfo t, string joinedVia)
	{
		if (!t)
			return "";
		bool onLan = IsLocalAddress(joinedVia) || (t.lanHost != "" && joinedVia == t.lanHost);
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
		if (rpc_type == DL_Const.RPC_KICK && IsClient())
		{
			string caption;
			string text;
			if (!ctx.Read(caption) || !ctx.Read(text))
				return;
			Print(DL_Const.LOG_TAG + "server sends us away: " + text);
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
				Print(DL_Const.LOG_TAG + "bad redirect: " + err);
				return;
			}
			string joinedVia;
			int joinedPort;
			GetHostAddress(joinedVia, joinedPort);
			string host = DL_Net.PickHost(r.target, joinedVia);
			Print(DL_Const.LOG_TAG + "redirect to " + r.target.name + " (" + r.reason + "): joined via " + joinedVia + " -> using " + host + ":" + r.target.port.ToString());
			if (host == "")
				return;
			m_DL_Next = r.target;
			m_DL_Next.lanHost = host; // the address we decided on; DL_Reconnect uses it
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
		Print(DL_Const.LOG_TAG + "connecting to " + t.name + " " + t.lanHost + ":" + t.port.ToString());
		Connect(parent, t.lanHost, t.port, t.password);
	}
}
