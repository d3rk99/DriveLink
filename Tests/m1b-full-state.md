# M1b - full state travel (v0.2 snapshot, b6a23e5)

## 0. Hub moved to SQLite (2026-10-18)
Rebuild the hub (Tools\build-hub.ps1) and start it: it drops the old postgres line from hub.json, creates
Hub\drivelink.db and starts EMPTY - the stuck trip from 2026-10-13 is gone with the old database, and your next
login on any server is a normal login that makes that server the owner. PostgreSQL is no longer needed.
Debug: `Hub\DriveLinkHub.exe dump` writes the newest pending snapshot to Hub\last-payload.json.

## 1. Build + start
Pull, Tools\build-mod.ps1 (+ re-sign, copy to client). Remove the old Profiles\DriveLink\selftest folders.
Restart both servers. No SCRIPT (E) lines expected; send them all if there are any.

## 2. Gear to carry (as many as possible, VPP spawn)
- clothes with cargo: shirt, pants, jacket or vest, backpack (items placed in specific spots, one rotated)
- pistol in hands WITH a magazine attached and a round in the chamber; a second full mag in cargo; a loose ammo pile
- rifle on the back with an optic (attachment on an attachment)
- canteen with water, a cooked or burned food item, a flashlight that is switched ON with a battery
- something on the quickbar (slots 1-3)
- be bleeding (cut yourself) and have eaten something just before
Screenshot inventory + note quickbar.

## 3. Trip Livonia -> Chernarus, then Chernarus -> Livonia
Arriving server must log:
`transfer for ...: N items in the snapshot`
`arrived ... X/N items restored (D on the ground, no room), health .., bleeding .., modifiers .., stomach .., cuffed ..`
X must equal N. D should be 0. Any `no room for ...` / `could not create ...` lines: send them.
Check after each trip: same spots in cargo, pistol still loaded + chambered (fire once), flashlight on, quickbar same,
still bleeding, food stage same. Screenshot inventory.
Then: zone again after the return (must count down), step 7 (hub stopped -> countdown + "Travel failed").
