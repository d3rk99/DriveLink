// The hub's web app: /admin (HTML page + Leaflet, embedded in the exe) and its JSON API.
//   Map      - leaving zones + arrival points per server (layouts table); servers pick edits up within ~30 s
//   Servers  - add / edit / remove DayZ servers, approve new ones that asked to join (saved to hub.json at once)
//   Players  - who is where, trips, snapshot contents; move / release / drop (src/players.rs)
//   Setup    - what a DayZ server needs in its config.json, hub settings, Windows Firewall rules, hub updates
// Password = hub.json adminPassword (browser login box, any user name); none needed on the hub's own machine.
use crate::api::Shared;
use crate::config::ServerEntry;
use axum::body::Bytes;
use axum::extract::{Path, Request, State};
use axum::http::{header, StatusCode};
use axum::middleware::Next;
use axum::response::{IntoResponse, Response};
use axum::routing::{get, post};
use axum::Router;
use base64::Engine;
use serde::Deserialize;
use serde_json::{json, Value};
use subtle::ConstantTimeEq;

const PAGE: &str = include_str!("../web/admin.html");
const LEAFLET_JS: &str = include_str!("../web/vendor/leaflet.js");
const LEAFLET_CSS: &str = include_str!("../web/vendor/leaflet.css");

pub fn routes(h: Shared) -> Router<Shared> {
    Router::new()
        .route("/admin", get(|| async { ([(header::CONTENT_TYPE, "text/html; charset=utf-8")], PAGE) }))
        .route("/admin/vendor/leaflet.js", get(|| async { ([(header::CONTENT_TYPE, "application/javascript")], LEAFLET_JS) }))
        .route("/admin/vendor/leaflet.css", get(|| async { ([(header::CONTENT_TYPE, "text/css")], LEAFLET_CSS) }))
        .route("/admin/api/state", get(state))
        .route("/admin/api/layout/{server}", post(save_layout))
        .route("/admin/api/maps/{map}/build", post(build_map))
        .route("/admin/api/maps/{map}/pbo", post(pin_map_pbo))
        .route("/admin/api/maps/{map}/image", post(map_image).layer(axum::extract::DefaultBodyLimit::max(200 << 20)))
        .route("/admin/maps/{map}/{z}/{file}", get(tile))
        .route("/admin/api/servers", post(save_server))
        .route("/admin/api/servers/{id}/delete", post(delete_server))
        .route("/admin/api/servers/{id}/travel", post(save_travel))
        .route("/admin/api/servers/{id}/helper-route", post(save_helper_route))
        .route("/admin/api/pending/{claim}/approve", post(approve))
        .route("/admin/api/pending/{claim}/dismiss", post(dismiss))
        .route("/admin/api/settings", post(save_settings))
        .route("/admin/api/firewall", get(firewall_state))
        .route("/admin/api/firewall/apply", post(firewall_apply))
        .route("/admin/api/cars", get(cars))
        .route("/admin/api/update", get(update_status))
        .route("/admin/api/update/check", post(update_check))
        .route("/admin/api/update/install", post(update_install))
        .route("/admin/api/players", get(crate::players::list))
        .route("/admin/api/players/{sid}", get(crate::players::detail))
        .route("/admin/api/players/{sid}/action", post(crate::players::action))
        .layer(axum::middleware::from_fn_with_state(h, auth))
}

// The hub's own machine (localhost) needs no password; any other computer does.
async fn auth(State(h): State<Shared>, axum::extract::ConnectInfo(addr): axum::extract::ConnectInfo<std::net::SocketAddr>, req: Request, next: Next) -> Response {
    if addr.ip().is_loopback() {
        return next.run(req).await;
    }
    let pw = h.cfg().admin_password;
    let ok = req
        .headers()
        .get(header::AUTHORIZATION)
        .and_then(|v| v.to_str().ok())
        .and_then(|v| v.strip_prefix("Basic "))
        .and_then(|b| base64::engine::general_purpose::STANDARD.decode(b).ok())
        .and_then(|b| String::from_utf8(b).ok())
        .map(|up| {
            let pass = up.split_once(':').map(|(_, p)| p).unwrap_or("");
            bool::from(pass.as_bytes().ct_eq(pw.as_bytes()))
        })
        .unwrap_or(false);
    if !ok {
        return (StatusCode::UNAUTHORIZED, [(header::WWW_AUTHENTICATE, "Basic realm=\"DriveLink Hub\"")], "password needed (hub.json adminPassword)").into_response();
    }
    next.run(req).await
}

fn guess_map(id: &str, name: &str) -> &'static str {
    let s = format!("{id} {name}").to_lowercase();
    if s.contains("chern") {
        "chernarusplus"
    } else if s.contains("livon") || s.contains("enoch") {
        "enoch"
    } else if s.contains("sakhal") {
        "sakhal"
    } else {
        ""
    }
}

struct Json(Value);
impl IntoResponse for Json {
    fn into_response(self) -> Response {
        ([(header::CONTENT_TYPE, "application/json")], self.0.to_string()).into_response()
    }
}

fn bad(msg: impl Into<String>) -> Response {
    (StatusCode::BAD_REQUEST, msg.into()).into_response()
}

// Map page car layer: travelled cars where their server last reported them, with their trips (src/cars.rs).
async fn cars(State(h): State<Shared>) -> Response {
    let db = h.db.lock().unwrap();
    match crate::cars::for_map(&db) {
        Ok(cars) => {
            let reports: serde_json::Map<String, Value> = h.car_reports.lock().unwrap().iter().map(|(k, (at, n))| (k.clone(), json!({"at": at, "here": n}))).collect();
            Json(json!({"cars": cars, "reports": reports})).into_response()
        }
        Err(e) => (StatusCode::INTERNAL_SERVER_ERROR, format!("database: {e}")).into_response(),
    }
}

fn update_json(h: &Shared) -> Value {
    let mode = h.cfg().auto_update;
    json!({"status": crate::update::status(), "mode": if mode.is_empty() { "notify".to_string() } else { mode }})
}

async fn update_status(State(h): State<Shared>) -> Response {
    Json(update_json(&h)).into_response()
}

async fn update_check(State(h): State<Shared>) -> Response {
    let hh = h.clone();
    match tokio::task::spawn_blocking(move || crate::update::check_and_maybe_install(&hh, false)).await {
        Ok(Ok(())) => Json(update_json(&h)).into_response(),
        Ok(Err(e)) => bad(e),
        Err(e) => bad(e.to_string()),
    }
}

async fn update_install(State(h): State<Shared>) -> Response {
    crate::logf!("setup page: update now");
    let hh = h.clone();
    match tokio::task::spawn_blocking(move || crate::update::install_now(&hh)).await {
        Ok(Ok(())) => Json(update_json(&h)).into_response(),
        Ok(Err(e)) => bad(e),
        Err(e) => bad(e.to_string()),
    }
}

fn hub_port(listen: &str) -> u16 {
    listen.rsplit(':').next().and_then(|p| p.parse().ok()).unwrap_or(0)
}

async fn state(State(h): State<Shared>) -> Response {
    let cfg = h.cfg();
    let db = h.db.lock().unwrap();
    let mut servers = Vec::new();
    for s in &cfg.servers {
        let l = crate::layout::get(&db, &s.id).ok().flatten();
        let map = l
            .as_ref()
            .map(|l| l.map.clone())
            .filter(|m| !m.is_empty())
            .or_else(|| Some(s.world.clone()).filter(|w| !w.is_empty()))
            .unwrap_or_else(|| guess_map(&s.id, &s.name).to_string());
        let layout = l.map(|l| crate::layout::to_json(&l)).unwrap_or(json!({"rev": 0, "zones": [], "arrivals": []}));
        // seconds since its last request (servers ask for their layout every 30 s)
        let ago: Option<i64> = db
            .query_row("SELECT CAST(strftime('%s','now') AS INTEGER) - CAST(strftime('%s', last_seen) AS INTEGER) FROM servers WHERE id=?1", [&s.id], |r| r.get(0))
            .ok();
        servers.push(json!({
            "id": s.id, "name": s.name, "lanHost": s.lan_host, "publicHost": s.public_host, "port": s.port,
            "queryPort": s.query_port, "password": s.password, "world": s.world, "map": map, "layout": layout,
            "seenSecondsAgo": ago, "local": h.is_local_server(&s.id), "travel": s.travel,
            "mod": h.versions.lock().unwrap().get(&s.id).cloned(), "hubProtocol": crate::api::PROTOCOL,
            "spots": h.spots.lock().unwrap().get(&s.id).cloned(),
            "clientMods": crate::mod_policy::report(&h,s), "helperRoutes": crate::mod_policy::routes(&h,&s.id),
        }));
    }
    drop(db);
    let avail = crate::maps::available(&cfg);
    let mut maps = serde_json::Map::new();
    let status = h.maps.lock().unwrap().clone();
    let mut names: Vec<String> = avail.keys().cloned().collect();
    for s in &servers {
        if let Some(m) = s["map"].as_str() {
            if !m.is_empty() && !names.iter().any(|n| n == m) {
                names.push(m.to_string());
            }
        }
    }
    names.sort();
    for n in names {
        let size = cfg.servers.iter().find(|s| s.world.eq_ignore_ascii_case(&n) && s.world_size > 0).map(|s| s.world_size);
        maps.insert(n.clone(), json!({"inGameFiles": avail.contains_key(&n), "pbo": avail.get(&n).map(|p| p.display().to_string()), "pinned": cfg.map_pbos.contains_key(&n),
                                      "meta": crate::maps::meta(&h.dir, &n), "status": status.get(&n), "worldSize": size}));
    }
    let tools = crate::maps::tools_dir(&cfg.dayz_tools_path);
    let pending: Vec<crate::api::Pending> = h.pending.lock().unwrap().values().cloned().collect();
    let lan = crate::net::lan_ip().map(|i| i.to_string()).unwrap_or_default();
    Json(json!({
        "servers": servers, "maps": maps, "tools": tools.map(|t| t.display().to_string()), "pending": pending,
        "hub": {
            "serverKey": cfg.server_key, "lanIp": lan, "publicHost": cfg.public_host, "homeServer": cfg.home_server, "experimentalPlanes": cfg.experimental_planes,
            "version": crate::update::VERSION, "update": crate::update::status(),
            "httpPort": hub_port(&cfg.listen), "httpsPort": hub_port(&cfg.listen_tls), "listen": cfg.listen, "listenTls": cfg.listen_tls,
            "dayzPath": cfg.dayz_path, "dayzToolsPath": cfg.dayz_tools_path, "hasPassword": !cfg.admin_password.is_empty(),
        },
    }))
    .into_response()
}

async fn save_layout(State(h): State<Shared>, Path(server): Path<String>, body: Bytes) -> Response {
    if h.cfg().server(&server).is_none() {
        return (StatusCode::NOT_FOUND, "unknown server").into_response();
    }
    let v: Value = match serde_json::from_slice(&body) {
        Ok(v) => v,
        Err(e) => return bad(format!("bad json: {e}")),
    };
    let zones = crate::layout::clean_zones(&v["zones"]);
    let arrivals = crate::layout::clean_arrivals(&v["arrivals"]);
    let map = v.get("map").and_then(Value::as_str);
    let db = h.db.lock().unwrap();
    match crate::layout::save(&db, &server, &zones.to_string(), &arrivals.to_string(), map) {
        Ok(rev) => {
            crate::logf!("map editor: layout of {server} saved, rev {rev} ({} zones, {} arrival points)", zones.as_array().map(|a| a.len()).unwrap_or(0), arrivals.as_array().map(|a| a.len()).unwrap_or(0));
            Json(json!({"ok": true, "rev": rev}))
        }
        .into_response(),
        Err(e) => (StatusCode::INTERNAL_SERVER_ERROR, format!("database: {e}")).into_response(),
    }
}

// ---------------- servers ----------------

#[derive(Deserialize)]
#[serde(rename_all = "camelCase")]
struct ServerForm {
    #[serde(default)]
    is_new: bool,
    id: String,
    name: String,
    #[serde(default)]
    lan_host: String,
    #[serde(default)]
    public_host: String,
    port: u16,
    #[serde(default)]
    password: String,
    /// approve: this join request is a server the hub has already (reinstalled, or set up before joining existed)
    #[serde(default)]
    link: bool,
}

fn valid_id(id: &str) -> bool {
    !id.is_empty() && id.len() <= 32 && id.chars().all(|c| c.is_ascii_lowercase() || c.is_ascii_digit() || c == '-' || c == '_')
}

fn apply_form(e: &mut ServerEntry, f: &ServerForm) {
    e.name = f.name.trim().to_string();
    e.lan_host = f.lan_host.trim().to_string();
    e.public_host = f.public_host.trim().to_string();
    e.port = f.port;
    e.password = f.password.clone();
}

// add (isNew) or edit a server. The id can't change later: trips and characters are stored under it.
async fn save_server(State(h): State<Shared>, body: Bytes) -> Response {
    let f: ServerForm = match serde_json::from_slice(&body) {
        Ok(f) => f,
        Err(e) => return bad(format!("bad form: {e}")),
    };
    if !valid_id(&f.id) {
        return bad("id: 1-32 characters, lower case letters, digits, - or _");
    }
    if f.name.trim().is_empty() {
        return bad("name is empty");
    }
    let res = h.update_cfg(|c| {
        match c.servers.iter_mut().find(|s| s.id == f.id) {
            Some(_) if f.is_new => return Err(format!("there is already a server with the id {:?}", f.id)),
            Some(e) => apply_form(e, &f),
            None if !f.is_new => return Err(format!("no server {:?}", f.id)),
            None => {
                let mut e = ServerEntry { id: f.id.clone(), ..Default::default() };
                apply_form(&mut e, &f);
                c.servers.push(e);
            }
        }
        Ok(())
    });
    match res {
        Ok(()) => {
            crate::logf!("servers page: {} {:?} saved", if f.is_new { "added" } else { "edited" }, f.id);
            Json(json!({"ok": true})).into_response()
        }
        Err(e) => bad(e),
    }
}

#[derive(Deserialize)]
#[serde(rename_all="camelCase")]
struct HelperRouteForm { target_id:String, mode:String }
async fn save_helper_route(State(h):State<Shared>,Path(id):Path<String>,body:Bytes)->Response {
    let f:HelperRouteForm=match serde_json::from_slice(&body){Ok(f)=>f,Err(e)=>return bad(format!("bad helper route: {e}"))};
    if !["auto","direct","restart"].contains(&f.mode.as_str()){return bad("mode must be auto, direct or restart");}
    let result=h.update_cfg(|cfg|{
        if id==f.target_id || cfg.server(&f.target_id).is_none(){return Err("unknown/self destination".into());}
        let source=cfg.servers.iter_mut().find(|s|s.id==id).ok_or("unknown source server")?;
        if f.mode=="auto"{source.helper_routes.remove(&f.target_id);}else{source.helper_routes.insert(f.target_id.clone(),f.mode.clone());}
        Ok(())
    });
    match result { Ok(())=>Json(json!({"ok":true})).into_response(),Err(e)=>bad(e) }
}

// travel rules for trips leaving this server; the DayZ server picks them up with its next layout poll (~30 s)
async fn save_travel(State(h): State<Shared>, Path(id): Path<String>, body: Bytes) -> Response {
    let t: crate::config::TravelRules = match serde_json::from_slice(&body) {
        Ok(t) => t,
        Err(e) => return bad(format!("bad travel rules: {e}")),
    };
    let t = t.clamped();
    let res = h.update_cfg(|c| match c.servers.iter_mut().find(|s| s.id == id) {
        Some(s) => {
            s.travel = t.clone();
            Ok(())
        }
        None => Err(format!("no server {id:?}")),
    });
    if let Err(e) = res {
        return bad(e);
    }
    let rev = crate::layout::bump(&h.db.lock().unwrap(), &id).ok().flatten();
    crate::logf!("servers page: travel rules of {id:?} saved (mode {}, time {}){}", t.mode, if t.time_enabled { "on" } else { "off" }, rev.map(|r| format!(", layout rev {r}")).unwrap_or_default());
    Json(json!({"ok": true, "travel": t, "rev": rev})).into_response()
}

async fn delete_server(State(h): State<Shared>, Path(id): Path<String>) -> Response {
    let res = h.update_cfg(|c| {
        let n = c.servers.len();
        c.servers.retain(|s| s.id != id);
        if c.servers.len() == n { Err(format!("no server {id:?}")) } else { Ok(()) }
    });
    match res {
        Ok(()) => {
            crate::logf!("servers page: {id:?} removed (its zones stay in the database in case it comes back)");
            Json(json!({"ok": true})).into_response()
        }
        Err(e) => bad(e),
    }
}

// a server that asked to join: make it a real one. The DayZ server learns its id on its next hello (~15 s).
async fn approve(State(h): State<Shared>, Path(claim): Path<String>, body: Bytes) -> Response {
    let f: ServerForm = match serde_json::from_slice(&body) {
        Ok(f) => f,
        Err(e) => return bad(format!("bad form: {e}")),
    };
    let Some(p) = h.pending.lock().unwrap().get(&claim).cloned() else {
        return bad("that join request is gone (the DayZ server stopped asking)");
    };
    if !valid_id(&f.id) {
        return bad("id: 1-32 characters, lower case letters, digits, - or _");
    }
    let res = h.update_cfg(|c| {
        if f.link {
            let Some(e) = c.servers.iter_mut().find(|s| s.id == f.id) else { return Err(format!("no server {:?}", f.id)) };
            e.claim = claim.clone();
            if p.port != 0 {
                e.port = p.port;
            }
            if p.query_port != 0 {
                e.query_port = p.query_port;
            }
            if !p.world.is_empty() {
                e.world = p.world.clone();
            }
            return Ok(());
        }
        if c.server(&f.id).is_some() {
            return Err(format!("there is already a server with the id {:?} (pick it under 'This is' to link it)", f.id));
        }
        let mut e = ServerEntry { id: f.id.clone(), claim: claim.clone(), query_port: p.query_port, world: p.world.clone(), ..Default::default() };
        apply_form(&mut e, &f);
        c.servers.push(e);
        Ok(())
    });
    match res {
        Ok(()) => {
            h.pending.lock().unwrap().remove(&claim);
            crate::logf!("servers page: join request from {} approved as {:?}{}", p.addr, f.id, if f.link { " (existing server)" } else { "" });
            Json(json!({"ok": true})).into_response()
        }
        Err(e) => bad(e),
    }
}

async fn dismiss(State(h): State<Shared>, Path(claim): Path<String>) -> Response {
    h.pending.lock().unwrap().remove(&claim);
    Json(json!({"ok": true})).into_response()
}

// ---------------- setup ----------------

#[derive(Deserialize, Default)]
#[serde(rename_all = "camelCase", default)]
struct SettingsForm {
    public_host: Option<String>,
    admin_password: Option<String>,
    dayz_path: Option<String>,
    dayz_tools_path: Option<String>,
    home_server: Option<String>,
    auto_update: Option<String>,
    new_server_key: bool,
}

async fn save_settings(State(h): State<Shared>, body: Bytes) -> Response {
    let f: SettingsForm = match serde_json::from_slice(&body) {
        Ok(f) => f,
        Err(e) => return bad(format!("bad form: {e}")),
    };
    if let Some(p) = &f.admin_password {
        if p.len() < 8 {
            return bad("the password needs 8+ characters");
        }
    }
    let res = h.update_cfg(|c| {
        if let Some(v) = &f.public_host {
            c.public_host = v.trim().to_string();
        }
        if let Some(v) = &f.admin_password {
            c.admin_password = v.clone();
        }
        if let Some(v) = &f.dayz_path {
            c.dayz_path = v.trim().to_string();
        }
        if let Some(v) = &f.dayz_tools_path {
            c.dayz_tools_path = v.trim().to_string();
        }
        if let Some(v) = &f.home_server {
            let v = v.trim();
            if !v.is_empty() && c.server(v).is_none() {
                return Err(format!("unknown server {v:?}"));
            }
            c.home_server = v.to_string();
        }
        if let Some(v) = &f.auto_update {
            c.auto_update = v.trim().to_string();
        }
        if f.new_server_key {
            c.server_key = crate::config::random_key();
        }
        Ok(())
    });
    match res {
        Ok(()) => {
            if let Some(v) = &f.home_server {
                crate::logf!("starting server: {}", if v.trim().is_empty() { "off (respawn where you die)" } else { v.trim() });
            }
            crate::logf!("setup page: settings saved{}", if f.new_server_key { " (NEW server key: every DayZ server needs it now)" } else { "" });
            Json(json!({"ok": true})).into_response()
        }
        Err(e) => bad(e),
    }
}

fn firewall_json(h: &Shared, rules: &[crate::firewall::Rule]) -> Value {
    let cfg = h.cfg();
    let remote: Vec<Value> = cfg
        .servers
        .iter()
        .filter(|s| s.port != 0 && !h.is_local_server(&s.id))
        .map(|s| json!({"id": s.id, "name": s.name, "powershell": crate::firewall::remote_powershell(&s.id, s.port, s.query_port), "seen": h.seen.lock().unwrap().get(&s.id).cloned()}))
        .collect();
    json!({"rules": rules, "commands": crate::firewall::hub_commands(rules), "remote": remote, "windows": cfg!(windows)})
}

async fn firewall_state(State(h): State<Shared>) -> Response {
    let hh = h.clone();
    let rules = tokio::task::spawn_blocking(move || crate::firewall::wanted(&hh.cfg(), &|id| hh.is_local_server(id))).await.unwrap_or_default();
    Json(firewall_json(&h, &rules)).into_response()
}

async fn firewall_apply(State(h): State<Shared>) -> Response {
    let hh = h.clone();
    let out = tokio::task::spawn_blocking(move || {
        let rules = crate::firewall::wanted(&hh.cfg(), &|id| hh.is_local_server(id));
        let res = crate::firewall::apply(&rules, &hh.dir);
        let after = crate::firewall::wanted(&hh.cfg(), &|id| hh.is_local_server(id));
        (res, after)
    })
    .await;
    let Ok((res, after)) = out else {
        return (StatusCode::INTERNAL_SERVER_ERROR, "firewall task failed").into_response();
    };
    let mut v = firewall_json(&h, &after);
    match res {
        Ok(m) => {
            crate::logf!("setup page: firewall: {m}");
            v["ok"] = json!(true);
            v["message"] = json!(m);
        }
        Err(e) => {
            crate::logf!("setup page: firewall: {e}");
            v["ok"] = json!(false);
            v["message"] = json!(e);
        }
    }
    Json(v).into_response()
}

// ---------------- map tiles ----------------

async fn build_map(State(h): State<Shared>, Path(map): Path<String>) -> Response {
    let cfg = h.cfg();
    let avail = crate::maps::available(&cfg);
    let Some(pbo) = avail.get(&map).cloned() else {
        return (StatusCode::NOT_FOUND, format!("no data PBO with satellite tiles found for {map} (install the map mod, or pick its PBO on the Map page)")).into_response();
    };
    let Some(tools) = crate::maps::tools_dir(&cfg.dayz_tools_path) else {
        return (StatusCode::NOT_FOUND, "DayZ Tools not found (install it from Steam, or set its folder on the Setup page)").into_response();
    };
    {
        let mut st = h.maps.lock().unwrap();
        if let Some(s) = st.get(&map) {
            if !s.starts_with("ready") && !s.starts_with("FAILED") {
                return Json(json!({"ok": true, "status": s})).into_response();
            }
        }
        st.insert(map.clone(), "starting".into());
    }
    let (dir, status) = (h.dir.clone(), h.maps.clone());
    std::thread::spawn(move || crate::maps::build(dir, map, pbo, tools, status));
    Json(json!({"ok": true, "status": "starting"})).into_response()
}

#[derive(serde::Deserialize)]
struct PinForm {
    #[serde(default)]
    path: String,
}

// POST /admin/api/maps/{map}/pbo {path}: this map's data PBO, by hand ("" = forget it, find it by itself again)
async fn pin_map_pbo(State(h): State<Shared>, Path(map): Path<String>, body: axum::body::Bytes) -> Response {
    let f: PinForm = match serde_json::from_slice(&body) {
        Ok(f) => f,
        Err(e) => return (StatusCode::BAD_REQUEST, format!("bad form: {e}")).into_response(),
    };
    let map = map.to_lowercase();
    let path = f.path.trim().trim_matches('"').to_string();
    if !path.is_empty() {
        let p = std::path::PathBuf::from(&path);
        if !p.is_file() {
            return (StatusCode::BAD_REQUEST, format!("no such file: {path}")).into_response();
        }
        if crate::maps::probe_pbo(&p).is_none() {
            return (StatusCode::BAD_REQUEST, "that PBO has no satellite tiles (layers\\s_*_lco.paa) the hub can read - upload a picture of the map instead").into_response();
        }
    }
    let res = h.update_cfg(|c| {
        if path.is_empty() {
            c.map_pbos.remove(&map);
        } else {
            c.map_pbos.insert(map.clone(), path.clone());
        }
        Ok(())
    });
    match res {
        Ok(()) => {
            crate::logf!("map {map}: data PBO {}", if path.is_empty() { "found by itself again".to_string() } else { path.clone() });
            Json(json!({"ok": true})).into_response()
        }
        Err(e) => (StatusCode::INTERNAL_SERVER_ERROR, e).into_response(),
    }
}

#[derive(serde::Deserialize)]
struct ImageQ {
    size: f64,
}

// POST /admin/api/maps/{map}/image?size=<metres>  body = a PNG / JPG of the whole map, north up
async fn map_image(State(h): State<Shared>, Path(map): Path<String>, axum::extract::Query(q): axum::extract::Query<ImageQ>, body: axum::body::Bytes) -> Response {
    let map = map.to_lowercase();
    if !map.chars().all(|c| c.is_ascii_alphanumeric() || c == '_' || c == '-') || map.is_empty() {
        return (StatusCode::BAD_REQUEST, "bad map name").into_response();
    }
    if !(q.size.is_finite() && (500.0..=100_000.0).contains(&q.size)) {
        return (StatusCode::BAD_REQUEST, "the map size must be 500 - 100000 m").into_response();
    }
    if body.len() < 100 {
        return (StatusCode::BAD_REQUEST, "no picture sent").into_response();
    }
    {
        let mut st = h.maps.lock().unwrap();
        if let Some(s) = st.get(&map) {
            if !s.starts_with("ready") && !s.starts_with("FAILED") {
                return Json(json!({"ok": true, "status": s})).into_response();
            }
        }
        st.insert(map.clone(), "starting".into());
    }
    let (dir, status, bytes, size) = (h.dir.clone(), h.maps.clone(), body.to_vec(), q.size);
    std::thread::spawn(move || crate::maps::build_from_image(dir, map, bytes, size, status));
    Json(json!({"ok": true, "status": "starting"})).into_response()
}

async fn tile(State(h): State<Shared>, Path((map, z, file)): Path<(String, String, String)>) -> Response {
    let ok = |s: &str| !s.is_empty() && s.chars().all(|c| c.is_ascii_alphanumeric() || c == '_' || c == '-' || c == '.') && !s.contains("..");
    if !ok(&map) || !ok(&z) || !ok(&file) {
        return StatusCode::BAD_REQUEST.into_response();
    }
    let p = h.dir.join("maps").join(&map).join(&z).join(&file);
    match std::fs::read(&p) {
        Ok(b) => ([(header::CONTENT_TYPE, "image/jpeg"), (header::CACHE_CONTROL, "max-age=86400")], b).into_response(),
        Err(_) => StatusCode::NOT_FOUND.into_response(),
    }
}
