modded class MissionGameplay
{
    protected bool m_DLR_TransferAnnounced;
    override void OnMissionStart()
    {
        super.OnMissionStart();
        GetDayZGame().DLR_WriteProgress(7,"Finalizing the destination player and journey story");
        Print("[DLR] ARRIVAL_STORY_IN_GAME helper does not suppress films or cast completion votes");
    }
    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        if (m_DLR_TransferAnnounced) return;
        PlayerBase traveller = PlayerBase.Cast(GetGame().GetPlayer());
        if (traveller && traveller.DL_IsTransferArrival())
        {
            GetDayZGame().DLR_ConfirmTransferReady();
            m_DLR_TransferAnnounced = true;
        }
    }
}
