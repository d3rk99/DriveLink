// Players page of the web app: who the hub knows, which server holds each character, his trips, what his last
// snapshot holds, and the admin fixes (every one is written to admin_log):
//   send     - a waiting trip goes to another server instead (the snapshot = his gear goes with it)
//   assign   - no trip waiting: this server holds him from now on (his next login anywhere goes there)
//   release  - the hub forgets where he is: his next login anywhere is a normal login there
//   drop     - throw a waiting trip away (the gear in its snapshot is LOST; the character on the source is gone)
use crate::api::Shared;
use axum::body::Bytes;
use axum::extract::{Path, Query, State};
use axum::http::{header, StatusCode};
use axum::response::{IntoResponse, Response};
use rusqlite::{params, OptionalExtension};
use serde::Deserialize;
use serde_json::{json, Value};
use std::collections::HashMap;

fn ok(v: Value) -> Response {
    ([(header::CONTENT_TYPE, "application/json")], v.to_string()).into_response()
}

fn bad(code: StatusCode, msg: impl Into<String>) -> Response {
    (code, msg.into()).into_response()
}

fn db_err(e: impl std::fmt::Display) -> Response {
    bad(StatusCode::INTERNAL_SERVER_ERROR, format!("database: {e}"))
}

#[derive(Deserialize, Default)]
#[serde(default)]
pub struct ListQ {
    q: String,
}

// GET /admin/api/players?q=  (name or steam id contains q)
pub async fn list(State(h): State<Shared>, Query(q): Query<ListQ>) -> Response {
    let db = h.db.lock().unwrap();
    let like = format!("%{}%", q.q.trim());
    let sql = "SELECT c.steam_id, c.name, c.owner_server, c.last_server, c.last_login, c.updated_at,
                      t.from_server, t.to_server, t.arrival, t.created_at, t.fail_count, t.last_error,
                      (SELECT COUNT(*) FROM transfers x WHERE x.steam_id = c.steam_id AND x.state = 'claimed'),
                      MAX(0, COALESCE(CAST(strftime('%s', t.ready_at) AS INTEGER) - CAST(strftime('%s', 'now') AS INTEGER), 0)),
                      t.party, t.party_role,
                      (SELECT COUNT(*) FROM transfers y WHERE y.party = t.party AND y.state = 'pending')
               FROM characters c
               LEFT JOIN transfers t ON t.steam_id = c.steam_id AND t.state = 'pending'
               WHERE c.steam_id LIKE ?1 OR IFNULL(c.name, '') LIKE ?1
               ORDER BY COALESCE(c.last_login, c.updated_at) DESC LIMIT 500";
    let mut st = match db.prepare(sql) {
        Ok(s) => s,
        Err(e) => return db_err(e),
    };
    let rows = st.query_map(params![like], |r| {
        let pending_to: Option<String> = r.get(7)?;
        Ok(json!({
            "steamId": r.get::<_, String>(0)?,
            "name": r.get::<_, Option<String>>(1)?,
            "owner": r.get::<_, Option<String>>(2)?,
            "lastServer": r.get::<_, Option<String>>(3)?,
            "lastLogin": r.get::<_, Option<String>>(4)?,
            "updatedAt": r.get::<_, String>(5)?,
            "pending": match pending_to {
                Some(to) => json!({"from": r.get::<_, String>(6)?, "to": to, "arrival": r.get::<_, String>(8)?,
                                   "since": r.get::<_, String>(9)?, "failCount": r.get::<_, i64>(10)?, "lastError": r.get::<_, Option<String>>(11)?,
                                   "readyIn": r.get::<_, i64>(13)?, "party": r.get::<_, Option<String>>(14)?,
                                   "role": r.get::<_, Option<String>>(15)?, "partySize": r.get::<_, i64>(16)?}),
                None => Value::Null,
            },
            "trips": r.get::<_, i64>(12)?,
        }))
    });
    match rows {
        Ok(it) => ok(json!({"players": it.filter_map(Result::ok).collect::<Vec<_>>()})),
        Err(e) => db_err(e),
    }
}

/// What a snapshot (the mod's DL_PlayerData JSON) holds, in a few numbers.
fn summary(text: &str) -> Value {
    let v: Value = match serde_json::from_str(&crate::numfix::plain_numbers(text)) {
        Ok(v) => v,
        Err(_) => return json!({"error": "snapshot is not readable JSON"}),
    };
    let mut types: HashMap<String, i64> = HashMap::new();
    let items = v["items"].as_array().cloned().unwrap_or_default();
    for it in &items {
        if let Some(t) = it["type"].as_str() {
            *types.entry(t.to_string()).or_default() += 1;
        }
    }
    let mut list: Vec<(String, i64)> = types.into_iter().collect();
    list.sort_by(|a, b| b.1.cmp(&a.1).then(a.0.cmp(&b.0)));
    let car = &v["car"];
    let car = if car["type"].as_str().map(|t| !t.is_empty()).unwrap_or(false) {
        json!({"type": car["type"], "parts": car["items"].as_array().map(|a| a.len()).unwrap_or(0), "fuel": car["fluids"].get(0)})
    } else {
        Value::Null
    };
    json!({
        "charType": v["charType"], "health": v["health"], "blood": v["blood"], "water": v["water"], "energy": v["energy"],
        "restrained": v["restrained"], "bleeding": v["bleeding"].as_array().map(|a| a.len()).unwrap_or(0),
        "items": items.len(), "itemTypes": list.into_iter().take(40).map(|(t, n)| json!({"type": t, "n": n})).collect::<Vec<_>>(),
        "car": car, "passengerOf": v["carGroup"], "bytes": text.len(),
    })
}

// GET /admin/api/players/{steamId}
pub async fn detail(State(h): State<Shared>, Path(sid): Path<String>) -> Response {
    let db = h.db.lock().unwrap();
    let row = db
        .query_row(
            "SELECT name, owner_server, last_server, last_login, updated_at FROM characters WHERE steam_id=?1",
            params![sid],
            |r| Ok(json!({"name": r.get::<_, Option<String>>(0)?, "owner": r.get::<_, Option<String>>(1)?, "lastServer": r.get::<_, Option<String>>(2)?, "lastLogin": r.get::<_, Option<String>>(3)?, "updatedAt": r.get::<_, String>(4)?})),
        )
        .optional();
    let Ok(Some(mut p)) = row else {
        return bad(StatusCode::NOT_FOUND, "the hub does not know that steam id");
    };
    let mut trips = Vec::new();
    if let Ok(mut st) = db.prepare(
        "SELECT from_server, to_server, arrival, state, created_at, claimed_at, fail_count, last_error, length(payload_text),
                MAX(0, COALESCE(CAST(strftime('%s', ready_at) AS INTEGER) - CAST(strftime('%s', 'now') AS INTEGER), 0))
         FROM transfers WHERE steam_id=?1 ORDER BY created_at DESC, rowid DESC LIMIT 30",
    ) {
        if let Ok(it) = st.query_map(params![sid], |r| {
            Ok(json!({"from": r.get::<_, String>(0)?, "to": r.get::<_, String>(1)?, "arrival": r.get::<_, String>(2)?, "state": r.get::<_, String>(3)?,
                      "at": r.get::<_, String>(4)?, "claimedAt": r.get::<_, Option<String>>(5)?, "failCount": r.get::<_, i64>(6)?,
                      "lastError": r.get::<_, Option<String>>(7)?, "bytes": r.get::<_, i64>(8)?, "readyIn": r.get::<_, i64>(9)?}))
        }) {
            trips = it.filter_map(Result::ok).collect();
        }
    }
    // newest snapshot: the waiting trip, else the last one that arrived
    let snap: Option<(String, String)> = db
        .query_row(
            "SELECT payload_text, state FROM transfers WHERE steam_id=?1 AND state IN ('pending','claimed')
             ORDER BY (state='pending') DESC, created_at DESC, rowid DESC LIMIT 1",
            params![sid],
            |r| Ok((r.get(0)?, r.get(1)?)),
        )
        .optional()
        .ok()
        .flatten();
    // the party of his waiting trip: everyone travelling with him, and where each is in it
    let party: Option<String> = db
        .query_row("SELECT party FROM transfers WHERE steam_id=?1 AND state='pending'", params![sid], |r| r.get(0))
        .optional()
        .ok()
        .flatten()
        .flatten();
    let mut members = Vec::new();
    if let Some(pid) = &party {
        if let Ok(mut st) = db.prepare(
            "SELECT t.steam_id, c.name, t.party_role FROM transfers t LEFT JOIN characters c ON c.steam_id = t.steam_id
             WHERE t.party=?1 AND t.state='pending' ORDER BY t.party_role, c.name",
        ) {
            if let Ok(it) = st.query_map(params![pid], |r| Ok(json!({"steamId": r.get::<_, String>(0)?, "name": r.get::<_, Option<String>>(1)?, "role": r.get::<_, Option<String>>(2)?}))) {
                members = it.filter_map(Result::ok).collect();
            }
        }
    }
    p["party"] = match &party {
        Some(pid) => json!({"id": pid, "members": members}),
        None => Value::Null,
    };
    let mut log = Vec::new();
    if let Ok(mut st) = db.prepare("SELECT at, action, detail FROM admin_log WHERE steam_id=?1 ORDER BY id DESC LIMIT 20") {
        if let Ok(it) = st.query_map(params![sid], |r| Ok(json!({"at": r.get::<_, String>(0)?, "action": r.get::<_, String>(1)?, "detail": r.get::<_, String>(2)?}))) {
            log = it.filter_map(Result::ok).collect();
        }
    }
    p["steamId"] = json!(sid);
    p["trips"] = json!(trips);
    p["snapshot"] = snap.map(|(t, st)| {
        let mut s = summary(&t);
        s["from"] = json!(if st == "pending" { "the waiting trip" } else { "his last trip (what he arrived with)" });
        s
    }).unwrap_or(Value::Null);
    p["log"] = json!(log);
    ok(p)
}

#[derive(Deserialize)]
pub struct ActionForm {
    action: String,
    #[serde(default)]
    server: String,
    /// send / arrive_now / drop: for everyone in his party (group, convoy, crew), so they stay together
    #[serde(default)]
    party: bool,
}

// POST /admin/api/players/{steamId}/action  {action, server}
pub async fn action(State(h): State<Shared>, Path(sid): Path<String>, body: Bytes) -> Response {
    let f: ActionForm = match serde_json::from_slice(&body) {
        Ok(f) => f,
        Err(e) => return bad(StatusCode::BAD_REQUEST, format!("bad form: {e}")),
    };
    let cfg = h.cfg();
    let mut db = h.db.lock().unwrap();
    let tx = match db.transaction() {
        Ok(t) => t,
        Err(e) => return db_err(e),
    };
    let known: Option<Option<String>> = match tx.query_row("SELECT owner_server FROM characters WHERE steam_id=?1", params![sid], |r| r.get(0)).optional() {
        Ok(k) => k,
        Err(e) => return db_err(e),
    };
    if known.is_none() {
        return bad(StatusCode::NOT_FOUND, "the hub does not know that steam id");
    }
    let pending: Option<(String, String, String)> = match tx
        .query_row("SELECT token, from_server, to_server FROM transfers WHERE steam_id=?1 AND state='pending'", params![sid], |r| Ok((r.get(0)?, r.get(1)?, r.get(2)?)))
        .optional()
    {
        Ok(p) => p,
        Err(e) => return db_err(e),
    };
    // the whole party: every member's waiting trip (the tokens), else just his
    let mut tokens: Vec<(String, String)> = Vec::new(); // (token, steam id)
    if let Some((tk, _, _)) = &pending {
        tokens.push((tk.clone(), sid.clone()));
        if f.party && ["send", "arrive_now", "drop"].contains(&f.action.as_str()) {
            let pid: Option<String> = tx.query_row("SELECT party FROM transfers WHERE token=?1", params![tk], |r| r.get(0)).optional().ok().flatten().flatten();
            if let Some(pid) = pid {
                if let Ok(mut st) = tx.prepare("SELECT token, steam_id FROM transfers WHERE party=?1 AND state='pending' AND token<>?2") {
                    if let Ok(it) = st.query_map(params![pid, tk], |r| Ok((r.get::<_, String>(0)?, r.get::<_, String>(1)?))) {
                        tokens.extend(it.filter_map(Result::ok));
                    }
                }
            }
        }
    }
    let who = if tokens.len() > 1 { format!(" (whole party: {} players)", tokens.len()) } else { String::new() };
    let need_server = || -> Result<String, Response> {
        match cfg.server(&f.server) {
            Some(s) => Ok(s.id.clone()),
            None => Err(bad(StatusCode::BAD_REQUEST, format!("unknown server {:?}", f.server))),
        }
    };
    let detail: String = match f.action.as_str() {
        "send" => {
            let Some((token, from, to)) = &pending else { return bad(StatusCode::CONFLICT, "no trip is waiting") };
            let target = match need_server() {
                Ok(t) => t,
                Err(r) => return r,
            };
            let _ = token;
            for (tk, _) in &tokens {
                if let Err(e) = tx.execute("UPDATE transfers SET to_server=?1, arrival='', fail_count=0, last_error=NULL WHERE token=?2", params![target, tk]) {
                    return db_err(e);
                }
            }
            format!("waiting trip {from} -> {to} now goes to {target} (first arrival point){who}")
        }
        "assign" => {
            if pending.is_some() {
                return bad(StatusCode::CONFLICT, "a trip is waiting: send it to a server, or drop it first");
            }
            let target = match need_server() {
                Ok(t) => t,
                Err(r) => return r,
            };
            if let Err(e) = tx.execute("UPDATE characters SET owner_server=?1, updated_at=CURRENT_TIMESTAMP WHERE steam_id=?2", params![target, sid]) {
                return db_err(e);
            }
            format!("assigned to {target} (was {})", known.clone().flatten().unwrap_or_else(|| "nobody".into()))
        }
        "release" => {
            if pending.is_some() {
                return bad(StatusCode::CONFLICT, "a trip is waiting: send it to a server, or drop it first");
            }
            // owner NULL with no trip = his next login anywhere is a normal one (api.rs arrive)
            if let Err(e) = tx.execute("UPDATE characters SET owner_server=NULL, updated_at=CURRENT_TIMESTAMP WHERE steam_id=?1", params![sid]) {
                return db_err(e);
            }
            format!("released (was on {}): next login anywhere is a normal login", known.clone().flatten().unwrap_or_else(|| "nobody".into()))
        }
        "arrive_now" => {
            let Some((token, from, to)) = &pending else { return bad(StatusCode::CONFLICT, "no trip is waiting") };
            let _ = token;
            for (tk, _) in &tokens {
                if let Err(e) = tx.execute("UPDATE transfers SET ready_at=NULL WHERE token=?1", params![tk]) {
                    return db_err(e);
                }
            }
            format!("trip {from} -> {to}: arrives now (travel time skipped){who}")
        }
        "drop" => {
            let Some((token, from, to)) = &pending else { return bad(StatusCode::CONFLICT, "no trip is waiting") };
            let _ = token;
            for (tk, _) in &tokens {
                if let Err(e) = tx.execute("UPDATE transfers SET state='expired' WHERE token=?1", params![tk]) {
                    return db_err(e);
                }
            }
            format!("waiting trip {from} -> {to} dropped (its snapshot is gone){who}")
        }
        other => return bad(StatusCode::BAD_REQUEST, format!("unknown action {other:?}")),
    };
    // logged for him and for each other member it touched
    let mut logged = vec![sid.clone()];
    for (_, msid) in &tokens {
        if !logged.contains(msid) {
            logged.push(msid.clone());
        }
    }
    for lsid in &logged {
        let d = if *lsid == sid { detail.clone() } else { format!("{detail} - done from {sid}'s page") };
        if let Err(e) = tx.execute("INSERT INTO admin_log(steam_id, action, detail) VALUES (?1, ?2, ?3)", params![lsid, f.action, d]) {
            return db_err(e);
        }
    }
    if let Err(e) = tx.commit() {
        return db_err(e);
    }
    crate::logf!("players page: {sid}: {detail}");
    ok(json!({"ok": true, "detail": detail}))
}
