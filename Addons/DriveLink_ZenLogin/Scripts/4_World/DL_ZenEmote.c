// Zen's wake-up (sit / lie down on login) must not run on a DriveLink arrival: the traveller is about to be seated in
// his vehicle, and a gesture blocks the seat command until it ends (2026-11-02: seated only after jumping out of it).
// 1) client: the wake-up gesture is not started while a DriveLink arrival is expected;
// 2) server: if one started anyway (the arrival was known too late), it is cancelled during the first 2 minutes after
//    the arrival. Vanilla API only, so it also compiles without Zen.
modded class EmoteManager
{
	protected float m_DL_ZenArrivalT = -1;     // seconds since this player was marked as a DriveLink arrival
	protected float m_DL_ZenCancelWait;

	protected bool DL_ZenWakeUpGesture(int id)
	{
		return id == EmoteConstants.ID_EMOTE_SITA || id == EmoteConstants.ID_EMOTE_LYINGDOWN;
	}

	override void CreateEmoteCBFromMenu(int id, bool interrupts_same = false)
	{
		if (String("ZenImmersiveLoginConfig").ToType() && !GetGame().IsDedicatedServer() && DL_ZenWakeUpGesture(id))
		{
			bool arriving = GetDayZGame().DL_IsTransferLogin();
			if (m_Player && m_Player.DL_IsTransferArrival() && m_DL_ZenArrivalT < 120)
				arriving = true;
			if (arriving)
			{
				DL_Log.Info("Zen arrival integration: blocked login wake-up gesture before vehicle seating");
				return;
			}
		}
		super.CreateEmoteCBFromMenu(id, interrupts_same);
	}

	override void Update(float deltaT)
	{
		super.Update(deltaT);
		if (!m_Player || !m_Player.DL_IsTransferArrival() || !String("ZenImmersiveLoginConfig").ToType())
			return;
		if (m_DL_ZenArrivalT < 0)
			m_DL_ZenArrivalT = 0;
		m_DL_ZenArrivalT += deltaT;
		if (!GetGame().IsDedicatedServer() || m_DL_ZenArrivalT > 120)
			return;
		m_DL_ZenCancelWait -= deltaT;
		if (m_DL_ZenCancelWait > 0 || !IsEmotePlaying() || !DL_ZenWakeUpGesture(m_CurrentGestureID))
			return;
		m_DL_ZenCancelWait = 1;
		GetGame().GameScript.CallFunctionParams(m_Player, "ZenImmersiveLoginFinished", null, null);  // Zen lets it be interrupted then
		ServerRequestEmoteCancel();
		DL_Log.Info("Zen arrival integration: cancelled the login wake-up gesture of a DriveLink arrival (" + m_CurrentGestureID.ToString() + ")");
	}
}
