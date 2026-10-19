// Windows service: "DriveLinkHub.exe install" registers it (auto start, LocalSystem, runs "DriveLinkHub.exe service"),
// "uninstall" removes it. Needs an administrator terminal. The service reads hub.json / drivelink.db / hub.log from
// the exe's own folder, like the console run.
#![cfg(windows)]
use std::ffi::OsString;
use std::time::Duration;
use windows_service::service::{
    ServiceAccess, ServiceControl, ServiceControlAccept, ServiceErrorControl, ServiceExitCode, ServiceInfo, ServiceStartType,
    ServiceState, ServiceStatus, ServiceType,
};
use windows_service::service_control_handler::{self, ServiceControlHandlerResult};
use windows_service::service_manager::{ServiceManager, ServiceManagerAccess};
use windows_service::{define_windows_service, service_dispatcher};

pub const NAME: &str = "DriveLinkHub";

pub fn install() -> Result<(), String> {
    let mgr = ServiceManager::local_computer(None::<&str>, ServiceManagerAccess::CONNECT | ServiceManagerAccess::CREATE_SERVICE)
        .map_err(|e| format!("cannot open the service manager ({e}) - run this in an ADMINISTRATOR terminal"))?;
    let exe = std::env::current_exe().map_err(|e| e.to_string())?;
    let info = ServiceInfo {
        name: OsString::from(NAME),
        display_name: OsString::from("DriveLink Hub"),
        service_type: ServiceType::OWN_PROCESS,
        start_type: ServiceStartType::AutoStart,
        error_control: ServiceErrorControl::Normal,
        executable_path: exe.clone(),
        launch_arguments: vec![OsString::from("service")],
        dependencies: vec![],
        account_name: None, // LocalSystem
        account_password: None,
    };
    let svc = mgr
        .create_service(&info, ServiceAccess::CHANGE_CONFIG | ServiceAccess::START)
        .map_err(|e| format!("cannot create the service ({e}); already installed? try uninstall first"))?;
    let _ = svc.set_description("DriveLink: moves DayZ characters and cars between servers. Config: hub.json next to the exe.");
    svc.start::<&str>(&[]).map_err(|e| format!("installed, but it did not start ({e}) - see hub.log"))?;
    println!("installed and started the {NAME} service ({}); it starts with Windows from now on", exe.display());
    Ok(())
}

pub fn uninstall() -> Result<(), String> {
    let mgr = ServiceManager::local_computer(None::<&str>, ServiceManagerAccess::CONNECT)
        .map_err(|e| format!("cannot open the service manager ({e}) - run this in an ADMINISTRATOR terminal"))?;
    let svc = mgr
        .open_service(NAME, ServiceAccess::QUERY_STATUS | ServiceAccess::STOP | ServiceAccess::DELETE)
        .map_err(|e| format!("no {NAME} service ({e})"))?;
    if let Ok(st) = svc.query_status() {
        if st.current_state != ServiceState::Stopped {
            let _ = svc.stop();
            for _ in 0..20 {
                std::thread::sleep(Duration::from_millis(500));
                if svc.query_status().map(|s| s.current_state == ServiceState::Stopped).unwrap_or(true) {
                    break;
                }
            }
        }
    }
    svc.delete().map_err(|e| e.to_string())?;
    println!("removed the {NAME} service");
    Ok(())
}

define_windows_service!(ffi_service_main, service_main);

/// Called for "DriveLinkHub.exe service" (by the Windows service manager, not by hand).
pub fn run_as_service() -> Result<(), String> {
    service_dispatcher::start(NAME, ffi_service_main).map_err(|e| format!("not started by the service manager ({e}); run without arguments for the console"))
}

fn service_main(_args: Vec<OsString>) {
    let (stop_tx, stop_rx) = tokio::sync::watch::channel(false);
    let handler = move |ev| match ev {
        ServiceControl::Stop | ServiceControl::Shutdown => {
            let _ = stop_tx.send(true);
            ServiceControlHandlerResult::NoError
        }
        ServiceControl::Interrogate => ServiceControlHandlerResult::NoError,
        _ => ServiceControlHandlerResult::NotImplemented,
    };
    let Ok(status) = service_control_handler::register(NAME, handler) else { return };
    let set = |state, accept, code| {
        let _ = status.set_service_status(ServiceStatus {
            service_type: ServiceType::OWN_PROCESS,
            current_state: state,
            controls_accepted: accept,
            exit_code: ServiceExitCode::Win32(code),
            checkpoint: 0,
            wait_hint: Duration::from_secs(10),
            process_id: None,
        });
    };
    set(ServiceState::Running, ServiceControlAccept::STOP | ServiceControlAccept::SHUTDOWN, 0);
    let code = match crate::serve(false, stop_rx) {
        Ok(()) => 0,
        Err(e) => {
            crate::logf!("hub stopped with an error: {e}");
            1
        }
    };
    set(ServiceState::Stopped, ServiceControlAccept::empty(), code);
}
