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
	string name;              // shown on the hub's Players page
}

// Map editor: zones + arrival points live in the hub. /api/hello sends this server's config.json copy (the hub
// imports it the first time) and gets the hub's layout back; /api/layout polls it by rev (live edits, no restart).
class DL_HelloReq
{
	string serverId;
	string key;
	// joining without an id: the hub lists this server on its Servers page to approve
	string claim;
	int port;
	int queryPort;
	string world;
	int worldSize;            // the map's size in metres (the hub's Map page: a map made from a picture)
	ref array<ref DL_Zone> zones;
	ref array<ref DL_ArrivalPoint> arrivals;
}

class DL_Layout
{
	int rev;
	string world; // "map" is a type name in Enforce
	bool changed;
	ref DL_TravelRules travel; // this server's travel rules (hub Servers page)
	ref array<ref DL_Zone> zones;
	ref array<ref DL_ArrivalPoint> arrivals;
}

class DL_HelloResp
{
	bool ok;
	bool pending;             // known key, but not approved on the hub's Servers page yet
	string serverId;          // the id the hub knows this server by
	string error;
	bool incompatible;        // another protocol: the error says whether the mod or the hub is the older one
	ref DL_Layout layout;
}

class DL_LayoutReq
{
	string serverId;
	string key;
	int rev;
}

// /api/payload, /api/confirm and /api/fail all name one transfer by its token.
class DL_TokenReq
{
	string serverId;
	string key;
	string steamId;
	string token;
	string reason;            // /api/fail only: why the character could not be built
}

class DL_ArriveResp
{
	bool ok;
	string error;
	string token;             // transfer token: fetch the snapshot with it (/api/payload), then /api/confirm
	string action;            // "transfer" | "redirect" | "normal" | "wait" (on the road, travel time)
	int seconds;              // "wait": seconds until he arrives
	string from;
	string arrival;
	int bytes;                // length of the snapshot /api/payload will send (to spot a cut-off body)
	ref DL_PlayerData payload; // NOT in the hub's JSON: filled from the /api/payload body by DL_Hub.ParsePayload
	ref DL_ServerInfo target;
}

class DL_DepartReq
{
	string serverId;
	string key;
	string steamId;
	string to;
	string arrival;
	int travelSeconds;        // travel time on: on the road this long (0 = at once)
	ref DL_PlayerData payload;
}

class DL_DepartResp
{
	bool ok;
	string error;
	string token;
	int readyIn;              // seconds on the road (0 = he goes straight to the target)
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
		DL_Hub.Handshake(data);
		Finish(true, data);
	}

	override void OnError(int errorCode)
	{
		DL_Log.Info("hub " + m_What + " failed, error " + errorCode.ToString());
		Finish(false, "");
	}

	override void OnTimeout()
	{
		DL_Log.Warn("hub " + m_What + " timed out");
		Finish(false, "");
	}
}

class DL_Hub
{
	protected static ref array<ref DL_HubCallback> s_Alive = new array<ref DL_HubCallback>;
	// the hub speaks another protocol (a different DriveLink build): no travel here until both match
	static bool s_Incompatible;

	// every hub answer: "incompatible" = this mod and the hub are different builds (the hub says which to update);
	// an ordinary answer after that = they match again (the hub or the mod was updated)
	static void Handshake(string body)
	{
		bool bad = body.Contains("\"incompatible\":true");
		if (bad && !s_Incompatible)
		{
			string why = body;
			int at = body.IndexOf("\"error\":\"");
			if (at >= 0)
			{
				why = body.Substring(at + 9, body.Length() - at - 9);
				int end = why.IndexOf("\"");
				if (end > 0)
					why = why.Substring(0, end);
			}
			DL_Log.Warn("TRAVEL IS OFF on this server: " + why);
		}
		else if (!bad && s_Incompatible && body.Contains("\"ok\":true"))
			DL_Log.Warn("the hub and this server's DriveLink match again: travel is back on");
		if (bad)
			s_Incompatible = true;
		else if (body.Contains("\"ok\":true"))
			s_Incompatible = false;
	}

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
		// the handshake rides on every call: this build's protocol and version (the hub refuses another protocol)
		string url = baseUrl + path + "&proto=" + DL_Const.PROTOCOL.ToString() + "&ver=" + DL_Const.VERSION;
		RestContext ctx = api.GetRestContext(url);
		DL_HubCallback cb = new DL_HubCallback(target, fn, what);
		s_Alive.Insert(cb); // keep it alive until the engine calls it
		if (s_Alive.Count() > 64)
			s_Alive.RemoveOrdered(0);
		if (what != "layout") // the 30 s layout poll would fill the log
		{
			string dlMsg171 = "POST " + baseUrl + path.Substring(0, path.IndexOf("?")) + " (";
			dlMsg171 += json.Length().ToString() + " bytes) for " + what;
			DL_Log.Detail(dlMsg171);
		}
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
			DL_Log.Info("arrive json: " + err);
		return json;
	}

	// The snapshot is the WHOLE body of the hub's /api/payload answer, the exact text the source server wrote,
	// parsed with JsonFileLoader<T>.LoadData (vanilla 3_game/tools/jsonfileloader.c:69).
	// Why not inside the /api/arrive JSON as a string (payloadJson, 2f8669a..b6a23e5): DayZ failed on it every time
	// ("Miss fraction part in number", e37611b + v0.2 test 2026-10-13) while the 0cd3377 self test parsed the SAME
	// text read from a file without error. So the text was damaged on its way out of the escaped JSON string.
	static DL_PlayerData ParsePayload(string json, out string err)
	{
		DL_PlayerData d;
		if (!JsonFileLoader<DL_PlayerData>.LoadData(json, d, err) || !d)
		{
			DL_Log.Warn("payload unreadable: " + err);
			if (err == "")
				err = "payload unreadable";
			return null;
		}
		return d;
	}

	// Items are a flat list since v0.2 (DL_ItemData.parent), so counting is just the list length.
	static int CountItems(array<ref DL_ItemData> items)
	{
		if (!items)
			return 0;
		return items.Count();
	}

	static string TokenJson(DL_TokenReq q)
	{
		string json;
		string err;
		if (!JsonFileLoader<DL_TokenReq>.MakeData(q, json, err, false))
			DL_Log.Info("token request json: " + err);
		return json;
	}

	static string DepartJson(DL_DepartReq q)
	{
		string json;
		string err;
		if (!JsonFileLoader<DL_DepartReq>.MakeData(q, json, err, false))
			DL_Log.Info("depart json: " + err);
		return json;
	}

}
