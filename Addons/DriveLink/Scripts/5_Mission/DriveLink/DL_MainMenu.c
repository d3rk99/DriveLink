// DriveLink - after the redirect disconnect the client lands in the main menu; connect to the target from there.
// MissionMainMenu.m_NoCutscene skips the intro scene (missionmainmenu.c:8-14); m_mainmenu is private there, so we
// pass the menu that is open now (UIManager.GetMenu) as Connect's parent.
// We wait a few seconds before connecting: connecting 0.1 s after the menu appeared crashed the game natively
// (access violation at 0x30 while loading the next map, 5 times on Chernarus -> Livonia, 2026-10-19/20) because the
// previous world was still being unloaded. DL_Go retries until the menu is open and the settle time has passed.
// After a RPC_KICK (travel could not finish) the server's message is shown here instead.
modded class MissionMainMenu
{
	static const int DL_SETTLE_MS = 3000;   // wait at least this long in the menu
	static const int DL_GIVE_UP_MS = 15000; // connect anyway after this long
	protected int m_DL_Waited;

	override void OnInit()
	{
		if (GetDayZGame().DL_HasPending())
			m_NoCutscene = true;
		super.OnInit();
	}

	override void OnMissionStart()
	{
		super.OnMissionStart();
		GetDayZGame().DL_Transition().MissionStarted("main menu");
		if (GetDayZGame().DL_HasPending())
		{
			m_DL_Waited = 0;
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_Go, 500, false);
		}
		else if (GetDayZGame().DL_HasNotice())
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_Notice, 500, false);
	}

	override void OnMissionFinish()
	{
		GetDayZGame().DL_Transition().MissionFinishing();
		super.OnMissionFinish();
	}

	protected void DL_Go()
	{
		m_DL_Waited += 500;
		UIScriptedMenu menu = g_Game.GetUIManager().GetMenu();
		bool ready = m_DL_Waited >= DL_SETTLE_MS && menu != null;
		if (!ready && m_DL_Waited < DL_GIVE_UP_MS)
		{
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_Go, 500, false);
			return;
		}
		GetDayZGame().DL_Reconnect(menu);
	}

	protected void DL_Notice()
	{
		GetDayZGame().DL_ShowNotice();
	}
}
