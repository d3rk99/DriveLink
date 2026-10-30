// Hub updates from GitHub releases. hub.json "autoUpdate": "off" | "notify" (default) | "auto".
//   notify - every 6 h, and on "Check now" (Setup page), look for a newer release and show it on the web app
//   auto   - also install it by itself, when it is a patch of this version (same major.minor, same protocol: the
//            DayZ servers' mod doesn't have to change) and no trip started or arrived in the last 5 minutes.
//            A bigger version is only shown: it goes with a mod update.
// Installing: download DriveLinkHub.exe + SHA256SUMS.txt from the release, check the hash, run "<new> version" to
// see that it starts and is the version the release says, move the running exe to DriveLinkHub.old.exe, put the new
// one in its place and restart into it:
//   service - "<new> finish-update service" (detached) waits for the service to stop, starts it, and puts the old exe
//             back if the new one doesn't stay running for 30 s
//   console - "<new> finish-update console <pid>" in a new console window waits for this process to end and runs the
//             hub there; if that fails at start, it puts the old exe back and starts it instead
use crate::api::Shared;
use serde::Serialize;
use serde_json::Value;
use sha2::{Digest, Sha256};
use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Mutex;
use std::time::Duration;

pub const VERSION: &str = env!("CARGO_PKG_VERSION");
const RELEASES: &str = "https://api.github.com/repos/d3rk99/DriveLink/releases/latest";
const EXE_ASSET: &str = "DriveLinkHub.exe";
const SUMS_ASSET: &str = "SHA256SUMS.txt";
const CHECK_EVERY: Duration = Duration::from_secs(6 * 3600);

/// The hub stops itself when this fires (an update was put in place; the new exe takes over).
pub static RESTART: tokio::sync::Notify = tokio::sync::Notify::const_new();
static CONSOLE: AtomicBool = AtomicBool::new(true);
static BUSY: AtomicBool = AtomicBool::new(false);
static STATUS: Mutex<Status> = Mutex::new(Status::new());

#[derive(Clone, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct Status {
    pub current: &'static str,
    /// the newest release, when it is newer than this hub
    pub latest: Option<String>,
    pub notes_url: Option<String>,
    /// the latest one can be installed without a mod update (same major.minor)
    pub patch: bool,
    pub checked_at: Option<String>,
    /// "" | "checking" | "downloading" | "installing" | "restarting"
    pub state: &'static str,
    pub error: Option<String>,
}

impl Status {
    const fn new() -> Self {
        Status { current: VERSION, latest: None, notes_url: None, patch: false, checked_at: None, state: "", error: None }
    }
}

pub fn status() -> Status {
    STATUS.lock().unwrap().clone()
}

fn set_state(s: &'static str, err: Option<String>) {
    let mut st = STATUS.lock().unwrap();
    st.state = s;
    if err.is_some() || s == "checking" {
        st.error = err;
    }
}

pub fn set_console(console: bool) {
    CONSOLE.store(console, Ordering::Relaxed);
}

/// "1.2.3" or "v1.2.3-rc1" -> (1, 2, 3, is a prerelease)
pub fn parse(v: &str) -> Option<(u64, u64, u64, bool)> {
    let v = v.trim().trim_start_matches(['v', 'V']);
    let (core, pre) = match v.split_once('-') {
        Some((c, _)) => (c, true),
        None => (v, false),
    };
    let mut it = core.split('.').map(|p| p.parse::<u64>());
    let a = it.next()?.ok()?;
    let b = it.next().unwrap_or(Ok(0)).ok()?;
    let c = it.next().unwrap_or(Ok(0)).ok()?;
    if it.next().is_some() {
        return None;
    }
    Some((a, b, c, pre))
}

/// Is `latest` newer than `current`? A release beats a prerelease of the same number.
pub fn newer(latest: &str, current: &str) -> bool {
    match (parse(latest), parse(current)) {
        (Some((a, b, c, lp)), Some((x, y, z, cp))) => (a, b, c) > (x, y, z) || ((a, b, c) == (x, y, z) && cp && !lp),
        _ => false,
    }
}

/// Same major.minor: installs without a mod update.
pub fn is_patch(latest: &str, current: &str) -> bool {
    match (parse(latest), parse(current)) {
        (Some((a, b, ..)), Some((x, y, ..))) => (a, b) == (x, y),
        _ => false,
    }
}

fn agent() -> ureq::Agent {
    ureq::Agent::config_builder()
        .timeout_global(Some(Duration::from_secs(300)))
        .user_agent(format!("DriveLinkHub/{VERSION}"))
        .build()
        .into()
}

struct Release {
    version: String,
    page: String,
    exe: String,
    sums: String,
}

fn latest_release(url: &str) -> Result<Release, String> {
    let body = agent()
        .get(url)
        .header("Accept", "application/vnd.github+json")
        .call()
        .map_err(|e| format!("release check: {e}"))?
        .body_mut()
        .read_to_string()
        .map_err(|e| format!("release check: {e}"))?;
    let v: Value = serde_json::from_str(&body).map_err(|e| format!("release check: {e}"))?;
    let tag = v["tag_name"].as_str().ok_or("release check: no tag_name")?;
    let asset = |name: &str| {
        v["assets"].as_array().and_then(|a| a.iter().find(|x| x["name"] == name)).and_then(|x| x["browser_download_url"].as_str()).map(String::from)
    };
    Ok(Release {
        version: tag.trim_start_matches(['v', 'V']).to_string(),
        page: v["html_url"].as_str().unwrap_or_default().to_string(),
        exe: asset(EXE_ASSET).ok_or(format!("release {tag} has no {EXE_ASSET}"))?,
        sums: asset(SUMS_ASSET).ok_or(format!("release {tag} has no {SUMS_ASSET}"))?,
    })
}

/// Looks for a newer release and records it for the web app. Some(release) when there is one.
fn check(h: &Shared) -> Result<Option<Release>, String> {
    let url = h.cfg().update_url.clone();
    let url = if url.is_empty() { RELEASES.to_string() } else { url };
    let r = latest_release(&url);
    let mut st = STATUS.lock().unwrap();
    st.checked_at = Some(chrono::Local::now().format("%Y-%m-%d %H:%M").to_string());
    let r = r?;
    let found = newer(&r.version, VERSION) && !parse(&r.version).map(|p| p.3).unwrap_or(true);
    st.latest = found.then(|| r.version.clone());
    st.notes_url = found.then(|| r.page.clone());
    st.patch = found && is_patch(&r.version, VERSION);
    Ok(found.then_some(r))
}

/// The hash for `name` in a SHA256SUMS.txt ("<hex>  <name>" or "<hex> *<name>")
pub fn sum_for(sums: &str, name: &str) -> Option<String> {
    sums.lines().find_map(|l| {
        let mut p = l.split_whitespace();
        let hash = p.next()?;
        let file = p.next()?.trim_start_matches('*');
        (file == name && hash.len() == 64).then(|| hash.to_ascii_lowercase())
    })
}

/// Fetches and checks the release's exe; returns where it is and the protocol it says it speaks.
fn download(r: &Release, dir: &Path) -> Result<(PathBuf, Option<u32>), String> {
    let sums = agent().get(&r.sums).call().map_err(|e| format!("download {SUMS_ASSET}: {e}"))?.body_mut().read_to_string().map_err(|e| e.to_string())?;
    let want = sum_for(&sums, EXE_ASSET).ok_or(format!("{SUMS_ASSET} has no line for {EXE_ASSET}"))?;
    let exe = agent()
        .get(&r.exe)
        .call()
        .map_err(|e| format!("download {EXE_ASSET}: {e}"))?
        .body_mut()
        .with_config()
        .limit(200 << 20)
        .read_to_vec()
        .map_err(|e| format!("download {EXE_ASSET}: {e}"))?;
    let got = hex::encode(Sha256::digest(&exe));
    if got != want {
        return Err(format!("{EXE_ASSET} does not match {SUMS_ASSET} (got {got}); not installed"));
    }
    if !exe.starts_with(b"MZ") {
        return Err(format!("{EXE_ASSET} is not a Windows program; not installed"));
    }
    let folder = dir.join("update");
    std::fs::create_dir_all(&folder).map_err(|e| format!("{}: {e}", folder.display()))?;
    let path = folder.join(format!("DriveLinkHub-{}.exe", r.version));
    std::fs::write(&path, &exe).map_err(|e| format!("{}: {e}", path.display()))?;
    // it must start and say it is that version, before it replaces anything
    let out = std::process::Command::new(&path).arg("version").output().map_err(|e| format!("the new hub does not start: {e}"))?;
    let said = String::from_utf8_lossy(&out.stdout).to_string();
    let mut words = said.split_whitespace();
    if words.next() != Some(r.version.as_str()) {
        return Err(format!("the new hub says it is {:?}, the release says {}; not installed", said.trim(), r.version));
    }
    let protocol = words.nth(1).and_then(|p| p.parse::<u32>().ok());
    Ok((path, protocol))
}

fn old_path(exe: &Path) -> PathBuf {
    exe.with_extension("old.exe")
}

/// Moves the running exe aside and puts `new` in its place.
fn swap(new: &Path) -> Result<PathBuf, String> {
    let exe = std::env::current_exe().map_err(|e| e.to_string())?;
    let old = old_path(&exe);
    if old.exists() {
        std::fs::remove_file(&old).map_err(|e| format!("{}: {e}", old.display()))?;
    }
    // Windows lets a running exe be renamed (not overwritten)
    std::fs::rename(&exe, &old).map_err(|e| format!("cannot move {} aside: {e}", exe.display()))?;
    if let Err(e) = std::fs::copy(new, &exe) {
        let _ = std::fs::rename(&old, &exe);
        return Err(format!("cannot put the new hub in place ({e}); kept the old one"));
    }
    let _ = std::fs::remove_file(new);
    Ok(exe)
}

/// update/failed-version: a version that didn't start here; it is never installed by itself again.
fn failed_file(dir: &Path) -> PathBuf {
    dir.join("update").join("failed-version")
}

fn failed_version(dir: &Path) -> String {
    std::fs::read_to_string(failed_file(dir)).unwrap_or_default().trim().to_string()
}

/// Puts DriveLinkHub.old.exe back (this new one failed; it stays as DriveLinkHub.failed.exe). Runs in the new exe.
pub fn roll_back(dir: &Path) -> Result<PathBuf, String> {
    let _ = std::fs::create_dir_all(dir.join("update"));
    let _ = std::fs::write(failed_file(dir), VERSION);
    let exe = std::env::current_exe().map_err(|e| e.to_string())?;
    let old = old_path(&exe);
    if !old.exists() {
        return Err(format!("no {} to go back to", old.display()));
    }
    let failed = exe.with_extension("failed.exe");
    let _ = std::fs::remove_file(&failed);
    std::fs::rename(&exe, &failed).map_err(|e| e.to_string())?;
    std::fs::rename(&old, &exe).map_err(|e| e.to_string())?;
    Ok(exe)
}

/// Download, check, swap and restart. Runs on a blocking thread; one at a time.
/// `auto` = not asked for on the web app: then only a hub that speaks this protocol goes in.
fn install(h: &Shared, r: &Release, auto: bool) -> Result<(), String> {
    set_state("downloading", None);
    let (new, protocol) = download(r, &h.dir)?;
    if auto && protocol != Some(crate::api::PROTOCOL) {
        return Err(format!("{} speaks another protocol with the DayZ servers: install it with the matching mod (Update now)", r.version));
    }
    set_state("installing", None);
    let exe = swap(&new)?;
    let console = CONSOLE.load(Ordering::Relaxed);
    let mut cmd = std::process::Command::new(&exe);
    cmd.arg("-config").arg(&h.cfg_path).arg("finish-update");
    if console {
        cmd.arg("console").arg(std::process::id().to_string());
    } else {
        cmd.arg("service");
    }
    #[cfg(windows)]
    {
        use std::os::windows::process::CommandExt;
        const CREATE_NEW_CONSOLE: u32 = 0x10;
        const DETACHED_PROCESS: u32 = 0x08;
        const CREATE_NEW_PROCESS_GROUP: u32 = 0x200;
        cmd.creation_flags(if console { CREATE_NEW_CONSOLE } else { DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP });
    }
    if let Err(e) = cmd.spawn() {
        // still the old exe running here: just put it back
        let _ = std::fs::remove_file(&exe);
        let _ = std::fs::rename(old_path(&exe), &exe);
        return Err(format!("cannot start the new hub ({e}); kept the old one"));
    }
    crate::logf!("update: {VERSION} -> {} installed, restarting{}", r.version, if console { " in a new window" } else { "" });
    set_state("restarting", None);
    RESTART.notify_one();
    Ok(())
}

/// A trip started or arrived in the last 5 minutes: don't restart under it.
fn trips_moving(h: &Shared) -> bool {
    let db = h.db.lock().unwrap();
    db.query_row(
        "SELECT count(*) FROM transfers WHERE created_at > datetime('now','-5 minutes') OR claimed_at > datetime('now','-5 minutes')",
        [],
        |r| r.get::<_, i64>(0),
    )
    .map(|n| n > 0)
    .unwrap_or(true)
}

/// "Check now" on the Setup page, and the timer. `auto` = may install.
pub fn check_and_maybe_install(h: &Shared, auto: bool) -> Result<(), String> {
    if BUSY.swap(true, Ordering::SeqCst) {
        return Err("an update check is already running".into());
    }
    set_state("checking", None);
    let res = (|| {
        let Some(r) = check(h)? else { return Ok(()) };
        crate::logf!("update: DriveLink Hub {} is out ({}), this is {VERSION}", r.version, r.page);
        if auto && is_patch(&r.version, VERSION) && !trips_moving(h) {
            if failed_version(&h.dir) == r.version {
                return Err(format!("{} did not start on this PC before and was rolled back (hub.log); not installing it by itself again", r.version));
            }
            install(h, &r, true)?;
        }
        Ok(())
    })();
    if status().state != "restarting" {
        set_state("", res.as_ref().err().cloned());
    }
    BUSY.store(false, Ordering::SeqCst);
    res
}

/// "Update now" on the Setup page: whatever the newest release is.
pub fn install_now(h: &Shared) -> Result<(), String> {
    if BUSY.swap(true, Ordering::SeqCst) {
        return Err("an update is already running".into());
    }
    set_state("checking", None);
    let res = (|| {
        let r = check(h)?.ok_or("this hub is up to date")?;
        install(h, &r, false)
    })();
    if let Err(e) = &res {
        crate::logf!("update failed: {e}");
        set_state("", Some(e.clone()));
    }
    BUSY.store(false, Ordering::SeqCst);
    res
}

/// The background timer: first look 1 min after start, then every 6 h.
pub async fn run(h: Shared) {
    tokio::time::sleep(Duration::from_secs(60)).await;
    loop {
        let mode = h.cfg().auto_update.clone();
        if mode != "off" {
            let hh = h.clone();
            let res = tokio::task::spawn_blocking(move || check_and_maybe_install(&hh, mode == "auto")).await;
            if let Ok(Err(e)) = res {
                crate::logf!("update check: {e}");
            }
        }
        tokio::time::sleep(CHECK_EVERY).await;
    }
}

/// "DriveLinkHub.exe finish-update service|console <pid>": the new exe takes over from the old one.
pub fn finish(args: &[String]) -> Result<(), String> {
    match args.first().map(String::as_str) {
        #[cfg(windows)]
        Some("service") => crate::service::restart_after_update(),
        Some("console") => {
            let pid: u32 = args.get(1).and_then(|p| p.parse().ok()).ok_or("usage: finish-update console <pid>")?;
            wait_for_exit(pid);
            let started = std::time::Instant::now();
            let (_tx, rx) = tokio::sync::watch::channel(false);
            match crate::serve(true, rx) {
                Err(e) if started.elapsed() < Duration::from_secs(60) => {
                    crate::logf!("the updated hub did not start ({e}); going back to the old one");
                    let cfg = crate::CONFIG.get().cloned().unwrap_or_default();
                    let exe = roll_back(cfg.parent().unwrap_or(Path::new(".")))?;
                    std::process::Command::new(&exe).arg("-config").arg(cfg).spawn().map_err(|e| e.to_string())?;
                    Ok(())
                }
                r => r,
            }
        }
        _ => Err("usage: finish-update service | console <pid>".into()),
    }
}

/// Waits (up to 60 s) until process `pid` has ended, so its ports and files are free.
fn wait_for_exit(pid: u32) {
    for _ in 0..120 {
        if !process_alive(pid) {
            return;
        }
        std::thread::sleep(Duration::from_millis(500));
    }
}

#[cfg(windows)]
fn process_alive(pid: u32) -> bool {
    let out = std::process::Command::new("tasklist").args(["/FI", &format!("PID eq {pid}"), "/NH", "/FO", "CSV"]).output();
    match out {
        Ok(o) => String::from_utf8_lossy(&o.stdout).contains(&format!("\"{pid}\"")),
        Err(_) => false,
    }
}

#[cfg(not(windows))]
fn process_alive(pid: u32) -> bool {
    Path::new(&format!("/proc/{pid}")).exists()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn versions() {
        assert!(newer("0.5.1", "0.5.0"));
        assert!(newer("v0.6.0", "0.5.9"));
        assert!(newer("0.4.9", "0.4.9-rc1"));
        assert!(!newer("0.4.9-rc1", "0.4.9"));
        assert!(!newer("0.5.0", "0.5.0"));
        assert!(!newer("0.4.10", "0.5.0"));
        assert!(!newer("junk", "0.5.0"));
        assert!(is_patch("0.5.3", "0.5.0"));
        assert!(!is_patch("0.6.0", "0.5.0"));
    }

    #[test]
    fn sums() {
        let s = "affb25550a7a7cff262463be9fe66877746c5342e5753e754d9a01f6ec1989da  DriveLinkHub-v0.5.0-windows-x64.zip\n\
                 064884CF967F86AC3E6A4911D05349BB916A1B56490C1DC462A603CBD28B57DB *DriveLinkHub.exe\n";
        assert_eq!(sum_for(s, "DriveLinkHub.exe").as_deref(), Some("064884cf967f86ac3e6a4911d05349bb916a1b56490c1dc462a603cbd28b57db"));
        assert_eq!(sum_for(s, "Other.exe"), None);
    }
}
