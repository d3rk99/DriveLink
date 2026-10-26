class DT_SetupMenu:UIScriptedMenu
{
    override Widget Init()
    {
        layoutRoot=GetGame().GetWorkspace().CreateWidgets("DriveLinkRestart/gui/layouts/tray_setup.layout");MultilineTextWidget instructions=MultilineTextWidget.Cast(layoutRoot.FindAnyWidget("Instructions"));
        if(instructions)instructions.SetText("DriveLink needs to restart DayZ for this route. Temporary GOD MODE protects you during this setup wait.\n\nFind this mod's folder and run Helper > DriveLink Setup.exe. The vanilla launcher's Open folder option may not work. Browse manually: your Steam library > steamapps > workshop > content > 221100 > 3824238331 > Helper > DriveLink Setup.exe. Workshop mod ID: 3824238331.\n\nIf you installed a local test copy, use the mod folder you added to the launcher instead.\n\nFinish setup and return here; readiness is detected automatically. Cancel to stay. Temporary protection ends when travel resumes, you cancel, leave the zone, or the three-minute wait expires.");
        return layoutRoot;
    }
    override bool OnClick(Widget w,int x,int y,int button)
    {
        if(w.GetName()=="Completed"){GetDayZGame().DT_WriteReadiness();return true;}
        if(w.GetName()=="Cancel"){GetDayZGame().DT_CancelSetup();return true;}
        return super.OnClick(w,x,y,button);
    }
    override void OnShow(){super.OnShow();GetGame().GetInput().ChangeGameFocus(1);GetGame().GetUIManager().ShowUICursor(true);}
    override void OnHide(){GetDayZGame().DT_AbandonSetup();super.OnHide();GetGame().GetInput().ResetGameFocus();GetGame().GetUIManager().ShowUICursor(false);}
}
modded class MissionBase
{
    override UIScriptedMenu CreateScriptedMenu(int id)
    {
        if(id==DayZGame.DT_MENU){DT_SetupMenu setup=new DT_SetupMenu();setup.SetID(id);return setup;}
        return super.CreateScriptedMenu(id);
    }
}
