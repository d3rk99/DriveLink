// One log line format for console and hub.log ("2026/10/18 21:00:00 message"), same as the Go hub's.
use std::fs::OpenOptions;
use std::io::Write;
use std::path::PathBuf;
use std::sync::{Mutex, OnceLock};

static FILE: OnceLock<Mutex<Option<std::fs::File>>> = OnceLock::new();
static CONSOLE: OnceLock<bool> = OnceLock::new();

pub fn init(path: PathBuf, console: bool) {
    let f = OpenOptions::new().create(true).append(true).open(path).ok();
    let _ = FILE.set(Mutex::new(f));
    let _ = CONSOLE.set(console);
}

pub fn line(msg: &str) {
    let l = format!("{} {}\n", chrono::Local::now().format("%Y/%m/%d %H:%M:%S"), msg);
    if *CONSOLE.get().unwrap_or(&true) {
        print!("{l}");
    }
    if let Some(m) = FILE.get() {
        if let Ok(mut g) = m.lock() {
            if let Some(f) = g.as_mut() {
                let _ = f.write_all(l.as_bytes());
            }
        }
    }
}

#[macro_export]
macro_rules! logf {
    ($($a:tt)*) => { $crate::log::line(&format!($($a)*)) };
}
