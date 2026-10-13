// DriveLink - client: show the travel countdown the server sends (RPC_COUNTDOWN, int seconds + zone name).
// Uses vanilla notifications (NotificationSystem.AddNotificationExtended, 3_game/client/notifications/notificationsystem.c)
// so no layout files are needed in M1. 0 seconds = cancelled.
modded class MissionGameplay
{
	override void OnMissionStart()
	{
		super.OnMissionStart();
		GetDayZGame().Event_OnRPC.Insert(DL_OnRpc);
	}

	override void OnMissionFinish()
	{
		GetDayZGame().Event_OnRPC.Remove(DL_OnRpc);
		super.OnMissionFinish();
	}

	protected void DL_OnRpc(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
	{
		if (rpc_type != DL_Const.RPC_COUNTDOWN)
			return;
		int seconds;
		string zone;
		if (!ctx.Read(seconds) || !ctx.Read(zone))
			return;
		if (seconds > 0)
			NotificationSystem.AddNotificationExtended(seconds, "DriveLink: " + zone, "Leaving in " + seconds.ToString() + " s. Step out of the zone to cancel.", "");
		else
			NotificationSystem.AddNotificationExtended(3, "DriveLink", "Travel cancelled.", "");
	}
}
