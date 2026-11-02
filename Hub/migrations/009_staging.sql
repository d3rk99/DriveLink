-- One staging area per server (Map page): where travelling groups wait out of sight, leaving (the trip film) and
-- arriving (vehicles assembled). JSON [x, 0, z]; '' = none.
ALTER TABLE layouts ADD COLUMN staging TEXT NOT NULL DEFAULT '';
