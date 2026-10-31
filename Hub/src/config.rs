// hub.json - the same file as the Go hub's (it keeps working unchanged), plus the optional HTTPS listener.
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
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
    /// that map's size in metres (World.GetWorldSize), reported by the server; 0 = not known yet
    #[serde(default, skip_serializing_if = "is_zero32")]
    pub world_size: u32,
    /// the join id the DayZ server made for itself before it had a serverId (approved in /admin)
    #[serde(default, skip_serializing_if = "String::is_empty")]
    pub claim: String,
    /// rules for trips that START here (Research/travel-cost-design.md); sent to the server with its layout
    #[serde(default)]
    pub travel: TravelRules,
    /// Optional per-destination override; absent entries use automatic advertised client-mod comparison.
    #[serde(default, skip_serializing_if = "HashMap::is_empty")]
    pub helper_routes: HashMap<String, String>,
}

/// Travel cost + time for trips leaving one server. Costs are per 10 km of the zone's distance (0 km = free).
#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
#[serde(rename_all = "camelCase", default)]
pub struct TravelRules {
    /// "refuse" = not enough supplies, no trip; "weakened" = the trip goes and the road decides
    pub mode: String,
    /// food (DayZ energy units = kcal) and water (ml) per 10 km, per person; a full DayZ bar is 5000 of each
    pub food_foot_kcal: f64,
    pub water_foot_ml: f64,
    pub food_car_kcal: f64,  // in a vehicle
    pub water_car_ml: f64,
    /// before kcal/ml: % of a full bar (x 50 = kcal or ml). Read from old hub.json / old clients, never written.
    #[serde(skip_serializing)]
    pub food_foot_pct: Option<f64>,
    #[serde(skip_serializing)]
    pub water_foot_pct: Option<f64>,
    #[serde(skip_serializing)]
    pub food_car_pct: Option<f64>,
    #[serde(skip_serializing)]
    pub water_car_pct: Option<f64>,
    pub fuel_liters: f64,    // from the car's tank
    /// fuel by each vehicle's in-game consumption (CfgVehicles fuelConsumption, as L/100 km) instead of fuel_liters
    pub fuel_per_vehicle: bool,
    pub fuel_scale: f64,      // x the in-game consumption
    pub fuel_boat_scale: f64, // boats: x this too (the game's boat burns far less than a real outboard)
    pub swim_food_x: f64,     // a boat party with no boat swims: food + water x this of the foot rates
    pub drown_pct: f64,       // weakened: drown chance per event leg swum (x2 the 2nd leg, x3 the 3rd ...)
    /// the road is played in legs of this many km: every chance below (finds, attacks, flats, drowning) is rolled
    /// once per leg. Costs (food, water, fuel, wear) stay per 10 km.
    pub event_km: f64,
    /// refuse mode: a player who is refused sees his supplies, what he needs and his chance of surviving, and can
    /// travel anyway (that trip is played like weakened mode)
    pub allow_anyway: bool,
    // weakened: what the road gives, chance per event leg
    pub find_gas_pct: f64,    // a working gas station / fuel dock: every tank + gasoline canister full
    pub find_food_pct: f64,   // food items into the trunk / bags
    pub find_water_pct: f64,  // a well / rain: everyone drinks, every bottle filled
    pub wear_engine_pct: f64, // engine health taken
    pub wear_wheels_pct: f64, // every wheel
    /// on = the character is "on the road" for distance / speed and can't log in anywhere until it arrives
    pub time_enabled: bool,
    pub speed_foot_kmh: f64,
    pub speed_car_kmh: f64,
    // weakened mode: the road's chances, in %
    pub luck_gas_pct: f64,       // out of fuel: chance to find some and keep the car
    pub bite_pct: f64,           // per event leg walked: infected attack (bleeding)
    pub bite_death_pct: f64,     // of those attacks: dies
    pub starve_death_pct: f64,   // short more than half of the food or water cost: dies
    /// refuse mode: car trips longer than this need a spare wheel or a TireRepairKit aboard (0 = off)
    pub spare_needed_km: f64,
    /// weakened mode (step 4): chance of a flat tyre per event leg, higher on worn wheels
    pub flat_pct: f64,
    /// the server's look in the road film: forest | field | desert | snow, "" = from the map
    pub biome: String,
    // group travel (Research/group-travel.md), for parties leaving here
    pub no_groups: bool,          // walkers who enter a zone together do NOT travel as a group
    pub no_convoys: bool,         // cars that enter a zone together do NOT travel as a convoy
    pub group_max: u32,           // players in a group on foot
    pub convoy_max: u32,          // cars in a convoy
    /// arriving HERE: how long a convoy's cars wait for one that is late before the others go (s)
    pub convoy_wait_seconds: u32,
    // the road film, for trips that END here
    pub film_off: bool,           // no film: the story only
    pub vote_skip_off: bool,      // the film can't be skipped (else: everyone watching presses Space)
}

impl Default for TravelRules {
    fn default() -> Self {
        TravelRules {
            mode: "refuse".into(),
            food_foot_kcal: 750.0,
            water_foot_ml: 750.0,
            food_car_kcal: 200.0,
            water_car_ml: 250.0,
            food_foot_pct: None,
            water_foot_pct: None,
            food_car_pct: None,
            water_car_pct: None,
            fuel_liters: 5.0,
            fuel_per_vehicle: false,
            fuel_scale: 1.0,
            fuel_boat_scale: 1.0,
            swim_food_x: 1.5,
            drown_pct: 6.0,
            event_km: 10.0,
            allow_anyway: false,
            find_gas_pct: 6.0,
            find_food_pct: 8.0,
            find_water_pct: 12.0,
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
            biome: String::new(),
            no_groups: false,
            no_convoys: false,
            group_max: 8,
            convoy_max: 4,
            convoy_wait_seconds: 120,
            film_off: false,
            vote_skip_off: false,
        }
    }
}

/// a full DayZ energy / water bar (PlayerConstants.SL_ENERGY_MAX / SL_WATER_MAX): 1 % = 50 kcal or 50 ml
pub const BAR: f64 = 5000.0;

impl TravelRules {
    /// old % settings (hub.json from before kcal/ml) into kcal / ml; true if anything was converted
    pub fn migrate(&mut self) -> bool {
        let mut any = false;
        for (old, new) in [(&mut self.food_foot_pct, &mut self.food_foot_kcal), (&mut self.water_foot_pct, &mut self.water_foot_ml),
                           (&mut self.food_car_pct, &mut self.food_car_kcal), (&mut self.water_car_pct, &mut self.water_car_ml)] {
            if let Some(p) = old.take() {
                *new = p * BAR / 100.0;
                any = true;
            }
        }
        any
    }

    /// keep what the browser sent inside sane limits
    pub fn clamped(mut self) -> Self {
        self.migrate();
        if self.mode != "weakened" {
            self.mode = "refuse".into();
        }
        if !["forest", "field", "desert", "snow"].contains(&self.biome.as_str()) {
            self.biome = String::new();
        }
        let pct = |v: f64| if v.is_finite() { v.clamp(0.0, 100.0) } else { 0.0 };
        let amount = |v: f64| if v.is_finite() { v.clamp(0.0, BAR * 4.0) } else { 0.0 };
        for v in [&mut self.food_foot_kcal, &mut self.water_foot_ml, &mut self.food_car_kcal, &mut self.water_car_ml] {
            *v = amount(*v);
        }
        for v in [&mut self.wear_engine_pct, &mut self.wear_wheels_pct,
                  &mut self.luck_gas_pct, &mut self.bite_pct, &mut self.bite_death_pct, &mut self.starve_death_pct, &mut self.flat_pct, &mut self.drown_pct,
                  &mut self.find_gas_pct, &mut self.find_food_pct, &mut self.find_water_pct] {
            *v = pct(*v);
        }
        self.event_km = if self.event_km.is_finite() && self.event_km > 0.0 { self.event_km.clamp(1.0, 1000.0) } else { 10.0 };
        self.spare_needed_km = if self.spare_needed_km.is_finite() { self.spare_needed_km.clamp(0.0, 100000.0) } else { 0.0 };
        let mult = |v: f64, d: f64| if v.is_finite() { v.clamp(0.0, 20.0) } else { d };
        self.fuel_scale = mult(self.fuel_scale, 1.0);
        self.fuel_boat_scale = mult(self.fuel_boat_scale, 1.0);
        self.swim_food_x = mult(self.swim_food_x, 1.5);
        self.fuel_liters = if self.fuel_liters.is_finite() { self.fuel_liters.clamp(0.0, 1000.0) } else { 0.0 };
        self.speed_foot_kmh = if self.speed_foot_kmh.is_finite() { self.speed_foot_kmh.clamp(0.5, 1000.0) } else { 5.0 };
        self.speed_car_kmh = if self.speed_car_kmh.is_finite() { self.speed_car_kmh.clamp(0.5, 1000.0) } else { 40.0 };
        self.group_max = self.group_max.clamp(2, 16);
        self.convoy_max = self.convoy_max.clamp(2, 8);
        self.convoy_wait_seconds = self.convoy_wait_seconds.clamp(10, 900);
        self
    }
}

fn is_zero(v: &u16) -> bool {
    *v == 0
}

fn is_zero32(v: &u32) -> bool {
    *v == 0
}

#[derive(Serialize, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
pub struct Config {
    /// plain HTTP, e.g. "0.0.0.0:7290"; "" = off
    /// map name -> its data PBO, set by hand on the Map page (a modded map the hub doesn't find by itself)
    #[serde(default, skip_serializing_if = "HashMap::is_empty")]
    pub map_pbos: HashMap<String, String>,
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
    /// the starting server: whoever dies on any other server starts his next life here (fresh spawn); "" = off,
    /// everyone respawns where he died
    #[serde(default)]
    pub home_server: String,
    /// show the plane tools on the Map page (plane zones, plane entry): LM's Planes support, on hold
    #[serde(default, skip_serializing_if = "std::ops::Not::not")]
    pub experimental_planes: bool,
    /// hub updates from GitHub releases (src/update.rs): "off" | "notify" | "auto"; "" = notify
    #[serde(default, skip_serializing_if = "String::is_empty")]
    pub auto_update: String,
    /// where to look for the latest release instead of GitHub's API (testing); "" = GitHub
    #[serde(default, skip_serializing_if = "String::is_empty")]
    pub update_url: String,
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
                map_pbos: HashMap::new(),
                public_host: String::new(),
                home_server: String::new(),
                experimental_planes: false,
                auto_update: String::new(),
                update_url: String::new(),
            };
            save(path, &c)?;
            return Ok((c, true));
        }
        Err(e) => return Err(format!("read {}: {e}", path.display())),
    };
    let mut c: Config = serde_json::from_str(&data).map_err(|e| format!("{} is not valid JSON: {e}", path.display()))?;
    // hub.json from before kcal / ml: travel food and water were % of a full bar
    let mut migrated = false;
    for s in &mut c.servers {
        migrated |= s.travel.migrate();
    }
    if migrated {
        save(path, &c)?;
    }
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
        if !["", "off", "notify", "auto"].contains(&self.auto_update.as_str()) {
            return Err(r#""autoUpdate" is "off", "notify" or "auto""#.into());
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
