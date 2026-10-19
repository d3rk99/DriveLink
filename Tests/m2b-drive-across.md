# M2b - drive across (driver only), Chernarus 2302 <-> Livonia 2402, same PC

Getting a car: with `Profiles\DriveLink\debug.json` on Chernarus (`seatTestCar: "OffroadHatchback"`), every NORMAL
login there gives you a kitted car with you in the driver seat. (Arrivals by travel don't get a test car.)

## 1. Drive Chernarus -> Livonia
1. Change the car so you can recognise it: an item in the trunk, a door or wheel off, use some fuel.
2. Drive into the "To Livonia" zone (x 13398, z 6373, 30 m) and STOP inside it (5 s countdown, leaving cancels).
   Chernarus logs: `leaving to livonia ..., driving OffroadHatchback (N parts/items, fuel ..)`,
   `OffroadHatchback <id> travels with ..., removing it here in 3 s`.
3. Livonia logs: `arrived ... with his OffroadHatchback (N/N parts/items, 0 on the ground)`, then
   `seat juncture received` (server + client) and `... seated in his OffroadHatchback: true (seat 0), fuel ..`.
4. In game: you sit in the driver seat of YOUR car (same parts, trunk, fuel), engine off. Start it and drive.
5. Chernarus: the car is gone (go back and look - no copy left at the zone).

## 2. Drive back Livonia -> Chernarus
"To Chernarus" zone (x 2189, z 7747) - 70 m north of where you arrive. Same checks the other way.

## 3. Refusals (no trip, a message instead)
- Passenger in the zone (someone else drives, or you sit in the passenger seat): "Only the driver ...".
- A second person in the car: "Passengers can't travel yet ...".
- Hub stopped: "Travel failed", the car stays (and can be driven again).

## 4. Crash guard (optional)
Right after a car trip, force-stop the SOURCE server before its next save, start it again: its log must show
`car OffroadHatchback <id> was driven to another server, removing the stale copy loaded from storage`.

## Report
Pass/fail per step, all SCRIPT (E) lines, and what you saw in game (seat, parts, trunk, fuel).

## Results 2026-10-18 (Derk, OffroadHatchback, DriveLink 90573-byte build)
| Check | Result |
|---|---|
| Chernarus -> Livonia by car (20:06) | PASS: 29/29 parts/items, fluids exact (fuel 0.999682), seated as driver |
| Livonia -> Chernarus by car (20:09 -> 20:53) | PASS: 22/22, 35/35 player items, fluids exact, seated |
| Source deletes the car | FAIL 20:06:25: deleted with the logging-out driver still in seat 0 -> Chernarus CRASHED (access violation). Fixed: delete only when the seats are empty (retry 1 s, 60 s max) |
| Car parked away until deleted (Derk's idea) | PASS 21:01: moved to the parkingSpot, held 1 s later, removed with empty seats, no crash |
| Crash guard (departed-cars.json) | PASS (real crash): on boot Chernarus removed the stale copy of the car that had left |
| Stale character after the crash | PASS: arrival replaced it ("this server had an old character for him") |
| Chernarus -> Livonia by car with the fixes (21:01) | PASS: 22/22, 35/35, bleeding kept, seated |
| Livonia -> Chernarus by car with the fixes (21:03) | PASS: parked on Livonia, removed with empty seats, 22/22, 35/35, fluids exact, seated |
Not tested yet: refusals (passenger / second person in the car), hub stopped while driving in.

## Solo session 2026-10-18 21:12-21:24 (M3 code, 99233-byte build)
| Check | Result |
|---|---|
| Solo car trips on the M3 code path (4 trips) | PASS: 29/29 parts/items, 35/35 items, fluids exact, seated every time |
| Empty-seat delete while the logout holds the driver 13 s | PASS twice (21:12, 21:22): waited, removed, no crash |
| Hub stopped while driving in | PASS: "Travel failed" after 2 s, player + car stayed |
| Zone cooldown after a failure | PASS: one attempt in > 1 min in the zone; stepped out/in with the hub up -> trip went |
| Failed arrival (target with 0 arrival points) | PASS: ARRIVAL FAILED + hub FAILED, transfer kept, player sent to the menu with the reason |
| DriveLinkHub.exe transfers / return | PASS: listed "FAILED 1x: ...", return turned it around, rejoin -> first arrival point, 35/35 + car 29/29 |
Not tested: alone in the passenger seat (refusal), the main-menu dialog text itself (log shows it was sent).
