# Vehicle economy across maps - design note (2026-10-29)

Status: built behind `carEconomy` (default "off") on branch `feature/vehicle-economy`. The CE questions are answered
headlessly (`Tests/CESpike`), and the two-server scenario runs in `Tests/EconomyTest`. A real player trip with a
tagged car passed on 2026-10-29 (see Results).

Goal: cars travel between maps without players losing them and without the network gaining cars over time.

## Problem

A car trip deletes the car on the source server and spawns a NEW object on the target (R2-cars.md). Each
server's Central Economy (CE) only sees its own map:

- Source: one car below nominal for that class -> CE spawns a replacement.
- Target: one car above nominal -> CE stops spawning that class until the count drops. CE does NOT delete
  existing cars to reach nominal. Cars go only when their lifetime runs out (refreshed by players) or as
  ruined-wreck cleanup. So "too many cars despawn" is not the real risk.
- Net effect: every trip adds one car to the network. Exploit (car farm): take cars on map A, drive them to B,
  A refills, repeat -> B fills up with as many cars as players care to move.

The Hub's transfer token already prevents duplicates within a trip. It does nothing about CE refilling the
source.

## Design: every car has a home map that keeps its slot

### 1. Car ledger on the Hub
- Each car gets a global ID (DLID) the first time DriveLink sees it (at departure is enough). It is stored on
  the car in OnStoreSave/OnStoreLoad, so it survives server restarts and travels in the snapshot.
- Hub table `cars`: `dlid, class, home_server, current_server, state, last_seen`.
  `state` = `on_map | in_transit | destroyed`.
- `home_server` = the map whose CE spawned the car (the first server that registers it).
- Same ledger fixes rollbacks: if a server restarts from a save made before a trip, the car loads, the server
  asks the Hub about its DLID, and deletes it if the Hub says it is elsewhere.

### 2. Away cars count at home
A car that is not on its home map (on another map or in transit) still uses a slot on its home map for its class.

### 3. Home map vetoes CE spawns
`EntityAI.EEOnCECreate()` runs on the server when CE creates an entity. For `CarScript`, the server mod checks:

```
here(C) + away_from_here(C) >= nominal(C)   ->  delete the new CE spawn
```

- `here(C)`: cars of class C currently on this map (track CarScript instances in EEInit/EEDelete).
- `away_from_here(C)`: cars with home = this server and current != this server, from the Hub, cached and
  refreshed on change.
- `nominal(C)`: **changed after the spike.** Vehicle counts live in `db/events.xml`, not `types.xml`, where every
  vehicle has nominal 0. The rule works per event (`VehicleOffroadHatchback`, nominal 8), counting all of the event's
  child classes together. The mod parses `$mission:db/events.xml` plus the event files listed in `cfgeconomycore.xml`.
- Only CE spawns are vetoed. Player-owned cars are never deleted by this rule.

### 4. Destroyed or despawned travelled cars
When a car with a DLID is ruined or deleted by lifetime on any map, that server reports it. The Hub sets
`state = destroyed`, `away_from_here` on its home drops by one, and CE there respawns it normally.

### 5. Importing maps: accept a mild under-spawn
Visitor cars push the target over nominal, so its CE spawns fewer of that class. Don't fight it: busy
destinations already have player cars, and it corrects itself as visitor cars die.

Testing changed this. An arrival made by script is never counted by the target's CE: B counted 8 while 10-12
were there, across 3 restarts. So there is no under-spawn at the target at all. Visitors are extra cars there,
owed to their home, and the network total stays exact.

### 6. How it's built
- Mod: `Addons/DriveLink/Scripts/4_World/DriveLink/DL_CarLedger.c`. Tags are kept by persistent ID in
  `$profile:DriveLink/car-ledger.json`, so the game's storage format is unchanged and removing the mod is safe.
  Hooks sit on `CarScript`, `BoatScript` and `MotorbikeScript`. `Transport` can't be modded, and the vanilla
  `EEOnCECreate` overrides don't call super.
- The veto only acts when a travelled car of this server is involved: one of its cars is away, or one came home
  by script. Otherwise CE runs exactly as vanilla, even with `carEconomy: "veto"`.
- Car snapshots carry `dlid` and `home` (`DL_CarData`). Older builds ignore the extra JSON fields, so the
  protocol is unchanged.
- Hub: `Hub/src/cars.rs`, migration `007_cars.sql`, `POST /api/cars` (reported every 60 s, the first report
  1 min after hello so storage has loaded), and `drivelink-hub.exe cars` to list the ledger.

Result: the network total for each class stays at the sum of nominals, however cars move.

## Alternative rejected
Set vehicle nominal to 0 everywhere and let the Hub spawn all cars from a global pool. Cleaner accounting, but it
rebuilds CE spawn logic (zones, events, restock) and breaks every server's existing vehicle setup.

## Open questions
1. Does a script-spawned car (DriveLink arrival, `CreateObjectEx`) count toward the target's nominal? Expected
   yes (admin spawns do), unconfirmed.
2. After a veto, how fast does CE retry the spawn? Expect a delete every CE cycle while the deficit lasts. Check
   log noise and server cost.
3. Is `EEOnCECreate` also called for CE vehicle events (`events.xml` `VehicleCivilianSedan` etc.) and not only
   for types.xml loot spawns? Vehicles come from events, so this is the critical path.
4. Can the mod read `db/types.xml` from the mission folder at runtime (`$mission:` path or the profile)?
   If not, nominals come from the Hub admin page.
5. Home policy: fixed forever, or move the home after N days on another map? It matters when a map leaves the
   network: its away cars need a new home or should simply stop counting.
6. Cars that exist before DriveLink is installed (no DLID): register at first departure with home = current
   server. Confirm that's acceptable for cars already moved by older DriveLink versions.
7. Hub unreachable: veto from the last cached `away` counts (safe direction) or allow all spawns? Proposal: use
   the cache, allow spawns only if there is no cache at all.
8. Vehicle classes listed under more than one event or with a modded parent: count per exact class or per
   event? The `types.xml` nominal is per class; events have their own nominal. Decide which one the veto uses.

## Test plan
Needs a two-server test setup (TestServer/TestServer2 were not restored after the move to E:; ask before
recreating). Use one vehicle class with a small nominal (e.g. `OffroadHatchback` nominal 3) and a short CE
restock so results show within minutes.

| # | Test | Pass when |
|---|---|---|
| T1 | Spawn a car from script on a map already at nominal, wait several CE cycles | CE does not spawn that class (answers Q1) |
| T2 | Log `EEOnCECreate` for every CarScript on startup and respawn, with events.xml vehicles only | Every CE-spawned car is logged (Q3) |
| T3 | Delete a CE car in `EEOnCECreate`, watch the logs | Retry interval recorded, no errors or crash, no leftover wreck (Q2) |
| T4 | Read `types.xml` from script on the server | Nominals for test classes match the file (Q4) |
| T5 | A->B trip with the veto off | Baseline: A respawns, network +1 (shows the problem) |
| T6 | A->B trip with the veto on | A stays at nominal-1, B at nominal+1, network total unchanged after many CE cycles |
| T7 | Ruin the travelled car on B | Hub marks destroyed, A respawns one within a CE cycle |
| T8 | Car-farm loop: move 5 cars A->B in a row | Network total unchanged, A never refills |
| T9 | Round trip A->B->A | A at nominal, `away` back to 0, no vetoes |
| T10 | Force-stop A before its save after a trip, restart A | Restored car with a DLID that is on B gets deleted on load (no dupe) |
| T11 | Hub down during A's CE cycle | Behaviour matches Q7's decision |
| T12 | Natural lifetime despawn of a travelled car (shortened lifetime) | Server reports it, home respawns |

## Build order (once T1-T4 pass)
1. DLID storage on CarScript + Hub `cars` table + register/move/destroy calls.
2. Per-server class counts and nominal reporting.
3. Veto in `EEOnCECreate`, behind a server setting (default off until T6-T12 pass).
4. Hub admin page: per-map imported/away counts per class.

## Results (2026-10-29, headless, no players)

### CE spike (`Tests/CESpike/README.md`)
- Q1: an arrival made by script is not counted by the target's event. (The spike also saw CE adopt script cars after a
  restart, but that run had idle mode on and no lifetime fix. The two-server run below did not see it again.)
- Q2: after a veto, CE respawns the whole deficit in one batch, about every 2.6 min. That's cheap.
- Q3: `EEOnCECreate` fires for every event vehicle. The hook must sit on `CarScript` / `BoatScript` / `MotorbikeScript`.
- Q4: `$mission:db/events.xml` and `cfgeconomycore.xml` are readable.
- CE idle mode (`globals.xml` `IdleModeCountdown` / `IdleModeStartup`) pauses all respawns and cleanup while no player
  is online. Headless tests must turn it off.

### Bug found and fixed in every arrival (not only with `carEconomy` on)
`types.xml` gives every vehicle class `lifetime` 3 s, because event vehicles are kept alive by their event. A car
DriveLink builds belongs to no event, so **CE cleanup deleted arrivals about 30 s after they were built** when no player
was near. `DL_CarSnapshot.Build` now sets `SetLifetimeMax` / `SetLifetime` to 45 days. With that, visitors survived 3
restarts on B. (Earlier live trips didn't show this because a player was always in or near the car.)

### Two-server scenario (`Tests/EconomyTest/README.md`, event `VehicleOffroadHatchback`, nominal 8)
| Test | Result |
|---|---|
| Baseline | A 8, B 8, reports accepted |
| A sends 4 to B | PASS: Hub has 4 cars with home `cher_a` on `cher_b`, and B keeps all 12 |
| T6: A restarts | PASS: CE tried to replace the 4 twice. All 8 attempts were vetoed (4 here + 4 away >= 8), A stays at 4 + 4 away |
| T7: 2 visitors wrecked on B | PASS: `gone` after 3 missed reports, A's away drops to 2. A's refill of 4: 2 allowed, 2 vetoed, so 6 here + 2 away |
| T9: the last 2 come home | PASS: Hub `now cher_a`, away 0, A has 8. CE still counted 6, so its 2 refills (twice) were vetoed by the came-home guard |
| Network total | Stayed at the sum of nominals in every step (16, then 14 after the 2 wrecks, then back to 16 as A refilled 2) |
| Target CE | B's CE counted 8 the whole time while 10-12 were there: visitors are never counted at the target, so there's no under-spawn there |

Two bugs found and fixed on the way: IDs made in the same moment collided (`Math.Randomize(-1)` reseeded on every
ID, so it's now seeded once with a counter), and the test helper looped over an array returned from a function.

### Not tested yet
- ~~A real player trip with `carEconomy` on.~~ Passed 2026-10-29: A -> B -> A by car, the car kept its ID and home,
  the Hub tracked it, and the parked arrival survived 11 min on A, 2 of them with nobody on the server
  (`Tests/EconomyTest/README.md`, Player trip). A single trip doesn't drop A below `min`, so no veto was involved.
- T10 (crash before save): the existing `DL_CarGuard` covers the duplicate. The ledger side (a `left` entry kept in
  `car-ledger.json` until the Hub takes it) wasn't exercised.
- T11 (Hub down): vetoes use the last answer saved in `car-ledger.json`. Untested.
- T12 (a visitor's lifetime runs out): with 45 days it now takes 45 days untouched. The Hub side (missing reports, then
  `gone`) is covered by T7.
- Boats and bikes: same hooks, not run.
