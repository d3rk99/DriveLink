// Runtime discovery keeps the same package usable with or without Zen.
modded class MissionGameplay
{
 protected bool m_DL_ZenInstalled;
 override void OnInit()
 {
  super.OnInit();
  m_DL_ZenInstalled = GetGame().ConfigIsExisting("CfgPatches ZenImmersiveLogin");
  if (m_DL_ZenInstalled && GetGame().IsClient())
   GetGame().GetUpdateQueue(CALL_CATEGORY_SYSTEM).Insert(DL_CheckZenArrival);
 }
 protected void DL_CheckZenArrival(float timeslice)
 {
  if (!m_DL_ZenInstalled || !GetGame().IsClient())
   return;
  PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
  if (!GetDayZGame().DL_IsTransferLogin() && (!player || !player.DL_IsTransferArrival()))
   return;
  Class zenClient;
  if (!EnScript.GetClassVar(this, "m_ZenImmersiveLogin", 0, zenClient) || !zenClient)
   return;
  GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).RemoveByName(zenClient, "ApplyHardBlack");
  GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).RemoveByName(zenClient, "LockControls");
  GetGame().GameScript.CallFunctionParams(zenClient, "Cleanup", null, null);
  if (player)
   GetGame().GameScript.CallFunctionParams(player, "ZenImmersiveLoginFinished", null, null);
  delete zenClient;
  DL_Log.Info("Zen Immersive Login: skipped wake-up for a DriveLink arrival");
 }
 override void OnUpdate(float timeslice)
 {
  DL_CheckZenArrival(timeslice);
  super.OnUpdate(timeslice);
 }
 override void OnMissionFinish()
 {
  if (m_DL_ZenInstalled && GetGame().IsClient())
   GetGame().GetUpdateQueue(CALL_CATEGORY_SYSTEM).Remove(DL_CheckZenArrival);
  super.OnMissionFinish();
 }
}
