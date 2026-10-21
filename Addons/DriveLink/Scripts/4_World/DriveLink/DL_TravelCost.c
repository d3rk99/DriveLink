// DriveLink travel cost (Research/travel-cost-design.md, step 2): what a trip through a zone costs, whether the crew
// can pay it, and charging it to the SNAPSHOTS that travel (never to the live character or car: a trip that fails
// costs nothing). Rules come from the hub (DL_TravelRules, per server); distance from the zone (0 km = free).
// Engine calls: PlayerBase.GetStatWater/GetStatEnergy (playerbase.c), PlayerConstants.SL_WATER_MAX/SL_ENERGY_MAX (5000,
// 3_game/playerconstants.c:45-51), Car.GetFluidFraction/GetFluidCapacity(CarFluid.FUEL) (car.c), EntityAI.GetMaxHealth,
// wheels = CarWheel attachments (their slot name is how the snapshot stores them, DL_ItemData.slot).

class DL_TravelPrice
{
	float km;
	bool byCar;
	float foodKcal;           // per person (DayZ energy units = kcal)
	float waterMl;
	float fuelL;              // whole car
	float engineWearPct;      // % of the engine's max health
	float wheelWearPct;       // % of each wheel's max health
	int seconds;              // on the road (travel time on), 0 = instant
	float spareKm;            // > 0: this car trip needs a spare wheel or a tyre repair kit (refuse mode rule)

	string Text()
	{
		string s = Math.Round(km).ToString() + " km: " + DL_TravelCost.Kcal(foodKcal) + " food, " + DL_TravelCost.Ml(waterMl) + " water";
		if (byCar)
			s += " each, " + Math.Round(fuelL).ToString() + " L fuel";
		if (seconds > 0)
			s += ", about " + DL_TravelCost.Duration(seconds);
		if (spareKm > 0)
			s += ", spare wheel or tyre repair kit needed";
		return s;
	}
}

class DL_TravelCost
{
	// null = free (no distance on the zone, or no rules)
	// fuel per 10 km by kind: a motorbike burns 40 % of what a car does, a boat as much as a car
	static float FuelScale(EntityAI veh)
	{
		if (DL_Veh.Kind(veh) == "bike")
			return 0.4;
		return 1.0;
	}

	// litres for km kilometres in veh: its in-game consumption (fuelPerVehicle) or the flat fuelLiters per 10 km
	static float FuelFor(DL_TravelRules t, EntityAI veh, float km)
	{
		if (t.fuelPerVehicle && veh)
		{
			float per100 = g_Game.ConfigGetFloat("CfgVehicles " + veh.GetType() + " fuelConsumption");
			if (per100 > 0)
			{
				float l = per100 / 100.0 * km * t.fuelScale;
				if (DL_Veh.IsBoat(veh))
					l *= t.fuelBoatScale;
				return l;
			}
		}
		return t.fuelLiters * km / 10.0 * FuelScale(veh);
	}

	// veh = the vehicle (its kind scales the fuel, a boat needs no spare wheel); null = a car (or on foot)
	static DL_TravelPrice Price(DL_TravelRules t, DL_Zone z, bool byCar, Transport veh = null)
	{
		if (!t || !z || z.distanceKm <= 0)
			return null;
		DL_TravelPrice p = new DL_TravelPrice();
		p.km = z.distanceKm;
		p.byCar = byCar;
		float k = z.distanceKm / 10.0;
		if (byCar)
		{
			p.foodKcal = t.foodCarKcal * k;
			p.waterMl = t.waterCarMl * k;
			p.fuelL = FuelFor(t, veh, z.distanceKm);
			p.engineWearPct = t.wearEnginePct * k;
			p.wheelWearPct = t.wearWheelsPct * k;
		}
		else
		{
			p.foodKcal = t.foodFootKcal * k;
			p.waterMl = t.waterFootMl * k;
		}
		if (byCar && t.spareNeededKm > 0 && z.distanceKm > t.spareNeededKm && !DL_Veh.IsBoat(veh))
			p.spareKm = t.spareNeededKm;
		if (t.timeEnabled)
		{
			float speed = t.speedFootKmh;
			if (byCar)
				speed = t.speedCarKmh;
			if (speed > 0)
				p.seconds = Math.Round(z.distanceKm / speed * 3600);
		}
		return p;
	}

	static string Duration(int seconds)
	{
		int m = Math.Round(seconds / 60.0);
		if (m < 60)
			return m.ToString() + " min";
		int mm = m % 60;
		return (m / 60).ToString() + " h " + mm.ToString() + " min";
	}

	// "3000 kcal"
	static string Kcal(float v)
	{
		return Math.Round(v).ToString() + " kcal";
	}

	// "750 ml" / "3.5 L"
	static string Ml(float v)
	{
		if (v < 1000)
			return Math.Round(v).ToString() + " ml";
		float l = Math.Round(v / 100.0) / 10.0;
		return l.ToString() + " L";
	}

	static float FoodPctOf(PlayerBase pb)
	{
		return pb.GetStatEnergy().Get() / PlayerConstants.SL_ENERGY_MAX * 100.0;
	}

	static float WaterPctOf(PlayerBase pb)
	{
		return pb.GetStatWater().Get() / PlayerConstants.SL_WATER_MAX * 100.0;
	}

	static float FuelLOf(Transport car)
	{
		return DL_Veh.Fraction(car, CarFluid.FUEL) * DL_Veh.Capacity(car, CarFluid.FUEL);
	}

	// "" = everyone can pay; else what is missing, per person: "Britney: water 12% of 30%; car: fuel 4 L of 30 L".
	// Counts bars + stomach + food/drink they carry (the driver also the trunk) and the tank + gasoline canisters.
	static string Shortfall(DL_TravelPrice p, array<PlayerBase> crew, Transport car, DL_Snapshot snap, DL_CarSnapshot carSnap)
	{
		if (!p)
			return "";
		DL_CarData cd;
		if (car && p.byCar)
			cd = carSnap.Capture(car);
		float needFood = p.foodKcal;
		float needWater = p.waterMl;
		float canisters = 0;
		array<string> miss = new array<string>;
		array<ref DL_PlayerData> snaps = new array<ref DL_PlayerData>;
		array<PlayerBase> who4 = new array<PlayerBase>;
		foreach (PlayerBase cp : crew)
		{
			if (!cp)
				continue;
			snaps.Insert(snap.Capture(cp));
			who4.Insert(cp);
		}
		array<DL_CarData> partyTrunks = new array<DL_CarData>;
		if (cd)
			partyTrunks.Insert(cd);
		float keep = PartyKeep(snaps, partyTrunks);
		for (int i = 0; i < who4.Count(); i++)
		{
			PlayerBase pb = who4[i];
			DL_PlayerData d = snaps[i];
			DL_CarData trunk;
			if (i == 0)
				trunk = cd; // the driver eats from the trunk too (it travels in his snapshot)
			float food;
			float water;
			float canL;
			Count(d, trunk, food, water, canL, keep);
			canisters += canL;
			string who = "you";
			if (crew.Count() > 1 && pb.GetIdentity())
				who = pb.GetIdentity().GetName();
			string m = "";
			if (food < needFood)
			{
				m += "food " + Math.Floor(food).ToString() + " of " + Kcal(Math.Ceil(needFood));
				int shut = ClosedCans(d, trunk);
				if (shut > 0 && keep <= 0)
					m += " (" + shut.ToString() + " closed cans, nothing to open them)";
			}
			if (water < needWater)
			{
				if (m != "")
					m += ", ";
				m += "water " + Ml(Math.Floor(water)) + " of " + Ml(Math.Ceil(needWater));
			}
			if (m != "")
				miss.Insert(who + ": " + m);
		}
		if (car && p.byCar)
		{
			float fuel = FuelLOf(car) + canisters;
			if (fuel < p.fuelL)
				miss.Insert(DL_Veh.Kind(car) + ": fuel " + Math.Floor(fuel).ToString() + " L of " + Math.Ceil(p.fuelL).ToString() + " L (tank + gasoline canisters)");
			if (p.spareKm > 0 && !HasSpareOrKit(car, cd, snaps))
				miss.Insert(DL_Veh.Kind(car) + ": a spare " + WheelTypeText(car) + " or a tyre repair kit (trips over " + Math.Round(p.spareKm).ToString() + " km)");
		}
		string res = "";
		foreach (string ms : miss)
		{
			if (res != "")
				res += "; ";
			res += ms;
		}
		return res;
	}


	// ---------------- spare wheel / tyre repair kit ----------------

	// the wheel types this car runs on (its CarWheel attachments), e.g. HatchbackWheel
	static array<string> WheelTypes(Transport car)
	{
		array<string> types = new array<string>;
		for (int i = 0; i < car.GetInventory().AttachmentCount(); i++)
		{
			EntityAI a = car.GetInventory().GetAttachmentFromIndex(i);
			if (a && a.IsInherited(CarWheel) && !a.IsInherited(CarWheel_Ruined) && types.Find(a.GetType()) < 0)
				types.Insert(a.GetType());
		}
		return types;
	}

	static bool IsSpareSlot(string slot)
	{
		string sl = slot;
		sl.ToLower();
		return sl.Contains("spare");
	}

	static string WheelTypeText(Transport car)
	{
		array<string> t = WheelTypes(car);
		if (t.Count() == 0)
			return "wheel";
		return t[0];
	}

	// a spare = a wheel of one of those types that is NOT on the car (trunk cargo, or anywhere in the crew's gear);
	// a kit = a TireRepairKit with uses left. Ruined wheels are other classes (..._Ruined), so they don't count.
	static bool HasSpareOrKit(Transport car, DL_CarData cd, array<ref DL_PlayerData> crew)
	{
		array<string> types = WheelTypes(car);
		if (types.Count() == 0)
			return true;                        // no wheels (a boat): nothing to carry
		array<ref DL_ItemData> all = new array<ref DL_ItemData>;
		if (cd)
		{
			foreach (DL_ItemData c : cd.items)
			{
				// parent 0 + a slot that isn't cargo = mounted on the car (its wheels, doors, battery...); a wheel in a
				// spare slot (OffroadHatchback "...Spare...") IS a spare
				if (c && (!(c.parent == 0 && c.slot != DL_Const.SLOT_CARGO) || IsSpareSlot(c.slot)))
					all.Insert(c);
			}
		}
		foreach (DL_PlayerData d : crew)
		{
			if (!d)
				continue;
			foreach (DL_ItemData pi : d.items)
				all.Insert(pi);
		}
		foreach (DL_ItemData it : all)
		{
			if (!it)
				continue;
			if (types.Find(it.type) >= 0 && it.health != 0)
				return true;
			if (g_Game.IsKindOf(it.type, "TireRepairKit") && it.quantity != 0 && it.health != 0)
				return true;
		}
		return false;
	}

	// ---------------- supplies: bars + stomach + carried food/drink, fuel tank + canisters ----------------
	// Nutrition is per 100 units of quantity / stomach amount, like vanilla digestion (playerstomach.c:90-93:
	// profile.GetEnergy() / 100 per unit). Food: Edible_Base.GetFoodEnergy/GetFoodWater(null, type, stage)
	// (edible_base.c:407-433); drinks: Liquid.GetNutritionalProfileByType (liquid.c:318). Units = stat units
	// (PlayerStats energy/water, max 5000).

	// energy + water per unit of what is in this item, false = nothing he would eat/drink on the road
	static bool Nutrition(DL_ItemData it, out float energyPerUnit, out float waterPerUnit)
	{
		energyPerUnit = 0;
		waterPerUnit = 0;
		if (!it || it.quantity <= 0 || it.agents != 0) // contaminated (dirty water, salmonella...): not on the road
			return false;
		if (it.liquid != 0)
		{
			if ((it.liquid & (LIQUID_GROUP_DRINKWATER | LIQUID_BEER)) == 0)
				return false; // fuel, disinfectant, blood, salt water...
			NutritionalProfile lp = Liquid.GetNutritionalProfileByType(it.liquid);
			if (!lp)
				return false;
			energyPerUnit = lp.GetEnergy() / 100;
			waterPerUnit = lp.GetWaterContent() / 100;
		}
		else
		{
			if (!g_Game.IsKindOf(it.type, "Edible_Base") || g_Game.IsKindOf(it.type, "Bottle_Base"))
				return false;
			if (it.foodStage == FoodStageType.ROTTEN || it.foodStage == FoodStageType.BURNED)
				return false;
			string t = it.type;
			t.ToLower();
			if (it.foodStage == FoodStageType.RAW && (t.Contains("meat") || t.Contains("lard") || t.Contains("guts")))
				return false; // raw meat makes you sick
			int stage = it.foodStage;
			if (stage < 0)
				stage = 0;
			energyPerUnit = Edible_Base.GetFoodEnergy(null, it.type, stage) / 100;
			waterPerUnit = Edible_Base.GetFoodWater(null, it.type, stage) / 100;
		}
		return energyPerUnit > 0 || waterPerUnit > 0;
	}

	// ---------------- closed cans ----------------
	// A closed can (BakedBeansCan, ...) has no quantity and no nutrition; vanilla opens it with a tool into
	// <type>_Opened (recipe OpenCan, opencan.c), spilling a random share in the tool's openItemSpillRange[] (%,
	// openitem.c: a CanOpener 0-0, knives 10-20, axes 20-40, bats 40-80). On the road: opened with the best tool
	// anyone of the party carries (vehicle trunks included), losing that tool's average spill; no tool = uneatable.

	// "<type>_Opened" when it is a closed can, else ""
	static string OpenedType(DL_ItemData it)
	{
		if (!it || it.liquid != 0 || it.quantity > 0 || it.agents != 0)
			return "";
		string o = it.type + "_Opened";
		if (!g_Game.ConfigIsExisting("CfgVehicles " + o) || !g_Game.IsKindOf(o, "Edible_Base"))
			return "";
		return o;
	}

	// what share of a can the best opener among these items leaves (1 = a can opener), -1 = nothing to open cans
	static float OpenKeep(array<ref DL_ItemData> items, float keep = -1)
	{
		foreach (DL_ItemData t : items)
		{
			if (!t || t.health == 0)
				continue;
			string path = "CfgVehicles " + t.type + " openItemSpillRange";
			if (!g_Game.ConfigIsExisting(path))
				continue;
			array<int> range = new array<int>;
			g_Game.ConfigGetIntArray(path, range);
			if (range.Count() < 2)
				continue;
			float k = 1 - (range[0] + range[1]) / 200.0;
			if (k > keep)
				keep = k;
		}
		return keep;
	}

	// the best opener of a whole party: everyone's gear + the vehicles' trunks
	static float PartyKeep(array<ref DL_PlayerData> people, array<DL_CarData> trunks)
	{
		float keep = -1;
		foreach (DL_PlayerData pd : people)
		{
			if (pd)
				keep = OpenKeep(pd.items, keep);
		}
		foreach (DL_CarData cd : trunks)
		{
			if (cd)
				keep = OpenKeep(cd.items, keep);
		}
		return keep;
	}

	// a closed can's food after opening it with keep: its opened type's full quantity x keep
	static float CanQuantity(string opened, float keep)
	{
		if (keep <= 0)
			return 0;
		float q = g_Game.ConfigGetFloat("CfgVehicles " + opened + " varQuantityInit");
		if (q <= 0)
			q = g_Game.ConfigGetFloat("CfgVehicles " + opened + " varQuantityMax");
		return q * keep;
	}

	static NutritionalProfile StomachProfile(DL_StomachItem s)
	{
		NutritionalProfile p = Liquid.GetNutritionalProfileByName(s.type);
		if (!p)
			p = Edible_Base.GetNutritionalProfile(null, s.type, s.stage);
		return p;
	}

	static bool IsFuel(DL_ItemData it)
	{
		return it && it.liquid == LIQUID_GASOLINE && it.quantity > 0;
	}

	// what one traveller (+ the car trunk, for the driver) has: food/water in stat units, canister fuel in litres
	// keep: the party's best can opener (PartyKeep), -2 = look in his own gear + this trunk only
	static void Count(DL_PlayerData d, DL_CarData car, out float food, out float water, out float canisterL, float keep = -2)
	{
		food = d.energy;
		water = d.water;
		canisterL = 0;
		foreach (DL_StomachItem s : d.stomach)
		{
			NutritionalProfile sp = StomachProfile(s);
			if (sp && s.amount > 0)
			{
				food += s.amount * sp.GetEnergy() / 100;
				water += s.amount * sp.GetWaterContent() / 100;
			}
		}
		array<ref DL_ItemData> lists = new array<ref DL_ItemData>;
		foreach (DL_ItemData a : d.items)
			lists.Insert(a);
		if (car)
		{
			foreach (DL_ItemData b : car.items)
				lists.Insert(b);
		}
		if (keep == -2)
			keep = OpenKeep(lists);
		foreach (DL_ItemData it : lists)
		{
			float e;
			float w;
			if (Nutrition(it, e, w))
			{
				food += it.quantity * e;
				water += it.quantity * w;
			}
			string opened = OpenedType(it);
			if (opened != "" && keep > 0)
			{
				float ce = Edible_Base.GetFoodEnergy(null, opened, 0) / 100;
				float cw = Edible_Base.GetFoodWater(null, opened, 0) / 100;
				float cq = CanQuantity(opened, keep);
				food += cq * ce;
				water += cq * cw;
			}
			if (IsFuel(it))
				canisterL += it.quantity / 1000;
		}
	}

	// closed cans among his gear (+ this trunk)
	static int ClosedCans(DL_PlayerData d, DL_CarData car)
	{
		int n = 0;
		foreach (DL_ItemData a : d.items)
		{
			if (OpenedType(a) != "")
				n++;
		}
		if (car)
		{
			foreach (DL_ItemData b : car.items)
			{
				if (OpenedType(b) != "")
					n++;
			}
		}
		return n;
	}

	// takes "units" of what is needed from one source; returns how many units it took
	static float Take(float have, float e, float w, inout float needFood, inout float needWater)
	{
		float units = 0;
		if (needFood > 0 && e > 0)
			units = Math.Max(units, needFood / e);
		if (needWater > 0 && w > 0)
			units = Math.Max(units, needWater / w);
		units = Math.Min(units, have);
		needFood = Math.Max(0, needFood - units * e);
		needWater = Math.Max(0, needWater - units * w);
		return units;
	}

	// eat on the road: the stomach first, then carried food/drink (his own, then the trunk), the bars last.
	// Changes the snapshots; returns what was eaten for the log.
	static string Eat(DL_PlayerData d, DL_CarData car, float needFood, float needWater)
	{
		float uf;
		float uw;
		return EatEx(d, car, needFood, needWater, uf, uw);
	}

	// Eat + what neither the supplies nor the bars could pay (unpaid > 0 only when the bars hit 0)
	static string EatEx(DL_PlayerData d, DL_CarData car, float needFood, float needWater, out float unpaidFood, out float unpaidWater)
	{
		array<DL_CarData> trunks = new array<DL_CarData>;
		if (car)
			trunks.Insert(car);
		return EatShared(d, trunks, needFood, needWater, unpaidFood, unpaidWater);
	}

	// as EatEx, from his own gear first and then from each of trunks in turn (a convoy shares its cars' trunks)
	// keep: the party's best can opener (PartyKeep), -2 = look in his own gear + these trunks only
	static string EatShared(DL_PlayerData d, array<DL_CarData> trunks, float needFood, float needWater, out float unpaidFood, out float unpaidWater, float keep = -2)
	{
		string eaten = "";
		if (keep == -2)
		{
			array<ref DL_PlayerData> me = new array<ref DL_PlayerData>;
			me.Insert(d);
			keep = PartyKeep(me, trunks);
		}
		foreach (DL_StomachItem s : d.stomach)
		{
			if (needFood <= 0 && needWater <= 0)
				break;
			NutritionalProfile sp = StomachProfile(s);
			if (!sp || s.amount <= 0)
				continue;
			s.amount -= Take(s.amount, sp.GetEnergy() / 100, sp.GetWaterContent() / 100, needFood, needWater);
		}
		array<ref array<ref DL_ItemData>> sources = new array<ref array<ref DL_ItemData>>;
		sources.Insert(d.items);
		foreach (DL_CarData tr : trunks)
		{
			if (tr)
				sources.Insert(tr.items);
		}
		foreach (array<ref DL_ItemData> list : sources)
		{
			for (int i = list.Count() - 1; i >= 0; i--)
			{
				if (needFood <= 0 && needWater <= 0)
					break;
				DL_ItemData it = list[i];
				float e;
				float w;
				string opened = OpenedType(it);
				if (opened != "" && keep > 0 && needFood > 0)
				{
					// opened now: it becomes the opened can, minus what the tool spilt
					it.type = opened;
					it.quantity = CanQuantity(opened, keep);
					it.foodStage = -1;
					if (eaten != "")
						eaten += ", ";
					eaten += "opened " + opened;
				}
				if (!Nutrition(it, e, w))
					continue;
				float units = Take(it.quantity, e, w, needFood, needWater);
				if (units <= 0)
					continue;
				it.quantity -= units;
				if (eaten != "")
					eaten += ", ";
				if (it.liquid != 0)
					eaten += it.type + " " + Math.Round(units).ToString() + " ml";
				else
					eaten += it.type + " " + Math.Round(units).ToString() + " g";
				// solid food that is all eaten is gone (nothing is attached to food); a bottle stays, empty
				if (it.liquid == 0 && it.quantity < 0.5)
					list.RemoveOrdered(i);
				else if (it.quantity < 0.5)
					it.quantity = 0;
			}
		}
		unpaidFood = Math.Max(0, needFood - d.energy); // what nothing could pay (weakened mode: hunger/thirst)
		unpaidWater = Math.Max(0, needWater - d.water);
		d.energy = Math.Max(0, d.energy - needFood);
		d.water = Math.Max(0, d.water - needWater);
		if (eaten == "")
			eaten = "nothing carried";
		return "ate " + eaten + ", bars took " + Math.Round(needFood).ToString() + " food / " + Math.Round(needWater).ToString() + " water";
	}

	// food + water for the trip: eaten on the road from the snapshot (stomach, carried food/drink, the trunk for the
	// driver = trunk != null), the bars pay the rest (never below 0). Returns what happened, for the log.
	static string ChargePlayer(DL_PlayerData d, DL_TravelPrice p, DL_CarData trunk)
	{
		if (!p || !d)
			return "";
		return Eat(d, trunk, p.foodKcal, p.waterMl);
	}

	// fuel + engine/wheel wear off the car's snapshot; the live car only gives the max health of its parts.
	static string ChargeCar(DL_CarData d, Transport car, DL_TravelPrice p, DL_PlayerData driver)
	{
		if (!p || !d || !car || !p.byCar)
			return "";
		string res = "";
		// fuel: fraction of the tank
		float cap = DL_Veh.Capacity(car, CarFluid.FUEL);
		if (cap <= 0 && d.fluidCaps.Count() > CarFluid.FUEL)
			cap = d.fluidCaps[CarFluid.FUEL];
		if (cap > 0 && d.fluids.Count() > CarFluid.FUEL)
		{
			float have = d.fluids[CarFluid.FUEL] * cap;
			float fromTank = Math.Min(have, p.fuelL);
			d.fluids[CarFluid.FUEL] = (have - fromTank) / cap;
			res += "fuel " + Math.Round(have).ToString() + " -> " + Math.Round(have - fromTank).ToString() + " L";
			// the rest from gasoline canisters: the trunk first, then what the driver carries
			float needL = p.fuelL - fromTank;
			float fromCans = 0;
			array<ref array<ref DL_ItemData>> cans = new array<ref array<ref DL_ItemData>>;
			cans.Insert(d.items);
			if (driver)
				cans.Insert(driver.items);
			foreach (array<ref DL_ItemData> list : cans)
			{
				foreach (DL_ItemData can : list)
				{
					if (needL <= 0)
						break;
					if (!IsFuel(can))
						continue;
					float takeL = Math.Min(needL, can.quantity / 1000);
					can.quantity -= takeL * 1000;
					needL -= takeL;
					fromCans += takeL;
				}
			}
			if (fromCans > 0)
				res += " + " + Math.Round(fromCans).ToString() + " L from canisters";
		}
		// engine: its damage zone
		float engineMax = car.GetMaxHealth("Engine", "Health");
		foreach (DL_Zone_Health zh : d.zones)
		{
			if (!zh)
				continue;
			string zn = zh.zone;
			zn.ToLower();
			if (zn == "engine" && engineMax > 0)
			{
				float before = zh.health;
				zh.health = Math.Max(0, zh.health - p.engineWearPct / 100.0 * engineMax);
				res += ", engine " + Math.Round(before / engineMax * 100).ToString() + " -> " + Math.Round(zh.health / engineMax * 100).ToString() + "%";
			}
		}
		// wheels: attachments of the car; match the snapshot item by its slot name
		int worn = 0;
		for (int i = 0; i < car.GetInventory().AttachmentCount(); i++)
		{
			EntityAI a = car.GetInventory().GetAttachmentFromIndex(i);
			if (!a || !a.IsInherited(CarWheel))
				continue;
			InventoryLocation il = new InventoryLocation();
			a.GetInventory().GetCurrentInventoryLocation(il);
			string slot = InventorySlots.GetSlotName(il.GetSlot());
			float wheelMax = a.GetMaxHealth("", ""); // same pair the snapshot reads (DL_Snapshot: GetHealth("", ""))
			foreach (DL_ItemData it : d.items)
			{
				if (it && it.parent == 0 && it.slot == slot && it.health >= 0 && wheelMax > 0)
				{
					it.health = Math.Max(0, it.health - p.wheelWearPct / 100.0 * wheelMax);
					worn++;
				}
			}
		}
		res += ", " + worn.ToString() + " wheels worn " + Math.Round(p.wheelWearPct).ToString() + "%";
		return res;
	}
}
