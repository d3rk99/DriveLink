-- Phase A (2026-10-18): a target server that cannot build an arriving character reports it (/api/fail) instead
-- of silently letting him play a wrong character. The transfer stays pending (retried on the next join);
-- an admin can send it back to the source with "DriveLinkHub.exe return <steamId>".
ALTER TABLE transfers ADD COLUMN fail_count INTEGER NOT NULL DEFAULT 0;
ALTER TABLE transfers ADD COLUMN last_error TEXT;
