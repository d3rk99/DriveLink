// DriveLink M2 - a car that left for another server must never come back here.
// The source deletes the car once the hub has the trip, but the game only writes its world storage now and then. A
// crash between the delete and the next save would load the old car again = the car exists twice. So the car's
// persistent id (EntityAI.GetPersistentID, entityai.c:3374 in 1.30) is written to $profile:DriveLink/departed-cars.json
// BEFORE it is deleted, and a car loaded from storage with a listed id deletes itself (AfterStoreLoad, entityai.c:1419).

class DL_DepartedCars
{
	ref array<string> ids = new array<string>;
}

class DL_CarGuard
{
	static const string FILE = "$profile:DriveLink/departed-cars.json";
	static const int KEEP = 500;
	protected static ref DL_DepartedCars s_List;

	static string IdOf(EntityAI e)
	{
		int b1;
		int b2;
		int b3;
		int b4;
		e.GetPersistentID(b1, b2, b3, b4);
		if (b1 == 0 && b2 == 0 && b3 == 0 && b4 == 0)
			return "";
		return b1.ToString() + "-" + b2.ToString() + "-" + b3.ToString() + "-" + b4.ToString();
	}

	protected static DL_DepartedCars List()
	{
		if (s_List)
			return s_List;
		string err;
		if (FileExist(FILE))
			JsonFileLoader<DL_DepartedCars>.LoadFile(FILE, s_List, err);
		if (!s_List)
			s_List = new DL_DepartedCars();
		return s_List;
	}

	// call BEFORE deleting the departing car
	static void Remember(EntityAI car)
	{
		string id = IdOf(car);
		if (id == "")
		{
			DL_Log.Info("car " + car.GetType() + " has no persistent id yet (never saved), nothing to remember");
			return;
		}
		DL_DepartedCars l = List();
		if (l.ids.Find(id) < 0)
			l.ids.Insert(id);
		while (l.ids.Count() > KEEP)
			l.ids.RemoveOrdered(0);
		string err;
		MakeDirectory(DL_Const.DIR);
		if (!JsonFileLoader<DL_DepartedCars>.SaveFile(FILE, l, err))
			DL_Log.Warn("WARNING could not write " + FILE + ": " + err);
	}

	static bool HasLeft(EntityAI car)
	{
		string id = IdOf(car);
		return id != "" && List().ids.Find(id) >= 0;
	}
}

modded class CarScript
{
	override void AfterStoreLoad()
	{
		super.AfterStoreLoad();
		if (g_Game.IsServer() && DL_CarGuard.HasLeft(this))
		{
			DL_Log.Info("car " + GetType() + " " + DL_CarGuard.IdOf(this) + " was driven to another server, removing the stale copy loaded from storage");
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_DeleteSelf, 1000, false); // a proto native (Delete) cannot go into the call queue
		}
	}

	void DL_DeleteSelf()
	{
		Delete();
	}
}

modded class MotorbikeScript
{
	override void AfterStoreLoad()
	{
		super.AfterStoreLoad();
		if (g_Game.IsServer() && DL_CarGuard.HasLeft(this))
		{
			DL_Log.Info("car " + GetType() + " " + DL_CarGuard.IdOf(this) + " was driven to another server, removing the stale copy loaded from storage");
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_DeleteSelf, 1000, false); // a proto native (Delete) cannot go into the call queue
		}
	}

	void DL_DeleteSelf()
	{
		Delete();
	}
}

modded class BoatScript
{
	override void AfterStoreLoad()
	{
		super.AfterStoreLoad();
		if (g_Game.IsServer() && DL_CarGuard.HasLeft(this))
		{
			DL_Log.Info("car " + GetType() + " " + DL_CarGuard.IdOf(this) + " was driven to another server, removing the stale copy loaded from storage");
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_DeleteSelf, 1000, false); // a proto native (Delete) cannot go into the call queue
		}
	}

	void DL_DeleteSelf()
	{
		Delete();
	}
}
