# DriveLink - plan v0 (decisions Derk 2026-10-12)

## Decisions
1. Name: DriveLink (repo d3rk99/DriveLink, private like PersistentBodies).
2. Backend: our own "DriveLink Hub" = HTTP API that every DayZ server talks to + PostgreSQL as the database.
   (Derk: "make it a database as well as the API that talks to each server".) Hub language: Go (single exe).
   Goal stays plug-and-play: hub installer sets up / checks Postgres, creates the DB + tables itself (migrations),
   writes hub config + a random server key, prints the lines to paste into each server's config.
3. Same PC first, but the address problem is solved from day 1:
   - Each server entry has a LAN/local address AND a public address (+ game port, password).
   - The client picks: vanilla CGame.GetHostAddress(out address, out port) (3_game/global/game.c:232, "Gets the
     server address (from client)") tells the player's game which address it used to join THIS server.
     If that address is local/private (127.x, 10.x, 192.168.x, 172.16-31.x, or equal to the server's LAN address)
     -> connect to the target's LAN address, else -> the target's public address. Hub/admin can override per server.
   - The server never has to guess the player's network.
4. Whole crew travels together (driver + passengers).
5. Cars first (CarScript). Boats/helis later.
6. Vanilla only: no CF, no Dabs. Own RestApi calls (CreateRestApi/GetRestContext, POST/GET).
7. Dead players: never travel. Handcuffed (IsRestrained, playerbase.c:2055): TRY to travel, restraint restored on
   arrival. Unconscious (IsUnconscious, playerbase.c:3669): try too, as passenger; if it proves unsafe -> stays.

## Parts (one repo)
- `Addons/DriveLink/` - the DayZ mod (one -mod on server + clients, server logic only on the dedicated server).
  3_Game: config + data classes + RestApi client.  4_World: zone watcher, player/car snapshot + rebuild,
  seat restore (StartCommand_Vehicle, human.c:1492).  5_Mission: MissionServer hooks (arrival in the ClientNew
  event), client redirect (disconnect -> g_Game.Connect(menu, ip, port, pw), game.c:251).
- `Hub/` - Go service: REST API + Postgres (migrations embedded), Windows service + Linux.
  Tables (first idea): servers, players(lock owner server, lease), transfers(token, state, payload), vehicles.
  Anti-dupe: one owner server per character/car; transfer token is single-use; source deletes only after the
  hub confirms; target spawns once per token.
- `Tools/` build/sign scripts (copied pattern from PersistentBodies), `Tests/`, `Research/`.

## First milestone (M1, "walk across")
On foot, same PC, 2 test servers (Chernarus 2302 + Livonia 2402): walk into an edge zone -> countdown -> arrive
on the other map with the same gear + health. No cars yet. Hub with Postgres.
M2 = cars (driver only), M3 = whole crew, M4 = handcuffed/unconscious, M5 = different machines / public IP test.

## Open (can wait)
- Arrival safety: protection time + what happens if the target server is full/down (bounce back).
- How the zones are drawn (config coords first; later an in-game admin tool).
