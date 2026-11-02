// DriveLink interactive trail (Research/interactive-trail-design.md): the road film played on the SOURCE server
// before the party leaves, stopping now and then on a choice card the party votes on. Cards are data
// ($profile:DriveLink/cards.json, written with the default deck on first start): a server owner adds, edits or turns
// off cards without a new build. What an outcome can do is fixed here (DL_TrailOutcome); the server applies the
// result to the travellers' snapshots and characters (DL_TrailServer). 3_Game: plain data + rules, no world types.

// What one outcome does. Every field is optional; text is the caption ({who} = the member it hit, {item} = the item
// that was used, {car} = the car).
class DL_TrailOutcome
{
	float weight = 1;
	float withHelp = -1;          // the weight when the party carries one of the option's helps (-1 = weight)
	string text;
	float minutes;                // + slower / - faster (travel time on); instant arrival turns + into risk
	float km;                     // + a detour / lost the way (more road, more fuel)
	float fuel;                   // share of every tank lost (0..1)
	int hurt;                     // members wounded (health -30)
	int kill;                     // members killed (a bandage or rags may save one)
	bool loseCar;                 // the car the card is about stays behind
	float food;                   // kcal per person, + found / - lost
	float water;                  // ml per person
	int sick;                     // members made sick (slows the party unless someone carries medicine)
	bool useItem;                 // the help (or needed) item that was carried is used up
	int loseItems;                // items taken from the party (a toll, a thief)
	string anim;                  // how the film plays it out: steps, e.g. "out shoot:npc back drive" (DL_FilmPlayer.ActStep)
}

class DL_TrailOption
{
	string label;
	string hint = "risky";        // safe | risky | dangerous (shown; the odds stay hidden)
	ref array<string> needs = new array<string>;   // one of these must be carried, else the option is not offered
	ref array<string> helps = new array<string>;   // carrying one of these changes the odds (withHelp)
	bool drive;                   // only when someone is still driving
	bool walk;                    // only when someone walks
	bool convoy;                  // only with two cars or more (a tow)
	ref array<ref DL_TrailOutcome> outcomes = new array<ref DL_TrailOutcome>;
}

class DL_TrailCard
{
	string id;
	string title;
	string text;
	bool off;                     // a server owner's switch
	float weight = 1;             // how often it comes up against the others
	ref array<string> biomes = new array<string>;  // forest | field | desert | snow; empty = any
	string mode = "any";          // drive | walk | any
	int minMembers = 1;
	int minCars;                  // e.g. a tow needs 2
	ref array<string> needs = new array<string>;   // the card only comes up when one of these is carried (a deer: a rifle)
	string scene;                 // what the film sets up while they vote, e.g. "barrier bandits:2" (DL_FilmPlayer.StageUp)
	ref array<ref DL_TrailOption> options = new array<ref DL_TrailOption>;
}

class DL_TrailDeck
{
	static const int VERSION = 4;  // 2: the film's card scenes and outcome animations, 3: the rest of the cards, 4: the fork's compass
	int version = 1;
	ref array<ref DL_TrailCard> cards = new array<ref DL_TrailCard>;

	static const string FILE = "$profile:DriveLink/cards.json";

	// the server's deck: cards.json, written with the default deck when it isn't there
	static DL_TrailDeck Load()
	{
		DL_TrailDeck d;
		string err;
		if (FileExist(FILE))
		{
			if (JsonFileLoader<DL_TrailDeck>.LoadFile(FILE, d, err) && d && d.cards.Count() > 0)
			{
				if (d.version < VERSION)
				{
					d.Upgrade(Default());
					JsonFileLoader<DL_TrailDeck>.SaveFile(FILE, d, err);
					DL_Log.Info("cards.json: the default cards got their film scenes (version " + VERSION.ToString() + ")");
				}
				return d;
			}
			DL_Log.Warn("cards.json unreadable (" + err + "): the default deck is used");
			return Default();
		}
		d = Default();
		JsonFileLoader<DL_TrailDeck>.SaveFile(FILE, d, err);
		DL_Log.Info("wrote " + FILE + " (" + d.cards.Count().ToString() + " trail cards): edit it to add or change cards");
		return d;
	}

	// an older cards.json: the default cards it still has (same id, same option and outcome texts) get the scenes and
	// animations of the new defaults; the owner's own cards and edited texts are left as they are
	void Upgrade(DL_TrailDeck def)
	{
		array<string> oldAnims = {"out work:1.5 back drive"};    // older default animations: these get the new default
		foreach (DL_TrailCard c : cards)
		{
			foreach (DL_TrailCard dc : def.cards)
			{
				if (dc.id != c.id)
					continue;
				if (c.scene == "")
					c.scene = dc.scene;
				for (int i = 0; i < c.options.Count() && i < dc.options.Count(); i++)
				{
					DL_TrailOption o = c.options[i];
					DL_TrailOption dop = dc.options[i];
					if (o.label != dop.label)
						continue;
					for (int k = 0; k < o.outcomes.Count() && k < dop.outcomes.Count(); k++)
					{
						bool old = o.outcomes[k].anim == "" || oldAnims.Find(o.outcomes[k].anim) >= 0;
						if (old && o.outcomes[k].text == dop.outcomes[k].text)
							o.outcomes[k].anim = dop.outcomes[k].anim;
					}
				}
			}
		}
		version = VERSION;
	}

	// ---------------- the default deck ----------------

	protected static DL_TrailCard Card(DL_TrailDeck d, string id, string title, string text, string mode = "any")
	{
		DL_TrailCard c = new DL_TrailCard();
		c.id = id;
		c.title = title;
		c.text = text;
		c.mode = mode;
		d.cards.Insert(c);
		return c;
	}

	protected static DL_TrailOption Opt(DL_TrailCard c, string label, string hint, string needs = "", string helps = "")
	{
		DL_TrailOption o = new DL_TrailOption();
		o.label = label;
		o.hint = hint;
		if (needs != "")
			needs.Split(",", o.needs);
		if (helps != "")
			helps.Split(",", o.helps);
		c.options.Insert(o);
		return o;
	}

	protected static DL_TrailOutcome Out(DL_TrailOption o, float weight, float withHelp, string text)
	{
		DL_TrailOutcome r = new DL_TrailOutcome();
		r.weight = weight;
		r.withHelp = withHelp;
		r.text = text;
		o.outcomes.Insert(r);
		return r;
	}

	static DL_TrailDeck Default()
	{
		DL_TrailDeck d = new DL_TrailDeck();
		d.version = VERSION;
		DL_TrailCard c;
		DL_TrailOption o;
		DL_TrailOutcome r;
		string medic = "BandageDressing,Rag,FirstAidKit";
		string rifle = "kind:Rifle_Base";

		c = Card(d, "river", "River crossing", "The bridge is gone. The river runs fast and brown.");
		c.scene = "river";
		o = Opt(c, "Ford it", "risky", "", "Rope");
		r = Out(o, 5, 7, "Across. Wet boots, nothing lost.");
		r.anim = "ford";
		r = Out(o, 3, 2, "The current takes a bag: {who} loses some gear.");
		r.anim = "ford splash";
		r.loseItems = 2;
		r = Out(o, 2, 1, "The water floods the engine. The car stays in the river.");
		r.anim = "stall all";
		r.loseCar = true;
		o.drive = true;
		o = Opt(c, "Look for another bridge", "safe");
		r = Out(o, 1, -1, "Found one upstream, two hours out of the way.");
		r.anim = "clear drive";
		r.minutes = 120;
		r.km = 15;
		o = Opt(c, "Wade across on foot", "dangerous", "", "Rope");
		o.walk = true;
		r = Out(o, 6, 9, "Across, soaked and cold.");
		r.anim = "wade";
		r = Out(o, 3, 1, "{who} slips and is dragged under, then pulled out half drowned.");
		r.anim = "wade hit";
		r.hurt = 1;
		r = Out(o, 1, 0, "{who} is swept away.");
		r.anim = "wade";
		r.kill = 1;

		c = Card(d, "roadblock", "Roadblock", "Cars across the road, rifles behind them. \"Toll road, friends.\"");
		c.scene = "barrier bandits:2";
		o = Opt(c, "Pay the toll", "safe");
		r = Out(o, 1, -1, "They take their share and wave you through.");
		r.anim = "out carry go:npc drop back clear drive";
		r.loseItems = 3;
		o = Opt(c, "Fight through", "dangerous", "kind:Weapon_Base", "kind:Rifle_Base");
		r = Out(o, 3, 6, "A short fight. They scatter.");
		r.anim = "out shoot:npc npcfire shoot:npc back clear drive";
		r = Out(o, 4, 2, "Through, but {who} is hit.");
		r.anim = "out npcfire hit shoot:npc npcflee back clear drive";
		r.hurt = 1;
		r = Out(o, 1, 0.5, "{who} doesn't make it through the fight.");
		r.anim = "out npcfire shoot:npc npcflee";
		r.kill = 1;
		o = Opt(c, "Turn off and go around", "safe");
		r = Out(o, 1, -1, "Back roads: an hour longer, more fuel burnt.");
		r.anim = "clear drive";
		r.minutes = 60;
		r.km = 12;

		c = Card(d, "horde", "Horde on the road", "Infected, dozens of them, shuffling down the road ahead.");
		c.scene = "horde:7";
		o = Opt(c, "Plow through", "risky", "", "");
		o.drive = true;
		r = Out(o, 5, -1, "Bodies on the bonnet, but through.");
		r.anim = "plow";
		r.fuel = 0.05;
		r = Out(o, 2, -1, "The radiator is crushed. The car dies a kilometre on.");
		r.anim = "plow wait:1.5 stop steam:black all";
		r.loseCar = true;
		o = Opt(c, "Sneak around them", "risky", "", "kind:Weapon_Base");
		r = Out(o, 5, 8, "Slow and quiet through the trees. Nobody saw you.");
		r.anim = "aside slow";
		r.minutes = 30;
		r = Out(o, 3, 1, "One of them turns. {who} is bitten.");
		r.anim = "aside out zed hit back slow";
		r.hurt = 1;
		r.minutes = 30;
		o = Opt(c, "Wait for them to pass", "safe");
		r = Out(o, 1, -1, "Three hours in a ditch. They pass.");
		r.anim = "all wait:1 pass wait:1.5 back drive";
		r.minutes = 180;

		c = Card(d, "fork", "Fork in the road", "The signs are rusted blank. Left or right?");
		c.scene = "fork";
		o = Opt(c, "Left", "risky");
		r = Out(o, 1, -1, "The right way.");
		r.anim = "drive";
		r = Out(o, 1, -1, "Wrong way. It takes a while to notice.");
		r.anim = "drive wait:2.5 stop think wait:0.5 drive";
		r.km = 20;
		r.minutes = 45;
		o = Opt(c, "Right", "risky");
		r = Out(o, 1, -1, "The right way.");
		r.anim = "drive";
		r = Out(o, 1, -1, "Wrong way. Back the way you came.");
		r.anim = "drive wait:2.5 stop think wait:0.5 drive";
		r.km = 20;
		r.minutes = 45;
		o = Opt(c, "Follow the compass", "safe", "Compass,GPSReceiver");
		r = Out(o, 1, -1, "{item} points the way. A shortcut, even.");
		r.anim = "out go:sign compass back drive";
		r.minutes = -20;

		c = Card(d, "radiator", "Radiator boiling", "Steam pours from under the bonnet. The needle is in the red.", "drive");
		c.scene = "steam";
		o = Opt(c, "Patch the leak", "safe", "EpoxyPutty,DuctTape");
		r = Out(o, 1, -1, "{who} patches it with {item}. Good as new.");
		r.anim = "out fix back drive";
		r.useItem = true;
		o = Opt(c, "Let it cool, top it up", "safe", "", "Canteen,WaterBottle");
		r = Out(o, 1, 3, "An hour by the road, then on.");
		r.anim = "all wait:2 steam:off back drive";
		r.minutes = 60;
		r = Out(o, 1, 0, "Nothing to top it up with. It cracks. The car stays.");
		r.anim = "all wait:1.5 steam:black";
		r.loseCar = true;
		o = Opt(c, "Push on", "dangerous");
		r = Out(o, 1, -1, "It holds. Somehow.");
		r.anim = "drive";
		r = Out(o, 1, -1, "The engine seizes. The car stays here.");
		r.anim = "drive wait:1.5 stop steam:black all";
		r.loseCar = true;

		c = Card(d, "deer", "Deer in the field", "A deer, grazing a hundred metres off the road.");
		c.scene = "deer";
		c.needs.Insert(rifle);
		o = Opt(c, "Hunt it", "risky", rifle, "Matchbox,PetrolLighter,HandDrillKit");
		r = Out(o, 0, 3, "A clean shot, cooked over a fire. Meat for everyone.");
		r.anim = "out shoot:deer go:deer work:1.5 fire work:2 back drive";
		r.minutes = 45;
		r.food = 1200;
		r = Out(o, 3, 0, "A clean shot, but nothing to make a fire with. Raw meat: {who} gets sick.");
		r.anim = "out shoot:deer go:deer work:2 back drive";
		r.minutes = 45;
		r.food = 800;
		r.sick = 1;
		r = Out(o, 2, 2, "Missed. It's gone.");
		r.anim = "out miss:deer wait:0.5 back drive";
		r.minutes = 20;
		o = Opt(c, "Keep going", "safe");
		r = Out(o, 1, -1, "It watches you go.");
		r.anim = "drive";

		c = Card(d, "lake", "A quiet lake", "A lake by the road, flat and grey. Fish jump now and then.");
		c.scene = "lake";
		o = Opt(c, "Fish", "safe", "FishingRod,Hook,BoneHook", "Matchbox,PetrolLighter,HandDrillKit");
		r = Out(o, 0, 3, "A good catch, cooked on the shore.");
		r.anim = "out go:lake fish:3 fire work:2 back drive";
		r.minutes = 60;
		r.food = 800;
		r.water = 500;
		r = Out(o, 3, 0, "A good catch, eaten raw. {who} gets sick.");
		r.anim = "out go:lake fish:3 work:1.5 ill back drive";
		r.minutes = 60;
		r.food = 600;
		r.sick = 1;
		o = Opt(c, "Fill the bottles", "safe");
		r = Out(o, 1, -1, "Everyone drinks.");
		r.anim = "all go:lake work:2 back drive";
		r.minutes = 15;
		r.water = 1000;
		o = Opt(c, "Keep going", "safe");
		r = Out(o, 1, -1, "On you go.");
		r.anim = "drive";

		c = Card(d, "stranger", "A stranger", "A man by the road, hands up. \"Room for one more? I know these roads.\"");
		c.scene = "stranger";
		o = Opt(c, "Take him along", "risky");
		r = Out(o, 3, -1, "He knows a shortcut.");
		r.anim = "join drive";
		r.minutes = -40;
		r = Out(o, 2, -1, "He eats a lot and says little.");
		r.anim = "join drive";
		r.food = -400;
		r = Out(o, 2, -1, "Gone in the night, and some of your gear with him.");
		r.anim = "join drive";
		r.loseItems = 3;
		o = Opt(c, "Leave him", "safe");
		r = Out(o, 1, -1, "He watches you go.");
		r.anim = "drive";
		o = Opt(c, "Rob him", "dangerous", "kind:Weapon_Base");
		r = Out(o, 3, -1, "He has some food. You take it.");
		r.anim = "out aim:npc go:npc work:1 back drive";
		r.food = 500;
		r = Out(o, 2, -1, "He was armed. {who} is shot.");
		r.anim = "out aim:npc npcfire hit back drive";
		r.hurt = 1;

		c = Card(d, "survivor", "Wounded survivor", "A woman sits against a car, bleeding through her jacket.");
		c.scene = "survivor";
		o = Opt(c, "Patch her up", "safe", medic);
		r = Out(o, 1, -1, "{who} uses {item} on her. She tells you a safe road: you save time.");
		r.anim = "out go:npc work:2.5 back drive";
		r.useItem = true;
		r.minutes = -45;
		o = Opt(c, "Leave her", "safe");
		r = Out(o, 1, -1, "You don't look back.");
		r.anim = "drive";

		c = Card(d, "helicrash", "Smoke on the horizon", "A black column of smoke: a helicopter came down not far off the road.");
		c.scene = "helicrash";
		o = Opt(c, "Investigate", "dangerous", "", "kind:Rifle_Base");
		r = Out(o, 3, 5, "Military rations in the wreck.");
		r.anim = "out go:wreck work:2 carry back drive";
		r.minutes = 60;
		r.food = 1000;
		r = Out(o, 3, 2, "Infected all around it. {who} is bitten getting away.");
		r.anim = "out go:wreck zeds:3 alert zed hit back drive";
		r.minutes = 60;
		r.hurt = 1;
		r = Out(o, 1, 0.3, "{who} doesn't come back from the wreck.");
		r.anim = "out go:wreck zeds:4 alert zed";
		r.minutes = 60;
		r.kill = 1;
		o = Opt(c, "Keep your distance", "safe");
		r = Out(o, 1, -1, "Not worth it.");
		r.anim = "drive";

		c = Card(d, "mud", "Stuck", "The wheels spin. The car is sunk to the axles.", "drive");
		c.scene = "mud";
		o = Opt(c, "Dig it out", "safe", "FieldShovel,Shovel,FarmingHoe");
		r = Out(o, 1, -1, "{who} digs with {item}. Out in half an hour.");
		r.anim = "out go:front work:2.5 unstick back drive";
		r.minutes = 30;
		o = Opt(c, "Tow it with another car", "safe", "Rope");
		r = Out(o, 1, -1, "The rope holds. Out.");
		r.anim = "out go:front work:1.5 unstick back drive";
		r.minutes = 20;
		o.convoy = true;
		o = Opt(c, "Push", "risky");
		r = Out(o, 2, -1, "Everyone pushes. Out after two hours.");
		r.anim = "all push back drive";
		r.minutes = 120;
		r = Out(o, 1, -1, "It won't move. The car stays.");
		r.anim = "all push:fail wait:0.5";
		r.loseCar = true;

		c = Card(d, "sick", "Fever", "{who} is shivering and pale. Food poisoning, maybe worse.");
		c.scene = "sick";
		c.weight = 0.5;
		o = Opt(c, "Rest a day", "safe");
		r = Out(o, 1, -1, "A day lost. The fever breaks.");
		r.anim = "all fire wait:3 cure back drive";
		r.minutes = 360;
		o = Opt(c, "Give medicine", "safe", "CharcoalTablets,TetracyclineAntibiotics,PainkillerTablets");
		r = Out(o, 1, -1, "{item} does it.");
		r.anim = "out go:sick work:1.5 cure back drive";
		r.useItem = true;
		o = Opt(c, "Push on", "dangerous");
		r = Out(o, 3, -1, "They hold on.");
		r.anim = "cure back drive";
		r = Out(o, 2, -1, "{who} gets worse.");
		r.anim = "hit back drive";
		r.hurt = 1;
		r = Out(o, 1, -1, "{who} doesn't wake up.");
		r.anim = "wait:1";
		r.kill = 1;

		c = Card(d, "night", "Night falls", "It's getting dark fast.");
		c.scene = "night";
		o = Opt(c, "Keep going", "risky", "", "Headtorch,Flashlight,Spotlight");
		r = Out(o, 3, 6, "A long night, but on schedule.");
		r.anim = "lights drive wait:3 dawn";
		r = Out(o, 2, 1, "In the dark you miss a turn.");
		r.anim = "lights drive wait:2 stop think wait:0.5 drive wait:1.5 dawn";
		r.km = 15;
		r.minutes = 40;
		r = Out(o, 1, 0.3, "Something in the dark. {who} is hurt.");
		r.anim = "lights drive wait:1.5 stop out zeds:1 zed hit back drive wait:1 dawn";
		r.hurt = 1;
		o = Opt(c, "Make camp", "safe", "", "Matchbox,PetrolLighter,HandDrillKit");
		r = Out(o, 3, 6, "A fire, some sleep. On at first light.");
		r.anim = "all fire wait:3 dawn back drive";
		r.minutes = 300;
		r = Out(o, 1, 0, "No fire. A cold night: {who} is sick in the morning.");
		r.anim = "all wait:3 dawn ill back drive";
		r.minutes = 300;
		r.sick = 1;

		c = Card(d, "blizzard", "Blizzard", "The snow comes sideways. You can't see the road.");
		c.scene = "blizzard";
		c.biomes.Insert("snow");
		o = Opt(c, "Shelter and wait", "safe", "", "Matchbox,PetrolLighter,HandDrillKit,Heatpack");
		r = Out(o, 3, 5, "It blows over by morning.");
		r.anim = "all fire wait:3 calm back drive";
		r.minutes = 240;
		r = Out(o, 1, 0, "No fire: {who} gets frostbite.");
		r.anim = "all wait:3 calm hit back drive";
		r.minutes = 240;
		r.hurt = 1;
		o = Opt(c, "Push on", "dangerous", "", "Compass,GPSReceiver");
		r = Out(o, 2, 5, "Through it.");
		r.anim = "slow wait:3 calm drive";
		r = Out(o, 2, 0, "Lost in the white.");
		r.anim = "slow wait:2 think wait:1 calm drive";
		r.km = 25;
		r.minutes = 90;
		r = Out(o, 1, 0, "{who} is lost in the snow.");
		r.anim = "slow wait:2 calm";
		r.kill = 1;

		c = Card(d, "toxic", "Gas zone", "Yellow haze over the road ahead. The birds are all dead.");
		c.scene = "gas";
		c.biomes.Insert("snow");
		o = Opt(c, "Masks on, straight through", "safe", "GasMask,GP5GasMask,AirborneMask,NBCHoodGray");
		r = Out(o, 1, -1, "Through the haze, breathing hard.");
		r.anim = "all mask back drive";
		o = Opt(c, "Go around", "safe");
		r = Out(o, 1, -1, "A long detour.");
		r.anim = "clear drive";
		r.km = 30;
		r.minutes = 90;
		o = Opt(c, "Hold your breath and run", "dangerous");
		r = Out(o, 1, -1, "Coughing, but through.");
		r.anim = "drive wait:2 stop out hit back drive";
		r.hurt = 1;
		r = Out(o, 1, -1, "{who} breathed too much of it.");
		r.anim = "drive wait:2 stop";
		r.kill = 1;
		return d;
	}
}

// ---------------- the party as the trail sees it ----------------

class DL_TrailMember
{
	string sid;
	string name;
	int car = -1;                 // DL_TrailCar.idx, -1 = on foot
	bool dead;
	bool gone;                    // turned back
	float health = 100;
	int sick;
	ref array<string> items = new array<string>;   // what he carries (types)
	ref array<string> used = new array<string>;    // used up on the trail (types, once each)
	int lost;                     // items lost (toll, thief, river): the server takes this many of his own
	float food;                   // kcal + found / - lost on the trail
	float water;
	bool fake;                    // dev test: a made-up voter
}

class DL_TrailCar
{
	int idx;
	string name;                  // "Derk's Olga 24"
	bool lost;
	ref array<string> items = new array<string>;   // its trunk
	ref array<string> used = new array<string>;    // used up out of the trunk
	float fuelLost;               // share of the tank lost on the trail
}

class DL_TrailState
{
	ref array<ref DL_TrailMember> members = new array<ref DL_TrailMember>;
	ref array<ref DL_TrailCar> cars = new array<ref DL_TrailCar>;
	string biome;
	float km;                     // the trip as planned
	float extraKm;                // detours, wrong turns
	float minutes;                // time added (-: saved)
	bool instant;                 // the hub sends them there at once: risk instead of time
	ref array<string> story = new array<string>;

	array<DL_TrailMember> Alive()
	{
		array<DL_TrailMember> a = new array<DL_TrailMember>;
		foreach (DL_TrailMember m : members)
		{
			if (!m.dead && !m.gone)
				a.Insert(m);
		}
		return a;
	}

	DL_TrailMember Member(string sid)
	{
		foreach (DL_TrailMember m : members)
		{
			if (m.sid == sid)
				return m;
		}
		return null;
	}

	bool Driving()
	{
		array<DL_TrailMember> alive1 = Alive();
		foreach (DL_TrailMember m : alive1)
		{
			if (m.car >= 0 && !cars[m.car].lost)
				return true;
		}
		return false;
	}

	bool Walking()
	{
		array<DL_TrailMember> alive2 = Alive();
		foreach (DL_TrailMember m : alive2)
		{
			if (m.car < 0)
				return true;
		}
		return false;
	}

	int CarsLeft()
	{
		int n = 0;
		foreach (DL_TrailCar c : cars)
		{
			if (!c.lost)
				n++;
		}
		return n;
	}

	// does an item type match a token? "kind:Rifle_Base" = a config class of that kind, else the type contains it
	static bool Matches(string type, string token)
	{
		if (token.IndexOf("kind:") == 0)
			return g_Game.IsKindOf(type, token.Substring(5, token.Length() - 5));
		string t = type;
		string k = token;
		t.ToLower();
		k.ToLower();
		return t.Contains(k);
	}

	// who carries one of tokens (alive, not used up yet): the member (null = in a trunk or nobody) and the item type
	bool Carried(array<string> tokens, out DL_TrailMember who, out string type, out int car)
	{
		who = null;
		type = "";
		car = -1;
		if (!tokens || tokens.Count() == 0)
			return false;
		foreach (string k : tokens)
		{
			array<DL_TrailMember> alive3 = Alive();
			foreach (DL_TrailMember m : alive3)
			{
				foreach (string it : m.items)
				{
					if (Matches(it, k) && m.used.Find(it) < 0)
					{
						who = m;
						type = it;
						return true;
					}
				}
			}
			foreach (DL_TrailCar c : cars)
			{
				if (c.lost)
					continue;
				foreach (string ct : c.items)
				{
					if (Matches(ct, k))
					{
						car = c.idx;
						type = ct;
						return true;
					}
				}
			}
		}
		return false;
	}

	// an item used up: off the carrier's list (or out of the trunk)
	void Use(DL_TrailMember who, string type, int car)
	{
		if (who)
		{
			who.used.Insert(type);
			int i = who.items.Find(type);
			if (i >= 0)
				who.items.Remove(i);
			return;
		}
		if (car >= 0 && car < cars.Count())
		{
			int ci = cars[car].items.Find(type);
			if (ci >= 0)
				cars[car].items.Remove(ci);
			cars[car].used.Insert(type);
		}
	}

	DL_TrailMember AnyAlive()
	{
		array<DL_TrailMember> a = Alive();
		if (a.Count() == 0)
			return null;
		return a[Math.RandomInt(0, a.Count())];
	}
}

// ---------------- what clients see ----------------

class DL_TrailChoice
{
	string label;
	string hint;
	string help;                  // "Epoxy putty (Derk)"
}

class DL_TrailVoteCard
{
	int n;                        // the card's number on this trip (1, 2 ...)
	int of;                       // how many there will be
	string title;
	string text;
	float seconds;
	ref array<ref DL_TrailChoice> choices = new array<ref DL_TrailChoice>;
	ref array<string> names = new array<string>;   // who votes (the living party), same order as picks
	ref array<string> sids = new array<string>;
	ref array<bool> alone = new array<bool>;       // may he turn back alone (on foot, or at the wheel)
	bool watch;                   // the server's choices make themselves (filmMode "watch"): nobody votes
	bool ask;                     // one player's own question (his driver turns back): his pick is the answer
}

// the votes so far: picks[i] = names[i]'s choice (-1 none, -2 turn back), locked[i]
class DL_TrailTally
{
	ref array<int> picks = new array<int>;
	ref array<bool> locked = new array<bool>;
	float left;
	int result = -9;              // -9 = still voting, else the winning choice (-2 = turn back)
	string outcome;               // the caption of what happened
}

class DL_TrailSummary
{
	string title;
	ref array<string> lines = new array<string>;
}

// more road for the film: a chapter, or what a choice did (last = the film may end after it)
class DL_TrailRoad
{
	ref array<ref DL_FilmEvent> events = new array<ref DL_FilmEvent>;
	float km;
	bool last;
}

// the zone's ready-up table: the supplies (DL_ShortInfo, as the "not enough" dialog has them) and who is ready
class DL_ReadyInfo
{
	ref DL_ShortInfo table;
	ref array<string> names = new array<string>;
	ref array<string> sids = new array<string>;
	ref array<bool> ready = new array<bool>;
	int seconds;                  // left to ready up
	bool short;                   // the party as a whole hasn't enough
	bool canGo = true;            // short and the server doesn't let them go anyway: nobody can ready up
	string to;                    // where to
	string why;                   // what is missing
}

// ---------------- the rules ----------------

class DL_TrailGame
{
	static const int TURN_BACK = -2;
	static const int NONE = -1;
	ref DL_TrailState s;
	ref DL_TrailDeck deck;
	ref array<string> usedCards = new array<string>;
	DL_TrailCard card;
	string cardWho;               // the member a card is about ("Fever")
	ref array<int> optionOf = new array<int>;      // shown choice -> card option
	float instantRiskPerHour = 15; // instant arrival: chance (%) someone dies per hour the outcome would have added
	string force;                 // dev tests: this card next (when it fits)
	int forceOutcome = -1;        // dev tests: this outcome of the chosen option

	void DL_TrailGame(DL_TrailState state, DL_TrailDeck d)
	{
		s = state;
		deck = d;
	}

	// how many cards a trip of this length gets
	static int Choices(float km, int maxChoices)
	{
		if (maxChoices < 1)
			maxChoices = 5;
		return Math.Clamp(Math.Floor(km / 20) + 1, 1, maxChoices);
	}

	protected bool Fits(DL_TrailCard c)
	{
		if (c.off || c.options.Count() == 0 || usedCards.Find(c.id) >= 0)
			return false;
		if (c.mode == "drive" && !s.Driving())
			return false;
		if (c.mode == "walk" && !s.Walking())
			return false;
		if (s.Alive().Count() < c.minMembers || s.CarsLeft() < c.minCars)
			return false;
		if (c.biomes.Count() > 0 && c.biomes.Find(s.biome) < 0)
			return false;
		DL_TrailMember w;
		string t;
		int car;
		if (c.needs.Count() > 0 && !s.Carried(c.needs, w, t, car))
			return false;
		// a real choice: two options at least besides turning back
		int offered = 0;
		foreach (DL_TrailOption o : c.options)
		{
			if (Offered(o))
				offered++;
		}
		return offered > 1;
	}

	protected bool Offered(DL_TrailOption o)
	{
		if (o.drive && !s.Driving())
			return false;
		if (o.walk && !s.Walking())
			return false;
		if (o.convoy && s.CarsLeft() < 2)
			return false;
		DL_TrailMember w;
		string t;
		int car;
		if (o.needs.Count() > 0 && !s.Carried(o.needs, w, t, car))
			return false;
		return true;
	}

	// the next card (null = none fits)
	DL_TrailCard Draw()
	{
		array<DL_TrailCard> fit = new array<DL_TrailCard>;
		float total = 0;
		foreach (DL_TrailCard c : deck.cards)
		{
			if (Fits(c))
			{
				fit.Insert(c);
				total += Math.Max(c.weight, 0.01);
			}
		}
		card = null;
		if (fit.Count() == 0)
			return null;
		foreach (DL_TrailCard fc : fit)
		{
			if (fc.id == force)
				card = fc;
		}
		if (card)
		{
			usedCards.Insert(card.id);
			DL_TrailMember fa = s.AnyAlive();
			cardWho = "";
			if (fa)
				cardWho = fa.name;
			return card;
		}
		float r = Math.RandomFloat01() * total;
		foreach (DL_TrailCard f : fit)
		{
			r -= Math.Max(f.weight, 0.01);
			if (r <= 0)
			{
				card = f;
				break;
			}
		}
		if (!card)
			card = fit[fit.Count() - 1];
		usedCards.Insert(card.id);
		DL_TrailMember about = s.AnyAlive();
		cardWho = "";
		if (about)
			cardWho = about.name;
		return card;
	}

	// the name players know; a "$STR_..." name needs the client's language table, so the type with spaces then
	// (EpoxyPutty -> Epoxy Putty)
	static string ItemName(string type)
	{
		string n = "";
		if (g_Game.ConfigIsExisting("CfgVehicles " + type + " displayName"))
			g_Game.ConfigGetText("CfgVehicles " + type + " displayName", n);
		else if (g_Game.ConfigIsExisting("CfgWeapons " + type + " displayName"))
			g_Game.ConfigGetText("CfgWeapons " + type + " displayName", n);
		if (n != "" && n.IndexOf("#") != 0 && n.IndexOf("$") != 0)
			return n;
		string out2 = "";
		for (int i = 0; i < type.Length(); i++)
		{
			string ch = type.Get(i);
			string up = ch;
			up.ToUpper();
			string low = ch;
			low.ToLower();
			if (i > 0 && ch == up && ch != low)
				out2 += " ";
			out2 += ch;
		}
		return out2;
	}

	protected string Fill(string text, string who, string item, string car)
	{
		string t = text;
		t.Replace("{who}", who);
		t.Replace("{item}", item);
		t.Replace("{car}", car);
		return t;
	}

	// the film's stage for the card that is up (null = the card has none)
	DL_FilmEvent SceneEvent(float at)
	{
		if (!card || card.scene == "")
			return null;
		return Ev("card", at, cardWho, card.scene);
	}

	// the card as clients see it
	DL_TrailVoteCard Show(int n, int of, float seconds)
	{
		DL_TrailVoteCard v = new DL_TrailVoteCard();
		v.n = n;
		v.of = of;
		v.title = card.title;
		v.text = Fill(card.text, cardWho, "", "");
		v.seconds = seconds;
		optionOf.Clear();
		for (int i = 0; i < card.options.Count(); i++)
		{
			DL_TrailOption o = card.options[i];
			if (!Offered(o))
				continue;
			DL_TrailChoice ch = new DL_TrailChoice();
			ch.label = o.label;
			ch.hint = o.hint;
			DL_TrailMember w;
			string t;
			int car;
			array<string> shown = o.needs;
			if (shown.Count() == 0)
				shown = o.helps;
			if (s.Carried(shown, w, t, car))
			{
				string by = "a trunk";
				if (w)
					by = w.name;
				ch.help = ItemName(t) + " (" + by + ")";
			}
			v.choices.Insert(ch);
			optionOf.Insert(i);
		}
		array<DL_TrailMember> alive4 = s.Alive();
		foreach (DL_TrailMember m : alive4)
		{
			v.names.Insert(m.name);
			v.sids.Insert(m.sid);
		}
		return v;
	}

	// the winner: most votes; a tie goes to the lead's pick (lead = the first who can vote), no votes = the safest
	// option; turning back as a group needs more than half of them
	int Winner(DL_TrailVoteCard v, DL_TrailTally t)
	{
		int choices = v.choices.Count();
		array<int> count = new array<int>;
		for (int i = 0; i < choices; i++)
			count.Insert(0);
		int back = 0;
		int voters = v.names.Count();
		for (int k = 0; k < t.picks.Count(); k++)
		{
			int p = t.picks[k];
			if (p == TURN_BACK)
				back++;
			else if (p >= 0 && p < choices)
				count[p] = count[p] + 1;
		}
		if (voters > 0 && back * 2 > voters)
			return TURN_BACK;
		int best = -1;
		int bestN = 0;
		for (int c = 0; c < choices; c++)
		{
			if (count[c] > bestN)
			{
				best = c;
				bestN = count[c];
			}
		}
		if (best < 0)
		{
			// nobody voted (or only to turn back, short of a majority): the safest
			for (int s2 = 0; s2 < choices; s2++)
			{
				if (v.choices[s2].hint == "safe")
					return s2;
			}
			return 0;
		}
		// a tie: the lead's pick when it is one of the tied
		int ties = 0;
		for (int ti = 0; ti < choices; ti++)
		{
			if (count[ti] == bestN)
				ties++;
		}
		if (ties > 1 && t.picks.Count() > 0)
		{
			int lead = t.picks[0];
			if (lead >= 0 && lead < choices && count[lead] == bestN)
				return lead;
		}
		return best;
	}

	// the shown choice was taken: roll the outcome, change the party. Film events for the outcome go into evs.
	string Resolve(int shown, float at, array<ref DL_FilmEvent> evs)
	{
		if (shown < 0 || shown >= optionOf.Count())
			return "";
		DL_TrailOption o = card.options[optionOf[shown]];
		DL_TrailMember helper;
		string helpType;
		int helpCar;
		bool helped = s.Carried(o.helps, helper, helpType, helpCar);
		DL_TrailMember needer;
		string needType;
		int needCar;
		bool needed = s.Carried(o.needs, needer, needType, needCar);
		float total = 0;
		foreach (DL_TrailOutcome w : o.outcomes)
			total += OutWeight(w, helped);
		DL_TrailOutcome r = null;
		float roll = Math.RandomFloat01() * total;
		foreach (DL_TrailOutcome ow : o.outcomes)
		{
			roll -= OutWeight(ow, helped);
			if (roll <= 0 && OutWeight(ow, helped) > 0)
			{
				r = ow;
				break;
			}
		}
		if (!r && o.outcomes.Count() > 0)
			r = o.outcomes[o.outcomes.Count() - 1];
		if (forceOutcome >= 0 && forceOutcome < o.outcomes.Count())
			r = o.outcomes[forceOutcome];
		if (!r)
			return "";
		// who it is about: the card's person, or whoever carried the item
		string who = cardWho;
		string itemName = "";
		DL_TrailMember itemBy = helper;
		string itemType = helpType;
		int itemCar = helpCar;
		if (needed)
		{
			itemBy = needer;
			itemType = needType;
			itemCar = needCar;
		}
		if (itemType != "")
			itemName = ItemName(itemType);
		if (itemBy && (o.needs.Count() > 0 || r.useItem))
			who = itemBy.name;
		DL_TrailCar car = null;
		foreach (DL_TrailCar ac : s.cars)
		{
			if (!ac.lost)
			{
				car = ac;
				break;
			}
		}
		string carName = "the car";
		if (car)
			carName = car.name;
		string text = r.text;
		array<string> extra = new array<string>;
		// the effects
		if (r.useItem && itemType != "")
			s.Use(itemBy, itemType, itemCar);
		if (r.minutes > 0 && s.instant)
		{
			// no travel time: what would have cost time is a risk instead
			if (DL_TrailRoll(r.minutes / 60 * instantRiskPerHour))
			{
				DL_TrailMember late = s.AnyAlive();
				if (late)
					extra.Insert(Kill(late, at, evs));
			}
		}
		else
			s.minutes += r.minutes;
		s.extraKm += r.km;
		if (r.fuel > 0)
		{
			foreach (DL_TrailCar fc : s.cars)
				fc.fuelLost = Math.Min(1, fc.fuelLost + r.fuel);
		}
		array<DL_TrailMember> alive5 = s.Alive();
		foreach (DL_TrailMember fm : alive5)
		{
			fm.food += r.food;
			fm.water += r.water;
		}
		// whoever it hits: the card's person first
		array<DL_TrailMember> pool = new array<DL_TrailMember>;
		DL_TrailMember first = null;
		array<DL_TrailMember> alive6 = s.Alive();
		foreach (DL_TrailMember am : alive6)
		{
			if (am.name == who)
				first = am;
			else
				pool.Insert(am);
		}
		if (first)
			pool.InsertAt(first, 0);
		else if (pool.Count() > 0)
			who = pool[0].name;
		int pi = 0;
		for (int h = 0; h < r.hurt && pi < pool.Count(); h++)
		{
			DL_TrailMember hm = pool[pi];
			pi++;
			hm.health = Math.Max(1, hm.health - 30);
			if (r.anim == "")
				evs.Insert(Ev("attack", at, hm.name, "hurt"));    // the animation shows the wound itself
		}
		for (int sk = 0; sk < r.sick && sk < pool.Count(); sk++)
			pool[sk].sick++;
		for (int k = 0; k < r.kill && k < pool.Count(); k++)
		{
			DL_TrailMember km = pool[k];
			if (km.dead)
				continue;
			// a bandage or rags: a deadly wound becomes a bad one
			DL_TrailMember medic;
			string medType;
			int medCar;
			array<string> meds = {"BandageDressing", "Rag", "FirstAidKit"};
			if (s.Carried(meds, medic, medType, medCar))
			{
				s.Use(medic, medType, medCar);
				km.health = Math.Max(1, km.health - 50);
				string mby = "";
				if (medic)
					mby = medic.name + "'s ";
				extra.Insert(km.name + " was badly hurt, but " + mby + ItemName(medType) + " kept them alive.");
				evs.Insert(Ev("attack", at, km.name, "hurt"));
				continue;
			}
			extra.Insert(Kill(km, at, evs));
		}
		for (int li = 0; li < r.loseItems; li++)
		{
			DL_TrailMember lm = s.AnyAlive();
			if (lm)
				lm.lost++;
		}
		if (r.loseCar && car)
			LoseCar(car, at, evs);
		text = Fill(text, who, itemName, carName);
		evs.InsertAt(Ev("note", at, who, text), 0);
		if (r.anim != "" && card.scene != "")
			evs.InsertAt(Ev("act", at, who, r.anim), 0);   // played out first, under the note's line
		string all = text;
		foreach (string x : extra)
		{
			if (x == "")
				continue;
			all += " " + x;
			evs.Insert(Ev("note", at, "", x));
		}
		s.story.Insert(card.title + ": " + o.label + ". " + all);
		return all;
	}

	protected float OutWeight(DL_TrailOutcome w, bool helped)
	{
		if (helped && w.withHelp >= 0)
			return w.withHelp;
		return w.weight;
	}

	protected string Kill(DL_TrailMember m, float at, array<ref DL_FilmEvent> evs)
	{
		m.dead = true;
		evs.Insert(Ev("died", at, m.name, "trail"));
		return "";
	}

	// a car stays behind: its crew climbs into free seats of the others (4 a car) or walks
	void LoseCar(DL_TrailCar car, float at, array<ref DL_FilmEvent> evs)
	{
		car.lost = true;
		array<DL_TrailMember> alive7 = s.Alive();
		foreach (DL_TrailMember m : alive7)
		{
			if (m.car != car.idx)
				continue;
			int to = -1;
			foreach (DL_TrailCar o : s.cars)
			{
				if (o.lost)
					continue;
				int seated = 0;
				array<DL_TrailMember> alive8 = s.Alive();
				foreach (DL_TrailMember sm : alive8)
				{
					if (sm.car == o.idx)
						seated++;
				}
				if (seated < 4)
				{
					to = o.idx;
					break;
				}
			}
			m.car = to;
			if (s.cars.Count() > 1)
			{
				string how = "";
				if (to < 0)
					how = "walk";
				evs.Insert(Ev("move", at, m.name, how, to));
			}
		}
	}

	static DL_FilmEvent Ev(string kind, float km, string who, string detail, int car = -1)
	{
		DL_FilmEvent e = new DL_FilmEvent();
		e.kind = kind;
		e.km = km;
		e.who = who;
		e.detail = detail;
		e.car = car;
		return e;
	}

	// the end of the trail: one line each for the summary card
	DL_TrailSummary Summary(bool turnedBack, int travelSeconds)
	{
		DL_TrailSummary sum = new DL_TrailSummary();
		if (turnedBack)
			sum.title = "You turned back";
		else if (s.Alive().Count() == 0)
			sum.title = "Nobody made it";
		else
			sum.title = "You made it";
		if (!turnedBack && !s.instant && travelSeconds > 0)
			sum.lines.Insert("Time on the road: " + DL_TrailTime(travelSeconds));
		if (s.minutes > 1)
			sum.lines.Insert("Delays: +" + DL_TrailTime(s.minutes * 60));
		else if (s.minutes < -1)
			sum.lines.Insert("Shortcuts: -" + DL_TrailTime(-s.minutes * 60));
		if (s.extraKm > 0.5)
			sum.lines.Insert("Detours: " + Math.Round(s.extraKm).ToString() + " km");
		foreach (DL_TrailMember m : s.members)
		{
			if (m.dead)
				sum.lines.Insert(m.name + " died on the road.");
			else if (m.gone)
				sum.lines.Insert(m.name + " turned back alone.");
			else if (m.health < 50)
				sum.lines.Insert(m.name + " is badly hurt.");
			else if (m.health < 100)
				sum.lines.Insert(m.name + " is hurt.");
			if (m.sick > 0 && !m.dead)
				sum.lines.Insert(m.name + " is sick.");
		}
		foreach (DL_TrailCar c : s.cars)
		{
			if (c.lost)
				sum.lines.Insert("Lost: " + c.name + ".");
		}
		return sum;
	}
}

bool DL_TrailRoll(float pct)
{
	return pct > 0 && Math.RandomFloat01() * 100.0 < pct;
}

string DL_TrailTime(float seconds)
{
	int m = Math.Round(seconds / 60);
	if (m < 60)
		return m.ToString() + " min";
	int h = m / 60;
	int r = m % 60;
	if (r == 0)
		return h.ToString() + " h";
	return h.ToString() + " h " + r.ToString() + " min";
}

// ---------------- the road in chapters ----------------

class DL_TrailChapters
{
	// the film's road cut at even km into choices + 1 chapters (a leg across a cut is split in two). Each chapter
	// starts with a leg, so there is road between two cards.
	static array<ref array<ref DL_FilmEvent>> Split(DL_Film f, int choices)
	{
		array<ref array<ref DL_FilmEvent>> parts = new array<ref array<ref DL_FilmEvent>>;
		for (int c = 0; c <= choices; c++)
			parts.Insert(new array<ref DL_FilmEvent>);
		float total = 0;
		foreach (DL_FilmEvent te : f.events)
		{
			if (IsLeg(te))
				total += te.km;
		}
		if (total <= 0)
			total = Math.Max(f.km, 1);
		float step = total / (choices + 1);
		float done = 0;
		int ch = 0;
		string lastLeg = "drive";
		if (f.cars.Count() == 0 && f.car == "")
			lastLeg = "walk";
		foreach (DL_FilmEvent e : f.events)
		{
			if (!IsLeg(e))
			{
				parts[ch].Insert(e);
				continue;
			}
			lastLeg = e.kind;
			float left = e.km;
			while (left > 0.001)
			{
				float cut = step * (ch + 1);
				if (ch >= choices)
					cut = total + 1;
				float take = Math.Min(left, cut - done);
				if (take > 0.001)
				{
					DL_FilmEvent part = Copy(e);
					part.km = take;
					parts[ch].Insert(part);
					done += take;
					left -= take;
				}
				if (done >= cut - 0.001 && ch < choices)
					ch++;
				else if (take <= 0.001)
					break;
			}
		}
		// a chapter with no road (a zero-km trip): a short leg
		foreach (array<ref DL_FilmEvent> chap : parts)
		{
			bool leg = false;
			foreach (DL_FilmEvent ce : chap)
			{
				if (IsLeg(ce))
					leg = true;
			}
			if (!leg)
			{
				DL_FilmEvent pad = new DL_FilmEvent();
				pad.kind = lastLeg;
				pad.km = 1;
				chap.InsertAt(pad, 0);
			}
		}
		return parts;
	}

	// a chapter that comes after choices changed the party: the dead and the ones who turned back drop out of it,
	// lost cars aren't driven, a detour makes the first leg longer
	static void Fix(array<ref DL_FilmEvent> chap, DL_TrailState s, float addKm)
	{
		for (int i = chap.Count() - 1; i >= 0; i--)
		{
			DL_FilmEvent e = chap[i];
			if (e.who != "")
			{
				bool here = false;
				array<DL_TrailMember> alive9 = s.Alive();
				foreach (DL_TrailMember m : alive9)
				{
					if (m.name == e.who)
						here = true;
				}
				if (!here)
				{
					if (e.kind == "died" || e.kind == "move" || e.kind == "attack")
					{
						chap.Remove(i);
						continue;
					}
					DL_TrailMember other = s.AnyAlive();
					if (other)
						e.who = other.name;
				}
			}
			if (e.car >= 0 && e.car < s.cars.Count() && s.cars[e.car].lost)
				e.car = -1;
			if (e.donor >= 0 && e.donor < s.cars.Count() && s.cars[e.donor].lost)
				e.donor = -1;
			if (e.kind == "drive" && !s.Driving())
				e.kind = "walk";
			if (e.kind == "drive" && s.Walking())
				e.kind = "walk";                       // a convoy keeps to walking pace
			if ((e.kind == "flat" || e.kind == "fuel" || (e.kind == "find" && e.detail == "gas")) && !s.Driving())
				chap.Remove(i);
		}
		if (addKm > 0)
		{
			foreach (DL_FilmEvent l : chap)
			{
				if (IsLeg(l))
				{
					l.km = l.km + addKm;
					break;
				}
			}
		}
	}

	static float Km(array<ref DL_FilmEvent> chap)
	{
		float k = 0;
		foreach (DL_FilmEvent e : chap)
		{
			if (IsLeg(e))
				k += e.km;
		}
		return k;
	}

	static float Seconds(array<ref DL_FilmEvent> chap)
	{
		float t = 0;
		foreach (DL_FilmEvent e : chap)
			t += DL_Film.SceneSeconds(e);
		return t;
	}

	static bool IsLeg(DL_FilmEvent e)
	{
		return e.kind == "drive" || e.kind == "walk" || e.kind == "swim";
	}

	static DL_FilmEvent Copy(DL_FilmEvent e)
	{
		DL_FilmEvent c = new DL_FilmEvent();
		c.kind = e.kind;
		c.km = e.km;
		c.who = e.who;
		c.detail = e.detail;
		c.car = e.car;
		c.donor = e.donor;
		c.times = e.times;
		return c;
	}
}
