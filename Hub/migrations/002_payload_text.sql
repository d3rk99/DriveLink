-- M1 test 2026-10-13: items went missing between departure and arrival although the stored snapshot had them.
-- JSONB re-orders keys and rewrites numbers; the DayZ side then read back only part of it. Keep the EXACT text the
-- source server sent and hand that back unchanged (the jsonb column stays for queries).
ALTER TABLE transfers ADD COLUMN IF NOT EXISTS payload_text TEXT;
