// DriveLink's log (script log of the server / client), by level: config.json "logLevel" on the server
//   0 = warnings and errors only, 1 = + trips, arrivals, deaths, hub contact (default), 2 = + every detail
// (waiting rooms, seating, films, spots, each hub call). Clients log details only in a dev build (DL_DEV).
class DL_Log
{
	static int s_Level = 1;

	static void Warn(string m)
	{
		Print(DL_Const.LOG_TAG + m);
	}

	static void Info(string m)
	{
		if (s_Level >= 1)
			Print(DL_Const.LOG_TAG + m);
	}

	static void Detail(string m)
	{
#ifdef DL_DEV
		Print(DL_Const.LOG_TAG + m);
#else
		if (s_Level >= 2)
			Print(DL_Const.LOG_TAG + m);
#endif
	}
}
