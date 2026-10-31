// DriveLink travel step 4 (Research/travel-cost-design.md section 4 + 4b): "arrive weakened" mode. The source server
// plays the road ONCE for the whole party before anyone leaves, in legs of eventKm (hub Travel settings, 10 km by default): fuel (luck when it runs dry, else the
// car is left), flat tyres (spare wheel / repair kit, else the car is left), infected on the walked legs, food and
// water (what nothing could pay = hunger/thirst damage, past half of the cost a chance to die). Everything happens to
// the SNAPSHOTS (DL_PlayerData / DL_CarData), the live characters are untouched. The result travels in each
// snapshot (journey = the story, diedOnRoad) and the target shows the story on the black screen.
// A party (Research/group-travel.md) is one crew in one car, a group on foot, or a convoy of up to 4 cars: they stay
// together the whole way. A convoy shares: jerry cans, spare wheels that fit, repair kits, the food in every trunk;
// the crew of a car that is left behind climbs into free seats of the others, whoever doesn't fit walks - and then
// the whole convoy goes at walking pace.

class DL_Traveller
{
	string sid;
	string name;
	ref DL_PlayerData d;
	int seat;                 // car seat, -1 = on foot
	int veh = -1;             // DL_JVehicle.idx he rides in, -1 = on foot
	bool dead;
	float needFood;           // stat units for the whole trip, for the starvation check
	float needWater;
	float unpaidFood;
	float unpaidWater;
}

// one car of the party
class DL_JVehicle
{
	int idx;                  // its place in the line at the start (= the film's car index)
	Transport live;
	ref DL_CarData car;       // the snapshot the road works on
	string holder;            // steam id of who drives it (his snapshot carries it to the target)
	int seats;
	bool lost;
	bool luckyFuel;           // found fuel: it takes them the rest of the way
}

// what a party is made of, for Run: a car with its crew (crew[0] = the driver), or one player on foot
class DL_PartyUnit
{
	PlayerBase leader;
	Transport car;
	ref array<PlayerBase> crew = new array<PlayerBase>;
}

class DL_Journey
{
	ref array<ref DL_Traveller> crew = new array<ref DL_Traveller>;
	ref array<ref DL_JVehicle> vehicles = new array<ref DL_JVehicle>;
	bool carLost;             // every car of the party was left on the road
	ref array<string> story = new array<string>;
	float drivenKm;
	float walkedKm;
	float swumKm;             // a boat party with no boat (seat) left swims; the walk legs of the water
	bool water;               // a party of boats (parties are by kind: boats only travel with boats)
	bool air;                 // a party of planes: no flats, nothing to find up there, no fuel but what they carry
	// swimming: the chance to drown on a leg grows with every leg already swum (drownPct, x2, x3 ...);
	// beside a boat of the party (holding on) half that, hurt (health under 50) half as much again
	static const float SWIM_SPEED_X = 0.5;    // at half the walking speed
	int seconds;              // on the road (travel time on), else 0
	ref DL_Film film = new DL_Film(); // the same road as scenes, for the arrival film

	string Story()
	{
		string s = "";
		foreach (string line : story)
		{
			if (s != "")
				s += "\n";
			s += line;
		}
		return s;
	}

	int Alive()
	{
		int n = 0;
		foreach (DL_Traveller t : crew)
		{
			if (!t.dead)
				n++;
		}
		return n;
	}

	DL_Traveller Find(string sid)
	{
		foreach (DL_Traveller t : crew)
		{
			if (t.sid == sid)
				return t;
		}
		return null;
	}

	// the chance (percent) that swimmer tv drowns on this leg (g = its share of a full event leg)
	protected float DrownPct(DL_TravelRules t, DL_Traveller tv, float g)
	{
		float pct = t.drownPct * g * (1 + swumKm / t.EventKm());
		if (!carLost)
			pct *= 0.5;                       // a boat of the party is there to hold on to
		if (tv.d && tv.d.health < 50)
			pct *= 1.5;
		return Math.Min(pct, 90);
	}

	// health under 50 (of 100): death chances from bites and hunger x1.5, like drowning (the hub's odds use the same)
	static float HurtX(DL_PlayerData d)
	{
		if (d && d.health < 50)
			return 1.5;
		return 1;
	}

	static bool Roll(float pct)
	{
		return pct > 0 && Math.RandomFloat01() * 100.0 < pct;
	}

	protected DL_Traveller AnyAlive()
	{
		array<DL_Traveller> alive = new array<DL_Traveller>;
		foreach (DL_Traveller t : crew)
		{
			if (!t.dead)
				alive.Insert(t);
		}
		if (alive.Count() == 0)
			return null;
		return alive[Math.RandomInt(0, alive.Count())];
	}

	// the living travellers in car v (v = -1: on foot)
	protected array<DL_Traveller> In(int v)
	{
		array<DL_Traveller> a = new array<DL_Traveller>;
		foreach (DL_Traveller t : crew)
		{
			if (!t.dead && t.veh == v)
				a.Insert(t);
		}
		return a;
	}

	protected DL_Traveller AnyIn(int v)
	{
		array<DL_Traveller> a = In(v);
		if (a.Count() == 0)
			return AnyAlive();
		return a[Math.RandomInt(0, a.Count())];
	}

	protected string Who(DL_Traveller t)
	{
		if (crew.Count() == 1)
			return "You";
		return t.name;
	}

	protected bool Convoy()
	{
		return vehicles.Count() > 1;
	}

	protected DL_JVehicle Veh(int idx)
	{
		if (idx < 0 || idx >= vehicles.Count())
			return null;
		return vehicles[idx];
	}

	// "the Olga 24", or in a convoy "Derk's Olga 24"
	protected string CarOf(DL_JVehicle v)
	{
		if (!Convoy())
			return "the " + CarName(v.car);
		DL_Traveller h = Find(v.holder);
		if (h)
			return h.name + "'s " + CarName(v.car);
		return "the " + CarName(v.car);
	}

	// ---------------- the road ----------------

	// one crew: crewPbs[0] = the driver when liveCar != null. weakened = road events on (refuse mode: costs only).
	static DL_Journey Run(DL_TravelRules t, DL_Zone z, array<PlayerBase> crewPbs, Transport liveCar, DL_Snapshot snap, DL_CarSnapshot carSnap, bool weakened)
	{
		array<ref DL_PartyUnit> units = new array<ref DL_PartyUnit>;
		DL_PartyUnit u = new DL_PartyUnit();
		if (crewPbs.Count() > 0)
			u.leader = crewPbs[0];
		u.car = liveCar;
		foreach (PlayerBase pb : crewPbs)
			u.crew.Insert(pb);
		units.Insert(u);
		return RunParty(t, z, units, snap, carSnap, weakened);
	}

	// a whole party: cars (each with its crew, the driver first) and players on foot, front of the line first
	static DL_Journey RunParty(DL_TravelRules t, DL_Zone z, array<ref DL_PartyUnit> units, DL_Snapshot snap, DL_CarSnapshot carSnap, bool weakened)
	{
		DL_Journey j = new DL_Journey();
		foreach (DL_PartyUnit u : units)
		{
			DL_JVehicle v = null;
			if (u.car)
			{
				v = new DL_JVehicle();
				v.idx = j.vehicles.Count();
				v.live = u.car;
				v.car = carSnap.Capture(u.car);
				v.seats = u.car.CrewSize();
				j.vehicles.Insert(v);
				j.film.cars.Insert(DL_FilmCast.CarSprite(u.car.GetType()));
			}
			for (int i = 0; i < u.crew.Count(); i++)
			{
				PlayerBase pb = u.crew[i];
				if (!pb || !pb.GetIdentity())
					continue;
				DL_Traveller tr = new DL_Traveller();
				tr.sid = pb.GetIdentity().GetPlainId();
				tr.name = pb.GetIdentity().GetName();
				tr.d = snap.Capture(pb);
				tr.seat = -1;
				if (v)
				{
					tr.veh = v.idx;
					tr.seat = u.car.CrewMemberIndex(pb);
					if (v.holder == "")
						v.holder = tr.sid;
				}
				j.crew.Insert(tr);
			}
		}
		if (j.vehicles.Count() == 1)
			j.film.car = j.film.cars[0];
		foreach (DL_JVehicle bv : j.vehicles)
		{
			if (DL_Veh.IsBoat(bv.live))
				j.water = true;
			if (DL_Veh.IsPlane(bv.live))
				j.air = true;
		}
		j.film.km = z.distanceKm;
		foreach (DL_Traveller ft : j.crew)
		{
			DL_FilmActor fa = DL_FilmCast.From(ft.name, ft.sid, ft.d);
			fa.car = ft.veh;
			j.film.crew.Insert(fa);
		}
		float km = z.distanceKm;
		float done = 0;
		float evKm = t.EventKm();
		if (km / evKm > 2000)
			evKm = km / 2000;                // a very long trip in 1 km legs: at most 2000 legs
		while (done < km - 0.001)
		{
			float leg = Math.Min(evKm, km - done);
			float f = leg / 10.0;            // costs and wear are per 10 km
			float g = leg / evKm;            // chances are per event leg
			float at = done + leg;
			// the road gives: a gas station, food, water (weakened mode; not when everyone is dead)
			if (weakened && j.Alive() > 0 && !(j.air && j.Driving() > 0 && j.In(-1).Count() == 0))
				j.Finds(t, g, done);         // not while flying
			// each car: a driver, fuel, wear, a flat
			foreach (DL_JVehicle cv : j.vehicles)
			{
				if (cv.lost)
					continue;
				if (!j.TakeWheel(cv, done))
					continue;
				j.DriveLeg(t, cv, f, g, done, at, weakened);
			}
			bool walking = j.In(-1).Count() > 0;
			// food + water for this leg: in a seat at car rates, walking at foot rates; own gear first, then the trunks.
			// Closed cans: opened with the best tool anyone (alive) or any trunk still has
			array<ref DL_PlayerData> living = new array<ref DL_PlayerData>;
			foreach (DL_Traveller lt : j.crew)
			{
				if (!lt.dead)
					living.Insert(lt.d);
			}
			float keep = DL_TravelCost.PartyKeep(living, j.Trunks(-1));
			foreach (DL_Traveller tv : j.crew)
			{
				if (tv.dead)
					continue;
				bool seated = tv.veh >= 0;
				float foodK = t.foodFootKcal;
				float waterM = t.waterFootMl;
				if (seated)
				{
					foodK = t.foodCarKcal;
					waterM = t.waterCarMl;
				}
				if (!seated && j.water)
				{
					foodK = foodK * t.swimFoodX;
					waterM = waterM * t.swimFoodX;
				}
				float nf = foodK * f;
				float nw = waterM * f;
				tv.needFood += nf;
				tv.needWater += nw;
				float uf;
				float uw;
				DL_TravelCost.EatShared(tv.d, j.Trunks(tv.veh), nf, nw, uf, uw, keep);
				tv.unpaidFood += uf;
				tv.unpaidWater += uw;
				// in the water: drowning (the longer the swim, the likelier), no infected out here
				if (!seated && j.water)
				{
					if (weakened && Roll(j.DrownPct(t, tv, g)))
					{
						tv.dead = true;
						j.story.Insert(j.Who(tv) + " drowned " + Math.Round(at).ToString() + " km in.");
						j.film.Add("died", at, tv.name, "drowned");
					}
					continue;
				}
				// on foot: the infected
				if (!seated && weakened && Roll(t.bitePct * g))
				{
					float biteDeath = t.biteDeathPct * HurtX(tv.d); // hurt before the bite: likelier to die of it
					tv.d.health = Math.Max(1, tv.d.health - 30);
					tv.d.blood = Math.Max(2500, tv.d.blood - 750);
					if (Roll(biteDeath))
					{
						tv.dead = true;
						j.story.Insert(j.Who(tv) + " died from a bite " + Math.Round(at).ToString() + " km in.");
						j.film.Add("attack", at, tv.name, "died");
					}
					else
					{
						string was = " was";
						if (j.crew.Count() == 1)
							was = " were"; // "You were", not "You was"
						j.story.Insert(j.Who(tv) + was + " attacked by infected " + Math.Round(at).ToString() + " km in.");
						j.film.Add("attack", at, tv.name, "hurt");
					}
				}
			}
			// the party keeps together: one walking means all go at walking pace
			if (walking && j.water)
			{
				j.swumKm += leg;
				j.film.Add("swim", leg);
			}
			else if (walking)
			{
				j.walkedKm += leg;
				j.film.Add("walk", leg);
			}
			else
			{
				j.drivenKm += leg;
				j.film.Add("drive", leg);
			}
			done += leg;
		}
		// hunger and thirst: what nothing could pay
		bool anyHungry = false;
		foreach (DL_Traveller th : j.crew)
		{
			if (th.dead)
				continue;
			float frac = 0;
			if (th.needFood > 0)
				frac = Math.Max(frac, th.unpaidFood / th.needFood);
			if (th.needWater > 0)
				frac = Math.Max(frac, th.unpaidWater / th.needWater);
			if (frac <= 0.001)
				continue;
			anyHungry = true;
			float starveDeath = t.starveDeathPct * HurtX(th.d);
			th.d.health = Math.Max(1, th.d.health - frac * 40);
			th.d.blood = Math.Max(2500, th.d.blood - frac * 1500);
			if (weakened && frac > 0.5 && Roll(Math.Min(starveDeath, 100)))
			{
				th.dead = true;
				j.story.Insert(j.Who(th) + " didn't make it: there wasn't enough food and water.");
				j.film.Add("died", km, th.name, "hunger");
			}
		}
		if (anyHungry && weakened)
		{
			j.story.Insert("Food and water ran low: everyone had to ration.");
			j.film.Add("ration", km);
		}
		if (t.timeEnabled)
			j.seconds = Math.Round(j.drivenKm / Math.Max(0.5, t.speedCarKmh) * 3600 + j.walkedKm / Math.Max(0.5, t.speedFootKmh) * 3600 + j.swumKm / Math.Max(0.25, t.speedFootKmh * SWIM_SPEED_X) * 3600);
		if (j.swumKm > 0)
		{
			if (j.Convoy() && !j.carLost)
				j.story.Insert("The boats kept to swimming pace for the last " + Math.Round(j.swumKm).ToString() + " km.");
			else if (j.Alive() > 0)
				j.story.Insert("Swam the last " + Math.Round(j.swumKm).ToString() + " km to the shore.");
		}
		if (j.walkedKm > 0 && j.vehicles.Count() > 0)
		{
			if (j.Convoy() && !j.carLost)
				j.story.Insert("The convoy kept to walking pace for the last " + Math.Round(j.walkedKm).ToString() + " km.");
			else
				j.story.Insert("Walked the last " + Math.Round(j.walkedKm).ToString() + " km.");
		}
		foreach (DL_JVehicle lv : j.vehicles)
		{
			if (lv.luckyFuel && !lv.lost && lv.car.fluids.Count() > CarFluid.FUEL)
				lv.car.fluids[CarFluid.FUEL] = Math.Max(lv.car.fluids[CarFluid.FUEL], 0.03); // arrives nearly dry
		}
		j.Finish(km);
		return j;
	}

	// ---------------- what the road gives ----------------
	static const string FOUND_FOOD = "BakedBeansCan,SpaghettiCan,SardinesCan,TacticalBaconCan,PeachesCan,Crackers,SaltySticks,Chips,Rice,Apple,Pear,Plum";

	protected void Finds(DL_TravelRules t, float f, float at)
	{
		DL_Traveller finder = AnyAlive();
		string name = "";
		if (finder)
			name = finder.name;
		bool rolling = In(-1).Count() == 0 && Driving() > 0;
		if (rolling && Roll(t.findGasPct * f))
		{
			int cans = FillCanisters();
			foreach (DL_JVehicle gv : vehicles)
			{
				if (!gv.lost && gv.car.fluids.Count() > CarFluid.FUEL)
					gv.car.fluids[CarFluid.FUEL] = 1;
			}
			string place = "a working gas station";
			if (water)
				place = "a fuel dock";
			string what = "every tank";
			if (cans > 0)
				what += " and " + cans.ToString() + " jerry can(s)";
			story.Insert(Cap(place) + " " + Math.Round(at).ToString() + " km in: " + what + " filled up.");
			film.Add("find", at, name, "gas", -1, Driving());   // donor = how many cars are filled (film length)
		}
		if (Roll(t.findWaterPct * f))
		{
			int bottles = FillBottles();
			foreach (DL_Traveller wt : crew)
			{
				if (!wt.dead)
					wt.d.water = Math.Min(PlayerConstants.SL_WATER_MAX, wt.d.water + 1500);
			}
			string src = "a well";
			if (water)
				src = "rain";
			story.Insert(Cap(src) + " " + Math.Round(at).ToString() + " km in: everyone drank, " + bottles.ToString() + " bottle(s) filled.");
			film.Add("find", at, name, "water");
		}
		if (Roll(t.findFoodPct * f))
		{
			int n = FindFood();
			if (n > 0)
			{
				story.Insert(Who(finder) + " found food " + Math.Round(at).ToString() + " km in (" + n.ToString() + " items).");
				film.Add("find", at, name, "food");
			}
		}
	}

	// cars still going with someone at the wheel
	protected int Driving()
	{
		int n = 0;
		foreach (DL_JVehicle v : vehicles)
		{
			if (!v.lost && In(v.idx).Count() > 0)
				n++;
		}
		return n;
	}

	// every list of items the party has: each living traveller's gear, each car's trunk
	protected array<ref array<ref DL_ItemData>> AllItems()
	{
		array<ref array<ref DL_ItemData>> lists = new array<ref array<ref DL_ItemData>>;
		foreach (DL_Traveller t : crew)
		{
			if (!t.dead)
				lists.Insert(t.d.items);
		}
		foreach (DL_JVehicle v : vehicles)
		{
			if (!v.lost)
				lists.Insert(v.car.items);
		}
		return lists;
	}

	protected int FillCanisters()
	{
		int n = 0;
		array<ref array<ref DL_ItemData>> lists = AllItems();
		foreach (array<ref DL_ItemData> l : lists)
		{
			foreach (DL_ItemData it : l)
			{
				if (!it || !g_Game.IsKindOf(it.type, "CanisterGasoline"))
					continue;
				it.liquid = LIQUID_GASOLINE;
				it.quantity = g_Game.ConfigGetFloat("CfgVehicles " + it.type + " varQuantityMax");
				n++;
			}
		}
		return n;
	}

	// bottles, canteens ... (not the gasoline canisters, not ones with something else in them): clean water
	protected int FillBottles()
	{
		int n = 0;
		array<ref array<ref DL_ItemData>> lists = AllItems();
		foreach (array<ref DL_ItemData> l : lists)
		{
			foreach (DL_ItemData it : l)
			{
				if (!it || !g_Game.IsKindOf(it.type, "Bottle_Base") || g_Game.IsKindOf(it.type, "CanisterGasoline"))
					continue;
				if (it.liquid != 0 && it.quantity > 0 && (it.liquid & LIQUID_GROUP_DRINKWATER) == 0)
					continue;
				float full = g_Game.ConfigGetFloat("CfgVehicles " + it.type + " varQuantityMax");
				if (full <= 0)
					continue;
				it.liquid = LIQUID_CLEANWATER;
				it.quantity = full;
				it.agents = 0;
				n++;
			}
		}
		return n;
	}

	// 1-3 food items per living traveller (no cans when nobody can open them), into a trunk or their clothes
	protected int FindFood()
	{
		array<string> all = new array<string>;
		FOUND_FOOD.Split(",", all);
		array<ref DL_PlayerData> living = new array<ref DL_PlayerData>;
		foreach (DL_Traveller lt : crew)
		{
			if (!lt.dead)
				living.Insert(lt.d);
		}
		bool opener = DL_TravelCost.PartyKeep(living, Trunks(-1)) > 0;
		array<string> pool = new array<string>;
		foreach (string ft : all)
		{
			if (!g_Game.ConfigIsExisting("CfgVehicles " + ft))
				continue;
			if (!opener && g_Game.ConfigIsExisting("CfgVehicles " + ft + "_Opened"))
				continue;
			pool.Insert(ft);
		}
		if (pool.Count() == 0 || living.Count() == 0)
			return 0;
		int n = living.Count() + Math.RandomIntInclusive(0, living.Count() * 2);
		for (int i = 0; i < n; i++)
			Stow(pool.GetRandomElement());
		return n;
	}

	// a found item into a car's trunk, else into the clothes / bag of someone alive (no room = on the ground there)
	protected void Stow(string type)
	{
		DL_ItemData it = new DL_ItemData();
		it.type = type;
		it.slot = DL_Const.SLOT_CARGO;
		it.quantity = g_Game.ConfigGetFloat("CfgVehicles " + type + " varQuantityInit");
		array<ref DL_ItemData> into;
		foreach (DL_JVehicle v : vehicles)
		{
			if (!v.lost)
			{
				into = v.car.items;
				it.parent = 0;
				break;
			}
		}
		if (!into)
		{
			DL_Traveller t = AnyAlive();
			if (!t)
				return;
			into = t.d.items;
			it.parent = 0;
			foreach (DL_ItemData c : into)
			{
				if (c && c.parent == 0 && (c.slot == "Back" || c.slot == "Body" || c.slot == "Vest" || c.slot == "Legs"))
				{
					it.parent = c.id;
					if (c.slot == "Back")
						break;
				}
			}
		}
		int id = 0;
		foreach (DL_ItemData x : into)
		{
			if (x && x.id > id)
				id = x.id;
		}
		it.id = id + 1;
		into.Insert(it);
	}

	// the trunks a traveller in car v can eat from: his own car's first, then the rest of the convoy's
	protected array<DL_CarData> Trunks(int v)
	{
		array<DL_CarData> a = new array<DL_CarData>;
		DL_JVehicle own = Veh(v);
		if (own && !own.lost)
			a.Insert(own.car);
		foreach (DL_JVehicle o : vehicles)
		{
			if (!o.lost && o != own)
				a.Insert(o.car);
		}
		return a;
	}

	// car v has a living driver (the next one in it takes the wheel, else someone on foot climbs in); false = nobody
	// is left to drive it, it stays
	protected bool TakeWheel(DL_JVehicle v, float at)
	{
		DL_Traveller h = Find(v.holder);
		if (h && !h.dead && h.veh == v.idx)
			return true;
		DL_Traveller next;
		array<DL_Traveller> inIt = In(v.idx);
		if (inIt.Count() > 0)
			next = inIt[0];
		else
		{
			array<DL_Traveller> walkers = In(-1);
			if (walkers.Count() > 0)
			{
				next = walkers[0];
				next.veh = v.idx;
				if (Convoy())
					film.Add("move", at, next.name, "", v.idx);
			}
		}
		if (!next)
		{
			v.lost = true;
			story.Insert("Nobody was left to drive " + CarOf(v) + ": it stayed on the road.");
			return false;
		}
		v.holder = next.sid;
		next.seat = 0;
		story.Insert(next.name + " took the wheel" + Of(v) + ".");
		return true;
	}

	// " of Derk's Gunter 2" in a convoy, "" with one car
	protected string Of(DL_JVehicle v)
	{
		if (!Convoy())
			return "";
		return " of " + CarOf(v);
	}

	// one leg of car v (f = its share of 10 km for fuel + wear, g = its share of an event leg for the flat):
	// fuel (its own, a jerry can from the convoy, luck, else it stays), wear, a flat
	protected void DriveLeg(DL_TravelRules t, DL_JVehicle v, float f, float g, float done, float at, bool weakened)
	{
		DL_TravelPrice cp = new DL_TravelPrice();
		cp.byCar = true;
		cp.fuelL = DL_TravelCost.FuelFor(t, v.live, 10.0 * f);
		cp.engineWearPct = t.wearEnginePct * f;
		cp.wheelWearPct = t.wearWheelsPct * f;
		if (v.luckyFuel)
			cp.fuelL = 0; // the fuel they found takes them the rest of the way
		DL_PlayerData drvD;
		DL_Traveller drv = Find(v.holder);
		if (drv)
			drvD = drv.d;
		float have = FuelHave(v, drvD);
		if (have + 0.01 < cp.fuelL)
		{
			string donor;
			int donorVeh = -1;
			float moved = ShareFuel(v, cp.fuelL - have, donor, donorVeh);
			if (moved > 0 && have + moved + 0.01 >= cp.fuelL)
			{
				story.Insert(CarOf(v) + " ran low on fuel " + Math.Round(at).ToString() + " km in: " + donor + "'s jerry can kept it going.");
				film.Add("fuel", done, donor, "shared", v.idx, donorVeh);
			}
			else
			{
				DL_Traveller finder = AnyIn(v.idx);
				if (DL_Veh.IsPlane(v.live))
				{
					// nowhere to find fuel up there: an emergency landing, on foot from where it came down
					film.Add("fuel", done, "", "landed", v.idx);
					Abandon(v, at, "Ran out of fuel " + Math.Round(at).ToString() + " km in: an emergency landing, and " + CarOf(v) + " stayed where it came down.");
					return;
				}
				if (weakened && finder && Roll(t.luckGasPct))
				{
					v.luckyFuel = true;
					cp.fuelL = FuelHave(v, drvD); // what was left goes first
					story.Insert(Cap(CarOf(v)) + " ran out of gas " + Math.Round(at).ToString() + " km in. Luckily, " + Who(finder) + " found some fuel.");
					film.Add("fuel", done, finder.name, "found", v.idx);
				}
				else
				{
					string seeker = "";
					if (finder)
						seeker = finder.name;
					film.Add("fuel", done, seeker, "lost", v.idx);
					Abandon(v, at, "Ran out of gas " + Math.Round(at).ToString() + " km in and had to leave " + CarOf(v) + " behind.");
					return;
				}
			}
		}
		DL_TravelCost.ChargeCar(v.car, v.live, cp, drvD);
		if (weakened)
			FlatTyre(t, g, v, at);
	}

	protected static string Cap(string s)
	{
		if (s == "")
			return s;
		string first = s.Substring(0, 1);
		first.ToUpper();
		return first + s.Substring(1, s.Length() - 1);
	}

	// tank + gasoline canisters (trunk, then what the driver carries), litres
	float FuelHave(DL_JVehicle v, DL_PlayerData holder)
	{
		if (!v || !v.car)
			return 0;
		float l = 0;
		if (v.car.fluids.Count() > CarFluid.FUEL)
			l = v.car.fluids[CarFluid.FUEL] * FuelCap(v);
		foreach (DL_ItemData it : v.car.items)
		{
			if (DL_TravelCost.IsFuel(it))
				l += it.quantity / 1000;
		}
		if (holder)
		{
			foreach (DL_ItemData hi : holder.items)
			{
				if (DL_TravelCost.IsFuel(hi))
					l += hi.quantity / 1000;
			}
		}
		return l;
	}

	protected float FuelCap(DL_JVehicle v)
	{
		float cap = DL_Veh.Capacity(v.live, CarFluid.FUEL);
		if (cap <= 0 && v.car.fluidCaps.Count() > CarFluid.FUEL)
			cap = v.car.fluidCaps[CarFluid.FUEL];
		return cap;
	}

	// a convoy pours jerry cans from the other cars' trunks and the other travellers into car v's tank: litres moved
	protected float ShareFuel(DL_JVehicle v, float need, out string donor, out int donorVeh)
	{
		donor = "";
		donorVeh = -1;
		if (!Convoy())
			return 0;
		float moved = 0;
		float cap = FuelCap(v);
		if (cap <= 0 || v.car.fluids.Count() <= CarFluid.FUEL)
			return 0;
		foreach (DL_JVehicle o : vehicles)
		{
			if (o == v)
				continue;
			foreach (DL_ItemData it : o.car.items)
			{
				if (moved >= need)
					break;
				if (!DL_TravelCost.IsFuel(it))
					continue;
				float take = Math.Min(it.quantity / 1000, need - moved);
				it.quantity -= take * 1000;
				moved += take;
				if (donor == "")
				{
					DL_Traveller oh = Find(o.holder);
					donor = "the convoy";
					if (oh)
						donor = oh.name;
					donorVeh = o.idx;
				}
			}
		}
		foreach (DL_Traveller tr : crew)
		{
			if (tr.dead || tr.sid == v.holder)
				continue;
			foreach (DL_ItemData pi : tr.d.items)
			{
				if (moved >= need)
					break;
				if (!DL_TravelCost.IsFuel(pi))
					continue;
				float ptake = Math.Min(pi.quantity / 1000, need - moved);
				pi.quantity -= ptake * 1000;
				moved += ptake;
				if (donor == "")
				{
					donor = tr.name;
					donorVeh = tr.veh;
				}
			}
		}
		v.car.fluids[CarFluid.FUEL] = Math.Min(1, v.car.fluids[CarFluid.FUEL] + moved / cap);
		return moved;
	}

	static string CarName(DL_CarData c)
	{
		if (!c)
			return "car";
		// the name players know (config displayName); "#STR_..." needs the client's language table, so not that
		string n = g_Game.ConfigGetTextOut("CfgVehicles " + c.type + " displayName");
		if (n != "" && n.IndexOf("#") != 0)
			return n;
		return c.type;
	}

	// car v left on the road: its crew climb into free seats of the other cars, whoever doesn't fit walks on
	protected void Abandon(DL_JVehicle v, float at, string line)
	{
		v.lost = true;
		story.Insert(line);
		array<DL_Traveller> left = In(v.idx);
		if (!Convoy())
		{
			foreach (DL_Traveller st : left)
			{
				st.veh = -1;
				st.seat = -1;
			}
			carLost = true;
			return;
		}
		string walkers = "";
		foreach (DL_Traveller t : left)
		{
			DL_JVehicle to = null;
			int seat = -1;
			foreach (DL_JVehicle o : vehicles)
			{
				if (o.lost)
					continue;
				seat = FreeSeat(o);
				if (seat >= 0)
				{
					to = o;
					break;
				}
			}
			if (to)
			{
				t.veh = to.idx;
				t.seat = seat;
				story.Insert(t.name + " climbed into " + CarOf(to) + ".");
				film.Add("move", at, t.name, "", to.idx);
			}
			else
			{
				t.veh = -1;
				t.seat = -1;
				if (walkers != "")
					walkers += " and ";
				walkers += t.name;
				if (water)
					film.Add("move", at, t.name, "swim", -1);
				else
					film.Add("move", at, t.name, "walk", -1);
			}
		}
		if (walkers != "" && water)
			story.Insert(walkers + " swam on beside the boats, holding on: no seat left.");
		else if (walkers != "")
			story.Insert(walkers + " walked on beside the convoy: no seat left.");
		carLost = true;
		foreach (DL_JVehicle c : vehicles)
		{
			if (!c.lost)
				carLost = false;
		}
	}

	// a free seat in car v (not the driver's), -1 = full
	protected int FreeSeat(DL_JVehicle v)
	{
		for (int s = 1; s < v.seats; s++)
		{
			bool taken;
			taken = false;
			array<DL_Traveller> inV = In(v.idx);
			foreach (DL_Traveller t : inV)
			{
				if (t.seat == s)
					taken = true;
			}
			if (!taken)
				return s;
		}
		return -1;
	}

	// a flat on this leg? fixed with a spare wheel, else a repair kit, else the car is left. In a convoy the spare
	// (one that fits this car) or the kit may come from any car or anyone in it.
	protected void FlatTyre(DL_TravelRules t, float f, DL_JVehicle v, float at)
	{
		if (DL_Veh.IsPlane(v.live))
			return;                      // in the air
		DL_CarData car = v.car;
		array<string> types = DL_TravelCost.WheelTypes(v.live);
		// the wheels it runs on (mounted, not in a spare slot), and how worn they are
		array<DL_ItemData> running = new array<DL_ItemData>;
		float frac = 0;
		foreach (DL_ItemData it : car.items)
		{
			if (it && it.parent == 0 && it.slot != DL_Const.SLOT_CARGO && !DL_TravelCost.IsSpareSlot(it.slot) && g_Game.IsKindOf(it.type, "CarWheel"))
			{
				running.Insert(it);
				float mx = g_Game.ConfigGetFloat("CfgVehicles " + it.type + " DamageSystem GlobalHealth Health hitpoints");
				if (mx > 0 && it.health >= 0)
					frac += it.health / mx;
				else
					frac += 1;
			}
		}
		if (running.Count() == 0)
			return;
		frac /= running.Count();
		if (!Roll(t.flatPct * f * (2.0 - frac)))
			return;
		DL_ItemData flat = running[Math.RandomInt(0, running.Count())];
		DL_Traveller fixer = AnyIn(v.idx);
		if (!fixer)
			fixer = crew[0];
		string where = Math.Round(at).ToString() + " km in";
		string onCar = "";
		if (Convoy())
			onCar = " on " + CarOf(v);
		// 1. a spare wheel: this car's spare slot or trunk, its crew's gear; then (convoy) the other cars and people
		DL_ItemData spare;
		array<ref DL_ItemData> spareList;
		int fromVeh = v.idx;
		string fromName = "";
		foreach (DL_ItemData c : car.items)
		{
			if (c && c != flat && IsLooseWheel(c, types))
			{
				spare = c;
				spareList = car.items;
				break;
			}
		}
		if (!spare)
		{
			foreach (DL_Traveller tv : crew)
			{
				if (tv.dead || tv.veh != v.idx)
					continue;
				foreach (DL_ItemData pi : tv.d.items)
				{
					if (pi && types.Find(pi.type) >= 0 && pi.health != 0)
					{
						spare = pi;
						spareList = tv.d.items;
						break;
					}
				}
				if (spare)
					break;
			}
		}
		if (!spare && Convoy())
		{
			foreach (DL_JVehicle o : vehicles)
			{
				if (o == v)
					continue;
				foreach (DL_ItemData oc : o.car.items)
				{
					if (oc && IsLooseWheel(oc, types))
					{
						spare = oc;
						spareList = o.car.items;
						fromVeh = o.idx;
						DL_Traveller oh = Find(o.holder);
						if (oh)
							fromName = oh.name;
						break;
					}
				}
				if (spare)
					break;
			}
		}
		if (!spare && Convoy())
		{
			foreach (DL_Traveller ov : crew)
			{
				if (ov.dead || ov.veh == v.idx)
					continue;
				foreach (DL_ItemData opi : ov.d.items)
				{
					if (opi && types.Find(opi.type) >= 0 && opi.health != 0)
					{
						spare = opi;
						spareList = ov.d.items;
						fromVeh = ov.veh;
						fromName = ov.name;
						break;
					}
				}
				if (spare)
					break;
			}
		}
		int filmFrom = -1;
		if (fromVeh != v.idx)
			filmFrom = fromVeh;
		if (spare)
		{
			// the spare goes where the flat was; the shredded tyre stays by the road
			string slot = flat.slot;
			car.items.RemoveItem(flat);
			if (spareList != car.items)
			{
				spareList.RemoveItem(spare);
				int maxId = 0;
				foreach (DL_ItemData ci : car.items)
					maxId = Math.Max(maxId, ci.id);
				spare.id = maxId + 1;
				car.items.Insert(spare);
			}
			spare.parent = 0;
			spare.slot = slot;
			spare.cIdx = -1;
			spare.quickbar = -1;
			if (fromName != "")
				story.Insert("Blew a tyre" + onCar + " " + where + ". " + fromName + "'s spare fit: " + Who(fixer) + " put it on.");
			else
				story.Insert("Blew a tyre" + onCar + " " + where + ". " + Who(fixer) + " put on the spare.");
			film.Add("flat", at, fixer.name, "spare", v.idx, filmFrom);
			return;
		}
		// 2. a tyre repair kit: patched to damaged health, the kit loses a use (vanilla: quantity 0-100, PluginRepairing)
		array<ref array<ref DL_ItemData>> lists = new array<ref array<ref DL_ItemData>>;
		array<int> listCar = new array<int>;
		lists.Insert(car.items);
		listCar.Insert(v.idx);
		foreach (DL_Traveller tk : crew)
		{
			if (!tk.dead && (tk.veh == v.idx || Convoy()))
			{
				lists.Insert(tk.d.items);
				listCar.Insert(tk.veh);
			}
		}
		if (Convoy())
		{
			foreach (DL_JVehicle ko : vehicles)
			{
				if (ko != v)
				{
					lists.Insert(ko.car.items);
					listCar.Insert(ko.idx);
				}
			}
		}
		for (int li = 0; li < lists.Count(); li++)
		{
			foreach (DL_ItemData kit : lists[li])
			{
				if (kit && g_Game.IsKindOf(kit.type, "TireRepairKit") && kit.quantity != 0 && kit.health != 0)
				{
					if (kit.quantity > 0)
						kit.quantity = Math.Max(0, kit.quantity - 25);
					float fmx = g_Game.ConfigGetFloat("CfgVehicles " + flat.type + " DamageSystem GlobalHealth Health hitpoints");
					if (fmx > 0)
						flat.health = fmx * 0.5;
					int kitFrom = -1;
					if (listCar[li] != v.idx && listCar[li] >= 0)
						kitFrom = listCar[li];
					story.Insert("Blew a tyre" + onCar + " " + where + ". " + Who(fixer) + " patched it with the repair kit.");
					film.Add("flat", at, fixer.name, "kit", v.idx, kitFrom);
					return;
				}
			}
		}
		// 3. nothing: the car stays here
		film.Add("flat", at, fixer.name, "none", v.idx);
		Abandon(v, at, "Blew a tyre" + onCar + " " + where + " with no spare and no repair kit: left " + CarOf(v) + " behind.");
	}

	// a wheel of these types lying in a spare slot or a trunk (not one the car runs on), not ruined
	protected bool IsLooseWheel(DL_ItemData c, array<string> types)
	{
		if (types.Find(c.type) < 0 || c.health == 0)
			return false;
		return DL_TravelCost.IsSpareSlot(c.slot) || !(c.parent == 0 && c.slot != DL_Const.SLOT_CARGO);
	}

	// ---------------- into the snapshots ----------------

	// who carries each car, crew counts, seats and the convoy order now that the road has been played
	protected void Finish(float km)
	{
		foreach (DL_JVehicle v : vehicles)
		{
			if (!v.lost)
				TakeWheel(v, km);
		}
		string text = Story();
		int slot = 0;
		map<int, int> lineIdx = new map<int, int>;
		foreach (DL_JVehicle lv : vehicles)
		{
			if (!lv.lost)
			{
				lineIdx.Set(lv.idx, slot);
				slot++;
			}
		}
		int walker = 0;
		foreach (DL_Traveller t : crew)
		{
			t.d.film = film;
			t.d.journey = text;
			t.d.diedOnRoad = t.dead;
			t.d.car = null;
			t.d.carGroup = "";
			t.d.crewCount = 0;
			t.d.carSeat = -1;
			t.d.convoyIdx = 0;
			t.d.partySlot = 0;
			if (t.dead)
				continue;
			DL_JVehicle mine = Veh(t.veh);
			if (!mine || mine.lost)
			{
				t.d.partySlot = walker; // on foot: no waiting room, a place beside the others
				walker++;
				continue;
			}
			t.d.crewCount = In(mine.idx).Count();
			t.d.vehKind = DL_Veh.Kind(mine.live);
			t.d.convoyIdx = lineIdx.Get(mine.idx);
			if (t.sid == mine.holder)
			{
				t.d.car = mine.car;
				t.d.carSeat = 0;
			}
			else
			{
				t.d.carGroup = mine.holder;
				t.d.carSeat = t.seat;
			}
		}
	}
}
