// hub.json - the same file as the Go hub's (it keeps working unchanged), plus the optional HTTPS listener.
use serde::{Deserialize, Serialize};
use std::path::Path;

#[derive(Serialize, Deserialize, Clone, Default)]
#[serde(rename_all = "camelCase")]
pub struct ServerEntry {
    pub id: String,          // short id, also set in that DayZ server's DriveLink config
    pub name: String,        // shown to players
    #[serde(default)]
    pub lan_host: String,    // address players on the same network use (e.g. 192.168.1.133)
    #[serde(default)]
    pub public_host: String, // address players from the internet use
    pub port: u16,           // game port
    #[serde(default)]
    pub password: String,    // server password, "" = none
}

#[derive(Serialize, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
pub struct Config {
    /// plain HTTP, e.g. "0.0.0.0:7290"; "" = off
    #[serde(default)]
    pub listen: String,
    /// HTTPS, e.g. "0.0.0.0:7291"; "" = off. Without tlsCert/tlsKey the hub makes a self-signed certificate.
    #[serde(default)]
    pub listen_tls: String,
    #[serde(default)]
    pub tls_cert: String,
    #[serde(default)]
    pub tls_key: String,
    /// SQLite file, relative to hub.json's folder
    #[serde(default)]
    pub database: String,
    /// the one shared secret every DayZ server sends
    pub server_key: String,
    #[serde(default)]
    pub servers: Vec<ServerEntry>,
    /// password for the map editor at /admin (made on the first start when empty)
    #[serde(default)]
    pub admin_password: String,
    /// optional: a DayZ (client or server) folder for the map images; empty = look in the Steam libraries
    #[serde(default)]
    pub dayz_path: String,
    /// optional: the "DayZ Tools" folder; empty = look in the Steam libraries
    #[serde(default)]
    pub dayz_tools_path: String,
}

/// Fills in an admin password when there is none; returns it when it was just made.
pub fn ensure_admin_password(path: &Path, c: &mut Config) -> Result<Option<String>, String> {
    if !c.admin_password.is_empty() {
        return Ok(None);
    }
    let b: [u8; 9] = rand::random();
    c.admin_password = hex::encode(b);
    save(path, c)?;
    Ok(Some(c.admin_password.clone()))
}

pub const DEFAULT_DATABASE: &str = "drivelink.db";

pub fn random_key() -> String {
    let b: [u8; 24] = rand::random();
    hex::encode(b)
}

fn save(path: &Path, c: &Config) -> Result<(), String> {
    let out = serde_json::to_string_pretty(c).map_err(|e| e.to_string())?;
    std::fs::write(path, out).map_err(|e| format!("write {}: {e}", path.display()))
}

/// (config, created). A new file gets a random server key and the two test servers.
pub fn load_or_create(path: &Path) -> Result<(Config, bool), String> {
    let data = match std::fs::read_to_string(path) {
        Ok(d) => d,
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => {
            let c = Config {
                listen: "0.0.0.0:7290".into(),
                listen_tls: "0.0.0.0:7291".into(),
                tls_cert: String::new(),
                tls_key: String::new(),
                database: DEFAULT_DATABASE.into(),
                server_key: random_key(),
                servers: vec![
                    ServerEntry { id: "chernarus".into(), name: "Chernarus".into(), lan_host: "192.168.1.133".into(), port: 2302, ..Default::default() },
                    ServerEntry { id: "livonia".into(), name: "Livonia".into(), lan_host: "192.168.1.133".into(), port: 2402, ..Default::default() },
                ],
                admin_password: String::new(),
                dayz_path: String::new(),
                dayz_tools_path: String::new(),
            };
            save(path, &c)?;
            return Ok((c, true));
        }
        Err(e) => return Err(format!("read {}: {e}", path.display())),
    };
    let mut c: Config = serde_json::from_str(&data).map_err(|e| format!("{} is not valid JSON: {e}", path.display()))?;
    // hub.json from before the SQLite switch (PostgreSQL URL, it held a password)
    if c.database.is_empty() || c.database.starts_with("postgres://") || c.database.starts_with("postgresql://") {
        c.database = DEFAULT_DATABASE.into();
        save(path, &c)?;
    }
    Ok((c, false))
}

impl Config {
    pub fn validate(&self) -> Result<(), String> {
        if self.server_key.len() < 16 {
            return Err(r#""serverKey" is too short (16+ characters)"#.into());
        }
        if self.listen.is_empty() && self.listen_tls.is_empty() {
            return Err(r#"set "listen" (HTTP) and/or "listenTls" (HTTPS)"#.into());
        }
        let mut seen = std::collections::HashSet::new();
        for s in &self.servers {
            if s.id.is_empty() || s.port == 0 || (s.lan_host.is_empty() && s.public_host.is_empty()) {
                return Err(format!("server {:?} needs id, port and lanHost or publicHost", s.id));
            }
            if !seen.insert(s.id.clone()) {
                return Err(format!("server id {:?} is listed twice", s.id));
            }
        }
        Ok(())
    }

    pub fn server(&self, id: &str) -> Option<&ServerEntry> {
        self.servers.iter().find(|s| s.id == id)
    }
}
