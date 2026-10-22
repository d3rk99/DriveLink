// Optional bridge. Zen's Workshop mod is loaded unchanged, without a hard dependency.
#ifdef ZenImmersiveLogin
modded class ZenImmersiveLoginClient
{
	protected bool DL_SkipWakeup()
	{
		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		return GetDayZGame().DL_IsTransferLogin() || (player && player.DL_IsTransferArrival());
	}

	override protected void ApplyHardBlack()
	{
		if (!DL_SkipWakeup())
			super.ApplyHardBlack();
	}

	override protected void LockControls()
	{
		if (!DL_SkipWakeup())
			super.LockControls();
	}

	override protected void TryPlayLoginEmote()
	{
		if (!DL_SkipWakeup())
			super.TryPlayLoginEmote();
	}

	override bool Update(float timeslice)
	{
		if (!DL_SkipWakeup())
			return super.Update(timeslice);
		if (!m_HasFinishedNaturally)
		{
			m_Player = PlayerBase.Cast(g_Game.GetPlayer());
			g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(ApplyHardBlack);
			g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(LockControls);
			m_HasFinishedNaturally = true;
			End(true);
			DL_Log.Info("Zen Immersive Login: skipped wake-up for a DriveLink arrival");
		}
		return true;
	}
}
#endif
