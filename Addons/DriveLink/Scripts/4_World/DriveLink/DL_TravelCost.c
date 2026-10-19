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
	float foodPct;            // per person, % of a full bar
	float waterPct;
	float fuelL;              // whole car
	float engineWearPct;      // % of the engine's max health
	float wheelWearPct;       // % of each wheel's max health
	int seconds;              // on the road (travel time on), 0 = instant

	string Text()
	{
		string s = Math.Round(km).ToString() + " km: " + Math.Round(foodPct).ToString() + "% food, " + Math.Round(waterPct).ToString() + "% water";
		if (byCar)
			s += " each, " + Math.Round(fuelL).ToString() + " L fuel";
		if (seconds > 0)
			s += ", about " + DL_TravelCost.Duration(seconds);
		return s;
	}
}

class DL_TravelCost
{
	// null = free (no distance on the zone, or no rules)
	static DL_TravelPrice Price(DL_TravelRules t, DL_Zone z, bool byCar)
	{
		if (!t || !z || z.distanceKm <= 0)
			return null;
		DL_TravelPrice p = new DL_TravelPrice();
		p.km = z.distanceKm;
		p.byCar = byCar;
		float k = z.distanceKm / 10.0;
		if (byCar)
		{
			p.foodPct = t.foodCarPct * k;
			p.waterPct = t.waterCarPct * k;
			p.fuelL = t.fuelLiters * k;
			p.engineWearPct = t.wearEnginePct * k;
			p.wheelWearPct = t.wearWheelsPct * k;
		}
		else
		{
			p.foodPct = t.foodFootPct * k;
			p.waterPct = t.waterFootPct * k;
		}
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

	static float FoodPctOf(PlayerBase pb)
	{
		return pb.GetStatEnergy().Get() / PlayerConstants.SL_ENERGY_MAX * 100.0;
	}

	static float WaterPctOf(PlayerBase pb)
	{
		return pb.GetStatWater().Get() / PlayerConstants.SL_WATER_MAX * 100.0;
	}

	static float FuelLOf(CarScript car)
	{
		return car.GetFluidFraction(CarFluid.FUEL) * car.GetFluidCapacity(CarFluid.FUEL);
	}

	// "" = everyone can pay; else what is missing, per person: "Britney: water 12% of 30%; car: fuel 4 L of 30 L"
	static string Shortfall(DL_TravelPrice p, array<PlayerBase> crew, CarScript car)
	{
		if (!p)
			return "";
		array<string> miss = new array<string>;
		foreach (PlayerBase pb : crew)
		{
			if (!pb)
				continue;
			string who = "you";
			if (crew.Count() > 1 && pb.GetIdentity())
				who = pb.GetIdentity().GetName();
			string m = "";
			float food = FoodPctOf(pb);
			float water = WaterPctOf(pb);
			if (food < p.foodPct)
				m += "food " + Math.Floor(food).ToString() + "% of " + Math.Ceil(p.foodPct).ToString() + "%";
			if (water < p.waterPct)
			{
				if (m != "")
					m += ", ";
				m += "water " + Math.Floor(water).ToString() + "% of " + Math.Ceil(p.waterPct).ToString() + "%";
			}
			if (m != "")
				miss.Insert(who + ": " + m);
		}
		if (car && p.byCar)
		{
			float fuel = FuelLOf(car);
			if (fuel < p.fuelL)
				miss.Insert("car: fuel " + Math.Floor(fuel).ToString() + " L of " + Math.Ceil(p.fuelL).ToString() + " L");
		}
		string res = "";
		foreach (string s : miss)
		{
			if (res != "")
				res += "; ";
			res += s;
		}
		return res;
	}

	// food + water off one traveller's snapshot (never below 0). Returns what was taken, for the log.
	static string ChargePlayer(DL_PlayerData d, DL_TravelPrice p)
	{
		if (!p || !d)
			return "";
		float food = p.foodPct / 100.0 * PlayerConstants.SL_ENERGY_MAX;
		float water = p.waterPct / 100.0 * PlayerConstants.SL_WATER_MAX;
		d.energy = Math.Max(0, d.energy - food);
		d.water = Math.Max(0, d.water - water);
		return "charged " + Math.Round(food).ToString() + " food, " + Math.Round(water).ToString() + " water (left " + Math.Round(d.energy).ToString() + " / " + Math.Round(d.water).ToString() + ")";
	}

	// fuel + engine/wheel wear off the car's snapshot; the live car only gives the max health of its parts.
	static string ChargeCar(DL_CarData d, CarScript car, DL_TravelPrice p)
	{
		if (!p || !d || !car || !p.byCar)
			return "";
		string res = "";
		// fuel: fraction of the tank
		float cap = car.GetFluidCapacity(CarFluid.FUEL);
		if (cap <= 0 && d.fluidCaps.Count() > CarFluid.FUEL)
			cap = d.fluidCaps[CarFluid.FUEL];
		if (cap > 0 && d.fluids.Count() > CarFluid.FUEL)
		{
			float have = d.fluids[CarFluid.FUEL] * cap;
			float left = Math.Max(0, have - p.fuelL);
			d.fluids[CarFluid.FUEL] = left / cap;
			res += "fuel " + Math.Round(have).ToString() + " -> " + Math.Round(left).ToString() + " L";
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
