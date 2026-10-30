// Vehicle economy ledger (table cars, migration 007, Research/vehicle-economy.md).
// A server reports the travelled cars it holds and the cars that just left it; the answer is how many of ITS cars
// (home = that server) are away, per class, so its mod can veto Central Economy respawns that would replace them.
// Each report also carries where the cars stand; the Map page shows them with their trips (migration 008).
use rusqlite::{params, Connection, OptionalExtension};
use serde::{Deserialize, Serialize};
use serde_json::{json, Value};
use std::collections::{BTreeMap, HashSet};

/// Reports in a row without a car before it counts as gone (a report comes every 60 s).
pub const GONE_AFTER: i64 = 3;

#[derive(Deserialize, Default, Clone)]
#[serde(rename_all = "camelCase")]
pub struct Car {
    #[serde(default)]
    pub dlid: String,
    #[serde(default, rename = "type")]
    pub kind: String,
    #[serde(default)]
    pub home: String,
    // where it stands (only in "here"; older mods send none)
    #[serde(default)]
    pub x: Option<f64>,
    #[serde(default)]
    pub z: Option<f64>,
    #[serde(default)]
    pub health: Option<f64>,
    #[serde(default)]
    pub fuel: Option<f64>,
}

#[derive(Serialize, Debug, PartialEq)]
pub struct Away {
    #[serde(rename = "type")]
    pub kind: String,
    pub n: i64,
}

fn valid(c: &Car) -> bool {
    !c.dlid.is_empty() && c.dlid.len() <= 80 && !c.kind.is_empty() && c.kind.len() <= 80 && c.home.len() <= 64
}

/// One report from `server`. `left` = cars that left it since the last answered report, `here` = every travelled car
/// it holds now. Returns how many cars of each class with home = `server` are not on `server`.
pub fn report(c: &mut Connection, server: &str, left: &[Car], here: &[Car]) -> rusqlite::Result<Vec<Away>> {
    let tx = c.transaction()?;
    for car in left.iter().filter(|c| valid(c)) {
        let home = if car.home.is_empty() { server } else { &car.home };
        // a car already seen on its target (the target reported before the source) stays there
        let moved = tx.execute(
            "INSERT INTO cars(dlid,type,home_server,current_server,state,missing) VALUES(?1,?2,?3,NULL,'in_transit',0)
             ON CONFLICT(dlid) DO UPDATE SET type=excluded.type, current_server=NULL, state='in_transit', missing=0,
             updated_at=CURRENT_TIMESTAMP WHERE cars.current_server IS ?4 OR cars.state='gone'",
            params![car.dlid, car.kind, home, server],
        )?;
        if moved > 0 {
            tx.execute(
                "INSERT INTO car_trips(dlid,from_server,left_at) VALUES(?1,?2,CURRENT_TIMESTAMP)",
                params![car.dlid, server],
            )?;
        }
    }
    let mut seen = HashSet::new();
    for car in here.iter().filter(|c| valid(c)) {
        let home = if car.home.is_empty() { server } else { &car.home };
        seen.insert(car.dlid.clone());
        // None = a car new to the ledger, Some(None) = on the road
        let before: Option<Option<String>> = tx
            .query_row("SELECT current_server FROM cars WHERE dlid=?1", params![car.dlid], |r| r.get(0))
            .optional()?;
        // the home is set once (the first server to name it); later reports only move the car
        tx.execute(
            "INSERT INTO cars(dlid,type,home_server,current_server,state,missing,x,z,health,fuel,seen_at)
             VALUES(?1,?2,?3,?4,'on_map',0,?5,?6,?7,?8,CURRENT_TIMESTAMP)
             ON CONFLICT(dlid) DO UPDATE SET type=excluded.type, current_server=?4, state='on_map', missing=0,
             x=?5, z=?6, health=?7, fuel=?8, seen_at=CURRENT_TIMESTAMP, updated_at=CURRENT_TIMESTAMP",
            params![car.dlid, car.kind, home, server, car.x, car.z, car.health, car.fuel],
        )?;
        arrived(&tx, &car.dlid, home, server, before)?;
    }
    // cars this server held before but did not report now: count up, gone after GONE_AFTER reports
    let held: Vec<String> = {
        let mut st = tx.prepare("SELECT dlid FROM cars WHERE current_server=?1 AND state='on_map'")?;
        let rows = st.query_map(params![server], |r| r.get::<_, String>(0))?;
        rows.collect::<rusqlite::Result<_>>()?
    };
    for dlid in held.iter().filter(|d| !seen.contains(*d)) {
        tx.execute(
            "UPDATE cars SET missing=missing+1, state=CASE WHEN missing+1>=?2 THEN 'gone' ELSE state END,
             updated_at=CURRENT_TIMESTAMP WHERE dlid=?1",
            params![dlid, GONE_AFTER],
        )?;
    }
    let away = away_from(&tx, server)?;
    tx.commit()?;
    Ok(away)
}

/// A trip ends when a car shows up on a server it wasn't on: close its open trip, or (the target reported before the
/// source said it left) record the whole trip from where it was. A car new to the ledger came from its home: the home
/// is the server that tagged it as it left.
fn arrived(c: &Connection, dlid: &str, home: &str, server: &str, before: Option<Option<String>>) -> rusqlite::Result<()> {
    let from = match before {
        Some(Some(s)) if s == server => return Ok(()),
        Some(prev) => prev,
        None if home != server => Some(home.to_string()),
        None => return Ok(()),
    };
    let closed = c.execute(
        "UPDATE car_trips SET to_server=?2, arrived_at=CURRENT_TIMESTAMP
         WHERE id=(SELECT MAX(id) FROM car_trips WHERE dlid=?1 AND to_server IS NULL)",
        params![dlid, server],
    )?;
    if closed == 0 {
        if let Some(from) = from {
            c.execute(
                "INSERT INTO car_trips(dlid,from_server,to_server,arrived_at) VALUES(?1,?2,?3,CURRENT_TIMESTAMP)",
                params![dlid, from, server],
            )?;
        }
    }
    Ok(())
}

/// The Map page (/admin/api/cars): every travelled car that is on a map or on the road, with its last 10 trips.
pub fn for_map(c: &Connection) -> rusqlite::Result<Vec<Value>> {
    let mut st = c.prepare(
        "SELECT dlid,type,home_server,current_server,state,x,z,health,fuel,seen_at,updated_at FROM cars
         WHERE state<>'gone' ORDER BY current_server,type,dlid",
    )?;
    let mut trips = c.prepare(
        "SELECT from_server,to_server,left_at,arrived_at FROM car_trips WHERE dlid=?1 ORDER BY id DESC LIMIT 10",
    )?;
    let rows = st.query_map([], |r| {
        Ok((
            r.get::<_, String>(0)?,
            json!({
                "dlid": r.get::<_, String>(0)?, "type": r.get::<_, String>(1)?, "home": r.get::<_, String>(2)?,
                "server": r.get::<_, Option<String>>(3)?, "state": r.get::<_, String>(4)?,
                "x": r.get::<_, Option<f64>>(5)?, "z": r.get::<_, Option<f64>>(6)?,
                "health": r.get::<_, Option<f64>>(7)?, "fuel": r.get::<_, Option<f64>>(8)?,
                "seenAt": r.get::<_, Option<String>>(9)?, "updatedAt": r.get::<_, String>(10)?,
            }),
        ))
    })?;
    let mut out = Vec::new();
    for row in rows {
        let (dlid, mut car) = row?;
        let t = trips.query_map(params![dlid], |r| {
            Ok(json!({
                "from": r.get::<_, String>(0)?, "to": r.get::<_, Option<String>>(1)?,
                "leftAt": r.get::<_, Option<String>>(2)?, "arrivedAt": r.get::<_, Option<String>>(3)?,
            }))
        })?;
        car["trips"] = Value::Array(t.collect::<rusqlite::Result<_>>()?);
        out.push(car);
    }
    Ok(out)
}

/// Cars with home = `server` that are on another server or on the road, per class.
pub fn away_from(c: &Connection, server: &str) -> rusqlite::Result<Vec<Away>> {
    let mut st = c.prepare(
        "SELECT type, COUNT(*) FROM cars WHERE home_server=?1 AND state<>'gone' AND current_server IS NOT ?1
         GROUP BY type ORDER BY type",
    )?;
    let rows = st.query_map(params![server], |r| Ok(Away { kind: r.get(0)?, n: r.get(1)? }))?;
    rows.collect()
}

/// DriveLinkHub.exe cars: every server's own cars away from home, and where they are.
pub fn list(c: &Connection) -> Result<(), String> {
    let mut st = c
        .prepare("SELECT dlid,type,home_server,COALESCE(current_server,'(on the road)'),state,missing,updated_at FROM cars ORDER BY home_server,type,dlid")
        .map_err(|e| e.to_string())?;
    let rows = st
        .query_map([], |r| {
            Ok((
                r.get::<_, String>(0)?, r.get::<_, String>(1)?, r.get::<_, String>(2)?, r.get::<_, String>(3)?,
                r.get::<_, String>(4)?, r.get::<_, i64>(5)?, r.get::<_, String>(6)?,
            ))
        })
        .map_err(|e| e.to_string())?;
    let mut away: BTreeMap<String, i64> = BTreeMap::new();
    let mut n = 0;
    for row in rows {
        let (dlid, kind, home, cur, state, missing, at) = row.map_err(|e| e.to_string())?;
        n += 1;
        let mut l = format!("{dlid}  {kind}  home {home}  now {cur}  {state}  {at} UTC");
        if missing > 0 && state == "on_map" {
            l += &format!("  (missing from {missing} report(s))");
        }
        println!("{l}");
        if state != "gone" && cur != home {
            *away.entry(home).or_default() += 1;
        }
    }
    if n == 0 {
        println!("No travelled cars yet.");
    }
    for (home, k) in away {
        println!("{home}: {k} of its cars away (its Central Economy does not replace them)");
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    fn db() -> Connection {
        let c = Connection::open_in_memory().unwrap();
        c.execute_batch(include_str!("../migrations/007_cars.sql")).unwrap();
        c.execute_batch(include_str!("../migrations/008_car_map.sql")).unwrap();
        c
    }
    fn car(dlid: &str, kind: &str, home: &str) -> Car {
        Car { dlid: dlid.into(), kind: kind.into(), home: home.into(), ..Default::default() }
    }
    fn away(kind: &str, n: i64) -> Away {
        Away { kind: kind.into(), n }
    }

    #[test]
    fn a_car_that_leaves_stays_counted_at_home_until_it_comes_back() {
        let mut c = db();
        let hatch = car("x1", "OffroadHatchback", "cher");
        assert_eq!(report(&mut c, "cher", &[hatch.clone()], &[]).unwrap(), vec![away("OffroadHatchback", 1)]);
        // on the road, then on livonia: still away from chernarus
        report(&mut c, "livonia", &[], &[hatch.clone()]).unwrap();
        assert_eq!(away_from(&c, "cher").unwrap(), vec![away("OffroadHatchback", 1)]);
        // livonia holds it but does not own it
        assert_eq!(away_from(&c, "livonia").unwrap(), vec![]);
        // back home
        report(&mut c, "livonia", &[hatch.clone()], &[]).unwrap();
        assert_eq!(report(&mut c, "cher", &[], &[hatch]).unwrap(), vec![]);
    }

    #[test]
    fn a_car_missing_from_its_server_is_gone_after_three_reports_and_frees_its_home_slot() {
        let mut c = db();
        let hatch = car("x1", "OffroadHatchback", "cher");
        report(&mut c, "cher", &[hatch.clone()], &[]).unwrap();
        report(&mut c, "livonia", &[], &[hatch]).unwrap();
        for _ in 0..GONE_AFTER - 1 {
            report(&mut c, "livonia", &[], &[]).unwrap();
            assert_eq!(away_from(&c, "cher").unwrap().len(), 1);
        }
        report(&mut c, "livonia", &[], &[]).unwrap();
        assert_eq!(away_from(&c, "cher").unwrap(), vec![]);
    }

    #[test]
    fn the_target_reporting_before_the_source_does_not_put_the_car_back_on_the_road() {
        let mut c = db();
        let hatch = car("x1", "OffroadHatchback", "cher");
        report(&mut c, "livonia", &[], &[hatch.clone()]).unwrap();
        report(&mut c, "cher", &[hatch], &[]).unwrap(); // the source's late "left"
        let cur: String = c.query_row("SELECT current_server FROM cars WHERE dlid='x1'", [], |r| r.get(0)).unwrap();
        assert_eq!(cur, "livonia");
        assert_eq!(away_from(&c, "cher").unwrap(), vec![away("OffroadHatchback", 1)]);
    }

    #[test]
    fn the_home_never_changes_and_bad_rows_are_ignored() {
        let mut c = db();
        report(&mut c, "cher", &[car("x1", "Sedan_02", "")], &[]).unwrap(); // empty home = the reporting server
        report(&mut c, "livonia", &[], &[car("x1", "Sedan_02", "livonia"), car("", "Sedan_02", "x")]).unwrap();
        let home: String = c.query_row("SELECT home_server FROM cars WHERE dlid='x1'", [], |r| r.get(0)).unwrap();
        assert_eq!(home, "cher");
        let n: i64 = c.query_row("SELECT COUNT(*) FROM cars", [], |r| r.get(0)).unwrap();
        assert_eq!(n, 1);
    }

    #[test]
    fn five_cars_farmed_to_another_map_are_all_still_owed_to_their_home() {
        let mut c = db();
        let cars: Vec<Car> = (0..5).map(|i| car(&format!("f{i}"), "OffroadHatchback", "cher")).collect();
        report(&mut c, "cher", &cars, &[]).unwrap();
        report(&mut c, "livonia", &[], &cars).unwrap();
        assert_eq!(away_from(&c, "cher").unwrap(), vec![away("OffroadHatchback", 5)]);
    }

    fn trips(c: &Connection, dlid: &str) -> Vec<(String, Option<String>, bool)> {
        let mut st = c.prepare("SELECT from_server,to_server,left_at IS NOT NULL FROM car_trips WHERE dlid=?1 ORDER BY id").unwrap();
        st.query_map(params![dlid], |r| Ok((r.get(0)?, r.get(1)?, r.get(2)?))).unwrap().map(|r| r.unwrap()).collect()
    }

    #[test]
    fn a_round_trip_is_two_trips_and_the_map_shows_where_the_car_stands() {
        let mut c = db();
        let hatch = car("x1", "OffroadHatchback", "cher");
        let mut parked = hatch.clone();
        (parked.x, parked.z, parked.health, parked.fuel) = (Some(157.7), Some(8950.1), Some(0.9), Some(0.5));
        report(&mut c, "cher", &[hatch.clone()], &[]).unwrap();
        report(&mut c, "livonia", &[], &[parked.clone()]).unwrap();
        report(&mut c, "livonia", &[], &[parked.clone()]).unwrap(); // a later report is no new trip
        report(&mut c, "livonia", &[hatch.clone()], &[]).unwrap();
        report(&mut c, "cher", &[], &[parked]).unwrap();
        let s = |v: &str| v.to_string();
        assert_eq!(trips(&c, "x1"), vec![(s("cher"), Some(s("livonia")), true), (s("livonia"), Some(s("cher")), true)]);
        let m = for_map(&c).unwrap();
        assert_eq!(m.len(), 1);
        assert_eq!((m[0]["server"].as_str(), m[0]["home"].as_str()), (Some("cher"), Some("cher")));
        assert_eq!((m[0]["x"].as_f64(), m[0]["z"].as_f64(), m[0]["fuel"].as_f64()), (Some(157.7), Some(8950.1), Some(0.5)));
        assert_eq!(m[0]["trips"].as_array().unwrap().len(), 2);
        assert_eq!(m[0]["trips"][0]["from"].as_str(), Some("livonia")); // newest first
    }

    #[test]
    fn a_trip_is_recorded_when_the_target_reports_before_the_source() {
        let mut c = db();
        let hatch = car("x1", "OffroadHatchback", "cher");
        report(&mut c, "livonia", &[], &[hatch.clone()]).unwrap(); // new car: it came from its home
        report(&mut c, "cher", &[hatch.clone()], &[]).unwrap(); // the late "left" adds nothing
        assert_eq!(trips(&c, "x1"), vec![("cher".to_string(), Some("livonia".to_string()), false)]);
    }
}
