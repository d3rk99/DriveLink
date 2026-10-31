// Chance of surviving a trip (POST /api/odds). A player who is short of supplies sees it before he decides to travel
// anyway (Travel rules: allowAnyway). The server sends what he has and what the trip costs; the hub works out the
// odds with that server's travel rules, the same way the mod plays the road (DL_Journey, weakened mode):
// - a vehicle that runs dry (and finds no gas station / no luck) or gets a flat without a spare is left: the rest is
//   walked (or swum, at sea);
// - every event leg walked: an infected attack (bitePct), which kills biteDeathPct of the time;
// - every event leg swum: drowning, likelier with every leg (drownPct x1, x2, x3 ...);
// - more than half short of food or water at the end (after what the road gave): starveDeathPct;
// - health under 50: death chances x1.5 (DL_Journey.HurtX).
// It is an estimate (the road is random); the mod shows it rounded.
use crate::config::TravelRules;
use serde::Deserialize;

#[derive(Deserialize, Default, Clone)]
#[serde(rename_all = "camelCase", default)]
pub struct Person {
    pub name: String,
    pub health: f64, // 0..100
    pub food: f64,   // kcal he can eat on the way (bars, stomach, carried food)
    pub need_food: f64,
    pub water: f64, // ml
    pub need_water: f64,
}

#[derive(Deserialize, Default)]
#[serde(rename_all = "camelCase", default)]
pub struct Trip {
    pub km: f64,
    /// "foot" | "land" (car, truck, motorbike) | "water" (boat)
    pub kind: String,
    pub people: Vec<Person>,
    pub fuel: f64, // litres in the tank + gasoline canisters
    pub need_fuel: f64,
    /// the trip needs a spare wheel / tyre repair kit and has one (or needs none)
    pub spare_ok: bool,
}

const WATER_FIND_ML: f64 = 1500.0; // a well: everyone drinks (DL_Journey.Finds)
const FOOD_FIND_KCAL: f64 = 800.0; // a few items, about a can of beans each

fn pct(v: f64) -> f64 {
    (v / 100.0).clamp(0.0, 1.0)
}

/// km of the trip expected on foot (or swimming) because the vehicle was left on the road
fn walked_km(t: &TravelRules, trip: &Trip) -> f64 {
    let km = trip.km.max(0.0);
    if trip.kind == "foot" || trip.kind.is_empty() {
        return km;
    }
    let ev = t.event_km.max(1.0);
    let mut walk = 0.0;
    if trip.need_fuel > 0.0 && trip.fuel < trip.need_fuel {
        let reach = (trip.fuel / trip.need_fuel).clamp(0.0, 1.0);
        let legs = km * reach / ev;
        let find_gas = 1.0 - (1.0 - pct(t.find_gas_pct)).powf(legs);
        let keep = 1.0 - (1.0 - pct(t.luck_gas_pct)) * (1.0 - find_gas);
        walk += km * (1.0 - reach) * (1.0 - keep);
    }
    if trip.kind == "land" && !trip.spare_ok {
        // a flat without a spare: the car is left, on average half way
        let flat = 1.0 - (1.0 - pct(t.flat_pct)).powf(km / ev);
        walk += (km - walk) * 0.5 * flat;
    }
    walk.clamp(0.0, km)
}

/// each person's chance (0..100) to arrive alive
pub fn survival(t: &TravelRules, trip: &Trip) -> Vec<u32> {
    let km = trip.km.max(0.0);
    let ev = t.event_km.max(1.0);
    let walk = walked_km(t, trip);
    let swim = trip.kind == "water";
    let legs = km / ev;
    trip.people
        .iter()
        .map(|p| {
            let hx = if p.health < 50.0 { 1.5 } else { 1.0 };
            let mut alive = 1.0;
            if swim {
                // leg i of the swim: drownPct x (1 + i), capped at 90 % (DL_Journey.DrownPct)
                let mut left = walk / ev;
                let mut i = 0.0;
                while left > 0.0 {
                    let share = left.min(1.0);
                    let d = (pct(t.drown_pct) * share * (1.0 + i) * hx).min(0.9);
                    alive *= 1.0 - d;
                    left -= 1.0;
                    i += 1.0;
                }
            } else {
                let per_leg = pct(t.bite_pct) * (pct(t.bite_death_pct) * hx).min(1.0);
                alive *= (1.0 - per_leg).powf(walk / ev);
            }
            // food and water: walking costs the foot rates, the road may give some
            let extra_food = if trip.kind == "foot" { 0.0 } else { (t.food_foot_kcal - t.food_car_kcal).max(0.0) * walk / 10.0 };
            let extra_water = if trip.kind == "foot" { 0.0 } else { (t.water_foot_ml - t.water_car_ml).max(0.0) * walk / 10.0 };
            let need_f = p.need_food + extra_food;
            let need_w = p.need_water + extra_water;
            let have_f = p.food + FOOD_FIND_KCAL * legs * pct(t.find_food_pct);
            let have_w = p.water + WATER_FIND_ML * legs * pct(t.find_water_pct);
            let short_f = if need_f > 0.0 { 1.0 - have_f / need_f } else { 0.0 };
            let short_w = if need_w > 0.0 { 1.0 - have_w / need_w } else { 0.0 };
            let short = short_f.max(short_w).clamp(0.0, 1.0);
            // the mod kills at "more than half short"; the road's finds are random, so blend around that line
            let over_half = ((short - 0.35) / 0.3).clamp(0.0, 1.0);
            alive *= 1.0 - over_half * (pct(t.starve_death_pct) * hx).min(1.0);
            (alive * 100.0).round().clamp(0.0, 100.0) as u32
        })
        .collect()
}

#[cfg(test)]
mod tests {
    use super::*;

    fn rules() -> TravelRules {
        TravelRules { mode: "weakened".into(), ..Default::default() }
    }
    fn person(health: f64, food: f64, water: f64) -> Person {
        Person { name: "p".into(), health, food, need_food: 1500.0, water, need_water: 1500.0 }
    }

    #[test]
    fn fed_and_fuelled_in_a_car_with_a_spare_is_safe() {
        let trip = Trip { km: 20.0, kind: "land".into(), people: vec![person(100.0, 5000.0, 5000.0)], fuel: 40.0, need_fuel: 10.0, spare_ok: true };
        assert_eq!(survival(&rules(), &trip), vec![100]);
    }

    #[test]
    fn starving_and_hurt_is_worse_than_starving() {
        let t = rules();
        let trip = |h| Trip { km: 20.0, kind: "foot".into(), people: vec![person(h, 0.0, 0.0)], ..Default::default() };
        let healthy = survival(&t, &trip(100.0))[0];
        let hurt = survival(&t, &trip(30.0))[0];
        assert!(hurt < healthy, "{hurt} < {healthy}");
        assert!(healthy < 100);
    }

    #[test]
    fn shorter_event_legs_roll_more_often() {
        let mut t = rules();
        let trip = Trip { km: 100.0, kind: "foot".into(), people: vec![person(100.0, 1e6, 1e6)], ..Default::default() };
        let ten = survival(&t, &trip)[0];
        t.event_km = 1.0;
        let one = survival(&t, &trip)[0];
        assert!(one < ten, "{one} < {ten}");
    }

    #[test]
    fn out_of_fuel_means_walking() {
        let t = rules();
        let full = Trip { km: 50.0, kind: "land".into(), people: vec![person(100.0, 1e6, 1e6)], fuel: 20.0, need_fuel: 10.0, spare_ok: true };
        let dry = Trip { fuel: 0.0, ..full };
        let full_k = walked_km(&t, &Trip { fuel: 20.0, km: 50.0, kind: "land".into(), need_fuel: 10.0, spare_ok: true, people: vec![] });
        assert_eq!(full_k, 0.0);
        assert!(walked_km(&t, &dry) > 20.0);
        assert!(survival(&t, &dry)[0] < 100);
    }
}
