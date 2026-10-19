# Task: set up a DayZ Livonia dedicated server for a two-machine DriveLink playtest (remote machine)

You are setting up ONE DayZ dedicated server (map: Livonia, terrain name `enoch`) on **Machine B**, a Windows PC or
VPS on a **different network** from Derk's. It joins an existing cross-server travel setup that Derk runs on
**Machine A**. Then you help Derk run the test. Do not change anything on Machine A.

---------------------------------------------------------------------------------------------------------------------

## 1. What DriveLink is (what matters for your choices)
DriveLink is a DayZ server mod. A player walks or drives into a "leaving zone" on one server. The server saves the
player's whole character (gear, health, and the car they drive) and sends it to the **DriveLink Hub**. The player's
game then reconnects to the target server, which rebuilds them there.
- Every DayZ server talks to the hub over **HTTP, outgoing from the DayZ server to TCP 7290**. The hub never connects
  to your server.
- Players' game clients connect directly to your server's **game port (UDP)**, like any DayZ server.
- Zones and arrival points are NOT set on your server. Derk draws them in the hub's web map editor, and your server
  downloads them by itself every 30 s.
- This test proves that a server on another network can join the hub and that players travel both ways between
  Chernarus (Machine A) and Livonia (Machine B), on foot and by car.

## 2. Machine A (Derk's side, for reference)
| What | Value |
|---|---|
| Hub URL for you | `http://136.46.143.240:7290` (Derk's public IP; he forwards TCP 7290 on his router) |
| Hub health check | `http://136.46.143.240:7290/health` returns `{"ok":true}` |
| Hub web app (Derk only) | Servers page (approves your server), Map page (zones), Setup page (server key) |
| Other game server | "Chernarus", id `chernarus`, at Derk's public IP, port 2302 |
| DayZ version | 1.30 stable |

## 3. Install
1. **DayZ Server**, Steam app **223350**, **stable branch, version 1.30**. SteamCMD needs a Steam account that owns
   DayZ (anonymous login does not work for 223350):
   `steamcmd +login <account> +force_install_dir <DIR> +app_update 223350 validate +quit`
   It must be the same build as the players' clients (`forceSameBuild = 1`). Do not use experimental.
2. **Mods.** Workshop app id 221100. Download with
   `steamcmd +login <account> +workshop_download_item 221100 <id> validate +quit`,
   copy each one into the server folder as `@Name`, and copy its `keys\*.bikey` into the server's `keys\` folder.

   | Load order | Folder | Workshop ID | Source |
   |---|---|---|---|
   | 1 | `@CF` | 1559212036 | Workshop (Community Framework) |
   | 2 | `@DabsFramework` | 2545327648 | Workshop |
   | 3 | `@VPPAdminTools` | 1828439124 | Workshop |
   | 4 | `@PersistentBodies` | 3818752519 | Workshop (Derk's mod) |
   | 5 | `@DriveLink` | none | **From Derk**: folder `@DriveLink\Addons\` (`DriveLink.pbo` + `DriveLink.pbo.DriveLinkLocalTest.bisign`) and key `DriveLinkLocalTest.bikey` |

   - Put `DriveLinkLocalTest.bikey` in `keys\` as well.
   - Folder names and order must be exactly as above. The players use the same five mods.
   - Turn OFF automatic Workshop updates for the test: every mod must be the same version as on Machine A.
3. **serverDZ.cfg**: only the settings that matter, leave the rest at their defaults.
   ```
   hostname = "DriveLink Test - Livonia B";
   password = "";
   passwordAdmin = "<pick one; give it to Derk privately>";
   maxPlayers = 10;
   verifySignatures = 2;
   forceSameBuild = 1;
   BattlEye = 0;                 // same as Derk's test servers
   instanceId = 1;
   steamQueryPort = 27016;      // must be HERE: DayZ ignores -steamQueryPort on the command line
   disableVoN = 0;
   class Missions { class DayZ { template = "dayzOffline.enoch"; }; };
   ```
4. **Start command** (from the DayZServer folder). Use a profiles folder you can find: logs and the DriveLink config
   live there.
   ```
   DayZServer_x64.exe -config=serverDZ.cfg -profiles=<PROFILES DIR> -port=2302 -dologs -adminlog -netlog -freezecheck "-mod=@CF;@DabsFramework;@VPPAdminTools;@PersistentBodies;@DriveLink"
   ```

## 4. Network (Machine B is outside Derk's network)
- **Inbound to Machine B** (players join), Windows Firewall in an administrator PowerShell:
  `New-NetFirewallRule -DisplayName 'DriveLink DayZ Livonia B' -Direction Inbound -Action Allow -Protocol UDP -LocalPort '2302-2305','27016'`
  - If Machine B is behind a router or NAT, forward **UDP 2302-2305 and 27016** to it.
  - On a VPS, also open these ports in the provider's firewall or security group.
- **Outbound from Machine B to the hub**: TCP 7290 to 136.46.143.240. This is usually allowed already.
- **Check before going on**, on Machine B in PowerShell:
  `Invoke-RestMethod http://136.46.143.240:7290/health`
  It must print `ok : True`. If it times out, Derk's port forward for TCP 7290 is not working yet: tell him and stop here.
- Note Machine B's **public IP**. Derk enters it as the server's public address, and players are sent there when
  they travel.

## 5. Connect the server to the hub
1. Start the server once with all the mods. Check the newest `<PROFILES DIR>\script_*.log` for
   `[DriveLink] v0.3 server ''` (the mod loaded) and `wrote $profile:DriveLink/config.json`. Stop the server.
2. Open `<PROFILES DIR>\DriveLink\config.json` and set **only** these two values:
   ```
   "hubUrl": "http://136.46.143.240:7290",
   "hubKey": "<server key from Derk>",
   ```
   - Leave `"serverId": ""`. The hub fills it in.
   - Leave `joinId`, `parkingSpot`, `crewWaitSeconds` and the zone lists as they are.
   - Get the key from Derk (hub web app, Setup page). Never guess it. Never paste it anywhere public, including logs
     you share: replace it with `***`.
3. Start the server. Within ~15 s the script log must show:
   `[DriveLink] the hub knows the key but not this server yet: approve it on the hub's Servers page`
   Tell Derk: **"Livonia B is asking to join."**
4. Derk approves it on the Servers page as a NEW server (id `livonia-b`, name "Livonia B", public address = Machine B's
   public IP, port 2302).
   - Within ~15 s the log must show
     `[DriveLink] the hub knows this server as 'livonia-b' (was ''), saved in config.json`, then
     `[DriveLink] layout rev N from the hub`.
   - Derk then draws the leaving zone and arrival point on the Map page. Your log shows a new `layout rev` line with
     the zone and arrival names within 30 s.
5. **One setting to agree with Derk.** `parkingSpot` in config.json is where a car that drove away waits until it
   can be deleted. Players must not reach it. On Livonia use `[12300, 0, 12300]` unless Derk says otherwise. If you
   change it, stop the server first.

## 6. Running the test with Derk
- Keep the server running. Only restart it when no player is on it: the script log shows
  `[Logout]: Player ... finished` for everyone. A server killed while players are on rolls back their gear.
- Derk and testers join Chernarus, drive into the zone and arrive on Livonia B, then go back.
- On a successful arrival your script log shows, in order:
  1. `hub says transfer for <steamId>`
  2. `transfer for <steamId>: N of N bytes`
  3. `<steamId> arrived from chernarus at <arrival> ..., X/X items restored`. With a car: `waiting room 1/1`,
     then `waiting room of ...: OffroadHatchback built (29/29 ...)`.
  4. `seated in his ...`
  5. `black screen lifted, engine start by the driver's client`
- On departure from Livonia B: `leaving to chernarus`, `parked: ... -> <parkingSpot>`, then
  `removed here (seats empty after N s)`.
- Watch for, and report at once: any `SCRIPT (E)` line, `NOT sent`, `ARRIVAL FAILED`, `hub hello FAILED`,
  `hub ... failed, error N`, or the server crashing or freezing.

## 7. Do not
- Do not enable BattlEye, change the mod order, or update mods during the test.
- Do not edit `serverId`, zones or arrival points by hand: they come from the hub.
- Do not wipe the server's storage folder during the test. Characters and cars are saved there.
- Do not share the hub key or `passwordAdmin` in public.

## 8. Report back to Derk
1. Machine B's public IP, its game port and query port, and whether it is a VPS or behind a home router.
2. The DayZ server build, the exact `-mod=` line, and the Workshop IDs and versions you installed.
3. The `/health` check result from Machine B.
4. The server shows in the in-game server browser, or the direct-connect `IP:port` that works.
5. All `[DriveLink]` lines and all `SCRIPT (E)` lines from the newest `script_*.log`, with the key masked.
6. During the test: the log lines from section 6 for each trip, with their times.
