// DriveLink starting server (hub Servers page): who died on this server. The hub is told (/api/died); when this is
// not the starting server, the hub hands his next life to the starting server and his respawn here is sent there.
// 4_World can't call the mission (5_Mission): queued here, MissionServer.OnUpdate takes it (DL_TakeDeaths).
class DL_Deaths
{
	protected static ref array<string> s_Queue;

	static array<string> Queue()
	{
		if (!s_Queue)
			s_Queue = new array<string>;
		return s_Queue;
	}
}

modded class PlayerBase
{
	override void EEKilled(Object killer)
	{
		super.EEKilled(killer);
		if (g_Game.IsServer() && GetIdentity())
			DL_Deaths.Queue().Insert(GetIdentity().GetPlainId());
	}
}
