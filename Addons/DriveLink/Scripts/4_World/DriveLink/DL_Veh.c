// DriveLink - every vehicle that can travel: cars and trucks (Car / CarScript), motorbikes (Motorbike /
// MotorbikeScript, DayZ 1.30: Motorbike_01, Motorbike_02) and boats (Boat / BoatScript: Boat_01). They are all a
// Transport (crew, seats, inventory), but engine and fluid calls are on each kind (car.c, motorbike.c, boat.c in
// 3_game/vehicles; the fluids are the same ETransportFluid list everywhere: FUEL, OIL, BRAKE, COOLANT, USER1-4).
// Helicopters are not travel vehicles.
// Planes (optional: LM's Planes, workshop 3639695989, by Llama): known by class name only, so DriveLink builds and
// runs the same without that mod; they travel through plane zones (DL_Zone.air) only, in flight.

class DL_Veh
{
	// the aircraft of LM's Planes: its vehicles are all CarScripts named LM_... (Cessna180, Tigermoth (MK2, MK3),
	// Spitfire, Catalina, DC_3, Patty_Wagon, StuntPlane, Z37_Bumblebee and their variants); its other LM_ things
	// (tracers, the hangar) are not CarScripts
	static bool IsPlaneType(string type)
	{
		string t = type;
		t.ToLower();
		return t.IndexOf("lm_") == 0 && g_Game.IsKindOf(type, "CarScript");
	}

	static bool IsPlane(EntityAI t)
	{
		return t && Transport.Cast(t) && IsPlaneType(t.GetType());
	}

	// a plane in the air: more than 15 m above the ground (or the sea)
	static bool Flying(EntityAI t)
	{
		if (!IsPlane(t))
			return false;
		vector p = t.GetPosition();
		float below = g_Game.SurfaceY(p[0], p[2]);
		if (g_Game.SurfaceIsSea(p[0], p[2]))
			below = Math.Max(below, g_Game.SurfaceGetSeaLevel());
		return p[1] - below > 15;
	}

	// "car" | "bike" | "boat" | "plane" | "" (not a travel vehicle)
	static string Kind(EntityAI t)
	{
		if (IsPlane(t))
			return "plane";
		if (Boat.Cast(t))
			return "boat";
		if (Motorbike.Cast(t))
			return "bike";
		if (Car.Cast(t))
			return "car";
		return "";
	}

	static bool IsBoat(EntityAI t)
	{
		return Boat.Cast(t) != null;
	}

	// a class name's kind (snapshots: DL_CarData.type)
	static string KindOf(string type)
	{
		if (IsPlaneType(type))
			return "plane";
		if (g_Game.IsKindOf(type, "Boat"))
			return "boat";
		if (g_Game.IsKindOf(type, "Motorbike"))
			return "bike";
		return "car";
	}

	// the travel vehicle he sits in (any seat), or null - helicopters don't count
	static Transport VehicleOf(PlayerBase pb)
	{
		if (!pb)
			return null;
		HumanCommandVehicle vc = pb.GetCommand_Vehicle();
		if (!vc)
			return null;
		Transport t = vc.GetTransport();
		if (Kind(t) == "")
			return null;
		return t;
	}

	static float Fraction(Transport t, int fluid)
	{
		Car c = Car.Cast(t);
		if (c)
			return c.GetFluidFraction(fluid);
		Motorbike m = Motorbike.Cast(t);
		if (m)
			return m.GetFluidFraction(fluid);
		Boat b = Boat.Cast(t);
		if (b)
			return b.GetFluidFraction(fluid);
		return 0;
	}

	static float Capacity(Transport t, int fluid)
	{
		Car c = Car.Cast(t);
		if (c)
			return c.GetFluidCapacity(fluid);
		Motorbike m = Motorbike.Cast(t);
		if (m)
			return m.GetFluidCapacity(fluid);
		Boat b = Boat.Cast(t);
		if (b)
			return b.GetFluidCapacity(fluid);
		return 0;
	}

	static void LeakAll(Transport t, int fluid)
	{
		Car c = Car.Cast(t);
		if (c)
		{
			c.LeakAll(fluid);
			return;
		}
		Motorbike m = Motorbike.Cast(t);
		if (m)
		{
			m.LeakAll(fluid);
			return;
		}
		Boat b = Boat.Cast(t);
		if (b)
			b.LeakAll(fluid);
	}

	static void Fill(Transport t, int fluid, float amount)
	{
		Car c = Car.Cast(t);
		if (c)
		{
			c.Fill(fluid, amount);
			return;
		}
		Motorbike m = Motorbike.Cast(t);
		if (m)
		{
			m.Fill(fluid, amount);
			return;
		}
		Boat b = Boat.Cast(t);
		if (b)
			b.Fill(fluid, amount);
	}

	static void EngineStart(Transport t)
	{
		Car c = Car.Cast(t);
		if (c)
		{
			c.EngineStart();
			return;
		}
		Motorbike m = Motorbike.Cast(t);
		if (m)
		{
			m.EngineStart();
			return;
		}
		Boat b = Boat.Cast(t);
		if (b)
			b.EngineStart();
	}

	static void EngineStop(Transport t)
	{
		Car c = Car.Cast(t);
		if (c)
		{
			c.EngineStop();
			return;
		}
		Motorbike m = Motorbike.Cast(t);
		if (m)
		{
			m.EngineStop();
			return;
		}
		Boat b = Boat.Cast(t);
		if (b)
			b.EngineStop();
	}

	static bool EngineIsOn(Transport t)
	{
		Car c = Car.Cast(t);
		if (c)
			return c.EngineIsOn();
		Motorbike m = Motorbike.Cast(t);
		if (m)
			return m.EngineIsOn();
		Boat b = Boat.Cast(t);
		if (b)
			return b.EngineIsOn();
		return false;
	}

	// keep it still while the hub answers (boats have no brake)
	static void Handbrake(Transport t, float v)
	{
		Car c = Car.Cast(t);
		if (c)
		{
			c.SetHandbrake(v);
			return;
		}
		Motorbike m = Motorbike.Cast(t);
		if (m)
			m.SetHandbrake(v);
	}

	// invisible to everyone while it is built and seated (DL_Hidden.c)
	static void SetHidden(Transport t, bool on)
	{
		CarScript c = CarScript.Cast(t);
		if (c)
		{
			c.DL_SetHidden(on);
			return;
		}
		MotorbikeScript m = MotorbikeScript.Cast(t);
		if (m)
		{
			m.DL_SetHidden(on);
			return;
		}
		BoatScript b = BoatScript.Cast(t);
		if (b)
			b.DL_SetHidden(on);
	}

	// ---------------- water (boats) ----------------

	// the water surface here, or -1 = no water at least minDepth deep (sea: the sea level; ponds/rivers: ground + depth)
	static float WaterY(float x, float z, float minDepth)
	{
		float ground = g_Game.SurfaceY(x, z);
		float level = -1;
		if (g_Game.SurfaceIsSea(x, z))
			level = g_Game.SurfaceGetSeaLevel();
		else if (g_Game.SurfaceIsPond(x, z))
			level = ground + g_Game.GetWaterDepth(Vector(x, ground, z));
		if (level < 0 || level - ground < minDepth)
			return -1;
		return level;
	}
}
