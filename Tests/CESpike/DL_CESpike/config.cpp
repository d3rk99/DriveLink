// DriveLink CE spike - server-only test addon (-serverMod=). Not part of the mod; see Tests/CESpike/README.md.
class CfgPatches
{
	class DL_CESpike
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data", "DZ_Scripts"};
	};
};

class CfgMods
{
	class DL_CESpike
	{
		dir = "DL_CESpike";
		name = "DriveLink CE spike";
		author = "Derk";
		type = "mod";
		dependencies[] = {"World", "Mission"};
		class defs
		{
			class worldScriptModule { value = ""; files[] = {"DL_CESpike/Scripts/4_World"}; };
			class missionScriptModule { value = ""; files[] = {"DL_CESpike/Scripts/5_Mission"}; };
		};
	};
};
