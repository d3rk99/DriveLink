// DriveLink - after the redirect disconnect the client lands in the main menu; connect to the target from there.
// MissionMainMenu.m_NoCutscene skips the intro scene (missionmainmenu.c:8-14); m_mainmenu is private there, so we
// pass the menu that is open now (UIManager.GetMenu) as Connect's parent.
modded class MissionMainMenu
{
	override void OnInit()
	{
		if (GetDayZGame().DL_HasPending())
			m_NoCutscene = true;
		super.OnInit();
	}

	override void OnMissionStart()
	{
		super.OnMissionStart();
		if (GetDayZGame().DL_HasPending())
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_Go, 200, false);
	}

	protected void DL_Go()
	{
		GetDayZGame().DL_Reconnect(g_Game.GetUIManager().GetMenu());
	}
}
