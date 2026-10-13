// DriveLink - talking to the DriveLink Hub over HTTP (server only).
// Vanilla RestApi (3_game/http/restapi.c): CreateRestApi/GetRestApi (:181-183), RestApi.GetRestContext (:155),
// RestApi.SetOption ERESTOPTION_* (:28-35, 3..120 s), RestContext.POST(cb, request, data) (:123) and SetHeader,
// which only sets Content-Type (:136-141) - so the hub key goes in the JSON body.
// RestCallback: OnSuccess(data, size) / OnError(code) / OnTimeout() (:50-90).

class DL_HubBase
{
	string serverId;
	string key;
}

class DL_ArriveReq : DL_HubBase
{
	string steamId;
}

class DL_ArriveResp
{
	bool ok;
	string error;
	string action;            // "transfer" | "redirect" | "normal"
	string from;
	string arrival;
	ref DL_PlayerData payload;
	ref DL_ServerInfo target;
}

class DL_DepartReq : DL_HubBase
{
	string steamId;
	string to;
	string arrival;
	ref DL_PlayerData payload;
}

class DL_DepartResp
{
	bool ok;
	string error;
	string token;
	ref DL_ServerInfo target;
}

// Calls back <target>.<fn>(bool ok, string body) once, on success, error or timeout.
class DL_HubCallback : RestCallback
{
	protected Managed m_Target;
	protected string m_Fn;
	protected string m_What;
	protected bool m_Done;

	void DL_HubCallback(Managed target, string fn, string what)
	{
		m_Target = target;
		m_Fn = fn;
		m_What = what;
	}

	protected void Finish(bool ok, string body)
	{
		if (m_Done)
			return;
		m_Done = true;
		if (m_Target)
			// ScriptCallQueue.CallByName(obj, fnName, Param) (2_gamelib/tools.c:59)
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallByName(m_Target, m_Fn, new Param2<bool, string>(ok, body));
	}

	override void OnSuccess(string data, int dataSize)
	{
		Finish(true, data);
	}

	override void OnError(int errorCode)
	{
		Print(DL_Const.LOG_TAG + "hub " + m_What + " failed, error " + errorCode.ToString());
		Finish(false, "");
	}

	override void OnTimeout()
	{
		Print(DL_Const.LOG_TAG + "hub " + m_What + " timed out");
		Finish(false, "");
	}
}

class DL_Hub
{
	protected static ref array<ref DL_HubCallback> s_Alive = new array<ref DL_HubCallback>;

	static void Post(string baseUrl, string path, string json, Managed target, string fn, string what)
	{
		RestApi api = GetRestApi();
		if (!api)
		{
			api = CreateRestApi();
			api.SetOption(ERestOption.ERESTOPTION_READOPERATION, 15);
			api.SetOption(ERestOption.ERESTOPTION_CONNECTION, 5);
		}
		RestContext ctx = api.GetRestContext(baseUrl);
		ctx.SetHeader("application/json");
		DL_HubCallback cb = new DL_HubCallback(target, fn, what);
		s_Alive.Insert(cb); // keep it alive until the engine calls it
		if (s_Alive.Count() > 64)
			s_Alive.RemoveOrdered(0);
		ctx.POST(cb, path, json);
	}

	static string ToJson(Class data)
	{
		string json;
		JsonSerializer js = new JsonSerializer();
		js.WriteToString(data, false, json);
		return json;
	}
}
