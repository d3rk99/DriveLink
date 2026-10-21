// DriveLink - config + data classes (3_Game: no world types here).
// Server config: $profile:DriveLink/config.json (written with an example on first start).

class DL_Const
{
	static const string VERSION = "0.4";
	// the hub <-> mod protocol (Hub/src/api.rs PROTOCOL): every hub call carries it; another one = travel off here
	static const int PROTOCOL = 1;
	static const string LOG_TAG = "[DriveLink] ";
	static const string DIR = "$profile:DriveLink";
	static const string CONFIG = "$profile:DriveLink/config.json";
	static const string DEBUG_FILE = "$profile:DriveLink/debug.json";
	static const int RPC_REDIRECT = 4810001;   // server -> client: go to this server
	static const int RPC_COUNTDOWN = 4810002;  // server -> client: leaving in N s (0 = cancelled)
	static const int RPC_FLY = 4810008;        // server -> a plane's pilot: vector velocity, keep it flying (arrival)
	static const int RPC_KICK = 4810003;       // server -> client: show this message (caption, text) and leave the server
	static const int RPC_FILM = 4810005;       // server -> client: the road film of his trip (DL_Film as JSON), in
	                                           // parts: int id, int part, int parts, string chunk (a long film's JSON
	                                           // in one string arrived "String CORRUPTED" on the client)
	static const int FILM_CHUNK = 900;
	static const int RPC_FILM_VOTE = 4810006;  // client -> server: he pressed Space to skip the film (or it ended)
	static const int RPC_FILM_SKIP = 4810007;  // server -> clients watching one film: votes, total, skip now
	static const int RPC_CREW_WAIT = 4810004;  // server -> client: crew waiting room (phase, here, expected, text, start engine)
	static const int WAIT_SHOW = 1;            // black screen + "n/m connected"
	static const int WAIT_DONE = 2;            // lift the black screen (and the driver starts the engine)
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
	string shape = "circle";  // "circle" (centre + radius) | "rect" (centre + width along x, depth along z)
	float width;
	float depth;
	bool air;                 // a plane zone: only aircraft in flight travel through it, and they through no other
	string to;                // target server id (from the hub)
	string arrival;           // arrival point name on the target
	float distanceKm;         // trip length: travel cost/time scale with it (0 = free, instant)
	string viaBiome;          // the road film's look on the way (between this server's and the target's), "" = none
}

// Travel cost + time for trips that START on this server (hub web app, Servers page, Travel). Costs are per 10 km of
// the zone's distance. Research/travel-cost-design.md. Same field names as the hub's TravelRules.
class DL_TravelRules
{
	string mode = "refuse";   // "refuse" = short of supplies, no trip; "weakened" = the trip goes, the road decides
	// food (DayZ energy = kcal) and water (ml) per 10 km, per person; a full bar is 5000 of each
	float foodFootKcal = 750;
	float waterFootMl = 750;
	float foodCarKcal = 200;  // in a vehicle
	float waterCarMl = 250;
	float fuelLiters = 5;
	// fuel by the vehicle's own in-game consumption (CfgVehicles fuelConsumption, read as litres per 100 km - the
	// vanilla values match the real cars' figures) instead of fuelLiters; x fuelScale, boats x fuelBoatScale too
	bool fuelPerVehicle;
	float fuelScale = 1;
	float fuelBoatScale = 1;
	float swimFoodX = 1.5;    // swimming (a boat party with no boat): food and water x this of the foot rates
	float drownPct = 6;       // weakened: chance to drown per 10 km swum, x2 the 2nd 10 km, x3 the 3rd ...
	// weakened: what the road may give, chance per 10 km (before anything runs out)
	float findGasPct = 6;     // a working gas station (a fuel dock for boats): every tank + gasoline canister full
	float findFoodPct = 8;    // food: a few items into the trunk / their bags
	float findWaterPct = 12;  // a well (rain at sea): everyone drinks, every bottle / canteen filled
	float wearEnginePct = 2;
	float wearWheelsPct = 3;
	bool timeEnabled;         // on the road for distance / speed, no login anywhere meanwhile
	float speedFootKmh = 5;
	float speedCarKmh = 40;
	float luckGasPct = 35;    // weakened mode chances, in %
	float bitePct = 5;
	float biteDeathPct = 10;
	float starveDeathPct = 20;
	float spareNeededKm;      // refuse mode: car trips longer than this need a spare wheel or a TireRepairKit (0 = off)
	float flatPct = 3;        // weakened mode (step 4): flat tyre chance per 10 km
	string biome;             // this server's look in the road film: forest|field|desert|snow, "" = from the map
	// group travel (Research/group-travel.md), parties leaving here; 0 = the default
	bool noGroups;            // walkers who enter a zone together do NOT travel as a group
	bool noConvoys;           // cars that enter a zone together do NOT travel as a convoy
	int groupMax = 8;
	int convoyMax = 4;
	int convoyWaitSeconds = 120; // arriving here: a convoy waits this long for a late car
	// the road film of trips that END here
	bool filmOff;             // story text only
	bool voteSkipOff;         // no skipping (else everyone watching presses Space)
}

// Where you come out when you arrive from another server.
class DL_ArrivalPoint
{
	string name;
	vector pos;
	float yaw;
	vector carPos;            // where a car that comes along is built; "0 0 0" = at pos (the driver then stands next to it)
	float carYaw;
	vector boatPos;           // where a boat that comes along is put on the water; "0 0 0" = the nearest free water
	float boatYaw;
	vector airPos;            // a plane arrives here in flight: x, height above the ground, z; "0 0 0" = above pos
	float airYaw;
}

class DL_Config
{
	string hubUrl = "http://127.0.0.1:7290";   // the DriveLink Hub
	string hubKey = "";                         // the hub's server key (web app, Setup page)
	string serverId = "";                       // filled in by the hub once this server is approved (Servers page)
	string joinId = "";                         // made on the first start: how the hub tells this server apart before it has an id
	string log = "normal";                     // "quiet" (warnings only) | "normal" (trips, arrivals) | "verbose" (every detail)
	int countdownSeconds = 5;
	float checkEverySeconds = 1;
	int arrivalProtectSeconds = 10;             // no damage right after arriving
	// M3 waiting room: a car crew arriving here waits (black screen) until all of them are in, at most this long;
	// then the car is built with whoever made it (no driver by then = the passengers go on on foot).
	int crewWaitSeconds = 120;
	// M2: a car that left is moved here (ground height) until its seats are empty and it can be deleted, so nobody
	// can loot its trunk in the meantime (those items would exist twice). Pick a spot far from players. "0 0 0" = off.
	vector parkingSpot;
	ref DL_TravelRules travel = new DL_TravelRules(); // from the hub with the layout
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
	// M2: the car he drove into the zone (driver only for now), and his seat in it. A null ref may come back from
	// JSON as an EMPTY object, so "has a car" = car && car.type != "".
	ref DL_CarData car;
	int carSeat = -1;
	// M3: a passenger travels WITHOUT the car: carGroup = the driver's steam id, carSeat = his seat. The target seats
	// him in the car the driver brings (only the driver's trip holds the car, so it can only be built once).
	string carGroup;
	// waiting room: how many players were in the car when it left (driver included). 0 = an old snapshot, no waiting.
	int crewCount;
	// a convoy: this car's place in the line (0 = the lead car; the target parks them one behind the other);
	// a group on foot: his place beside the others at the arrival point
	int convoyIdx;
	int partySlot;
	string partyId;           // everyone who travels together (group, convoy, crew) has the same one; "" = alone
	string vehKind;           // what he arrives in: "car" | "bike" | "boat" | "" (on foot) - a boat's crew is built on land
	// a convoy: its id and how many of its cars arrive (the target lifts the black screen for all of them at once)
	string convoyId;
	int convoyCars;
	// travel step 4 (weakened mode): what happened on the road (shown on the arrival black screen), and whether he
	// died there (the target then gives him a fresh character: the road kept his gear)
	string journey;
	bool diedOnRoad;
	ref DL_Film film;         // the road film of the trip (DL_FilmData.c), played on the arrival screen
}

// A car (M2). Parts (wheels, doors, battery, plugs, radiator...) are attachments and the trunk is cargo, so they are
// the same flat item list as a player's gear (parent 0 = the car). Research/R2-cars.md.
class DL_CarData
{
	int version = 1;
	string type;
	vector pos;
	vector ori;
	float health = -1;
	ref array<float> fluids = new array<float>;   // fraction 0..1 per CarFluid (FUEL..USER4, car.c:18-29), index = enum value
	ref array<float> fluidCaps = new array<float>; // capacity per CarFluid on the source (a brand-new car may read 0 at first)
	ref array<ref DL_Zone_Health> zones = new array<ref DL_Zone_Health>;
	ref array<ref DL_ItemData> items = new array<ref DL_ItemData>;
	vector vel;               // its velocity when it left, m/s (a plane goes on flying with it)
}

// $profile:DriveLink/debug.json - test switches, read only by a dev build (DL_DEV, Tools/build-mod.ps1 -Dev) and only
// when the file exists.
class DL_DebugZone
{
	vector center;
	float radius = 10;
}

class DL_Debug
{
	// M2 spike 1: an EMPTY car standing 5 s in this circle is snapshotted, deleted, sent through JSON and rebuilt on
	// the same spot; the log compares before/after. null/radius 0 = off.
	ref DL_DebugZone carRebuildZone;
	// M2 spike 2: on every login a car of this type (vanilla debug kit: wheels, parts, fluids) appears next to the
	// player and 3 s later he is put in the driver seat. "" = off.
	string seatTestCar;
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
