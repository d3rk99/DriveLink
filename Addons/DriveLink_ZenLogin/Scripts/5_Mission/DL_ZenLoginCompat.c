// Runtime discovery keeps the same package usable with or without Zen.
modded class MissionGameplay
{
 protected bool m_DL_ZenInstalled;
 protected float m_DL_PlainLoginT;   // an "owner" redirect that was a plain login: no DriveLink arrival came
 override void OnMissionStart()
 {
  super.OnMissionStart();
  m_DL_ZenInstalled = false;
  if (String("ZenImmersiveLoginClient").ToType())
   m_DL_ZenInstalled = true;
  DL_Log.Info("Zen arrival integration: client manager available " + m_DL_ZenInstalled.ToString());
  if (m_DL_ZenInstalled && !GetGame().IsDedicatedServer())
  {
   GetGame().GetUpdateQueue(CALL_CATEGORY_SYSTEM).Insert(DL_CheckZenArrival);
   DL_CheckZenArrival(0);
  }
 }
 protected void DL_CheckZenArrival(float timeslice)
 {
  if (!m_DL_ZenInstalled || GetGame().IsDedicatedServer())
   return;
  PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
  if (GetDayZGame().DL_IsTransferLogin() && player && !player.DL_IsTransferArrival())
  {
   // expected an arrival but none came within 45 s of being in game: a plain login, give the gestures back
   m_DL_PlainLoginT += timeslice;
   if (m_DL_PlainLoginT > 45)
   {
    GetDayZGame().DL_ClearTransferLogin();
    DL_Log.Info("Zen arrival integration: no DriveLink arrival came, a plain login");
   }
  }
  if (!GetDayZGame().DL_IsTransferLogin() && (!player || !player.DL_IsTransferArrival()))
   return;
  Class zenClient;
  EnScript.GetClassVar(GetGame().GetMission(), "m_ZenImmersiveLogin", 0, zenClient);
  if (!zenClient)
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
  if (m_DL_ZenInstalled && !GetGame().IsDedicatedServer())
   GetGame().GetUpdateQueue(CALL_CATEGORY_SYSTEM).Remove(DL_CheckZenArrival);
  super.OnMissionFinish();
 }
}
