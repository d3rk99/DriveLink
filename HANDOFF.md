# Handoff 2026-10-19: map editor + live layout (UNTESTED in game)

## What is new
- Hub keeps each server's leaving zones + arrival points (SQLite `layouts` table, migration 003).
  - `/api/hello`: the server sends its config.json zones/arrivals; the hub stores them only if it has none yet (`layout::import_if_missing`) and answers with the hub's layout.
  - `/api/layout` (`{serverId,key,rev}`): the mod polls every 30 s and applies a newer rev (replaces config zones/arrivals, saves config.json, logs each one). No restart.
  - The JSON key is `world`, not `map` (`map` is a type name in Enforce: "Variable name 'map' already used as type name").
- Map editor: `http://localhost:7290/admin` (no password from the hub PC; other PCs use hub.json `adminPassword`).
  Leaflet page embedded in the exe (Hub/web). Draw a zone (click, drag the radius), an arrival (click, drag the facing). Auto-saves.
- Map tiles are built from the owner's own game files (worlds_<map>_data.pbo + DayZ Tools: BankRev, CfgConvert, ImageToPAA) into Hub/maps/ (gitignored).
  Use the editor's Build button or `DriveLinkHub.exe map <name>`. No images from third-party sites (no licence).

## State when handed over
- Hub builds (`Tools/build-hub.ps1`). `Tools/hub-api-check.sh` passed before the `map`→`world` rename.
- 2026-10-19 01:38: mod rebuilt + signed, both servers compile, apply the hub layout at hello, and Chernarus picked up
  an editor save (rev 20) 30 s later with no restart. Not yet tried: a trip through an editor-made zone.
- The hub DB holds TEST layouts made while trying the editor (chernarus rev 19, livonia rev 5). Ask Derk whether to clear them:
  `sqlite3 Hub/drivelink.db "DELETE FROM layouts"`. Then the servers re-import their real config.json zones on the next hello.
  Backups of the real configs: TestServer*/Profiles/DriveLink/config.json.bak-2026-10-18.

## Next steps
1. With DayZ closed: build + sign the mod (Tools/build-mod.ps1, DSSignFile), start the hub (`cd Hub; ./DriveLinkHub.exe`), start both servers.
2. Check each server log for the layout lines after hello. Edit a zone in /admin, then within 30 s the log should show "layout rev N from the hub".
3. Walk/drive a trip through an editor-made zone.
4. Later: the crew waiting room (blackout + "n/m connected" UI, then spawn the car, seat everyone, start the engine, lift the blackout); M3 crew test with a second player; passenger-seat refusal; HTTPS trip test; `DriveLinkHub.exe install` as a service.
