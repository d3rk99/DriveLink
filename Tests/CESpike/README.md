# CE spike: how the Central Economy treats vehicles (2026-10-29)

Answers the first four open questions in `Research/vehicle-economy.md` on one isolated, headless Chernarus server.
No Hub, no players and no DriveLink mod. Only the server-only addon `DL_CESpike` (`-serverMod`).

```
powershell -ExecutionPolicy Bypass -File Tests\CESpike\run-spike.ps1 -Phase baseline -Fresh
powershell -ExecutionPolicy Bypass -File Tests\CESpike\run-spike.ps1 -Stop
```

- The mission `mpmissions\dlcetest.chernarusplus` is a copy of the vanilla mission with a marker file. It has CE vehicle,
  event and statistics logging on, and CE idle mode off (`IdleModeCountdown` 0, `IdleModeStartup` 0). Idle mode
  pauses all respawns while no player is online, which hid every respawn in the first runs.
- It uses `instanceId=77`, ports 2702 / 27017, and the profile in `Build\profile` (gitignored).
- Phases live in `phases\*.json`. Every line in the script log starts with `[DLCE]`. CE's own count per event is in
  the RPT, e.g. `VehicleOffroadHatchback (8)`.
- Stopping with `-Stop` kills the process, so changes since the last autosave are lost. Phases that must persist end
  with `exitAt` (a clean `RequestExit`).

## Results

| Question | Result |
|---|---|
| Q1: does a car made by script count toward its event? | **Not until a restart.** After 3 deleted + 3 script-spawned hatchbacks, CE counted 5 while 8 existed. After the next restart CE counted 8: it adopts cars loaded from storage. |
| Q2: what does CE do after a veto? | It respawns the **whole deficit in one batch** and tries again **about every 2.6 min** (t=231 s and t=389 s, 4 cars each time). That's cheap. |
| Q3: is `EEOnCECreate` called for event vehicles? | **Yes**, for every event vehicle, after `EEInit`. Vanilla `CarScript`, `BoatScript` and `MotorbikeScript` override it **without calling super**, so hooks must go on those classes. `Transport` can't be modded (it's declared under `#ifdef`). |
| Q4: can a mod read the economy files? | **Yes**, through `$mission:db/events.xml` and `$mission:cfgeconomycore.xml`. |
| Where the vehicle nominal lives | `events.xml` (`VehicleOffroadHatchback`: nominal 8, min 5). In `types.xml` every vehicle has nominal 0. |
| When CE refills an event | At 4 (below `min` 5), it refilled to nominal 8 within about 4 min of boot. At exactly `min` it didn't refill, but that run still had idle mode on, so it's unconfirmed. |
