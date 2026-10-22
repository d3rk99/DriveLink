class CfgPatches
{
	class DriveLink_ZenLogin
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DriveLink", "ZenImmersiveLogin"};
	};
};
class CfgMods
{
	class DriveLink_ZenLogin
	{
		type = "mod";
		dependencies[] = {"World", "Mission"};
		class defs
		{
			class worldScriptModule { value = ""; files[] = {"DriveLink_ZenLogin/Scripts/4_World"}; };
			class missionScriptModule { value = ""; files[] = {"DriveLink_ZenLogin/Scripts/5_Mission"}; };
		};
	};
};
