class CfgPatches
{
    class DriveLinkRestart
    {
        units[]={}; weapons[]={}; requiredVersion=0.1;
        requiredAddons[]={"DriveLink"};
    };
};
class CfgMods
{
    class DriveLinkRestart
    {
        type="mod"; dependencies[]={"Game","World","Mission"};
        class defs
        {
            class gameScriptModule { value=""; files[]={"DriveLinkRestart/Scripts/3_Game"}; };
            class worldScriptModule { value=""; files[]={"DriveLinkRestart/Scripts/4_World"}; };
            class missionScriptModule { value=""; files[]={"DriveLinkRestart/Scripts/5_Mission"}; };
        };
    };
};
