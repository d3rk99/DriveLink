-- Players page (2026-10-19): who a steam id is and where he last logged in (sent by the mod on every login), and a
-- log of what admins changed on the Players page.
ALTER TABLE characters ADD COLUMN name TEXT;
ALTER TABLE characters ADD COLUMN last_server TEXT;
ALTER TABLE characters ADD COLUMN last_login TEXT;
CREATE TABLE IF NOT EXISTS admin_log (
  id        INTEGER PRIMARY KEY AUTOINCREMENT,
  at        TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
  steam_id  TEXT NOT NULL,
  action    TEXT NOT NULL,
  detail    TEXT NOT NULL
);
CREATE INDEX IF NOT EXISTS admin_log_steam ON admin_log (steam_id);
