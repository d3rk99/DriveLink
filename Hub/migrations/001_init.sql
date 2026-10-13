-- DriveLink Hub schema v1 (M1: players on foot).
-- A character is owned by exactly ONE server at a time (owner_server). A transfer moves ownership:
-- source server snapshots the player -> POST /transfer (token, payload) -> owner becomes "in transit" ->
-- target server claims it on login (single use) -> owner = target. Nothing is ever spawned twice per token.
CREATE TABLE IF NOT EXISTS schema_version (version INT PRIMARY KEY);

CREATE TABLE IF NOT EXISTS servers (
  id           TEXT PRIMARY KEY,          -- short id from hub.json, e.g. "chernarus"
  name         TEXT NOT NULL,
  last_seen    TIMESTAMPTZ
);

CREATE TABLE IF NOT EXISTS characters (
  steam_id     TEXT PRIMARY KEY,          -- PlayerIdentity.GetPlainId()
  owner_server TEXT,                      -- server that owns the live character, NULL = in transit / none
  updated_at   TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS transfers (
  token        TEXT PRIMARY KEY,          -- random, made by the hub, single use
  steam_id     TEXT NOT NULL,
  from_server  TEXT NOT NULL,
  to_server    TEXT NOT NULL,
  arrival      TEXT NOT NULL,             -- arrival point name on the target
  payload      JSONB NOT NULL,            -- player snapshot from the mod
  state        TEXT NOT NULL,             -- 'pending' -> 'claimed' | 'expired' | 'returned'
  created_at   TIMESTAMPTZ NOT NULL DEFAULT now(),
  claimed_at   TIMESTAMPTZ
);
CREATE INDEX IF NOT EXISTS transfers_pending ON transfers (steam_id) WHERE state = 'pending';
