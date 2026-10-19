// DriveLink M2 - put a player into a car seat from server script.
// Same mechanism as the vanilla debug spawner (DEVELOPER builds only there): plugindeveloper.c:631-636 sends
// DayZPlayerSyncJunctures.SendGetInVehicle (dayzplayersyncjunctures.c:44-49); the juncture runs on the server AND on
// the owning client (PlayerBase.OnSyncJuncture, playerbase.c:8201-8205), and the next CommandHandler tick starts the
// vehicle command there (playerbase.c:3292-3307): StartCommand_Vehicle (human.c:1492) + SetVehicleType(GetAnimInstance),
// as ActionGetInTransport.Start does (actiongetintransport.c:84-97). A human command can only be started from the
// command handler, which is why this goes through a juncture + member instead of calling StartCommand_Vehicle directly.

modded class PlayerBase
{
	// vanilla ids are 0..26 and 200 (dayzplayersyncjunctures.c:6-34). 4810 (first try, 2026-10-18) never arrived: the id
	// is most likely sent as one byte, so it has to stay below 256.
	static const int DL_SJ_SEAT = 233;
	protected Transport m_DL_SeatIn;
	protected int m_DL_SeatIdx = -1;

	// server
	void DL_SeatIn(Transport t, int crewIdx)
	{
		ScriptJunctureData ctx = new ScriptJunctureData();
		ctx.Write(t);
		ctx.Write(crewIdx);
		SendSyncJuncture(DL_SJ_SEAT, ctx);
		Print(DL_Const.LOG_TAG + "seat juncture " + DL_SJ_SEAT.ToString() + " sent for " + t.GetType() + " crew " + crewIdx.ToString());
	}

	override void OnSyncJuncture(int pJunctureID, ParamsReadContext pCtx)
	{
		if (pJunctureID == DL_SJ_SEAT)
		{
			EntityAI ent;
			int idx;
			bool ok = pCtx.Read(ent) && pCtx.Read(idx);
			if (ok)
			{
				m_DL_SeatIn = Transport.Cast(ent);
				m_DL_SeatIdx = idx;
			}
			Print(DL_Const.LOG_TAG + "seat juncture received (server " + g_Game.IsServer().ToString() + "): read " + ok.ToString() + ", transport " + (m_DL_SeatIn != null).ToString() + ", crew " + idx.ToString());
			return;
		}
		super.OnSyncJuncture(pJunctureID, pCtx);
	}

	override void CommandHandler(float pDt, int pCurrentCommandID, bool pCurrentCommandFinished)
	{
		super.CommandHandler(pDt, pCurrentCommandID, pCurrentCommandFinished);
		if (!m_DL_SeatIn)
			return;
		Transport t = m_DL_SeatIn;
		int idx = m_DL_SeatIdx;
		m_DL_SeatIn = null;
		HumanCommandVehicle vc = StartCommand_Vehicle(t, idx, t.GetSeatAnimationType(idx));
		if (vc)
			vc.SetVehicleType(t.GetAnimInstance());
		string side = "client";
		if (g_Game.IsServer())
			side = "server";
		Print(DL_Const.LOG_TAG + "seat (" + side + "): StartCommand_Vehicle " + t.GetType() + " crew " + idx.ToString() + " -> " + (vc != null).ToString());
	}
}
