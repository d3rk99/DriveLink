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

// One round in a magazine / weapon (Magazine.GetCartridgeAtIndex, magazine.c:97; Weapon.GetCartridgeInfo, weapon.c:159).
class DL_Round
{
	string type;
	float dmg;
}

// One item. FLAT list on purpose: every item has an id, its parent's id (0 = on the player) and its exact place.
// No nesting = no recursion anywhere (0cd3377/5e724b0: recursive code lost everything after the first element),
// and parents always come before their children in the list (LEVELORDER, see DL_Snapshot).
class DL_ItemData
{
	int id;
	int parent;               // 0 = the player
	string type;
	string slot;              // attachment slot name, "#cargo", "#hands"
	int cIdx = -1;            // cargo position (InventoryLocation.GetIdx/GetRow/GetCol/GetFlip, inventorylocation.c:77-95)
	int cRow;
	int cCol;
	bool cFlip;
	int quickbar = -1;        // PlayerBase.FindQuickBarEntityIndex (playerbase.c:2218)
	float health = -1;        // absolute global health
	float quantity = -1;
	float wet;
	float temperature = -10000; // -10000 = not saved
	int liquid;
	int agents;
	int cleanness;
	float energy = -1;        // ComponentEnergyManager.GetEnergy (componentenergymanager.c:1259)
	bool isOn;
	int foodStage = -1;       // Edible_Base.GetFoodStageType (edible_base.c:531)
	ref array<ref DL_Round> rounds;        // magazine contents, in order
	ref array<ref DL_Round> chambers;      // weapon: per muzzle, "" = empty chamber
	ref array<ref DL_Round> internalMag;   // weapon: internal magazine rounds (muzzle 0)
	int muzzle = -1;
	ref array<int> fireModes;
}

class DL_Modifier
{
	int id;
	float time;
}

class DL_Bleeding
{
	int bit;
	int time;
	int type;
}

class DL_StomachItem
{
	string type;
	float amount;
	int stage;
	int agents;
	float temperature;
}

class DL_Zone_Health
{
	string zone;
	float health;
}

// Everything that travels with a player.
class DL_PlayerData
{
	int version = 2;
	string charType;          // character class (face/body), e.g. SurvivorM_Mirek
	float health = 100;
	float blood = 5000;
	float shock = 0;
	ref array<ref DL_Zone_Health> zones = new array<ref DL_Zone_Health>;
	float water = 1800;
	float energy = 1800;
	float heatComfort = 0;
	int bloodType = -1;
	bool bloodTypeVisible;
	int lifespan;             // beard
	int lastShaved;
	int bloodyHands;
	int brokenLegs;
	int persistentFlags;
	float playtime;           // AnalyticsManagerServer stats
	float killsPlayers;
	float killsInfected;
	float distance;
	float longestShot;
	bool restrained;
	string restrainItem;
	bool unconscious;
	ref array<ref DL_Modifier> modifiers = new array<ref DL_Modifier>;
	ref array<ref DL_Modifier> agentPool = new array<ref DL_Modifier>;   // id = agent, time = count
	ref array<ref DL_Bleeding> bleeding = new array<ref DL_Bleeding>;
	ref array<ref DL_StomachItem> stomach = new array<ref DL_StomachItem>;
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
