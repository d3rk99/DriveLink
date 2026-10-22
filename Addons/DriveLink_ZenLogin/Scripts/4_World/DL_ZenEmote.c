// Stop the wake-up request before Zen can put the traveller in a gesture command.
// This uses only the vanilla API and therefore also compiles without Zen.
modded class EmoteManager
{
	override void CreateEmoteCBFromMenu(int id, bool interrupts_same = false)
	{
		if (String("ZenImmersiveLoginConfig").ToType() && !GetGame().IsDedicatedServer() && GetDayZGame().DL_IsTransferLogin())
		{
			if (id == EmoteConstants.ID_EMOTE_SITA || id == EmoteConstants.ID_EMOTE_LYINGDOWN)
			{
				DL_Log.Info("Zen arrival integration: blocked login wake-up gesture before vehicle seating");
				return;
			}
		}
		super.CreateEmoteCBFromMenu(id, interrupts_same);
	}
}
