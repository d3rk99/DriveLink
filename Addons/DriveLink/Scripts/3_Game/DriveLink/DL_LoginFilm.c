// Keep the client-owned scene visible during both normal and static login timers.
// Do not change the server's timer, respawn countdowns, or native cancel behavior.
modded class LoginTimeBase
{
	override void SetTime(int time)
	{
		super.SetTime(time);
		if (!IsRespawn())
			GetDayZGame().DL_Transition().LoginTimer(layoutRoot, time);
	}

	override void Show()
	{
		super.Show();
		if (!IsRespawn())
			GetDayZGame().DL_Transition().LoginTimer(layoutRoot, -1);
	}

	override void Hide()
	{
		GetDayZGame().DL_Transition().LoginTimerClosed(layoutRoot);
		super.Hide();
	}

	void ~LoginTimeBase()
	{
		if (GetGame())
			GetDayZGame().DL_Transition().LoginTimerClosed(layoutRoot);
	}
}
