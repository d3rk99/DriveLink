# M1 - walk across (on foot), Chernarus 2302 <-> Livonia 2402, same PC

Pull main. NOTHING here is tested in game yet: the scripts are written against the vanilla 1.29 sources but have
never been compiled by DayZ. Expect compile errors on the first boot; send me the exact SCRIPT (E) lines.

## 1. Hub
1. `powershell -ExecutionPolicy Bypass -File Tools\build-hub.ps1` -> Hub\DriveLinkHub.exe
2. Run `Hub\DriveLinkHub.exe` once: it writes Hub\hub.json, prints a key and stops.
3. Edit Hub\hub.json: database = `postgres://drivelink:<password from DriveLink-secrets.txt>@localhost:5432/drivelink`.
   Servers: chernarus 2302 + livonia 2402, lanHost 192.168.1.133, publicHost 136.46.143.240 (already the defaults
   except publicHost).
4. Run it again (keep the window open). Expect `database: applied 001_init.sql` and `listening on 0.0.0.0:7290`.
   Check http://localhost:7290/health -> {"ok":true}.

## 2. Mod
1. `powershell -ExecutionPolicy Bypass -File Tools\build-mod.ps1`
2. Both servers' start scripts: add `;@DriveLink` to -mod (server 1 AND server 2). Client: load @DriveLink too.
3. Start both servers once: each writes `<profiles>\DriveLink\config.json` (an example) and logs
   `[DriveLink] wrote example ...`. Stop them.
4. Edit server 1 (Chernarus) config.json: hubUrl `http://127.0.0.1:7290`, hubKey = the key, serverId `chernarus`.
   Zone: pick a spot you can walk to quickly (VPP: copy your position), radius 30, to `livonia`, arrival `west_road`.
   Arrivals: one point `east_road` somewhere safe on Chernarus.
5. Server 2 (Livonia) config.json: same hubUrl/hubKey, serverId `livonia`. Zone back: to `chernarus`, arrival
   `east_road`. Arrivals: `west_road` = a safe spot on Livonia.
6. Start both servers + the hub. Script logs must show `[DriveLink] v0.1 server 'chernarus' ... key set true`
   (and livonia). Report every SCRIPT (E) line.

## 3. Travel
1. Join Chernarus (via 192.168.1.133:2302). Hub log: `arrive <id> on chernarus: normal login, owner set`.
2. Note your gear (screenshot of inventory), health, water/food. Put an apple, a loaded mag and a full canteen on you.
3. Walk into the zone. You should get "Leaving in 5 s". Step out -> "Travel cancelled". Step in and wait.
4. Expect: you get disconnected, the game connects to Livonia by itself, you spawn at west_road with the SAME
   character (face), gear (incl. mag ammo, canteen water), health, water/food. Screenshot inventory.
   Logs to send: Chernarus `[DriveLink] ... leaving to livonia`, `handed over`, hub `depart ...` + `arrive ... transfer
   claimed`, Livonia `[DriveLink] ... arrived from chernarus ...`, client script log `[DriveLink] redirect to Livonia
   ... joined via ... -> using ...`.
5. Anti-dupe: quit the game, join CHERNARUS directly. Expect: "Your character is on Livonia" and you get sent
   there; NO second copy of your character on Chernarus. Hub: `owned by livonia, redirect`.
6. Walk back through the Livonia zone -> arrive on Chernarus at east_road with everything.
7. Hub down: stop the hub, walk into a zone -> "Travel failed: hub not reachable", you keep everything, can still play.
8. Handcuffed (if you can test with a 2nd account or VPP): cuffed player walked into the zone arrives still cuffed.

## Report
Pass/fail per step, all SCRIPT (E) lines (server + client logs), the hub window text, and the inventory screenshots.
