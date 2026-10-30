// Vehicle economy ledger (table cars, migration 007, Research/vehicle-economy.md).
// A server reports the travelled cars it holds and the cars that just left it; the answer is how many of ITS cars
// (home = that server) are away, per class, so its mod can veto Central Economy respawns that would replace them.
use rusqlite::{params, Connection};
use serde::{Deserialize, Serialize};
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
        tx.execute(
            "INSERT INTO cars(dlid,type,home_server,current_server,state,missing) VALUES(?1,?2,?3,NULL,'in_transit',0)
             ON CONFLICT(dlid) DO UPDATE SET type=excluded.type, current_server=NULL, state='in_transit', missing=0,
             updated_at=CURRENT_TIMESTAMP WHERE cars.current_server IS ?4 OR cars.state='gone'",
            params![car.dlid, car.kind, home, server],
        )?;
    }
    let mut seen = HashSet::new();
    for car in here.iter().filter(|c| valid(c)) {
        let home = if car.home.is_empty() { server } else { &car.home };
        seen.insert(car.dlid.clone());
        // the home is set once (the first server to name it); later reports only move the car
        tx.execute(
            "INSERT INTO cars(dlid,type,home_server,current_server,state,missing) VALUES(?1,?2,?3,?4,'on_map',0)
             ON CONFLICT(dlid) DO UPDATE SET type=excluded.type, current_server=?4, state='on_map', missing=0,
             updated_at=CURRENT_TIMESTAMP",
            params![car.dlid, car.kind, home, server],
        )?;
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
        c
    }
    fn car(dlid: &str, kind: &str, home: &str) -> Car {
        Car { dlid: dlid.into(), kind: kind.into(), home: home.into() }
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
}
