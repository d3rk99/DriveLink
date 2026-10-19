# Task: set up a DayZ Livonia dedicated server for a two-machine DriveLink playtest

You are setting up ONE DayZ dedicated server (map: Livonia / `enoch`) on **Machine B**, a Windows PC or VPS.
It joins an existing cross-server travel setup run by Derk on **Machine A**. Do not change anything on Machine A.

## What DriveLink is (so you know what matters)
DriveLink is a DayZ server mod. Players walk or drive into a zone on one server and arrive on another server with
their gear (and car). Every DayZ server talks over HTTP to one **DriveLink Hub** program running on Machine A.
This test proves a server on a **second machine** can join the hub and take part in travel.

## Machine A (already running, for reference only)
- DriveLink Hub web app: `http://192.168.1.133:7290/admin` (LAN). Derk's public IP: `136.46.143.240`.
- Hub API port: TCP 7290 (HTTP). TCP 7291 is HTTPS but is not used in this test.
- Machine A also runs Chernarus (port 2302) and a local Livonia (port 2402). Your server is a separate, new Livonia.

## Requirements for the server on Machine B
1. **DayZ Server**: Steam app 223350, **stable branch, version 1.30**. It must be exactly the same build as the
   players' DayZ client (`forceSameBuild = 1`). Do not use experimental.
2. **Mods, in this exact load order**:
   `-mod=@CF;@DabsFramework;@VPPAdminTools;@PersistentBodies;@DriveLink`
   - @CF (Community Framework), @DabsFramework and @VPPAdminTools come from the Steam Workshop. Look up and
     confirm each Workshop ID before downloading. Do not guess.
   - @PersistentBodies is Derk's published Workshop mod. Ask Derk for its Workshop ID.
   - **@DriveLink is NOT on the Workshop.** Derk provides two things: the `@DriveLink` folder
     (`Addons/DriveLink.pbo` + `Addons/DriveLink.pbo.DriveLinkLocalTest.bisign`) and the key
     `DriveLinkLocalTest.bikey`.
   - Copy every mod's `.bikey` (including `DriveLinkLocalTest.bikey`) into the server's `keys\` folder.
     Keep `verifySignatures = 2`.
3. **serverDZ.cfg** (only the settings that matter; leave the rest at their defaults):
   ```
   hostname = "DriveLink Test - Livonia (Machine B)";
   password = "";
   passwordAdmin = "<pick one, tell Derk privately>";
   maxPlayers = 10;
   verifySignatures = 2;
   forceSameBuild = 1;
   BattlEye = 0;              // matches the test servers on Machine A
   instanceId = 1;
   steamQueryPort = 27016;
   class Missions { class DayZ { template = "dayzOffline.enoch"; }; };
   ```
4. **Launch command** (from the DayZServer folder; use your own paths). Keep a profiles folder you can find,
   because the logs and the DriveLink config live there:
   ```
   DayZServer_x64.exe -config=serverDZ.cfg -profiles=<PROFILES DIR> -port=2302 -steamQueryPort=27016 -dologs -adminlog -netlog -freezecheck "-mod=@CF;@DabsFramework;@VPPAdminTools;@PersistentBodies;@DriveLink"
   ```
5. **Windows Firewall on Machine B** (administrator PowerShell):
   ```
   New-NetFirewallRule -DisplayName 'DriveLink DayZ Livonia B' -Direction Inbound -Action Allow -Protocol UDP -LocalPort '2302-2305','27016'
   ```
6. **Network path to the hub**
   - **Machine B on the same LAN as Machine A:** the hub URL is `http://192.168.1.133:7290`.
   - **Machine B on another network (e.g. a VPS):** the hub URL is `http://136.46.143.240:7290`. Tell Derk he must
     forward **TCP 7290** on his router to 192.168.1.133.
   - If Machine B is behind its own router, forward **UDP 2302-2305 and 27016** to Machine B so players can join.
   - Check from Machine B before going on:
     `Invoke-RestMethod <hub URL>/health` must return `ok = True`.

## Connect it to the DriveLink Hub
1. Start the server once with the mods. DriveLink writes `<PROFILES DIR>\DriveLink\config.json`. Stop the server.
2. In that file set **only these two values**. Leave `"serverId"` as `""` and do not touch the other fields:
   ```
   "hubUrl": "<hub URL from step 6>",
   "hubKey": "<the server key>",
   ```
   Get the server key from Derk. It is shown on the hub's web app, Setup page. Never invent it, never post it publicly.
3. Start the server. Within ~15 s its script log (`<PROFILES DIR>\script_*.log`) must show:
   `[DriveLink] the hub knows the key but not this server yet: approve it on the hub's Servers page`
4. Tell Derk: "Machine B Livonia is asking to join". He approves it on the hub's **Servers** page as a **new server**
   (suggested id `livonia-b`, name "Livonia B"). Do NOT link it to the existing `livonia`.
5. Within ~15 s of approval the log must show:
   `[DriveLink] the hub knows this server as 'livonia-b' ... saved in config.json`
   and then a `[DriveLink] layout rev ...` line.

## Do not
- Do not enable BattlEye, change the mod order, or let the Workshop auto-update mods during the test.
- Do not edit `serverId`, zones or arrival points by hand. Derk draws them in the hub's map editor.
- Do not expose the hub key or `passwordAdmin` in public logs, chats or screenshots.

## Report back to Derk
- Machine B's LAN IP, its public IP (if different), and whether it is on Derk's LAN or another network.
- The DayZ server version/build and the exact `-mod=` line used.
- The `/health` check result.
- Every `[DriveLink]` line and every `SCRIPT (E)` line from the newest `script_*.log`.
- That the server shows in the DayZ in-game server browser (or the direct-connect IP:port that works).
