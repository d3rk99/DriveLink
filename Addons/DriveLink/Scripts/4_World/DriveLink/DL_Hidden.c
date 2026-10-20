// DriveLink - arriving travellers and their car are invisible to everyone while they are being built and seated (the
// arrival black screen + road film covers it for the travellers themselves), so people nearby don't watch characters
// pop up and get teleported into a car. The server sets a synced flag; every client hides the entity, its attachments
// (clothes, wheels, doors...) and the item in its hands with Object.SetInvisible (entity.c:42), and keeps re-hiding
// once a second while the flag is up (the snapshot puts items in hands a bit later). A safety timer always lifts it.

class DL_Hide
{
	static const int SAFETY_MS = 120000;

	static void Apply(EntityAI e, bool on)
	{
		if (!e)
			return;
		e.SetInvisible(on);
		GameInventory inv = e.GetInventory();
		if (inv)
		{
			for (int i = 0; i < inv.AttachmentCount(); i++)
				Apply(inv.GetAttachmentFromIndex(i), on);
		}
		PlayerBase pb = PlayerBase.Cast(e);
		if (pb && pb.GetHumanInventory())
			Apply(pb.GetHumanInventory().GetEntityInHands(), on);
	}
}

modded class PlayerBase
{
	protected bool m_DL_Hidden;
	protected bool m_DL_HiddenShown;   // client: what was applied

	void PlayerBase()
	{
		RegisterNetSyncVariableBool("m_DL_Hidden");
	}

	// server
	void DL_SetHidden(bool on)
	{
		m_DL_Hidden = on;
		SetSynchDirty();
		if (on)
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SetHidden, DL_Hide.SAFETY_MS, false, false);
		else
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(DL_SetHidden);
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
		if (g_Game.IsDedicatedServer() || m_DL_Hidden == m_DL_HiddenShown)
			return;
		m_DL_HiddenShown = m_DL_Hidden;
		DL_Hide.Apply(this, m_DL_Hidden);
		if (m_DL_Hidden)
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_HideTick, 1000, true);
		else
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(DL_HideTick);
	}

	protected void DL_HideTick()
	{
		if (!m_DL_Hidden)
		{
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(DL_HideTick);
			return;
		}
		DL_Hide.Apply(this, true);
	}
}

modded class CarScript
{
	protected bool m_DL_Hidden;
	protected bool m_DL_HiddenShown;

	void CarScript()
	{
		RegisterNetSyncVariableBool("m_DL_Hidden");
	}

	// server
	void DL_SetHidden(bool on)
	{
		m_DL_Hidden = on;
		SetSynchDirty();
		if (on)
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SetHidden, DL_Hide.SAFETY_MS, false, false);
		else
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(DL_SetHidden);
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
		if (g_Game.IsDedicatedServer() || m_DL_Hidden == m_DL_HiddenShown)
			return;
		m_DL_HiddenShown = m_DL_Hidden;
		DL_Hide.Apply(this, m_DL_Hidden);
		if (m_DL_Hidden)
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_HideTick, 1000, true);
		else
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(DL_HideTick);
	}

	protected void DL_HideTick()
	{
		if (!m_DL_Hidden)
		{
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(DL_HideTick);
			return;
		}
		DL_Hide.Apply(this, true);
	}
}

modded class MotorbikeScript
{
	protected bool m_DL_Hidden;
	protected bool m_DL_HiddenShown;

	void MotorbikeScript()
	{
		RegisterNetSyncVariableBool("m_DL_Hidden");
	}

	// server
	void DL_SetHidden(bool on)
	{
		m_DL_Hidden = on;
		SetSynchDirty();
		if (on)
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SetHidden, DL_Hide.SAFETY_MS, false, false);
		else
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(DL_SetHidden);
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
		if (g_Game.IsDedicatedServer() || m_DL_Hidden == m_DL_HiddenShown)
			return;
		m_DL_HiddenShown = m_DL_Hidden;
		DL_Hide.Apply(this, m_DL_Hidden);
		if (m_DL_Hidden)
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_HideTick, 1000, true);
		else
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(DL_HideTick);
	}

	protected void DL_HideTick()
	{
		if (!m_DL_Hidden)
		{
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(DL_HideTick);
			return;
		}
		DL_Hide.Apply(this, true);
	}
}

modded class BoatScript
{
	protected bool m_DL_Hidden;
	protected bool m_DL_HiddenShown;

	void BoatScript()
	{
		RegisterNetSyncVariableBool("m_DL_Hidden");
	}

	// server
	void DL_SetHidden(bool on)
	{
		m_DL_Hidden = on;
		SetSynchDirty();
		if (on)
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_SetHidden, DL_Hide.SAFETY_MS, false, false);
		else
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(DL_SetHidden);
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
		if (g_Game.IsDedicatedServer() || m_DL_Hidden == m_DL_HiddenShown)
			return;
		m_DL_HiddenShown = m_DL_Hidden;
		DL_Hide.Apply(this, m_DL_Hidden);
		if (m_DL_Hidden)
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_HideTick, 1000, true);
		else
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(DL_HideTick);
	}

	protected void DL_HideTick()
	{
		if (!m_DL_Hidden)
		{
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(DL_HideTick);
			return;
		}
		DL_Hide.Apply(this, true);
	}
}
