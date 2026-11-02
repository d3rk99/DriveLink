// DriveLink interactive trail: what a player pressed (ready up, a vote, lock in, turn back alone) comes to the server
// through his own player as RPC_TRAIL_IN "<what>|<value>", like the film skip vote. 4_World can't call the mission:
// queued here as "<steam id>|<what>|<value>", MissionServer.OnUpdate takes it (DL_TrailInputs).
class DL_TrailAnswers
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
		if (rpc_type != DL_Const.RPC_TRAIL_IN || !g_Game.IsServer() || !sender)
			return;
		string what;
		if (!ctx.Read(what) || what.Length() > 40)
			return;
		DL_TrailAnswers.Queue().Insert(sender.GetPlainId() + "|" + what);
	}
}
