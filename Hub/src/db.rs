// SQLite: the same file, schema and migrations as the Go hub (an existing drivelink.db just keeps working).
// ONE connection behind a mutex: every transaction runs after the other, so "read owner, then change it" can never
// race. WAL = the admin commands can read/write while the hub runs; busy_timeout = wait instead of failing.
use rusqlite::{params, Connection, OptionalExtension};
use std::path::Path;

const MIGRATIONS: &[(i64, &str, &str)] = &[
    (1, "001_init.sql", include_str!("../migrations/001_init.sql")),
    (2, "002_fail_info.sql", include_str!("../migrations/002_fail_info.sql")),
    (3, "003_layouts.sql", include_str!("../migrations/003_layouts.sql")),
    (4, "004_players.sql", include_str!("../migrations/004_players.sql")),
    (5, "005_travel_time.sql", include_str!("../migrations/005_travel_time.sql")),
    (6, "006_parties.sql", include_str!("../migrations/006_parties.sql")),
    (7, "007_cars.sql", include_str!("../migrations/007_cars.sql")),
];

pub fn open(path: &Path) -> Result<Connection, String> {
    let c = Connection::open(path).map_err(|e| format!("cannot open the database {}: {e}", path.display()))?;
    c.busy_timeout(std::time::Duration::from_secs(5)).map_err(|e| e.to_string())?;
    c.pragma_update(None, "journal_mode", "WAL").map_err(|e| e.to_string())?;
    c.pragma_update(None, "synchronous", "FULL").map_err(|e| e.to_string())?;
    Ok(c)
}

pub fn migrate(c: &mut Connection) -> Result<(), String> {
    c.execute("CREATE TABLE IF NOT EXISTS schema_version (version INTEGER PRIMARY KEY)", []).map_err(|e| e.to_string())?;
    let have: i64 = c
        .query_row("SELECT COALESCE(MAX(version),0) FROM schema_version", [], |r| r.get(0))
        .map_err(|e| e.to_string())?;
    for (v, name, sql) in MIGRATIONS {
        if *v <= have {
            continue;
        }
        let tx = c.transaction().map_err(|e| e.to_string())?;
        tx.execute_batch(sql).map_err(|e| format!("migration {name}: {e}"))?;
        tx.execute("INSERT INTO schema_version(version) VALUES (?1)", params![v]).map_err(|e| e.to_string())?;
        tx.commit().map_err(|e| e.to_string())?;
        crate::logf!("database: applied {name}");
    }
    Ok(())
}

// ---------------- admin commands (DriveLinkHub.exe transfers | return | drop | dump) ----------------

pub fn list_transfers(c: &Connection) -> Result<(), String> {
    let mut st = c
        .prepare("SELECT steam_id,from_server,to_server,arrival,created_at,fail_count,COALESCE(last_error,''),length(payload_text) FROM transfers WHERE state='pending' ORDER BY created_at")
        .map_err(|e| e.to_string())?;
    let rows = st
        .query_map([], |r| {
            Ok((
                r.get::<_, String>(0)?, r.get::<_, String>(1)?, r.get::<_, String>(2)?, r.get::<_, String>(3)?,
                r.get::<_, String>(4)?, r.get::<_, i64>(5)?, r.get::<_, String>(6)?, r.get::<_, i64>(7)?,
            ))
        })
        .map_err(|e| e.to_string())?;
    let mut n = 0;
    for row in rows {
        let (sid, from, to, arrival, created, fails, last, size) = row.map_err(|e| e.to_string())?;
        n += 1;
        let mut l = format!("{sid}  {from} -> {to} ({arrival})  since {created} UTC  {size} bytes");
        if fails > 0 {
            l += &format!("  FAILED {fails}x: {last}");
        }
        println!("{l}");
    }
    if n == 0 {
        println!("no pending transfers");
    }
    Ok(())
}

fn pending_of(tx: &rusqlite::Transaction, steam_id: &str) -> Result<(String, String, String), String> {
    tx.query_row(
        "SELECT token,from_server,to_server FROM transfers WHERE steam_id=?1 AND state='pending' ORDER BY created_at DESC, rowid DESC LIMIT 1",
        params![steam_id],
        |r| Ok((r.get(0)?, r.get(1)?, r.get(2)?)),
    )
    .optional()
    .map_err(|e| e.to_string())?
    .ok_or_else(|| format!("{steam_id} has no pending transfer"))
}

/// The trip turns around: same snapshot, now to the server he left; arrival "" = its first arrival point.
pub fn return_transfer(c: &mut Connection, steam_id: &str) -> Result<(), String> {
    let tx = c.transaction().map_err(|e| e.to_string())?;
    let (token, from, to) = pending_of(&tx, steam_id)?;
    tx.execute(
        "UPDATE transfers SET from_server=?1, to_server=?2, arrival='', fail_count=0, last_error=NULL WHERE token=?3",
        params![to, from, token],
    )
    .map_err(|e| e.to_string())?;
    tx.commit().map_err(|e| e.to_string())?;
    println!("{steam_id}: trip {from} -> {to} turned around, he arrives on {from} (first arrival point) next time he joins");
    Ok(())
}

pub fn drop_transfer(c: &mut Connection, steam_id: &str) -> Result<(), String> {
    let tx = c.transaction().map_err(|e| e.to_string())?;
    let (token, from, to) = pending_of(&tx, steam_id)?;
    tx.execute("UPDATE transfers SET state='expired' WHERE token=?1", params![token]).map_err(|e| e.to_string())?;
    tx.commit().map_err(|e| e.to_string())?;
    println!("{steam_id}: trip {from} -> {to} dropped (snapshot discarded); his next login anywhere is a normal login");
    Ok(())
}

pub fn dump(c: &Connection, out: &Path) -> Result<(), String> {
    let row: Option<(String, String, String, String)> = c
        .query_row(
            "SELECT steam_id,from_server,to_server,payload_text FROM transfers WHERE state='pending' ORDER BY created_at DESC, rowid DESC LIMIT 1",
            [],
            |r| Ok((r.get(0)?, r.get(1)?, r.get(2)?, r.get(3)?)),
        )
        .optional()
        .map_err(|e| e.to_string())?;
    match row {
        None => println!("no pending transfer"),
        Some((sid, from, to, text)) => {
            std::fs::write(out, &text).map_err(|e| e.to_string())?;
            println!("pending {sid}: {from} -> {to}, wrote {} ({} bytes)", out.display(), text.len());
        }
    }
    Ok(())
}
