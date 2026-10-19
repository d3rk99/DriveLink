// The map editor: /admin (HTML page + Leaflet, embedded in the exe) and its JSON API. Password = hub.json
// adminPassword (browser login box, any user name). Edits go into the layouts table; the DayZ servers pick them up
// within ~30 s (/api/layout) - no restart.
use crate::api::Shared;
use axum::body::Bytes;
use axum::extract::{Path, Request, State};
use axum::http::{header, StatusCode};
use axum::middleware::Next;
use axum::response::{IntoResponse, Response};
use axum::routing::{get, post};
use axum::Router;
use base64::Engine;
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
        .route("/admin/maps/{map}/{z}/{file}", get(tile))
        .layer(axum::middleware::from_fn_with_state(h, auth))
}

// The hub's own machine (localhost) needs no password; any other computer does.
async fn auth(State(h): State<Shared>, axum::extract::ConnectInfo(addr): axum::extract::ConnectInfo<std::net::SocketAddr>, req: Request, next: Next) -> Response {
    if addr.ip().is_loopback() {
        return next.run(req).await;
    }
    let ok = req
        .headers()
        .get(header::AUTHORIZATION)
        .and_then(|v| v.to_str().ok())
        .and_then(|v| v.strip_prefix("Basic "))
        .and_then(|b| base64::engine::general_purpose::STANDARD.decode(b).ok())
        .and_then(|b| String::from_utf8(b).ok())
        .map(|up| {
            let pass = up.split_once(':').map(|(_, p)| p).unwrap_or("");
            bool::from(pass.as_bytes().ct_eq(h.cfg.admin_password.as_bytes()))
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

async fn state(State(h): State<Shared>) -> Response {
    let db = h.db.lock().unwrap();
    let mut servers = Vec::new();
    for s in &h.cfg.servers {
        let l = crate::layout::get(&db, &s.id).ok().flatten();
        let map = l.as_ref().map(|l| l.map.clone()).filter(|m| !m.is_empty()).unwrap_or_else(|| guess_map(&s.id, &s.name).to_string());
        let layout = l.map(|l| crate::layout::to_json(&l)).unwrap_or(json!({"rev": 0, "zones": [], "arrivals": []}));
        servers.push(json!({"id": s.id, "name": s.name, "map": map, "layout": layout}));
    }
    drop(db);
    let avail = crate::maps::available(&h.cfg.dayz_path);
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
        maps.insert(
            n.clone(),
            json!({
                "inGameFiles": avail.contains_key(&n),
                "meta": crate::maps::meta(&h.dir, &n),
                "status": status.get(&n),
            }),
        );
    }
    let tools = crate::maps::tools_dir(&h.cfg.dayz_tools_path);
    Json(json!({"servers": servers, "maps": maps, "tools": tools.map(|t| t.display().to_string())})).into_response()
}

struct Json(Value);
impl IntoResponse for Json {
    fn into_response(self) -> Response {
        ([(header::CONTENT_TYPE, "application/json")], self.0.to_string()).into_response()
    }
}

async fn save_layout(State(h): State<Shared>, Path(server): Path<String>, body: Bytes) -> Response {
    if h.cfg.server(&server).is_none() {
        return (StatusCode::NOT_FOUND, "unknown server").into_response();
    }
    let v: Value = match serde_json::from_slice(&body) {
        Ok(v) => v,
        Err(e) => return (StatusCode::BAD_REQUEST, format!("bad json: {e}")).into_response(),
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

async fn build_map(State(h): State<Shared>, Path(map): Path<String>) -> Response {
    let avail = crate::maps::available(&h.cfg.dayz_path);
    let Some(pbo) = avail.get(&map).cloned() else {
        return (StatusCode::NOT_FOUND, format!("no worlds_{map}_data.pbo found (set dayzPath in hub.json)")).into_response();
    };
    let Some(tools) = crate::maps::tools_dir(&h.cfg.dayz_tools_path) else {
        return (StatusCode::NOT_FOUND, "DayZ Tools not found (install it from Steam, or set dayzToolsPath in hub.json)").into_response();
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
