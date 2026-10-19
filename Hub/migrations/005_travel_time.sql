-- Travel time (2026-10-19, Research/travel-cost-design.md section 3): a trip can't be claimed before ready_at (UTC);
-- until then every server answers /api/arrive with "wait". NULL = ready at once (as before).
ALTER TABLE transfers ADD COLUMN ready_at TEXT;
