# M1b - full state travel (v0.2 snapshot, b6a23e5)

## 0. Clear the stuck v1 trip (its snapshot is the OLD format and can't be read by v0.2)
psql -U drivelink -d drivelink -h localhost -c "UPDATE transfers SET state='expired' WHERE state='pending'" -c "UPDATE characters SET owner_server='livonia' WHERE owner_server IS NULL"
(You are standing on Livonia with the old character; this makes Livonia his owner again.)

## 1. Build + start
Pull, Tools\build-mod.ps1 (+ re-sign, copy to client). Hub unchanged. Remove the old Profiles\DriveLink\selftest folders.
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
