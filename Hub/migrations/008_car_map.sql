-- Cars on the hub's Map page (2026-10-29): where each travelled car was at its server's last report, and its trips.
ALTER TABLE cars ADD COLUMN x REAL;            -- world position (metres), NULL until a server reports it
ALTER TABLE cars ADD COLUMN z REAL;
ALTER TABLE cars ADD COLUMN health REAL;       -- body health 0..1
ALTER TABLE cars ADD COLUMN fuel REAL;         -- 0..1, NULL for vehicles without a fuel tank
ALTER TABLE cars ADD COLUMN seen_at TEXT;      -- when its server last reported it
CREATE TABLE IF NOT EXISTS car_trips (
  id          INTEGER PRIMARY KEY,
  dlid        TEXT NOT NULL,
  from_server TEXT NOT NULL,
  to_server   TEXT,                            -- NULL = still on the road
  left_at     TEXT,                            -- NULL when the target reported the car before the source said it left
  arrived_at  TEXT
);
CREATE INDEX IF NOT EXISTS car_trips_dlid ON car_trips (dlid, id);
