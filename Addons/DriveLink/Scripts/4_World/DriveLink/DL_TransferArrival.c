// Transient marker on a restored traveller; never stored in the character snapshot.
modded class PlayerBase
{
	protected bool m_DL_TransferArrival;

	override void Init()
	{
		super.Init();
		RegisterNetSyncVariableBool("m_DL_TransferArrival");
	}

	bool DL_IsTransferArrival()
	{
		return m_DL_TransferArrival;
	}

	void DL_MarkTransferArrival()
	{
		m_DL_TransferArrival = true;
		SetSynchDirty();
		DL_OnTransferArrival();
	}

	// Optional addons implement compatibility here after their dependencies have compiled.
	void DL_OnTransferArrival()
	{
	}
}
