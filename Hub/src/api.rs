// The HTTP API every DayZ server talks to - same paths, fields and answers as the Go hub, so the mod is unchanged.
//
// DayZ's script RestApi sends the body as application/octet-stream and can only set the Content-Type header
// (vanilla 3_game/http/restapi.c), so: the body is read as raw bytes (no Json extractor, it insists on
// application/json) and the key travels in the JSON body, with the URL query as a fallback (M1 retest 2026-10-13).
use crate::config::Config;
use crate::numfix::plain_numbers;
use axum::body::Bytes;
use axum::extract::{ConnectInfo, Query, Request, State};
use axum::http::{header, HeaderMap, StatusCode};
use axum::middleware::Next;
use axum::response::{IntoResponse, Response};
use axum::routing::{get, post};
use axum::Router;
use rusqlite::{params, Connection, OptionalExtension};
use serde::Deserialize;
use serde_json::{json, value::RawValue, Value};
use std::collections::HashMap;
use std::net::SocketAddr;
use std::sync::{Arc, Mutex, RwLock};
use std::time::Instant;
use subtle::ConstantTimeEq;

const MAX_BODY: usize = 4 << 20; // 4 MB per request (a player + car snapshot is ~25 kB)

pub struct Hub {
    pub dir: std::path::PathBuf, // hub.json folder (maps/ lives here)
    pub cfg_path: std::path::PathBuf,
    pub maps: crate::maps::Status,
    pub cfg: RwLock<Config>, // edited live from /admin (Servers, Setup) and saved to hub.json
    pub pending: Mutex<HashMap<String, Pending>>, // claim -> a DayZ server asking to join
    pub seen: Mutex<HashMap<String, String>>,     // server id -> the address its last request came from
    pub db: Mutex<Connection>,
}

/// A DayZ server with the right key that the hub doesn't know yet; it shows up in /admin to be approved.
#[derive(Clone, serde::Serialize)]
#[serde(rename_all = "camelCase")]
pub struct Pending {
    pub claim: String,
    pub addr: String,
    pub local: bool, // on the hub's own machine
    pub port: u16,
    pub query_port: u16,
    pub world: String,
    pub wanted_id: String,
    pub last_seen: String,
    // what to fill in for it: same machine -> the hub's LAN address, LAN -> its address, internet -> public
    pub suggest_lan: String,
    pub suggest_public: String,
}

impl Hub {
    pub fn cfg(&self) -> Config {
        self.cfg.read().unwrap().clone()
    }

    /// Change hub.json: edit a copy, check it, write it, then use it.
    pub fn update_cfg(&self, f: impl FnOnce(&mut Config) -> Result<(), String>) -> Result<(), String> {
        let mut cur = self.cfg.write().unwrap();
        let mut next = cur.clone();
        f(&mut next)?;
        next.validate()?;
        crate::config::save(&self.cfg_path, &next)?;
        *cur = next;
        Ok(())
    }

    /// Did this server's last request come from the hub's own machine?
    pub fn is_local_server(&self, id: &str) -> bool {
        if let Some(a) = self.seen.lock().unwrap().get(id) {
            return crate::net::is_this_machine(a);
        }
        // not heard from since the hub started: go by its LAN address
        self.cfg().server(id).map(|s| crate::net::is_this_machine(&s.lan_host)).unwrap_or(false)
    }
}

pub type Shared = Arc<Hub>;

pub fn routes(h: Shared) -> Router {
    Router::new()
        .route("/api/hello", post(hello))
        .route("/api/depart", post(depart))
        .route("/api/arrive", post(arrive))
        .route("/api/confirm", post(confirm))
        .route("/api/payload", post(payload))
        .route("/api/fail", post(fail))
        .route("/api/layout", post(layout))
        .route("/health", get(health))
        .merge(crate::admin::routes(h.clone()))
        .layer(axum::extract::DefaultBodyLimit::max(MAX_BODY + 1))
        .layer(axum::middleware::from_fn(log_requests))
        .with_state(h)
}

// One line per request: path WITHOUT the query (it can hold the key), status, time, size, content type.
async fn log_requests(ConnectInfo(addr): ConnectInfo<SocketAddr>, req: Request, next: Next) -> Response {
    let start = Instant::now();
    let method = req.method().clone();
    let path = req.uri().path().to_string();
    let len = req.headers().get(header::CONTENT_LENGTH).and_then(|v| v.to_str().ok()).unwrap_or("0").to_string();
    let ctype = req.headers().get(header::CONTENT_TYPE).and_then(|v| v.to_str().ok()).unwrap_or("").to_string();
    let res = next.run(req).await;
    crate::logf!("{addr} {method} {path} -> {} ({} ms, {len} bytes in, content-type {ctype:?})", res.status().as_u16(), start.elapsed().as_millis());
    res
}

fn reply(code: StatusCode, v: Value) -> Response {
    (code, [(header::CONTENT_TYPE, "application/json")], v.to_string()).into_response()
}

fn refuse(code: StatusCode, msg: &str) -> Response {
    reply(code, json!({"ok": false, "error": msg}))
}

fn db_error(e: impl std::fmt::Display) -> Response {
    crate::logf!("database error: {e}");
    refuse(StatusCode::INTERNAL_SERVER_ERROR, &format!("database: {e}"))
}

// First 160 bytes of a body for the log, with any "key" value masked.
fn preview(b: &[u8]) -> String {
    let mut s = String::from_utf8_lossy(b).to_string();
    if let Some(i) = s.find(r#""key":""#) {
        if let Some(j) = s[i + 7..].find('"') {
            s = format!("{}***{}", &s[..i + 7], &s[i + 7 + j..]);
        }
    }
    if s.len() > 160 {
        let mut cut = 160;
        while !s.is_char_boundary(cut) {
            cut -= 1;
        }
        s = format!("{}...", &s[..cut]);
    }
    s
}

#[derive(Deserialize, Default)]
#[serde(rename_all = "camelCase", default)]
struct Base {
    server_id: String,
    key: String,
    // /api/hello only: who a new server is (it has no serverId yet, see Pending)
    claim: String,
    port: u16,
    query_port: u16,
    world: String,
}

/// Checks the key + server id (body, else URL). Ok(server id) or the refusal to send back.
fn check(h: &Hub, path: &str, addr: &SocketAddr, q: &HashMap<String, String>, body: &[u8]) -> Result<String, Response> {
    let mut b: Base = match serde_json::from_slice(body) {
        Ok(b) => b,
        Err(e) => {
            crate::logf!("refused {path} from {addr}: bad json ({e}), body starts {:?}", preview(body));
            return Err(refuse(StatusCode::BAD_REQUEST, "bad json"));
        }
    };
    if b.key.is_empty() {
        b.key = q.get("key").cloned().unwrap_or_default();
    }
    if b.server_id.is_empty() {
        b.server_id = q.get("serverId").cloned().unwrap_or_default();
    }
    let cfg = h.cfg();
    if !bool::from(b.key.as_bytes().ct_eq(cfg.server_key.as_bytes())) {
        crate::logf!("refused {path} from {addr}: wrong key (got {} chars, serverId {:?}), body starts {:?}", b.key.len(), b.server_id, preview(body));
        return Err(refuse(StatusCode::UNAUTHORIZED, "wrong key"));
    }
    // a server approved in /admin may still say hello with its claim only (it learns its id from this answer)
    let found = cfg.server(&b.server_id).or_else(|| if b.claim.is_empty() { None } else { cfg.servers.iter().find(|s| s.claim == b.claim) });
    let Some(server) = found.cloned() else {
        if path == "/api/hello" && !b.claim.is_empty() {
            let ip = addr.ip().to_string();
            let here = crate::net::is_this_machine(&ip);
            let (suggest_lan, suggest_public) = if here {
                (crate::net::lan_ip().map(|i| i.to_string()).unwrap_or_else(|| ip.clone()), String::new())
            } else if crate::net::is_private(&ip) {
                (ip.clone(), String::new())
            } else {
                (String::new(), ip.clone())
            };
            let p = Pending {
                suggest_lan,
                suggest_public,
                claim: b.claim.clone(),
                local: here,
                addr: ip,
                port: b.port,
                query_port: b.query_port,
                world: b.world.to_lowercase(),
                wanted_id: b.server_id.clone(),
                last_seen: chrono::Utc::now().to_rfc3339_opts(chrono::SecondsFormat::Secs, true),
            };
            let new = h.pending.lock().unwrap().insert(b.claim.clone(), p).is_none();
            if new {
                crate::logf!("new DayZ server asks to join from {addr} (world {:?}, port {}): approve it in the map editor, Servers", b.world, b.port);
            }
            return Err(reply(StatusCode::OK, json!({"ok": false, "pending": true, "error": "waiting for approval in the hub (map editor, Servers page)"})));
        }
        crate::logf!("refused {path}: unknown serverId {:?}", b.server_id);
        return Err(refuse(StatusCode::BAD_REQUEST, &format!("unknown serverId {} (add it on the hub's Servers page)", b.server_id)));
    };
    h.seen.lock().unwrap().insert(server.id.clone(), addr.ip().to_string());
    // the server tells its game/query port on every hello: keep hub.json right without anyone typing it
    let world = b.world.to_lowercase();
    if path == "/api/hello" && ((b.port != 0 && b.port != server.port) || (b.query_port != 0 && b.query_port != server.query_port) || (!world.is_empty() && world != server.world)) {
        let id = server.id.clone();
        let res = h.update_cfg(|c| {
            if let Some(s) = c.servers.iter_mut().find(|s| s.id == id) {
                if b.port != 0 {
                    s.port = b.port;
                }
                if b.query_port != 0 {
                    s.query_port = b.query_port;
                }
                if !world.is_empty() {
                    s.world = world.clone();
                }
            }
            Ok(())
        });
        match res {
            Ok(()) => crate::logf!("{id}: game port {} / query port {} / map {world:?} saved (reported by the server)", b.port, b.query_port),
            Err(e) => crate::logf!("{id}: could not save its ports: {e}"),
        }
    }
    if let Ok(db) = h.db.lock() {
        let _ = db.execute(
            "INSERT INTO servers(id,name,last_seen) VALUES(?1,?2,CURRENT_TIMESTAMP) ON CONFLICT (id) DO UPDATE SET last_seen = CURRENT_TIMESTAMP",
            params![server.id, server.name],
        );
    }
    Ok(server.id)
}

fn server_out(h: &Hub, id: &str) -> Value {
    match h.cfg().server(id) {
        Some(s) => json!({"id": s.id, "name": s.name, "lanHost": s.lan_host, "publicHost": s.public_host, "port": s.port, "password": s.password}),
        None => Value::Null,
    }
}

fn parse<T: for<'de> Deserialize<'de>>(path: &str, body: &[u8]) -> Result<T, Response> {
    serde_json::from_slice(body).map_err(|e| {
        crate::logf!("refused {path}: body does not fit ({e}), starts {:?}", preview(body));
        refuse(StatusCode::BAD_REQUEST, &format!("bad json: {e}"))
    })
}

fn set_owner(tx: &rusqlite::Transaction, steam_id: &str, owner: Option<&str>) -> rusqlite::Result<usize> {
    tx.execute(
        "INSERT INTO characters(steam_id,owner_server,updated_at) VALUES(?1,?2,CURRENT_TIMESTAMP)
         ON CONFLICT (steam_id) DO UPDATE SET owner_server=excluded.owner_server, updated_at=CURRENT_TIMESTAMP",
        params![steam_id, owner],
    )
}

type Q = Query<HashMap<String, String>>;
type Addr = ConnectInfo<SocketAddr>;

// POST /api/hello  {serverId,key} -> the server list
async fn hello(State(h): State<Shared>, ConnectInfo(a): Addr, Query(q): Q, body: Bytes) -> Response {
    let sid = match check(&h, "/api/hello", &a, &q, &body) {
        Ok(s) => s,
        Err(r) => return r,
    };
    let servers: Vec<Value> = h.cfg().servers.iter().map(|s| server_out(&h, &s.id)).collect();
    // Map editor: the server sends the zones/arrival points from its config.json; the first time, they become the
    // hub's copy. From then on the hub's copy wins (edited on the map) and goes back in "layout".
    let sent: Value = serde_json::from_slice(&body).unwrap_or(Value::Null);
    let db = h.db.lock().unwrap();
    if sent.get("zones").map(Value::is_array).unwrap_or(false) {
        let z = crate::layout::clean_zones(&sent["zones"]).to_string();
        let a = crate::layout::clean_arrivals(sent.get("arrivals").unwrap_or(&Value::Null)).to_string();
        match crate::layout::import_if_missing(&db, &sid, &z, &a) {
            Ok(true) => crate::logf!("layout of {sid} imported from its config.json"),
            Ok(false) => {}
            Err(e) => return db_error(e),
        }
    }
    let travel = h.cfg().server(&sid).map(|s| s.travel.clone()).unwrap_or_default();
    let layout = match crate::layout::get(&db, &sid) {
        Ok(l) => l.map(|l| crate::layout::for_server(&l, &travel)).unwrap_or(Value::Null),
        Err(e) => return db_error(e),
    };
    crate::logf!("hello from {sid}");
    reply(StatusCode::OK, json!({"ok": true, "serverId": sid, "servers": servers, "layout": layout, "time": chrono::Utc::now().to_rfc3339_opts(chrono::SecondsFormat::Secs, true)}))
}

#[derive(Deserialize, Default)]
#[serde(default)]
struct RevReq {
    rev: i64,
}

// POST /api/layout  {serverId,key,rev} -> this server's zones + arrival points, or unchanged when it has that rev
async fn layout(State(h): State<Shared>, ConnectInfo(a): Addr, Query(q): Q, body: Bytes) -> Response {
    let sid = match check(&h, "/api/layout", &a, &q, &body) {
        Ok(s) => s,
        Err(r) => return r,
    };
    let r: RevReq = serde_json::from_slice(&body).unwrap_or_default();
    let db = h.db.lock().unwrap();
    match crate::layout::get(&db, &sid) {
        Ok(Some(l)) if l.rev != r.rev => {
            crate::logf!("layout rev {} sent to {sid}", l.rev);
            let travel = h.cfg().server(&sid).map(|s| s.travel.clone()).unwrap_or_default();
            let mut out = crate::layout::for_server(&l, &travel);
            out["ok"] = json!(true);
            out["changed"] = json!(true);
            reply(StatusCode::OK, out)
        }
        Ok(l) => reply(StatusCode::OK, json!({"ok": true, "changed": false, "rev": l.map(|l| l.rev).unwrap_or(0)})),
        Err(e) => db_error(e),
    }
}

#[derive(Deserialize)]
#[serde(rename_all = "camelCase")]
struct DepartReq {
    #[serde(default)]
    steam_id: String,
    #[serde(default)]
    to: String,
    #[serde(default)]
    arrival: String,
    payload: Option<Box<RawValue>>, // kept as the exact text the source server wrote
    /// travel time on: seconds on the road before the target may build him (0 = at once)
    #[serde(default)]
    travel_seconds: i64,
}

// POST /api/depart  the source server hands the player over (only the owner, or any server for a new player).
async fn depart(State(h): State<Shared>, ConnectInfo(a): Addr, Query(q): Q, body: Bytes) -> Response {
    let from = match check(&h, "/api/depart", &a, &q, &body) {
        Ok(s) => s,
        Err(r) => return r,
    };
    let r: DepartReq = match parse("/api/depart", &body) {
        Ok(r) => r,
        Err(e) => return e,
    };
    let payload = r.payload.as_ref().map(|p| p.get().to_string()).unwrap_or_default();
    let target_known = h.cfg().server(&r.to).is_some();
    if r.steam_id.is_empty() || !target_known || r.to == from || payload.is_empty() || payload == "null" {
        crate::logf!("depart refused: steamId {:?}, to {:?} (known {target_known}), from {from:?}, payload {} bytes", r.steam_id, r.to, payload.len());
        return refuse(StatusCode::BAD_REQUEST, "need steamId, a different known target server and a JSON payload");
    }
    let mut db = h.db.lock().unwrap();
    let tx = match db.transaction() {
        Ok(t) => t,
        Err(e) => return db_error(e),
    };
    let owner: Option<Option<String>> = match tx
        .query_row("SELECT owner_server FROM characters WHERE steam_id=?1", params![r.steam_id], |row| row.get(0))
        .optional()
    {
        Ok(o) => o,
        Err(e) => return db_error(e),
    };
    // None = first time the hub sees him: the server he stands on owns him
    if let Some(cur) = &owner {
        if cur.as_deref() != Some(from.as_str()) {
            let who = cur.clone().unwrap_or_else(|| "nobody (in transit)".into());
            crate::logf!("depart REFUSED {} from {from}: owned by {who}", r.steam_id);
            return refuse(StatusCode::CONFLICT, &format!("this character is owned by {who}"));
        }
    }
    let token = crate::config::random_key();
    let travel = r.travel_seconds.clamp(0, 7 * 24 * 3600); // a week at most
    let res = tx
        .execute("UPDATE transfers SET state='superseded' WHERE steam_id=?1 AND state='pending'", params![r.steam_id])
        .and_then(|_| {
            tx.execute(
                "INSERT INTO transfers(token,steam_id,from_server,to_server,arrival,payload_text,state,ready_at)
                 VALUES(?1,?2,?3,?4,?5,?6,'pending', CASE WHEN ?7 > 0 THEN datetime('now', '+' || ?7 || ' seconds') END)",
                params![token, r.steam_id, from, r.to, r.arrival, payload, travel],
            )
        })
        .and_then(|_| set_owner(&tx, &r.steam_id, None));
    if let Err(e) = res {
        return db_error(e);
    }
    if let Err(e) = tx.commit() {
        return db_error(e);
    }
    crate::logf!("depart {}: {from} -> {} ({}), {} bytes, token {}..{}", r.steam_id, r.to, r.arrival, payload.len(), &token[..8], if travel > 0 { format!(", on the road {travel} s") } else { String::new() });
    reply(StatusCode::OK, json!({"ok": true, "token": token, "target": server_out(&h, &r.to), "readyIn": travel}))
}

#[derive(Deserialize, Default)]
#[serde(rename_all = "camelCase", default)]
struct SteamReq {
    steam_id: String,
    token: String,
    reason: String,
    name: String, // /api/arrive: the player's name (newer mod builds), for the Players page
}

// every login: remember his name and where he logged in (only for a steam id the hub knows already)
fn touch(tx: &rusqlite::Transaction, r: &SteamReq, here: &str) -> rusqlite::Result<usize> {
    tx.execute(
        "UPDATE characters SET name=COALESCE(NULLIF(?2,''),name), last_server=?3, last_login=CURRENT_TIMESTAMP WHERE steam_id=?1",
        params![r.steam_id, r.name, here],
    )
}

// POST /api/arrive  every server asks this when a player connects:
//   transfer (token + bytes; the snapshot itself comes from /api/payload) | redirect (target) | normal
async fn arrive(State(h): State<Shared>, ConnectInfo(a): Addr, Query(q): Q, body: Bytes) -> Response {
    let here = match check(&h, "/api/arrive", &a, &q, &body) {
        Ok(s) => s,
        Err(r) => return r,
    };
    let r: SteamReq = match parse("/api/arrive", &body) {
        Ok(r) => r,
        Err(e) => return e,
    };
    if r.steam_id.is_empty() {
        crate::logf!("arrive refused: no steamId in the body");
        return refuse(StatusCode::BAD_REQUEST, "need steamId");
    }
    let mut db = h.db.lock().unwrap();
    let tx = match db.transaction() {
        Ok(t) => t,
        Err(e) => return db_error(e),
    };
    if let Err(e) = touch(&tx, &r, &here) {
        return db_error(e);
    }
    let pending: Option<(String, String, String, String, String, i64)> = match tx
        .query_row(
            "SELECT token,from_server,to_server,arrival,payload_text,
                    COALESCE(CAST(strftime('%s', ready_at) AS INTEGER) - CAST(strftime('%s', 'now') AS INTEGER), 0)
             FROM transfers WHERE steam_id=?1 AND state='pending' ORDER BY created_at DESC, rowid DESC LIMIT 1",
            params![r.steam_id],
            |row| Ok((row.get(0)?, row.get(1)?, row.get(2)?, row.get(3)?, row.get(4)?, row.get(5)?)),
        )
        .optional()
    {
        Ok(p) => p,
        Err(e) => return db_error(e),
    };
    if let Some((token, from, to, arrival, text, left)) = pending {
        // on the road: no server may have him yet (not even the one he left) - the "pseudo-ban"
        if left > 0 {
            let _ = tx.commit();
            crate::logf!("arrive {} on {here}: on the road to {to}, {left} s left, refused", r.steam_id);
            return reply(StatusCode::OK, json!({"ok": true, "action": "wait", "seconds": left, "target": server_out(&h, &to)}));
        }
        if to != here {
            let _ = tx.commit();
            crate::logf!("arrive {} on {here}: travelling to {to}, redirect", r.steam_id);
            return reply(StatusCode::OK, json!({"ok": true, "action": "redirect", "target": server_out(&h, &to)}));
        }
        // two-phase: stays pending until the target confirms it BUILT him; asking again returns the same trip
        let _ = tx.commit();
        crate::logf!("arrive {} on {here} from {from} ({arrival}): transfer offered, waiting for confirm", r.steam_id);
        return reply(StatusCode::OK, json!({"ok": true, "action": "transfer", "token": token, "from": from, "arrival": arrival, "bytes": plain_numbers(&text).len()}));
    }
    let owner: Option<Option<String>> = match tx
        .query_row("SELECT owner_server FROM characters WHERE steam_id=?1", params![r.steam_id], |row| row.get(0))
        .optional()
    {
        Ok(o) => o,
        Err(e) => return db_error(e),
    };
    if let Some(Some(o)) = &owner {
        if *o != here && h.cfg().server(o).is_some() {
            let _ = tx.commit();
            crate::logf!("arrive {} on {here}: owned by {o}, redirect", r.steam_id);
            return reply(StatusCode::OK, json!({"ok": true, "action": "redirect", "target": server_out(&h, o)}));
        }
    }
    if let Err(e) = set_owner(&tx, &r.steam_id, Some(&here)).and_then(|_| touch(&tx, &r, &here)) {
        return db_error(e);
    }
    if let Err(e) = tx.commit() {
        return db_error(e);
    }
    crate::logf!("arrive {} on {here}: normal login, owner set", r.steam_id);
    reply(StatusCode::OK, json!({"ok": true, "action": "normal"}))
}

// POST /api/confirm  the target built him from this trip: claim it, it owns him now
async fn confirm(State(h): State<Shared>, ConnectInfo(a): Addr, Query(q): Q, body: Bytes) -> Response {
    let here = match check(&h, "/api/confirm", &a, &q, &body) {
        Ok(s) => s,
        Err(r) => return r,
    };
    let r: SteamReq = match parse("/api/confirm", &body) {
        Ok(r) => r,
        Err(e) => return e,
    };
    let mut db = h.db.lock().unwrap();
    let tx = match db.transaction() {
        Ok(t) => t,
        Err(e) => return db_error(e),
    };
    let n = match tx.execute(
        "UPDATE transfers SET state='claimed', claimed_at=CURRENT_TIMESTAMP WHERE token=?1 AND steam_id=?2 AND to_server=?3 AND state='pending'",
        params![r.token, r.steam_id, here],
    ) {
        Ok(n) => n,
        Err(e) => return db_error(e),
    };
    if n != 1 {
        crate::logf!("confirm REFUSED {} on {here}: no pending transfer with that token", r.steam_id);
        return refuse(StatusCode::CONFLICT, "no pending transfer with that token");
    }
    if let Err(e) = set_owner(&tx, &r.steam_id, Some(&here)) {
        return db_error(e);
    }
    if let Err(e) = tx.commit() {
        return db_error(e);
    }
    crate::logf!("confirm {} on {here}: transfer claimed, owner set", r.steam_id);
    reply(StatusCode::OK, json!({"ok": true}))
}

// POST /api/payload  {steamId, token} -> the snapshot as the WHOLE body, exact text (exponents made plain)
async fn payload(State(h): State<Shared>, ConnectInfo(a): Addr, Query(q): Q, body: Bytes) -> Response {
    let here = match check(&h, "/api/payload", &a, &q, &body) {
        Ok(s) => s,
        Err(r) => return r,
    };
    let r: SteamReq = match parse("/api/payload", &body) {
        Ok(r) => r,
        Err(e) => return e,
    };
    let db = h.db.lock().unwrap();
    let text: Option<String> = match db
        .query_row(
            "SELECT payload_text FROM transfers WHERE token=?1 AND steam_id=?2 AND to_server=?3 AND state='pending'",
            params![r.token, r.steam_id, here],
            |row| row.get(0),
        )
        .optional()
    {
        Ok(t) => t,
        Err(e) => return db_error(e),
    };
    let Some(text) = text else {
        crate::logf!("payload REFUSED {} on {here}: no pending transfer with that token", r.steam_id);
        return refuse(StatusCode::CONFLICT, "no pending transfer with that token");
    };
    let out = plain_numbers(&text);
    crate::logf!("payload {} to {here}: {} bytes", r.steam_id, out.len());
    (StatusCode::OK, [(header::CONTENT_TYPE, "application/json")], out).into_response()
}

// POST /api/fail  {steamId, token, reason}: the target could not build him; the trip stays pending
async fn fail(State(h): State<Shared>, ConnectInfo(a): Addr, Query(q): Q, body: Bytes) -> Response {
    let here = match check(&h, "/api/fail", &a, &q, &body) {
        Ok(s) => s,
        Err(r) => return r,
    };
    let r: SteamReq = match parse("/api/fail", &body) {
        Ok(r) => r,
        Err(e) => return e,
    };
    let db = h.db.lock().unwrap();
    let n = match db.execute(
        "UPDATE transfers SET fail_count=fail_count+1, last_error=?1 WHERE token=?2 AND steam_id=?3 AND to_server=?4 AND state='pending'",
        params![r.reason, r.token, r.steam_id, here],
    ) {
        Ok(n) => n,
        Err(e) => return db_error(e),
    };
    if n != 1 {
        return refuse(StatusCode::CONFLICT, "no pending transfer with that token");
    }
    crate::logf!("FAILED arrival {} on {here}: {} (transfer kept; 'DriveLinkHub.exe return {}' sends him back)", r.steam_id, r.reason, r.steam_id);
    reply(StatusCode::OK, json!({"ok": true}))
}

async fn health(State(h): State<Shared>, _headers: HeaderMap) -> Response {
    let ok = h.db.lock().map(|d| d.query_row("SELECT 1", [], |r| r.get::<_, i64>(0)).is_ok()).unwrap_or(false);
    if ok {
        reply(StatusCode::OK, json!({"ok": true}))
    } else {
        refuse(StatusCode::SERVICE_UNAVAILABLE, "database down")
    }
}
