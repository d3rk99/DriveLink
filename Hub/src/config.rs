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
    /// Steam query port (UDP, serverDZ.cfg steamQueryPort), reported by the server; 0 = not known yet
    #[serde(default, skip_serializing_if = "is_zero")]
    pub query_port: u16,
    /// the map it runs (GetWorldName, lower case: chernarusplus, enoch, sakhal...), reported by the server
    #[serde(default, skip_serializing_if = "String::is_empty")]
    pub world: String,
    /// the join id the DayZ server made for itself before it had a serverId (approved in /admin)
    #[serde(default, skip_serializing_if = "String::is_empty")]
    pub claim: String,
    /// rules for trips that START here (Research/travel-cost-design.md); sent to the server with its layout
    #[serde(default)]
    pub travel: TravelRules,
}

/// Travel cost + time for trips leaving one server. Costs are per 10 km of the zone's distance (0 km = free).
#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
#[serde(rename_all = "camelCase", default)]
pub struct TravelRules {
    /// "refuse" = not enough supplies, no trip; "weakened" = the trip goes and the road decides
    pub mode: String,
    pub food_foot_pct: f64,  // % of a full stomach (DayZ max 5000) per 10 km on foot
    pub water_foot_pct: f64,
    pub food_car_pct: f64,   // per person in a car
    pub water_car_pct: f64,
    pub fuel_liters: f64,    // from the car's tank
    pub wear_engine_pct: f64, // engine health taken
    pub wear_wheels_pct: f64, // every wheel
    /// on = the character is "on the road" for distance / speed and can't log in anywhere until it arrives
    pub time_enabled: bool,
    pub speed_foot_kmh: f64,
    pub speed_car_kmh: f64,
    // weakened mode: the road's chances, in %
    pub luck_gas_pct: f64,       // out of fuel: chance to find some and keep the car
    pub bite_pct: f64,           // per 10 km walked: infected attack (bleeding)
    pub bite_death_pct: f64,     // of those attacks: dies
    pub starve_death_pct: f64,   // short more than half of the food or water cost: dies
    /// refuse mode: car trips longer than this need a spare wheel or a TireRepairKit aboard (0 = off)
    pub spare_needed_km: f64,
    /// weakened mode (step 4): chance of a flat tyre per 10 km, higher on worn wheels
    pub flat_pct: f64,
}

impl Default for TravelRules {
    fn default() -> Self {
        TravelRules {
            mode: "refuse".into(),
            food_foot_pct: 15.0,
            water_foot_pct: 15.0,
            food_car_pct: 4.0,
            water_car_pct: 5.0,
            fuel_liters: 5.0,
            wear_engine_pct: 2.0,
            wear_wheels_pct: 3.0,
            time_enabled: false,
            speed_foot_kmh: 5.0,
            speed_car_kmh: 40.0,
            luck_gas_pct: 35.0,
            bite_pct: 5.0,
            bite_death_pct: 10.0,
            starve_death_pct: 20.0,
            spare_needed_km: 0.0,
            flat_pct: 3.0,
        }
    }
}

impl TravelRules {
    /// keep what the browser sent inside sane limits
    pub fn clamped(mut self) -> Self {
        if self.mode != "weakened" {
            self.mode = "refuse".into();
        }
        let pct = |v: f64| if v.is_finite() { v.clamp(0.0, 100.0) } else { 0.0 };
        for v in [&mut self.food_foot_pct, &mut self.water_foot_pct, &mut self.food_car_pct, &mut self.water_car_pct, &mut self.wear_engine_pct, &mut self.wear_wheels_pct,
                  &mut self.luck_gas_pct, &mut self.bite_pct, &mut self.bite_death_pct, &mut self.starve_death_pct, &mut self.flat_pct] {
            *v = pct(*v);
        }
        self.spare_needed_km = if self.spare_needed_km.is_finite() { self.spare_needed_km.clamp(0.0, 100000.0) } else { 0.0 };
        self.fuel_liters = if self.fuel_liters.is_finite() { self.fuel_liters.clamp(0.0, 1000.0) } else { 0.0 };
        self.speed_foot_kmh = if self.speed_foot_kmh.is_finite() { self.speed_foot_kmh.clamp(0.5, 1000.0) } else { 5.0 };
        self.speed_car_kmh = if self.speed_car_kmh.is_finite() { self.speed_car_kmh.clamp(0.5, 1000.0) } else { 40.0 };
        self
    }
}

fn is_zero(v: &u16) -> bool {
    *v == 0
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
    /// optional: this hub's internet address (name or IP), only used to show the DayZ servers what to put in hubUrl
    #[serde(default)]
    pub public_host: String,
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

pub fn save(path: &Path, c: &Config) -> Result<(), String> {
    let out = serde_json::to_string_pretty(c).map_err(|e| e.to_string())?;
    std::fs::write(path, out).map_err(|e| format!("write {}: {e}", path.display()))
}

/// (config, created). A new file gets a random server key and no servers yet.
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
                servers: Vec::new(), // added on the Servers page (or a DayZ server asks to join and is approved there)
                admin_password: String::new(),
                dayz_path: String::new(),
                dayz_tools_path: String::new(),
                public_host: String::new(),
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
