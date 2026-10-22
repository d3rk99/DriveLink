// This addon depends on both mods, so all overridden methods already exist.
modded class PlayerBase
{
	override void DL_OnTransferArrival()
	{
		super.DL_OnTransferArrival();
		ZenImmersiveLoginFinished();
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
		if (DL_IsTransferArrival())
			ZenImmersiveLoginFinished();
	}

	override bool ZenLoginOverrideAI()
	{
		return DL_IsTransferArrival() || super.ZenLoginOverrideAI();
	}

	override void ZenImmersiveLogin_OnPlayerLoaded()
	{
		if (DL_IsTransferArrival())
			ZenImmersiveLoginFinished();
		else
			super.ZenImmersiveLogin_OnPlayerLoaded();
	}

	override void CreateZenSpawnFire()
	{
		if (!DL_IsTransferArrival())
			super.CreateZenSpawnFire();
	}
}
