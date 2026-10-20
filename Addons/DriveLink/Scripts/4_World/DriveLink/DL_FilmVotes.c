// DriveLink road film: a client pressed Space to vote the film skipped (or his film ended). The RPC targets his own
// player (PlayerBase.OnRPC, the reliable way into the server for a client RPC); 4_World can't call the mission
// (5_Mission), so the vote is queued here and MissionServer.OnUpdate takes it (DL_TakeFilmVotes).
class DL_FilmVotes
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
		if (rpc_type == DL_Const.RPC_FILM_VOTE && g_Game.IsServer() && sender)
			DL_FilmVotes.Queue().Insert(sender.GetPlainId());
	}
}
