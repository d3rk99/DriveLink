// DriveLink - config + data classes (3_Game: no world types here).
// Server config: $profile:DriveLink/config.json (written with an example on first start).

class DL_Const
{
	static const string LOG_TAG = "[DriveLink] ";
	static const string DIR = "$profile:DriveLink";
	static const string CONFIG = "$profile:DriveLink/config.json";
	static const int RPC_REDIRECT = 4810001;   // server -> client: go to this server
	static const int RPC_COUNTDOWN = 4810002;  // server -> client: leaving in N s (0 = cancelled)
	static const string SLOT_CARGO = "#cargo";
	static const string SLOT_HANDS = "#hands";
	static const int MAX_ITEMS = 600;
}

// A zone at the map edge. Standing (or later: driving) inside it for countdownSeconds sends you to "to".
class DL_Zone
{
	string name;              // shown to players
	vector center;            // x y z (y ignored)
	float radius = 30;        // metres
	string to;                // target server id (from the hub)
	string arrival;           // arrival point name on the target
}

// Where you come out when you arrive from another server.
class DL_ArrivalPoint
{
	string name;
	vector pos;
	float yaw;
}

class DL_Config
{
	string hubUrl = "http://127.0.0.1:7290";   // the DriveLink Hub
	string hubKey = "";                         // serverKey from the hub's hub.json
	string serverId = "chernarus";              // this server's id in hub.json
	int countdownSeconds = 5;
	float checkEverySeconds = 1;
	int arrivalProtectSeconds = 10;             // no damage right after arriving
	ref array<ref DL_Zone> zones = new array<ref DL_Zone>;
	ref array<ref DL_ArrivalPoint> arrivals = new array<ref DL_ArrivalPoint>;

	DL_ArrivalPoint FindArrival(string name)
	{
		foreach (DL_ArrivalPoint a : arrivals)
		{
			if (a && a.name == name)
				return a;
		}
		return null;
	}
}

// One item, attachments + cargo nested (same shape as PersistentBodies' PB_ItemData, same author).
class DL_ItemData
{
	string type;
	string slot;
	float health01 = 1;
	float quantity = -1;
	int ammo = -1;
	float wet = 0;
	int liquid = 0;
	ref array<ref DL_ItemData> children = new array<ref DL_ItemData>;
}

// Everything that travels with a player.
class DL_PlayerData
{
	int version = 1;
	string charType;          // character class (face/body), e.g. SurvivorM_Mirek
	float health = 100;       // "" "Health"
	float blood = 5000;       // "" "Blood"
	float shock = 0;          // "" "Shock"
	float water = 1800;       // GetStatWater
	float energy = 1800;      // GetStatEnergy
	float heatComfort = 0;
	bool restrained;          // handcuffed / tied (PlayerBase.IsRestrained)
	string restrainItem;      // the locked restraint item in the hands, e.g. HandcuffsLocked
	bool unconscious;
	ref array<ref DL_ItemData> items = new array<ref DL_ItemData>;
}

// Server address entry (from the hub's /api/hello and redirect answers).
class DL_ServerInfo
{
	string id;
	string name;
	string lanHost;
	string publicHost;
	int port;
	string password;
}

// Sent to the client in RPC_REDIRECT as JSON.
class DL_Redirect
{
	ref DL_ServerInfo target;
	string reason;
}
