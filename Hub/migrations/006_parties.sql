-- Group travel (2026-10-20, Research/group-travel.md): which party a trip belongs to (a group on foot, a convoy, or a
-- crew in one car; the mod sends partyId in the snapshot) and the traveller's place in it, for the Players page and
-- its whole-party actions. NULL = travelled alone (or an older mod build).
ALTER TABLE transfers ADD COLUMN party TEXT;
ALTER TABLE transfers ADD COLUMN party_role TEXT;
CREATE INDEX IF NOT EXISTS transfers_party ON transfers (party);
