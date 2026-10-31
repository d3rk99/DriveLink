// DriveLink main menu scene: a character of the menu's offline world sits down in a vehicle there. Same vehicle
// command as the arrival seating (DL_PlayerSeat.c), started from the command handler, but local: no juncture, no
// server. Spike 2026-10-30 (Research/main-menu-scene.md): both seats taken within 2 ticks.
modded class PlayerBase
{
	protected Transport m_DL_MenuSeatIn;
	protected int m_DL_MenuSeatIdx = -1;
	protected int m_DL_MenuSeatTries;

	void DL_MenuSeat(Transport t, int idx)
	{
		m_DL_MenuSeatIn = t;
		m_DL_MenuSeatIdx = idx;
		m_DL_MenuSeatTries = 0;
	}

	// The menu scene's walkers: the scene says where each one heads (yaw, degrees) and how fast (0 idle, 1 walk,
	// 2 jog); the heading model turns him that way, the movement override walks him forward. His own animation,
	// footsteps and the ground under his feet, like a player walking.
	protected bool m_DL_MenuWalk;
	protected float m_DL_MenuWalkYaw;
	protected float m_DL_MenuWalkSpeed;

	void DL_MenuWalk(float yaw, float speed)
	{
		m_DL_MenuWalk = true;
		m_DL_MenuWalkYaw = yaw;
		m_DL_MenuWalkSpeed = speed;
	}

	override bool HeadingModel(float pDt, SDayZPlayerHeadingModel pModel)
	{
		if (!m_DL_MenuWalk)
			return super.HeadingModel(pDt, pModel);
		// heading is in radians, the same way round as the yaw here (negated, the group walked off 46 degrees to the
		// left of a road heading 23); turned at most ~120 degrees a second
		float want = m_DL_MenuWalkYaw * Math.DEG2RAD;
		float diff = want - pModel.m_fHeadingAngle;
		while (diff > Math.PI)
			diff -= Math.PI2;
		while (diff < -Math.PI)
			diff += Math.PI2;
		float maxTurn = 2.1 * pDt;
		pModel.m_fHeadingAngle = pModel.m_fHeadingAngle + Math.Clamp(diff, -maxTurn, maxTurn);
		return true;
	}

	override void CommandHandler(float pDt, int pCurrentCommandID, bool pCurrentCommandFinished)
	{
		super.CommandHandler(pDt, pCurrentCommandID, pCurrentCommandFinished);
		if (m_DL_MenuWalk && GetInputController())
		{
			GetInputController().OverrideMovementSpeed(HumanInputControllerOverrideType.ENABLED, m_DL_MenuWalkSpeed);
			GetInputController().OverrideMovementAngle(HumanInputControllerOverrideType.ENABLED, 0);
		}
		if (!m_DL_MenuSeatIn)
			return;
		Transport t = m_DL_MenuSeatIn;
		HumanCommandVehicle cur = GetCommand_Vehicle();
		m_DL_MenuSeatTries++;
		if ((cur && cur.GetTransport() == t) || m_DL_MenuSeatTries > 200)
		{
			if (!cur)
				DL_Log.Warn("menu scene: " + GetType() + " could not sit in the " + t.GetType());
			m_DL_MenuSeatIn = null;
			return;
		}
		HumanCommandVehicle vc = StartCommand_Vehicle(t, m_DL_MenuSeatIdx, t.GetSeatAnimationType(m_DL_MenuSeatIdx));
		if (vc)
			vc.SetVehicleType(t.GetAnimInstance());
	}
}

// The menu scene drives its cars: the controls only take when set in OnInput, after the input step (the seated
// driver's own input overwrites them otherwise; vanilla transport.c OnInput: "store state of custom inputs elsewhere
// and call Setters here"). DL_MenuDrive stores them, OnInput applies them.
modded class CarScript
{
	protected bool m_DL_MenuDriven;
	protected float m_DL_MenuThrottle;
	protected float m_DL_MenuSteer;
	protected float m_DL_MenuBrake;

	void DL_MenuDrive(float throttle, float steer, float brake)
	{
		m_DL_MenuDriven = true;
		m_DL_MenuThrottle = throttle;
		m_DL_MenuSteer = steer;
		m_DL_MenuBrake = brake;
	}

	override void OnInput(float dt)
	{
		super.OnInput(dt);
		if (!m_DL_MenuDriven)
			return;
		SetThrottle(m_DL_MenuThrottle);
		SetSteering(m_DL_MenuSteer);
		SetBrake(m_DL_MenuBrake);
		SetHandbrake(0);
		SetBrakesActivateWithoutDriver(false);
	}
}

modded class MotorbikeScript
{
	protected bool m_DL_MenuDriven;
	protected float m_DL_MenuThrottle;
	protected float m_DL_MenuSteer;
	protected float m_DL_MenuBrake;

	void DL_MenuDrive(float throttle, float steer, float brake)
	{
		m_DL_MenuDriven = true;
		m_DL_MenuThrottle = throttle;
		m_DL_MenuSteer = steer;
		m_DL_MenuBrake = brake;
	}

	override void OnInput(float dt)
	{
		super.OnInput(dt);
		if (!m_DL_MenuDriven)
			return;
		SetThrottle(m_DL_MenuThrottle);
		SetSteering(m_DL_MenuSteer);
		SetBrake(m_DL_MenuBrake);
		SetHandbrake(0);
		SetKickstand(0);
	}
}

modded class BoatScript
{
	protected bool m_DL_MenuDriven;
	protected float m_DL_MenuThrottle;
	protected float m_DL_MenuSteer;

	void DL_MenuDrive(float throttle, float steer)
	{
		m_DL_MenuDriven = true;
		m_DL_MenuThrottle = throttle;
		m_DL_MenuSteer = steer;
	}

	override void OnInput(float dt)
	{
		super.OnInput(dt);
		if (!m_DL_MenuDriven)
			return;
		SetThrottle(m_DL_MenuThrottle);
		SetSteering(m_DL_MenuSteer);
		SetClutch(0);                                // engaged: the propeller turns with the engine
		if (GetGear() <= GetNeutralGear())
			ShiftTo(GetNeutralGear() + 1);
	}
}

// The menu convoy drives cars and motorbikes alike: the same controls, two classes (Car, Motorbike).
class DL_V
{
	static float Speed(Transport t)      // km/h
	{
		Car c = Car.Cast(t);
		if (c)
			return c.GetSpeedometerAbsolute();
		Motorbike m = Motorbike.Cast(t);
		if (m)
			return m.GetSpeedometerAbsolute();
		return GetVelocity(t).Length() * 3.6;
	}
	static bool EngineOn(Transport t)    { return DL_Veh.EngineIsOn(t); }
	static void EngineStart(Transport t) { DL_Veh.EngineStart(t); }
	static float Rpm(Transport t)
	{
		Car c = Car.Cast(t);
		if (c)
			return c.EngineGetRPM();
		Motorbike m = Motorbike.Cast(t);
		if (m)
			return m.EngineGetRPM();
		return 0;
	}
	static float RpmIdle(Transport t)
	{
		Car c = Car.Cast(t);
		if (c)
			return c.EngineGetRPMIdle();
		Motorbike m = Motorbike.Cast(t);
		if (m)
			return m.EngineGetRPMIdle();
		return 0;
	}
	static float RpmRedline(Transport t)
	{
		Car c = Car.Cast(t);
		if (c)
			return c.EngineGetRPMRedline();
		Motorbike m = Motorbike.Cast(t);
		if (m)
			return m.EngineGetRPMRedline();
		return 1;
	}
	static int Gear(Transport t)
	{
		Car c = Car.Cast(t);
		if (c)
			return c.GetGear();
		Motorbike m = Motorbike.Cast(t);
		if (m)
			return m.GetGear();
		return 0;
	}
	static int NeutralGear(Transport t)
	{
		Car c = Car.Cast(t);
		if (c)
			return c.GetNeutralGear();
		Motorbike m = Motorbike.Cast(t);
		if (m)
			return m.GetNeutralGear();
		return 0;
	}
	static int GearCount(Transport t)
	{
		Car c = Car.Cast(t);
		if (c)
			return c.GetGearCount();
		Motorbike m = Motorbike.Cast(t);
		if (m)
			return m.GetGearCount();
		return 0;
	}
	static bool Manual(Transport t)
	{
		Car c = Car.Cast(t);
		if (c)
			return c.GearboxGetType() == CarGearboxType.MANUAL;
		Motorbike m = Motorbike.Cast(t);
		if (m)
			return m.GearboxGetType() == CarGearboxType.MANUAL;
		return false;
	}
	static void ShiftUp(Transport t)
	{
		Car c = Car.Cast(t);
		if (c)
			c.ShiftUp();
		Motorbike m = Motorbike.Cast(t);
		if (m)
			m.ShiftUp();
	}
	static void ShiftDown(Transport t)
	{
		Car c = Car.Cast(t);
		if (c)
			c.ShiftDown();
		Motorbike m = Motorbike.Cast(t);
		if (m)
			m.ShiftDown();
	}
	static void ShiftTo(Transport t, int gear)
	{
		Car c = Car.Cast(t);
		if (c)
			c.ShiftTo(gear);
		Motorbike m = Motorbike.Cast(t);
		if (m)
			m.ShiftTo(gear);
	}
	static void Drive(Transport t, float throttle, float steer, float brake)
	{
		CarScript cs = CarScript.Cast(t);
		if (cs)
			cs.DL_MenuDrive(throttle, steer, brake);
		MotorbikeScript ms = MotorbikeScript.Cast(t);
		if (ms)
			ms.DL_MenuDrive(throttle, steer, brake);
		BoatScript bs = BoatScript.Cast(t);
		if (bs)
			bs.DL_MenuDrive(throttle, steer);
	}
	// fluids full, brakes and kickstand off, engine on, first gear
	static void Prepare(Transport t)
	{
		Car c = Car.Cast(t);
		if (c)
		{
			c.Fill(CarFluid.FUEL, c.GetFluidCapacity(CarFluid.FUEL));
			c.Fill(CarFluid.COOLANT, c.GetFluidCapacity(CarFluid.COOLANT));
			c.Fill(CarFluid.OIL, c.GetFluidCapacity(CarFluid.OIL));
			c.Fill(CarFluid.BRAKE, c.GetFluidCapacity(CarFluid.BRAKE));
			c.SetBrakesActivateWithoutDriver(false);
			c.SetHandbrake(0);
			c.SetBrake(0);
		}
		Boat b = Boat.Cast(t);
		if (b)
		{
			b.Fill(BoatFluid.FUEL, b.GetFluidCapacity(BoatFluid.FUEL));
			b.EngineStart();
			b.ShiftTo(b.GetNeutralGear() + 1);
			return;
		}
		Motorbike m = Motorbike.Cast(t);
		if (m)
		{
			m.Fill(MotorbikeFluid.FUEL, m.GetFluidCapacity(MotorbikeFluid.FUEL));
			m.SetKickstand(0);
			m.SetHandbrake(0);
			m.SetBrake(0);
		}
		EngineStart(t);
		ShiftTo(t, NeutralGear(t) + 1);
	}
}
