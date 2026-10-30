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
