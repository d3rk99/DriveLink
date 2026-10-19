-- Map editor (2026-10-18): each server's leaving zones and arrival points live in the hub. The JSON is exactly the
-- mod's own shape (DL_Zone / DL_ArrivalPoint arrays), so the server takes it as it is. rev goes up on every edit;
-- servers poll /api/layout with the rev they have and only reload when it changed. map = the world for the editor
-- (chernarusplus, enoch, ...).
CREATE TABLE IF NOT EXISTS layouts (
  server_id  TEXT PRIMARY KEY,
  zones      TEXT NOT NULL,
  arrivals   TEXT NOT NULL,
  map        TEXT NOT NULL DEFAULT '',
  rev        INTEGER NOT NULL,
  updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
