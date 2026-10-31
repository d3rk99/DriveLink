// DriveLink "travel anyway" (Travel rules: allowAnyway): the leader of a refused trip answered the "not enough
// supplies" dialog (RPC_GO_ANYWAY to his own player, like the film vote). 4_World can't call the mission: queued
// here as "<steam id>|1" (go) or "<steam id>|0" (stay), MissionServer.OnUpdate takes it (DL_TakeAnyway).
class DL_AnywayAnswers
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
	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);
		if (rpc_type != DL_Const.RPC_GO_ANYWAY || !g_Game.IsServer() || !sender)
			return;
		bool go;
		if (!ctx.Read(go))
			return;
		string a = sender.GetPlainId() + "|0";
		if (go)
			a = sender.GetPlainId() + "|1";
		DL_AnywayAnswers.Queue().Insert(a);
	}
}
