# Vehicle economy test (two servers, no players)

Tests the vehicle economy (`Research/vehicle-economy.md`, `DL_CarLedger.c`, `Hub/src/cars.rs`). It uses a test Hub and
two headless Chernarus servers, A (`cher_a`) and B (`cher_b`).

No player can drive on a headless server, so the trip itself is a file. A dev build's `debug.json` `econ*` switches
(`DL_MissionServerEconTest.c`) snapshot cars into `econ-outbox.json`, and the runner moves that file to the other
server's `econ-inbox.json`. Everything else is the real code: `DL_CarSnapshot.Capture/Build`, `DL_CarLedger`
(Stamp, Left, Arrived, veto, reports) and the Hub's `/api/cars`.

```
powershell -ExecutionPolicy Bypass -File Tests\EconomyTest\run-econ.ps1 -Step build
powershell -ExecutionPolicy Bypass -File Tests\EconomyTest\run-scenario.ps1        # about 50 min
powershell -ExecutionPolicy Bypass -File Tests\EconomyTest\run-econ.ps1 -Step cars # the hub's ledger
```

- Run `Tests\CESpike\run-spike.ps1` once first. It creates the A mission (CE logging on, idle mode off), and B's
  mission is copied from it.
- Server A: `dlcetest.chernarusplus`, `instanceId` 77, port 2702. Server B: `dlcetest2.chernarusplus`, 78, port 2712.
  The test Hub runs on `127.0.0.1:7390` with its own database in `Build\hub`. Everything under `Build\` is gitignored.
- Each step's `debug.json` is in `steps\`. Every step ends with a clean exit, so the world storage is saved.
- The event under test is `VehicleOffroadHatchback` (nominal 8, min 5). CE refills it only below `min`, which is why
  4 cars leave, not 3.

## Scenario (run-scenario.ps1)

| Step | Expect |
|---|---|
| reset, A + B baseline | 8 hatchbacks each, reports accepted |
| A send4 -> B receive | Hub: 4 cars with home `cher_a`, now on `cher_b` |
| A idle7min (T6) | A's CE refills 4 (count 4 < min 5): all 4 vetoed, A stays at 4 here + 4 away |
| B ruin2 (T7) | after 3 reports without them, the 2 wrecked cars are `gone` and A's away drops to 2 |
| A idle7min (T7) | refill of 4: 2 allowed, 2 vetoed, so A has 6 here + 2 away |
| B sendback2 -> A receive (T9) | the 2 cars are back home on A, nothing is away |
| A idle7min (T9) | A has 8 again. CE still counts 6, so its refills are vetoed by the came-home guard |

## Player trip (needs a player)

The same two servers stay up, and a player drives a car from A to B and back. This checks the live trip path with
`carEconomy: "veto"`: the car is tagged at departure, the Hub tracks it, and it's built on B with its home and a
45-day lifetime.

```
powershell -ExecutionPolicy Bypass -File Tests\EconomyTest\run-econ.ps1 -Step build
powershell -ExecutionPolicy Bypass -File Tests\EconomyTest\run-econ.ps1 -Step trip     # reset, then A + B stay up
powershell -ExecutionPolicy Bypass -File Tests\EconomyTest\run-econ.ps1 -Step client   # the game joins A
powershell -ExecutionPolicy Bypass -File Tests\EconomyTest\run-econ.ps1 -Step cars
powershell -ExecutionPolicy Bypass -File Tests\EconomyTest\run-econ.ps1 -Step stop
```

- Both servers are Chernarus with the same layout: fresh spawns at x 190, z 8990 on the west edge, the arrival point
  "west" at x 157.7, z 8950.1, and the leaving zone to the other server 160 m away (x 50.5, z 8827.1, radius 137, 0 km).
- A gives every normal login an `OffroadHatchback` with the player in the driver seat (`steps\trip-A.json`).
- Wait for `HUB_POLICY ... known=true restart=false` in both script logs (about 30 s after hello), so the client
  reconnects directly.

| Step | Pass when |
|---|---|
| 1. Join A, drive into the zone (south-west, toward the map edge) and stop 5 s | A logs the departure; you land in the car on B at "west" |
| 2. On B, wait 2 min | `-Step cars`: the car has home `cher_a`, now `cher_b`; B's log has `vehicle economy: ... arrived (home cher_a)` |
| 3. Leave the car on B, walk 100 m away, wait 2 min | The car is still there (the 45-day lifetime; before the fix it vanished after about 30 s) |
| 4. Drive back into B's zone | You land on A in the car; `-Step cars` shows `now cher_a`, nothing away |

### Results 2026-10-29 (Derk, OffroadHatchback)
| Step | Result |
|---|---|
| 1. A -> B by car (20:48) | PASS: tagged `cher_a-705582108-1` (home `cher_a`), A counted it away; built on B 29/29, counted as a visitor |
| 2. B -> A by car (20:49) | PASS: built on A 29/29 as "own back home", Hub `now cher_a`, away 0 after the next report |
| 3. Parked arrival on A | PASS: the car stood from 20:50 until at least 21:01, including 2 min with nobody on A (an accidental walk to B and back on foot, 20:59-21:01) |
| Counts | B back to its own 8, A 8 + the returned car, no vetoes needed, Hub reports accepted every minute |

Right after each build the log reads fuel/oil/coolant 0, then 4 s later the engine runs with the saved fluids. That's the
known load order, not a loss.
