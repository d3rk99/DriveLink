# M1b - full state travel (mod v0.3 + hub on SQLite, Phase A)

What changed since the 2026-10-13 run (v0.2 b6a23e5, arrival failed "Miss fraction part in number"):
- The snapshot no longer travels as an escaped string inside the /api/arrive answer. The arriving server fetches it
  from the new /api/payload, where it is the WHOLE response body (the 0cd3377 self test proved DayZ reads that exact
  text fine from a file).
- An arrival that can't be built is no longer a silent "normal login" (which left the hub owner empty = every later
  trip refused 409). The server reports it to the hub (/api/fail), the trip stays pending, and the player is sent
  to the main menu with the reason. Admin tools: `Hub\DriveLinkHub.exe transfers | return <steamId> | drop <steamId>`.
- A hub answer that comes after the character was already made is no longer dropped (see step 5).

## 0. Hub moved to SQLite (2026-10-18)
Rebuild the hub (Tools\build-hub.ps1) and start it: it drops the old postgres line from hub.json, creates
Hub\drivelink.db and starts EMPTY - the stuck trip from 2026-10-13 is gone with the old database, and your next
login on any server is a normal login that makes that server the owner. PostgreSQL is no longer needed.
Expect `database: applied 001_init.sql`, `database: applied 002_fail_info.sql`, `listening on 0.0.0.0:7290`.

## 1. Build + start
Pull, Tools\build-mod.ps1 (+ re-sign, copy to client). Remove the old Profiles\DriveLink\selftest folders.
Restart both servers. Each must log `[DriveLink] v0.3 server '...'` and `hub hello OK`.
No SCRIPT (E) lines expected; send them all if there are any.

## 2. Gear to carry (as many as possible, VPP spawn)
- clothes with cargo: shirt, pants, jacket or vest, backpack (items placed in specific spots, one rotated)
- pistol in hands WITH a magazine attached and a round in the chamber; a second full mag in cargo; a loose ammo pile
- rifle on the back with an optic (attachment on an attachment)
- canteen with water, a cooked or burned food item, a flashlight that is switched ON with a battery
- something on the quickbar (slots 1-3)
- be bleeding (cut yourself) and have eaten something just before
Screenshot inventory + note quickbar.

## 3. Trip Livonia -> Chernarus, then Chernarus -> Livonia
Hub must log `arrive ... transfer offered`, `payload ... to chernarus: N bytes`, `confirm ... transfer claimed`.
Arriving server must log:
`transfer for ...: A of B bytes, N items in the snapshot`   (A must equal B; if not, the body was cut off: send it)
`arrived ... X/N items restored (D on the ground, no room), health .., bleeding .., modifiers .., stomach .., cuffed ..`
X must equal N. D should be 0. Any `no room for ...` / `could not create ...` / `ARRIVAL FAILED` lines: send them.
Check after each trip: same spots in cargo, pistol still loaded + chambered (fire once), flashlight on, quickbar same,
still bleeding, food stage same. Screenshot inventory.
Then: zone again after the return (must count down), hub stopped -> countdown + "Travel failed".

## 4. Failed arrival (only if step 3 failed, or forced: rename the arrival points list in the TARGET's config.json
to "arrivalsX" so it has none, restart it, then travel there)
Expect on the target: `ARRIVAL FAILED for ...: <reason>`, `hub took the failure report`, `sending ... away: ...`,
and in the game: back to the main menu with a DriveLink dialog showing the reason. Hub: `FAILED arrival ...`.
`Hub\DriveLinkHub.exe transfers` (second window, hub keeps running) lists him with `FAILED 1x: <reason>`.
Then `Hub\DriveLinkHub.exe return <steamId>` and join the SOURCE server: he arrives there at its first arrival
point with all his gear (`arrived ... X/N`). Undo the config rename afterwards.

## 5. Late hub answer (can't be forced easily; just watch for it)
If a server logs `WARNING ... logged in before the hub answered`, it must be followed by `late hub answer for ...`.
For a trip that is `Your travel data arrived late...` in the main menu; rejoining after 30 s must finish the trip.

## Report
Pass/fail per step, every SCRIPT (E) line (both servers + client), the hub window text, inventory screenshots.

## Results 2026-10-18 (Derk, same PC, hub on SQLite; a176888)
Trips 18:06 -> 19:03, Chernarus <-> Livonia, every one `A of A bytes` and `N/N items restored, 0 on the ground`.
| Check | Result |
|---|---|
| Snapshot transport (/api/payload) | PASS, every trip (1886 .. 10711 bytes) |
| Worn clothing + weapons in slots | PASS |
| Items inside clothing, exact grid spot, rotated item | PASS after a176888 (CollectTree; before: cargo was never captured) |
| Attachment on attachment (optic on B95) | PASS |
| Quantities (eaten apple/mushroom, half bottle of water) | PASS |
| Loose ammo piles | PASS after a176888 (before: arrived with 0 rounds, combining crashed CombineItems) |
| MP5K: attached mag 8 rounds + chambered round, fires | PASS |
| Flashlight ON with battery (in hands and in cargo) | PASS |
| Quickbar slots 1-3 | PASS |
| Bleeding (1 source) | PASS |
| Stomach contents | PASS (stomach 1 on arrival) |
| Anti-dupe: join the old server -> redirected to owner | PASS |
| Hub stopped -> "Travel failed", gear kept | PASS (retry every 5 s while in zone -> cooldown added in a176888, not retested) |
| Cuffed travel, both ways (`cuffed true`) | PASS 19:10 + 19:11 + 19:13 |
| Rejoin the server you just left (cuffed run) | FAIL 19:06 "Player with same UID is already in game": the 1.5 s cleanup deleted the still-connected character mid-logout, vanilla never called DisconnectPlayer. Fixed (vanilla PlayerDisconnected path), PASS 19:11 + 19:13 |
Not tested yet: unconscious travel, failed-arrival kick dialog + `DriveLinkHub.exe return`, late hub answer,
zone cooldown after a failure.
