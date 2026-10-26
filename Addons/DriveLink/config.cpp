// DriveLink - walk (and later drive) between DayZ servers/maps. One mod for server AND clients (-mod=).
// Server logic only runs on the dedicated server (MissionServer); clients only handle redirect + countdown.
class CfgPatches
{
	class DriveLink
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data", "DZ_Scripts"};
	};
};

class CfgMods
{
	class DriveLink
	{
		dir = "DriveLink";
		name = "DriveLink";
		author = "Derk";
		version = "0.4.9-rc1";
		type = "mod";
		dependencies[] = {"Game", "World", "Mission"};
		class defs
		{
			class gameScriptModule  { value = ""; files[] = {"DriveLink/Scripts/3_Game"}; };
			class worldScriptModule { value = ""; files[] = {"DriveLink/Scripts/4_World"}; };
			class missionScriptModule { value = ""; files[] = {"DriveLink/Scripts/5_Mission"}; };
		};
	};
};
