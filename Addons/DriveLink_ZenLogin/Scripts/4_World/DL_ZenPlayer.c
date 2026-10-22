// Runtime integration avoids a dependency on Zen types and script load order.
modded class PlayerBase
{
 protected void DL_FinishZenArrival()
 {
  if (DL_IsTransferArrival() && String("ZenImmersiveLoginConfig").ToType())
   GetGame().GameScript.CallFunctionParams(this, "ZenImmersiveLoginFinished", null, null);
 }
 override void DL_OnTransferArrival()
 {
  super.DL_OnTransferArrival();
  DL_FinishZenArrival();
 }
 override void OnPlayerLoaded()
 {
  super.OnPlayerLoaded();
  if (DL_IsTransferArrival() && String("ZenImmersiveLoginConfig").ToType())
   GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DL_FinishZenArrival, 1, false);
 }
 override void OnVariablesSynchronized()
 {
  super.OnVariablesSynchronized();
  DL_FinishZenArrival();
 }
}
