// DriveLink Hub (Rust) - the one program every DayZ server talks to.
//
//   DriveLinkHub.exe                    run in this console (Ctrl+C stops it)
//   DriveLinkHub.exe install|uninstall  Windows service (administrator terminal)
//   DriveLinkHub.exe transfers | return <steamId> | drop <steamId> | dump [file]   admin commands
//   -config <file>                      another hub.json (default: next to the exe)
//
// First start writes hub.json with a random server key and stops: put that key in every DayZ server's
// DriveLink config. Same API, database and hub.json as the Go hub it replaces.
mod api;
mod config;
mod db;
mod log;
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
            return if p.is_absolute() { p } else { dir.join(p) };
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
    let (cfg, created) = config::load_or_create(&cfg_path)?;
    if created {
        let msg = format!(
            "First start: wrote {}\n1. Check the \"servers\" list (id, lanHost, publicHost, port).\n2. Put this in EVERY DayZ server's DriveLink config as hubKey:\n     {}\nThen start the hub again.",
            cfg_path.display(),
            cfg.server_key
        );
        if console {
            println!("\n{msg}");
            return Ok(());
        }
        logf!("{}", msg.replace('\n', " | "));
        return Err("new hub.json written, fill it in and start the service again".into());
    }
    cfg.validate().map_err(|e| format!("hub.json: {e}"))?;
    let db_path = {
        let p = PathBuf::from(&cfg.database);
        if p.is_absolute() { p } else { dir.join(p) }
    };
    let mut conn = db::open(&db_path)?;
    db::migrate(&mut conn)?;
    let hub = Arc::new(api::Hub { cfg: cfg.clone(), db: Mutex::new(conn) });

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
        "transfers" | "return" | "drop" | "dump" => admin(&cmd, &args[1..]),
        other => Err(format!("unknown command {other:?} (none = run here, install, uninstall, transfers, return <steamId>, drop <steamId>, dump [file])")),
    };
    if let Err(e) = res {
        eprintln!("{e}");
        logf!("{e}");
        std::process::exit(1);
    }
}
