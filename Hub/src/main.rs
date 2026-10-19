// DriveLink Hub (Rust) - the one program every DayZ server talks to.
//
//   DriveLinkHub.exe                    run in this console (Ctrl+C stops it)
//   DriveLinkHub.exe install|uninstall  Windows service (administrator terminal)
//   DriveLinkHub.exe transfers | return <steamId> | drop <steamId> | dump [file]   admin commands
//   -config <file>                      another hub.json (default: next to the exe)
//
// First start writes hub.json with a random server key and keeps running: everything else (servers, zones, hub
// settings, firewall) is done in the web app at http://localhost:7290/admin.
mod admin;
mod api;
mod config;
mod db;
mod firewall;
mod layout;
mod log;
mod maps;
mod net;
mod numfix;
#[cfg(windows)]
mod service;
mod tls;

use std::net::SocketAddr;
use std::path::PathBuf;
use std::sync::{Arc, Mutex};
use std::time::Duration;

fn config_path(args: &mut Vec<String>) -> PathBuf {
    let dir = std::env::current_exe().ok().and_then(|p| p.parent().map(|d| d.to_path_buf())).unwrap_or_else(|| PathBuf::from("."));
    if let Some(i) = args.iter().position(|a| a == "-config" || a == "--config") {
        if i + 1 < args.len() {
            let p = PathBuf::from(args.remove(i + 1));
            args.remove(i);
            // relative to where you run it (2026-10-18: relative to the exe made a stray hub.json in target\release)
            return if p.is_absolute() { p } else { std::env::current_dir().map(|c| c.join(&p)).unwrap_or(p) };
        }
    }
    dir.join("hub.json")
}

static CONFIG: std::sync::OnceLock<PathBuf> = std::sync::OnceLock::new();

/// Runs the hub until `stop` turns true (service) or Ctrl+C (console).
pub fn serve(console: bool, mut stop: tokio::sync::watch::Receiver<bool>) -> Result<(), String> {
    let cfg_path = CONFIG.get().cloned().unwrap_or_else(|| PathBuf::from("hub.json"));
    let dir = cfg_path.parent().map(|d| d.to_path_buf()).unwrap_or_default();
    log::init(dir.join("hub.log"), console);
    let (mut cfg, created) = config::load_or_create(&cfg_path)?;
    if let Some(p) = config::ensure_admin_password(&cfg_path, &mut cfg)? {
        logf!("web app password made (needed from other PCs only; change it on the Setup page): {p}");
    }
    if created {
        // nothing to fill in by hand any more: servers, key and firewall are all on the web app
        logf!("first start: wrote {} with a new server key; open the web app to add your DayZ servers", cfg_path.display());
    }
    cfg.validate().map_err(|e| format!("hub.json: {e}"))?;
    let db_path = {
        let p = PathBuf::from(&cfg.database);
        if p.is_absolute() { p } else { dir.join(p) }
    };
    let mut conn = db::open(&db_path)?;
    db::migrate(&mut conn)?;
    let hub = Arc::new(api::Hub {
        dir: dir.clone(),
        cfg_path: cfg_path.clone(),
        maps: Default::default(),
        cfg: std::sync::RwLock::new(cfg.clone()),
        pending: Default::default(),
        seen: Default::default(),
        db: Mutex::new(conn),
    });

    let rt = tokio::runtime::Builder::new_multi_thread().enable_all().build().map_err(|e| e.to_string())?;
    rt.block_on(async move {
        let app = api::routes(hub.clone());
        let handle = axum_server::Handle::<SocketAddr>::new();
        let mut tasks = Vec::new();
        let mut where_ = Vec::new();
        if !cfg.listen.is_empty() {
            let addr: SocketAddr = cfg.listen.parse().map_err(|e| format!("listen {:?}: {e}", cfg.listen))?;
            let srv = axum_server::bind(addr).handle(handle.clone()).serve(app.clone().into_make_service_with_connect_info::<SocketAddr>());
            tasks.push(tokio::spawn(srv));
            where_.push(format!("http://{addr}"));
        }
        if !cfg.listen_tls.is_empty() {
            let addr: SocketAddr = cfg.listen_tls.parse().map_err(|e| format!("listenTls {:?}: {e}", cfg.listen_tls))?;
            let _ = rustls::crypto::ring::default_provider().install_default();
            let (cert, key) = tls::cert_files(&cfg, &dir)?;
            let tls_cfg = axum_server::tls_rustls::RustlsConfig::from_pem_file(&cert, &key)
                .await
                .map_err(|e| format!("TLS certificate {}: {e}", cert.display()))?;
            let srv = axum_server::bind_rustls(addr, tls_cfg).handle(handle.clone()).serve(app.into_make_service_with_connect_info::<SocketAddr>());
            tasks.push(tokio::spawn(srv));
            where_.push(format!("https://{addr}"));
        }
        logf!("DriveLink Hub (rust) listening on {}, {} servers, database {}", where_.join(" + "), cfg.servers.len(), db_path.display());
        if let Some(w) = where_.first() {
            logf!("web app (servers, map, setup): {}/admin", w.replace("0.0.0.0", "localhost"));
        }
        if console {
            tokio::select! {
                _ = tokio::signal::ctrl_c() => {}
                _ = stop.changed() => {}
            }
        } else {
            let _ = stop.changed().await;
        }
        logf!("DriveLink Hub stopping");
        handle.graceful_shutdown(Some(Duration::from_secs(5)));
        for t in tasks {
            if let Ok(Err(e)) = t.await {
                logf!("listener ended with an error: {e}");
            }
        }
        Ok::<(), String>(())
    })
}

fn admin(cmd: &str, rest: &[String]) -> Result<(), String> {
    let cfg_path = CONFIG.get().cloned().unwrap();
    let dir = cfg_path.parent().map(|d| d.to_path_buf()).unwrap_or_default();
    log::init(dir.join("hub.log"), true);
    let (cfg, _) = config::load_or_create(&cfg_path)?;
    let db_path = {
        let p = PathBuf::from(&cfg.database);
        if p.is_absolute() { p } else { dir.join(p) }
    };
    let mut c = db::open(&db_path)?;
    db::migrate(&mut c)?;
    let need_id = || rest.first().cloned().ok_or_else(|| format!("usage: DriveLinkHub.exe {cmd} <steamId>"));
    match cmd {
        "transfers" => db::list_transfers(&c),
        "return" => db::return_transfer(&mut c, &need_id()?),
        "drop" => db::drop_transfer(&mut c, &need_id()?),
        "dump" => {
            let out = rest.first().map(PathBuf::from).unwrap_or_else(|| dir.join("last-payload.json"));
            db::dump(&c, &out)
        }
        "map" => {
            // DriveLinkHub.exe map            -> list maps found in the game files
            // DriveLinkHub.exe map <name>     -> build its tiles now (what the editor's button does)
            let avail = maps::available(&cfg.dayz_path);
            let Some(name) = rest.first() else {
                for (m, p) in &avail {
                    println!("{m}: {}{}", p.display(), if maps::meta(&dir, m).is_some() { "  (built)" } else { "" });
                }
                if avail.is_empty() {
                    println!("no worlds_*_data.pbo found; set dayzPath in hub.json");
                }
                return Ok(());
            };
            let pbo = avail.get(name).cloned().ok_or(format!("no worlds_{name}_data.pbo found"))?;
            let tools = maps::tools_dir(&cfg.dayz_tools_path).ok_or("DayZ Tools not found; set dayzToolsPath in hub.json")?;
            let st: maps::Status = Default::default();
            maps::build(dir.clone(), name.clone(), pbo, tools, st.clone());
            let s = st.lock().unwrap().get(name).cloned().unwrap_or_default();
            if s.starts_with("FAILED") { Err(s) } else { Ok(()) }
        }
        _ => unreachable!(),
    }
}

fn main() {
    let mut args: Vec<String> = std::env::args().skip(1).collect();
    let _ = CONFIG.set(config_path(&mut args));
    let cmd = args.first().cloned().unwrap_or_default();
    let res = match cmd.as_str() {
        "" => {
            let (_tx, rx) = tokio::sync::watch::channel(false);
            serve(true, rx)
        }
        #[cfg(windows)]
        "service" => service::run_as_service(),
        #[cfg(windows)]
        "install" => service::install(),
        #[cfg(windows)]
        "uninstall" => service::uninstall(),
        "transfers" | "return" | "drop" | "dump" | "map" => admin(&cmd, &args[1..]),
        other => Err(format!("unknown command {other:?} (none = run here, install, uninstall, transfers, return <steamId>, drop <steamId>, dump [file])")),
    };
    if let Err(e) = res {
        eprintln!("{e}");
        logf!("{e}");
        std::process::exit(1);
    }
}
