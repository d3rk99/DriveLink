// DriveLink - talking to the DriveLink Hub over HTTP (server only).
// Vanilla RestApi (3_game/http/restapi.c): CreateRestApi/GetRestApi (:181-183), RestApi.GetRestContext (:155),
// RestApi.SetOption ERESTOPTION_* (:28-35, 3..120 s), RestContext.POST(cb, request, data) (:123) and SetHeader,
// (whose comment says Content-Type, :136-141, but UniversalApi uses it for its auth token). The key goes in the body + URL.
// RestCallback: OnSuccess(data, size) / OnError(code) / OnTimeout() (:50-90).

// M1 retest (2026-10-13): the hub got "wrong key" although both keys were equal. Suspect: DayZ's JsonSerializer
// does not write fields inherited from a base class, so serverId/key were missing. Every request class is now flat
// (no base class), and the key ALSO travels in the URL (?serverId=..&key=..), which the hub accepts as well.
class DL_ArriveReq
{
	string serverId;
	string key;
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

class DL_DepartReq
{
	string serverId;
	string key;
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
	protected ref Managed m_Target;   // strong ref: the waiter objects are made inline and nothing else holds them
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

	// M1 retest 2 (2026-10-13): every call ended in OnError(5) = EREST_ERROR (restapi.c:16) and the hub logged
	// nothing. Changed to the pattern a working mod uses (DaemonForge UniversalApi UniversalRest.c:14-22, read only):
	// the WHOLE url goes into GetRestContext and the request part of POST stays "". No SetHeader call: the hub
	// reads the body as JSON whatever the content type is.
	static void Post(string baseUrl, string path, string json, Managed target, string fn, string what)
	{
		RestApi api = GetRestApi();
		if (!api)
		{
			api = CreateRestApi();
			api.SetOption(ERestOption.ERESTOPTION_READOPERATION, 15);
			api.SetOption(ERestOption.ERESTOPTION_CONNECTION, 5);
		}
		string url = baseUrl + path;
		RestContext ctx = api.GetRestContext(url);
		DL_HubCallback cb = new DL_HubCallback(target, fn, what);
		s_Alive.Insert(cb); // keep it alive until the engine calls it
		if (s_Alive.Count() > 64)
			s_Alive.RemoveOrdered(0);
		Print(DL_Const.LOG_TAG + "POST " + baseUrl + path.Substring(0, path.IndexOf("?")) + " (" + json.Length().ToString() + " bytes) for " + what);
		ctx.POST(cb, "", json);
	}

	// a8ae263 test: ToJson(Class data) sent "{}" (2 bytes): the serializer only sees the static type Class, which has
	// no fields. Serialize with the REAL type instead: JsonFileLoader<T>.MakeData(T inputData, out string, out string err, bool pretty)
	// (vanilla 3_game/tools/jsonfileloader.c:85-96).
	static string ArriveJson(DL_ArriveReq q)
	{
		string json;
		string err;
		if (!JsonFileLoader<DL_ArriveReq>.MakeData(q, json, err, false))
			Print(DL_Const.LOG_TAG + "arrive json: " + err);
		return json;
	}

	static string DepartJson(DL_DepartReq q)
	{
		string json;
		string err;
		if (!JsonFileLoader<DL_DepartReq>.MakeData(q, json, err, false))
			Print(DL_Const.LOG_TAG + "depart json: " + err);
		return json;
	}

}
