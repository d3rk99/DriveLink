// Leaving zones + arrival points per server (table layouts, migration 003). Stored as the mod's own JSON arrays.
use rusqlite::{params, Connection, OptionalExtension};
use serde_json::Value;

pub struct Layout {
    pub zones: String,
    pub arrivals: String,
    pub map: String,
    pub rev: i64,
}

pub fn get(c: &Connection, server: &str) -> rusqlite::Result<Option<Layout>> {
    c.query_row("SELECT zones,arrivals,map,rev FROM layouts WHERE server_id=?1", params![server], |r| {
        Ok(Layout { zones: r.get(0)?, arrivals: r.get(1)?, map: r.get(2)?, rev: r.get(3)? })
    })
    .optional()
}

/// First contact: the server's config.json zones become the hub's copy (nothing is lost when switching over).
pub fn import_if_missing(c: &Connection, server: &str, zones: &str, arrivals: &str) -> rusqlite::Result<bool> {
    if get(c, server)?.is_some() {
        return Ok(false);
    }
    c.execute(
        "INSERT INTO layouts(server_id,zones,arrivals,map,rev) VALUES(?1,?2,?3,'',1)",
        params![server, zones, arrivals],
    )?;
    Ok(true)
}

/// Editor save: replaces zones/arrivals (and map when given), rev + 1. Returns the new rev.
pub fn save(c: &Connection, server: &str, zones: &str, arrivals: &str, map: Option<&str>) -> rusqlite::Result<i64> {
    let cur = get(c, server)?;
    let rev = cur.as_ref().map(|l| l.rev).unwrap_or(0) + 1;
    let map = map.map(|m| m.to_string()).or(cur.map(|l| l.map)).unwrap_or_default();
    c.execute(
        "INSERT INTO layouts(server_id,zones,arrivals,map,rev,updated_at) VALUES(?1,?2,?3,?4,?5,CURRENT_TIMESTAMP)
         ON CONFLICT (server_id) DO UPDATE SET zones=excluded.zones, arrivals=excluded.arrivals, map=excluded.map,
         rev=excluded.rev, updated_at=CURRENT_TIMESTAMP",
        params![server, zones, arrivals, map, rev],
    )?;
    Ok(rev)
}

/// Something else the server gets with its layout changed (its travel rules): make it reload. No layout yet = nothing
/// to do, its first hello gets everything.
pub fn bump(c: &Connection, server: &str) -> rusqlite::Result<Option<i64>> {
    c.execute("UPDATE layouts SET rev=rev+1, updated_at=CURRENT_TIMESTAMP WHERE server_id=?1", params![server])?;
    Ok(get(c, server)?.map(|l| l.rev))
}

/// The layout as a DayZ server gets it, with that server's travel rules.
pub fn for_server(l: &Layout, travel: &crate::config::TravelRules) -> Value {
    let mut v = to_json(l);
    v["travel"] = serde_json::to_value(travel).unwrap_or(Value::Null);
    v
}

pub fn to_json(l: &Layout) -> Value {
    serde_json::json!({
        "rev": l.rev,
        "world": l.map, // not "map": a type name in Enforce Script
        "zones": serde_json::from_str::<Value>(&l.zones).unwrap_or(Value::Array(vec![])),
        "arrivals": serde_json::from_str::<Value>(&l.arrivals).unwrap_or(Value::Array(vec![])),
    })
}

/// The editor sends whatever the browser made; keep only what the mod reads, in its own shape.
pub fn clean_zones(v: &Value) -> Value {
    let arr = v.as_array().cloned().unwrap_or_default();
    Value::Array(
        arr.iter()
            .map(|z| {
                serde_json::json!({
                    "name": z.get("name").and_then(Value::as_str).unwrap_or("Zone"),
                    "center": [num(z, "center", 0), 0.0, num(z, "center", 2)],
                    "radius": z.get("radius").and_then(Value::as_f64).unwrap_or(30.0).max(1.0),
                    // "circle" (centre + radius) | "rect" (centre + width along x + depth along z, axis-aligned:
                    // a whole map border can be one zone)
                    "shape": if z.get("shape").and_then(Value::as_str) == Some("rect") { "rect" } else { "circle" },
                    "width": z.get("width").and_then(Value::as_f64).filter(|v| v.is_finite()).unwrap_or(0.0).clamp(0.0, 100000.0),
                    "depth": z.get("depth").and_then(Value::as_f64).filter(|v| v.is_finite()).unwrap_or(0.0).clamp(0.0, 100000.0),
                    // a plane zone: only aircraft in flight travel through it (and they through no other zone)
                    "air": z.get("air").and_then(Value::as_bool).unwrap_or(false),
                    "to": z.get("to").and_then(Value::as_str).unwrap_or(""),
                    "arrival": z.get("arrival").and_then(Value::as_str).unwrap_or(""),
                    // travel cost/time scale with this (0 = free, instant)
                    "distanceKm": z.get("distanceKm").and_then(Value::as_f64).filter(|d| d.is_finite()).unwrap_or(0.0).clamp(0.0, 100000.0),
                    // the road film's middle stretch (forest | field | desert | snow), "" = none
                    "viaBiome": z.get("viaBiome").and_then(Value::as_str).filter(|b| ["forest", "field", "desert", "snow"].contains(b)).unwrap_or(""),
                })
            })
            .collect(),
    )
}

pub fn clean_arrivals(v: &Value) -> Value {
    let arr = v.as_array().cloned().unwrap_or_default();
    Value::Array(
        arr.iter()
            .map(|a| {
                serde_json::json!({
                    "name": a.get("name").and_then(Value::as_str).unwrap_or("arrival"),
                    "pos": [num(a, "pos", 0), num(a, "pos", 1), num(a, "pos", 2)],
                    "yaw": a.get("yaw").and_then(Value::as_f64).unwrap_or(0.0).rem_euclid(360.0),
                    "carPos": [num(a, "carPos", 0), 0.0, num(a, "carPos", 2)],
                    "carYaw": a.get("carYaw").and_then(Value::as_f64).unwrap_or(0.0).rem_euclid(360.0),
                    // a boat that comes along goes on the water here; 0 0 = the nearest free water (the server looks)
                    "boatPos": [num(a, "boatPos", 0), 0.0, num(a, "boatPos", 2)],
                    "boatYaw": a.get("boatYaw").and_then(Value::as_f64).unwrap_or(0.0).rem_euclid(360.0),
                    // a plane arrives here in flight: x, height above the terrain (m), z; 0 0 = above the point
                    "airPos": [num(a, "airPos", 0), num(a, "airPos", 1), num(a, "airPos", 2)],
                    "airYaw": a.get("airYaw").and_then(Value::as_f64).unwrap_or(0.0).rem_euclid(360.0),
                    "stagingEnabled": a.get("stagingEnabled").and_then(Value::as_bool).unwrap_or(false),
                    "stagingPos": [num(a, "stagingPos", 0), 0.0, num(a, "stagingPos", 2)],
                    "stagingYaw": a.get("stagingYaw").and_then(Value::as_f64).unwrap_or(0.0).rem_euclid(360.0),
                    "stagingBoatPos": [num(a, "stagingBoatPos", 0), 0.0, num(a, "stagingBoatPos", 2)],
                    "stagingBoatYaw": a.get("stagingBoatYaw").and_then(Value::as_f64).unwrap_or(0.0).rem_euclid(360.0),
                    "stagingSpacing": a.get("stagingSpacing").and_then(Value::as_f64).unwrap_or(25.0).clamp(15.0, 100.0),
                    "convoySpacing": a.get("convoySpacing").and_then(Value::as_f64).unwrap_or(20.0).clamp(12.0, 100.0),
                })
            })
            .collect(),
    )
}

fn num(o: &Value, key: &str, i: usize) -> f64 {
    o.get(key).and_then(|v| v.get(i)).and_then(Value::as_f64).filter(|v| v.is_finite()).unwrap_or(0.0).clamp(-100000.0, 100000.0)
}

#[cfg(test)]
mod staging_tests {
    use super::*;
    use serde_json::json;
    #[test]
    fn legacy_arrivals_do_not_enable_staging() {
        let v=clean_arrivals(&json!([{"name":"dock","pos":[100,0,200]}]));
        assert_eq!(v[0]["stagingEnabled"],false);
        assert_eq!(v[0]["stagingSpacing"],25.0);
    }
    #[test]
    fn staging_survives_layout_save_and_reload() {
        let c=Connection::open_in_memory().unwrap();
        c.execute_batch("CREATE TABLE layouts(server_id TEXT PRIMARY KEY,zones TEXT,arrivals TEXT,map TEXT,rev INTEGER,updated_at TEXT);").unwrap();
        let a=clean_arrivals(&json!([{"name":"dock","pos":[100,0,200],"stagingEnabled":true,"stagingPos":[1000,900,2000],"stagingYaw":450,"stagingBoatPos":[1200,0,2100],"stagingSpacing":30,"convoySpacing":25}]));
        save(&c,"test","[]",&a.to_string(),None).unwrap();
        let loaded=to_json(&get(&c,"test").unwrap().unwrap());
        assert_eq!(loaded["arrivals"][0]["stagingPos"],json!([1000.0,0.0,2000.0]));
        assert_eq!(loaded["arrivals"][0]["stagingYaw"],90.0);
        assert_eq!(loaded["arrivals"][0]["stagingBoatPos"],json!([1200.0,0.0,2100.0]));
        assert_eq!(loaded["arrivals"][0]["convoySpacing"],25.0);
    }
    #[test]
    fn spacing_is_bounded_and_terrain_controls_height() {
        let a=clean_arrivals(&json!([{"stagingPos":[1e90,500,-1e90],"stagingSpacing":-2,"convoySpacing":999}]));
        assert_eq!(a[0]["stagingPos"],json!([100000.0,0.0,-100000.0]));
        assert_eq!(a[0]["stagingSpacing"],15.0);
        assert_eq!(a[0]["convoySpacing"],100.0);
    }
}
