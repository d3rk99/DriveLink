// DriveLink travel step 4 (Research/travel-cost-design.md section 4 + 4b): "arrive weakened" mode. The source server
// plays the road ONCE for the whole crew before anyone leaves, in 10 km legs: fuel (luck when it runs dry, else the
// car is left), flat tyres (spare wheel / repair kit, else the car is left), infected on the walked legs, food and
// water (what nothing could pay = hunger/thirst damage, past half of the cost a chance to die). Everything happens to
// the SNAPSHOTS (DL_PlayerData / DL_CarData), the live characters are untouched. The result travels in each
// snapshot (journey = the story, diedOnRoad) and the target shows the story on the black screen.

class DL_Traveller
{
	string sid;
	string name;
	ref DL_PlayerData d;
	int seat;                 // car seat, -1 = on foot
	bool dead;
	float needFood;           // stat units for the whole trip, for the starvation check
	float needWater;
	float unpaidFood;
	float unpaidWater;
}

class DL_Journey
{
	ref array<ref DL_Traveller> crew = new array<ref DL_Traveller>;
	ref DL_CarData car;       // the car snapshot; null = on foot from the start
	bool carLost;
	string carHolder;         // steam id whose snapshot carries the car (the driver, or who took the wheel)
	ref array<string> story = new array<string>;
	float drivenKm;
	float walkedKm;
	int seconds;              // on the road (travel time on), else 0

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

	protected string Who(DL_Traveller t)
	{
		if (crew.Count() == 1)
			return "You";
		return t.name;
	}

	// ---------------- the road ----------------

	// crewPbs[0] = the driver when car != null. weakened = road events on (refuse mode: costs only, all payable).
	static DL_Journey Run(DL_TravelRules t, DL_Zone z, array<PlayerBase> crewPbs, CarScript liveCar, DL_Snapshot snap, DL_CarSnapshot carSnap, bool weakened)
	{
		DL_Journey j = new DL_Journey();
		for (int i = 0; i < crewPbs.Count(); i++)
		{
			PlayerBase pb = crewPbs[i];
			if (!pb || !pb.GetIdentity())
				continue;
			DL_Traveller tr = new DL_Traveller();
			tr.sid = pb.GetIdentity().GetPlainId();
			tr.name = pb.GetIdentity().GetName();
			tr.d = snap.Capture(pb);
			tr.seat = -1;
			if (liveCar)
				tr.seat = liveCar.CrewMemberIndex(pb);
			j.crew.Insert(tr);
		}
		if (liveCar && j.crew.Count() > 0)
		{
			j.car = carSnap.Capture(liveCar);
			j.carHolder = j.crew[0].sid;
		}
		float km = z.distanceKm;
		float done = 0;
		bool luckyFuel = false;
		while (done < km - 0.001)
		{
			float leg = Math.Min(10.0, km - done);
			float f = leg / 10.0;
			float at = done + leg;
			bool driving = j.car != null && !j.carLost;
			if (driving)
			{
				DL_TravelPrice cp = new DL_TravelPrice();
				cp.byCar = true;
				cp.fuelL = t.fuelLiters * f;
				cp.engineWearPct = t.wearEnginePct * f;
				cp.wheelWearPct = t.wearWheelsPct * f;
				if (luckyFuel)
					cp.fuelL = 0; // the fuel they found takes them the rest of the way
				DL_Traveller drv = j.Find(j.carHolder);
				DL_PlayerData drvD;
				if (drv)
					drvD = drv.d;
				if (j.FuelHave(liveCar, drvD) + 0.01 < cp.fuelL)
				{
					DL_Traveller finder = j.AnyAlive();
					if (weakened && finder && Roll(t.luckGasPct))
					{
						luckyFuel = true;
						cp.fuelL = j.FuelHave(liveCar, drvD); // what was left goes first
						j.story.Insert("Ran out of gas " + Math.Round(at).ToString() + " km in. Luckily, " + j.Who(finder) + " found some fuel.");
					}
					else
					{
						j.carLost = true;
						j.story.Insert("Ran out of gas " + Math.Round(at).ToString() + " km in and had to leave the " + DL_Journey.CarName(j.car) + " behind.");
						driving = false;
					}
				}
				if (driving)
				{
					DL_TravelCost.ChargeCar(j.car, liveCar, cp, drvD);
					if (weakened && j.FlatTyre(t, f, liveCar, at))
						driving = false;
				}
			}
			// food + water for this leg, at car or walking rates
			foreach (DL_Traveller tv : j.crew)
			{
				if (tv.dead)
					continue;
				float foodPct = t.foodFootPct;
				float waterPct = t.waterFootPct;
				if (driving)
				{
					foodPct = t.foodCarPct;
					waterPct = t.waterCarPct;
				}
				float nf = foodPct * f / 100.0 * PlayerConstants.SL_ENERGY_MAX;
				float nw = waterPct * f / 100.0 * PlayerConstants.SL_WATER_MAX;
				tv.needFood += nf;
				tv.needWater += nw;
				DL_CarData trunk;
				if (driving && tv.sid == j.carHolder)
					trunk = j.car;
				float uf;
				float uw;
				DL_TravelCost.EatEx(tv.d, trunk, nf, nw, uf, uw);
				tv.unpaidFood += uf;
				tv.unpaidWater += uw;
				// on foot: the infected
				if (!driving && weakened && Roll(t.bitePct * f))
				{
					tv.d.health = Math.Max(1, tv.d.health - 30);
					tv.d.blood = Math.Max(2500, tv.d.blood - 750);
					if (Roll(t.biteDeathPct))
					{
						tv.dead = true;
						j.story.Insert(j.Who(tv) + " died from a bite " + Math.Round(at).ToString() + " km in.");
					}
					else
						j.story.Insert(j.Who(tv) + " was attacked by infected " + Math.Round(at).ToString() + " km in.");
				}
			}
			if (driving)
				j.drivenKm += leg;
			else
				j.walkedKm += leg;
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
			th.d.health = Math.Max(1, th.d.health - frac * 40);
			th.d.blood = Math.Max(2500, th.d.blood - frac * 1500);
			if (weakened && frac > 0.5 && Roll(t.starveDeathPct))
			{
				th.dead = true;
				j.story.Insert(j.Who(th) + " didn't make it: there wasn't enough food and water.");
			}
		}
		if (anyHungry && weakened)
			j.story.Insert("Food and water ran low: everyone had to ration.");
		if (t.timeEnabled)
			j.seconds = Math.Round(j.drivenKm / Math.Max(0.5, t.speedCarKmh) * 3600 + j.walkedKm / Math.Max(0.5, t.speedFootKmh) * 3600);
		if (j.walkedKm > 0 && j.car)
			j.story.Insert("Walked the last " + Math.Round(j.walkedKm).ToString() + " km.");
		if (luckyFuel && j.car && !j.carLost && j.car.fluids.Count() > CarFluid.FUEL)
			j.car.fluids[CarFluid.FUEL] = Math.Max(j.car.fluids[CarFluid.FUEL], 0.03); // arrives nearly dry
		j.Finish();
		return j;
	}

	// tank + gasoline canisters (trunk, then what the car holder carries), litres
	float FuelHave(CarScript liveCar, DL_PlayerData holder)
	{
		if (!car)
			return 0;
		float cap = liveCar.GetFluidCapacity(CarFluid.FUEL);
		if (cap <= 0 && car.fluidCaps.Count() > CarFluid.FUEL)
			cap = car.fluidCaps[CarFluid.FUEL];
		float l = 0;
		if (car.fluids.Count() > CarFluid.FUEL)
			l = car.fluids[CarFluid.FUEL] * cap;
		foreach (DL_ItemData it : car.items)
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

	// a flat on this leg? fixed with a spare wheel, else a repair kit, else the car is left. true = car lost.
	protected bool FlatTyre(DL_TravelRules t, float f, CarScript liveCar, float at)
	{
		array<string> types = DL_TravelCost.WheelTypes(liveCar);
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
			return false;
		frac /= running.Count();
		if (!Roll(t.flatPct * f * (2.0 - frac)))
			return false;
		DL_ItemData flat = running[Math.RandomInt(0, running.Count())];
		DL_Traveller fixer = AnyAlive();
		if (!fixer)
			fixer = crew[0];
		string where = Math.Round(at).ToString() + " km in";
		// 1. a spare wheel: the car's spare slot, the trunk, or the crew's gear
		DL_ItemData spare;
		array<ref DL_ItemData> spareList;
		foreach (DL_ItemData c : car.items)
		{
			if (c && c != flat && types.Find(c.type) >= 0 && c.health != 0 && (DL_TravelCost.IsSpareSlot(c.slot) || !(c.parent == 0 && c.slot != DL_Const.SLOT_CARGO)))
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
			story.Insert("Blew a tyre " + where + ". " + Who(fixer) + " put on the spare.");
			return false;
		}
		// 2. a tyre repair kit: patched to damaged health, the kit loses a use (vanilla: quantity 0-100, PluginRepairing)
		array<ref array<ref DL_ItemData>> lists = new array<ref array<ref DL_ItemData>>;
		lists.Insert(car.items);
		foreach (DL_Traveller tk : crew)
			lists.Insert(tk.d.items);
		foreach (array<ref DL_ItemData> list : lists)
		{
			foreach (DL_ItemData kit : list)
			{
				if (kit && g_Game.IsKindOf(kit.type, "TireRepairKit") && kit.quantity != 0 && kit.health != 0)
				{
					if (kit.quantity > 0)
						kit.quantity = Math.Max(0, kit.quantity - 25);
					float fmx = g_Game.ConfigGetFloat("CfgVehicles " + flat.type + " DamageSystem GlobalHealth Health hitpoints");
					if (fmx > 0)
						flat.health = fmx * 0.5;
					story.Insert("Blew a tyre " + where + ". " + Who(fixer) + " patched it with the repair kit.");
					return false;
				}
			}
		}
		// 3. nothing: the car stays here
		carLost = true;
		story.Insert("Blew a tyre " + where + " with no spare and no repair kit: left the " + CarName(car) + " behind.");
		return true;
	}

	// ---------------- into the snapshots ----------------

	// who carries the car, crew counts and seats now that the road has been played
	protected void Finish()
	{
		string text = Story();
		DL_Traveller holder = Find(carHolder);
		// the driver died but the car goes on: the first one alive takes the wheel
		if (car && !carLost && holder && holder.dead)
		{
			DL_Traveller next;
			foreach (DL_Traveller tn : crew)
			{
				if (!tn.dead)
				{
					next = tn;
					break;
				}
			}
			if (next)
			{
				story.Insert(next.name + " took the wheel.");
				text = Story();
				carHolder = next.sid;
				holder = next;
				holder.seat = 0;
			}
			else
				carLost = true; // nobody left to drive it
		}
		bool withCar = car && !carLost;
		int alive = Alive();
		foreach (DL_Traveller t : crew)
		{
			t.d.journey = text;
			t.d.diedOnRoad = t.dead;
			t.d.car = null;
			t.d.carGroup = "";
			t.d.crewCount = 0;
			t.d.carSeat = -1;
			if (t.dead || !withCar)
				continue; // on foot (or dead): no waiting room
			t.d.crewCount = alive;
			if (t.sid == carHolder)
			{
				t.d.car = car;
				t.d.carSeat = 0;
			}
			else
			{
				t.d.carGroup = carHolder;
				t.d.carSeat = t.seat;
			}
		}
	}
}
