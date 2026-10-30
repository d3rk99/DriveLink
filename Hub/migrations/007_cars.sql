-- Vehicle economy (2026-10-29, Research/vehicle-economy.md): every car that has travelled has a DriveLink id and a
-- home server. While it is away from home (on another server or on the road) it keeps its place on the home server,
-- whose mod then vetoes Central Economy respawns of that vehicle event. Servers report the travelled cars they hold
-- (POST /api/cars); a car its server stops reporting is counted gone after a few reports (ruined, despawned, deleted).
CREATE TABLE IF NOT EXISTS cars (
  dlid           TEXT PRIMARY KEY,          -- made by the mod the first time the car leaves a server
  type           TEXT NOT NULL,             -- class name, e.g. OffroadHatchback_Blue
  home_server    TEXT NOT NULL,             -- the server whose Central Economy spawned it
  current_server TEXT,                      -- NULL = on the road
  state          TEXT NOT NULL,             -- 'on_map' | 'in_transit' | 'gone'
  missing        INTEGER NOT NULL DEFAULT 0, -- reports in a row from current_server without this car
  updated_at     TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX IF NOT EXISTS cars_home ON cars (home_server) WHERE state <> 'gone';
CREATE INDEX IF NOT EXISTS cars_current ON cars (current_server) WHERE state = 'on_map';
