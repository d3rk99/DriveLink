# R0 - Research for Derk's new mod: seamless server/map travel ON FOOT AND BY CAR (2026-10-12)
Working name: "DriveLink" (Derk picks the real name)

## Derk's goals (2026-10-12)
- Walk OR drive into an area (usually a map edge) -> automatic transfer, NO terminal, no menu.
- Saves player state (health, stats, all items) AND the car (state, parts, fuel, cargo) and rebuilds both on the
  other server, player back in the seat.
- Own backend software: plug-and-play for server owners (the current UAPI/Mongo setup makes most owners give up).

## How the existing map-link mods work (read from source)
DaemonForge DayZ-MapLink (github.com/DaemonForge/DayZ-MapLink, AGPL-3.0, last commit 2026-04-18) + Universal API:
- Server -> client RPC MAPLINK_REDIRECTKICKED with {IP, Port, Password} (ServerData.c:71-78), client then
  DisconnectSessionForce + stores ConnectTo (3_Game/MapLink/DayZGame.c:84-90); when the client is back at the main
  menu, MissionGameplay calls HiveDoReconnect -> g_Game.Connect(menu, ip, port, password) (DayZGame.c:34-40,
  ModdedMissionGameplay.c:63). Engine: CGame.Connect(UIScriptedMenu parent, string IpAddress, int port, string
  password) (vanilla 3_game/global/game.c:251).  => client-side server switch is PROVEN possible.
- Before redirect: server saves the player to the DB, SetAllowDamage(false), kills+deletes the old character 350 ms
  later (PlayerBase.c:420-428, UFKillAndDeletePlayer :382).
- On the target: OnClientPrepareEvent loads the player from the DB (async), then the ClientNew event builds the
  character from the stored data (CreateWithIdentity), SelectPlayer, InvokeOnConnect, sets protection time
  (MissionServer.c:57-140). Wrong server / missing arrival point -> redirect back.
- Travel trigger = a "Hive_Terminal" object + a menu (departure point -> arrival point, optional currency cost).
- Players only, NO vehicles. Item blacklist per server.
- Backend = Universal API "DayZWebService": Node.js + MongoDB + HTTPS cert (+ Discord, TensorFlow toxicity,
  Libre Translate, LetsEncrypt...) (DayZWebService/package.json, sample-config.json). AGPL-3.0.
- Script HTTP: RestApi via CreateRestApi()/GetRestContext(url), ctx.SetHeader(token), ctx.POST(callback, "", json),
  POST_now for sync (UniversalApi DiscordRest.c:3-44).
- Pain points: the popular repack "MapLink Hive" (Workshop 3478983598, 15k subs) was REMOVED by Steam and broke with
  DayZ 1.29; its servers got delisted from DZSA because the mod id vanished (comments Jun 2026). Needs CF + UAPI
  installer + Mongo + cert + video guides; Expansion spawn-select conflict ("EnableSpawnSelection": 0); other mods'
  data needs extra compat PBOs.

## Engine facts for cars (vanilla 1.29 scripts, BohemiaInteractive/DayZ-Script-Diff)
- Put a player into a seat from script: StartCommand_Vehicle(Transport, crewIndex, seatAnimType, fromUnconscious)
  (3_game/human.c:1492), used by ActionGetInTransport (actiongetintransport.c:91) and by the unconscious-in-car
  restore (dayzplayerimplement.c:2376); a DEVELOPER-only helper does exactly "put me in seat 0" (playerbase.c:3292-
  3307). => seating a freshly spawned player is possible, needs an in-game test (server+client sync).
- Not checked yet (R1): car fluids/engine/parts API, how the car is held "reserved" while the driver loads in,
  multiple passengers.

## Licence note
MapLink + UAPI are AGPL-3.0: we can READ them to learn, but copying their code makes our mod AGPL too. Plan: write
our own code, cite only vanilla engine functions.

## Proposed design (for Derk to approve)
Mod (one -mod, like PersistentBodies):
- Edge ZONES from a config file (polygon or circle + target server + arrival point). Server polls player/vehicle
  positions (no terminal). Short countdown on screen ("Leaving to Livonia in 5... drive back to cancel").
- On go: freeze car + crew, snapshot player(s) (reuse PersistentBodies' item-tree snapshot) + car (type, parts,
  fluids, health, cargo), POST to backend as ONE transfer with a one-time token, then redirect every crew member.
- Target server: arrivals wait in the backend; first crew member to load spawns the car at the arrival point,
  each crew member is seated in their seat; car is protected until the driver arrives (timeout -> car parks).
Backend "hub" (our own software):
- ONE file (Go or .NET single exe), embedded SQLite, no Mongo, no Node, no cert setup on a LAN.
- First start: writes hub.json + a random key, prints "paste this into each server's config". Optional web page.
- Anti-dupe: a character/car is LOCKED to one server at a time (lease); transfer = idempotent token, source deletes
  only after the hub confirms; target only spawns once per token.
- Windows service + Linux binary.

## Open questions for Derk
1. Name? 2. Hub language: Go (tiny single exe) or .NET? 3. Same PC only first, or different machines/internet from
day 1 (needs HTTPS)? 4. Passengers: whole crew travels together? 5. What travels: any vehicle, or cars only first
(boats/helis later)? 6. Do we need CF/Dabs, or vanilla only (fewer dependencies = easier)? 7. Dead/unconscious/
restrained players can't travel?
