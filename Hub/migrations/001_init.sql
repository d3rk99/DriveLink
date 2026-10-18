-- DriveLink Hub schema v1 (SQLite, one file next to the hub: drivelink.db).
-- A character is owned by exactly ONE server at a time (owner_server). A transfer moves ownership:
-- source server snapshots the player -> /api/depart (token, payload) -> owner becomes NULL = "in transit" ->
-- target server builds the character and calls /api/confirm (single use) -> owner = target.
-- Times are UTC text (CURRENT_TIMESTAMP, 'YYYY-MM-DD HH:MM:SS').

CREATE TABLE IF NOT EXISTS servers (
  id           TEXT PRIMARY KEY,          -- short id from hub.json, e.g. "chernarus"
  name         TEXT NOT NULL,
  last_seen    TEXT
);

CREATE TABLE IF NOT EXISTS characters (
  steam_id     TEXT PRIMARY KEY,          -- PlayerIdentity.GetPlainId()
  owner_server TEXT,                      -- server that owns the live character, NULL = in transit / none
  updated_at   TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS transfers (
  token        TEXT PRIMARY KEY,          -- random, made by the hub, single use
  steam_id     TEXT NOT NULL,
  from_server  TEXT NOT NULL,
  to_server    TEXT NOT NULL,
  arrival      TEXT NOT NULL,             -- arrival point name on the target
  payload_text TEXT NOT NULL,             -- player snapshot EXACTLY as the source server wrote it (never re-encoded)
  state        TEXT NOT NULL,             -- 'pending' -> 'claimed' | 'superseded' | 'expired' | 'returned'
  created_at   TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
  claimed_at   TEXT
);
CREATE INDEX IF NOT EXISTS transfers_pending ON transfers (steam_id) WHERE state = 'pending';
