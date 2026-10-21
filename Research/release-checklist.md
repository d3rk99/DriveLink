# DriveLink - release checklist (public testing) - 2026-10-21

Planes (LM's Planes) are on hold: the code stays, dormant, until that mod is stable.

## 1. Blockers (can't publish without these)

| # | What | Where | Notes |
|---|---|---|---|
| B1 | ~~MiniDayZ art~~ **resolved 2026-10-21: Bohemia gave the OK** to use it | `Tools/build-anim-assets.py` MINIDAYZ list, `build-anim-characters.py`; built locally | Keep Bohemia's written OK on file; credit "MiniDayZ art (c) Bohemia Interactive, used with permission" on the Workshop page and in the README. The textures are still built from the local archive (gitignored); the release PBO packs them. |
| B2 | **Release signing key** | `@DriveLink` is signed with `DriveLinkLocalTest` | Make a release key pair; ship `keys/DriveLink.bikey` inside `@DriveLink` (it has no keys folder now). Keep the .biprivatekey out of every repo / zip. |
| B3 | **Hub distribution** | `Hub/` (Rust, Windows exe) | A release zip/installer (exe + README), maybe a Linux build. `DriveLinkHub.exe install` (Windows service) exists - document it. |
| B4 | **Server-owner docs** | `README.md` is one line | Install (mod on every server + every client, hub on one machine), `config.json` (hubUrl, hubKey), approving servers, firewall, drawing zones/arrivals, travel settings + presets, starting server, troubleshooting. |
| B5 | **Workshop page** | `WorkshopArt/` has a preview | Title, description, `mod.cpp` (name, picture, logo, tooltip, author, version), changelog, required mods (CF? none?), "needs the DriveLink Hub" link. |

## 2. Debug / test-only code to remove or gate

Done 2026-10-21: D1, D2 (dev builds only: `Tools/build-mod.ps1 -Dev` adds the DL_DEV define), D3 (config.json
`"log": "quiet" | "normal" | "verbose"`, DL_Log), D4 (hub.json `"experimentalPlanes": true` shows the plane tools),
D6 (test servers back to CF, Dabs, VPP, PersistentBodies, DriveLink).

| # | What | Where | Proposal |
|---|---|---|---|
| D1 | **Test car on every login** (seat spike) + **car rebuild zone** (spike 1) | `DL_MissionServer.c` 176-330, `DL_Debug` in `DL_Config.c`; switched on by `$profile:DriveLink/debug.json` | Only runs when debug.json exists, but it's live code a server owner can trip over. Move behind `#ifdef DL_DEV` (a define only our test build sets) or delete. Delete `TestServer/Profiles/DriveLink/debug.json` when testing the release build. |
| D2 | **F9 demo film** | `DL_ClientHud.c` 155-166, `DL_FilmPlayer.Demo()` | Any player can press F9 on a live server. Gate behind `DL_DEV`, or admins only. |
| D3 | **Log volume** | ~all `Print(DL_Const.LOG_TAG ...)`, also on clients (seat juncture, film) | Add a log level in config.json (`logLevel`: normal / verbose); keep travel/arrival/error lines, drop per-frame and spike chatter. |
| D4 | **Plane UI** | hub Map page: "+ Plane zone", zone "Planes in flight only", arrival "Plane entry" | Hide while planes are on hold (experimental switch in hub.json). Mod-side plane code is inert without LM's mod. |
| D5 | Old spike names / comments | `DL_FilmSpike.c` (class name, header says "debug feature: remove or gate") | Rename to DL_FilmWidgets, fix the header. Cosmetic. |
| D6 | Test servers' extra mods | start scripts load @PwwNVehFix, @LMsPlanes | Drop them from the test servers while planes are on hold (or keep for later). |

## 3. Safety / robustness

| # | What | Proposal |
|---|---|---|
| S1 | Hub web app needs no password from localhost | Fine on a PC; behind a reverse proxy every request looks local. Document, or add `requirePasswordAlways`. |
| S2 | First-run admin password is written to hub.log | Show it once on the console only, or make the owner set one on first visit. |
| S3 | ~~Version handshake~~ done 2026-10-21: protocol 1 (mod DL_Const.PROTOCOL, hub api::PROTOCOL) on every call; a mismatch turns travel off on that server with a message saying which side to update, and the Servers page shows each server's DriveLink version | Raise PROTOCOL on both sides with every change the other side would misread. |
| S4 | Hub database backups | drivelink.db holds characters in transit: a nightly copy + "Export" on the Setup page. |
| S5 | HTTPS with a self-signed cert from DayZ's RestApi | Verify a remote server can talk to the hub over HTTPS, or document HTTP + firewall. |
| S6 | Defaults for a fresh server | No zones = nothing happens (good). Pick the default travel preset (Medium?) and starting server "off". |

## 4. Not tested in game yet (one test pass before release)

- Crew of 2+: crewWaitSeconds timeout, a passenger arriving before the driver, passenger release.
- Unconscious traveller; late hub answer; hub down mid-trip (car).
- Starting server: road death on Livonia -> death screen -> respawn redirected to Chernarus fresh; in-game death on Livonia; death on Chernarus.
- Long trip film (sent in parts); closed cans with / without an opener; road finds (gas station scene with convoy refill, well pump, food).
- Boat: out of gas -> swim, drowning, swimmers ashore; boat arrival point on water.
- Convoys with mixed cars + bikes; groups on foot; skip vote with 2+ players.
- Remote Linux server (livonia2): needs this build zipped (dist/) and the 1.30.164200 server update.

## 5. Nice to have for public testing

- Localisation of player-facing texts (English only now, ~10 notification texts + film captions).
- A player-facing "where am I going" HUD line during the countdown (exists as a notification).
- In-game admin command to see a player's trip (Players page covers it).
