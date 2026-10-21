// DriveLink M2 - a car's whole state <-> DL_CarData. Server only. Research/R2-cars.md.
// Engine calls (vanilla scripts in C:\DayZMods\PDrive\scripts):
//   Car.GetFluidCapacity/GetFluidFraction/LeakAll/Fill(CarFluid)  car.c:359-376;  CarFluid FUEL..USER4  car.c:18-29
//   DamageSystem.GetDamageZoneMap  damagesystem.c:27 (same as the player zones)
//   parts + trunk = attachments + cargo -> DL_Snapshot.CaptureTree/ApplyTree with the car as root
//   CGame.CreateObjectEx(type, pos, ECE_*)  ECE flags centraleconomy.c:7-40

class DL_CarSnapshot
{
	static const int FLUIDS = 8; // CarFluid has 8 values (FUEL, OIL, BRAKE, COOLANT, USER1..USER4)
	protected ref DL_Snapshot m_Items = new DL_Snapshot();

	DL_CarData Capture(Transport car)
	{
		DL_CarData d = new DL_CarData();
		d.type = car.GetType();
		d.pos = car.GetPosition();
		d.ori = car.GetOrientation();
		d.health = car.GetHealth("", "Health");
		d.vel = GetVelocity(car);
		for (int f = 0; f < FLUIDS; f++)
		{
			d.fluids.Insert(DL_Veh.Fraction(car, f));
			d.fluidCaps.Insert(DL_Veh.Capacity(car, f));
		}
		DamageZoneMap zones = new DamageZoneMap;
		if (DamageSystem.GetDamageZoneMap(car, zones))
		{
			for (int i = 0; i < zones.Count(); i++)
			{
				DL_Zone_Health z = new DL_Zone_Health();
				z.zone = zones.GetKey(i);
				z.health = car.GetHealth(z.zone, "Health");
				d.zones.Insert(z);
			}
		}
		m_Items.CaptureTree(car, null, null, d.items);
		return d;
	}

	// A NEW car of d.type at pos/ori, with every part, the trunk, fluids and damage of the snapshot.
	// made/dropped as in DL_Snapshot.ApplyTree (dropped = put on the ground next to the car, no room).
	Transport Build(DL_CarData d, vector pos, vector ori, out int made, out int dropped)
	{
		int flags = ECE_SETUP | ECE_UPDATEPATHGRAPH | ECE_CREATEPHYSICS | ECE_TRACE;
		if (DL_Veh.KindOf(d.type) == "boat")
			flags = ECE_SETUP | ECE_UPDATEPATHGRAPH | ECE_CREATEPHYSICS; // floats where it is put (a trace = the sea floor)
		Transport car = Transport.Cast(g_Game.CreateObjectEx(d.type, pos, flags));
		if (!car)
			return null;
		car.SetOrientation(ori);
		made = m_Items.ApplyTree(car, null, d.items, dropped);
		for (int f = 0; f < FLUIDS && f < d.fluids.Count(); f++)
		{
			float cap = DL_Veh.Capacity(car, f);
			if (cap <= 0 && f < d.fluidCaps.Count())
				cap = d.fluidCaps[f];
			DL_Veh.LeakAll(car, f);
			if (cap > 0 && d.fluids[f] > 0)
				DL_Veh.Fill(car, f, d.fluids[f] * cap);
		}
		// spike 2026-10-18: GetFluidFraction read 0 for every fluid right after this, but in game the car had its
		// fuel and started: the values show up later. Log them 1 s later.
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(LogFluids, 1000, false, car);
		if (d.health >= 0)
			car.SetHealth("", "Health", d.health);
		for (int z = 0; z < d.zones.Count(); z++)
		{
			if (d.zones[z])
				car.SetHealth(d.zones[z].zone, "Health", d.zones[z].health);
		}
		car.SetSynchDirty();
		return car;
	}

	void LogFluids(Transport car)
	{
		if (car)
			DL_Log.Detail("car fluids of the rebuilt " + car.GetType() + " 1 s later: " + FluidText(FluidsOf(car)));
	}

	// "fuel 0.80 oil 1.00 brake 1.00 coolant 0.50" for the log
	static string FluidText(array<float> fr)
	{
		string s = "";
		array<string> names = {"fuel", "oil", "brake", "coolant"};
		for (int i = 0; i < names.Count() && i < fr.Count(); i++)
			s += names[i] + " " + fr[i].ToString() + " ";
		return s;
	}

	static array<float> FluidsOf(Transport car)
	{
		array<float> fr = new array<float>;
		for (int f = 0; f < FLUIDS; f++)
			fr.Insert(DL_Veh.Fraction(car, f));
		return fr;
	}
}
