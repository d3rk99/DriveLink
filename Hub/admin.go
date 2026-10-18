package main

// Admin commands, run next to a running hub (SQLite + WAL lets both use drivelink.db at once):
//   DriveLinkHub.exe transfers           list the pending trips (who, from -> to, failed builds + last reason)
//   DriveLinkHub.exe return <steamId>    send his pending trip back to the server he left (it rebuilds him there)
//   DriveLinkHub.exe drop <steamId>      give up his pending trip: the snapshot is discarded (gear lost), his next
//                                        login anywhere is a normal login
//   DriveLinkHub.exe dump [file]         write the newest pending snapshot to a file (default last-payload.json)

import (
	"database/sql"
	"errors"
	"fmt"
	"path/filepath"
)

func adminCommand(db *sql.DB, args []string, dir string) error {
	switch args[0] {
	case "transfers":
		return listTransfers(db)
	case "return", "drop":
		if len(args) < 2 {
			return fmt.Errorf("usage: DriveLinkHub.exe %s <steamId>", args[0])
		}
		if args[0] == "return" {
			return returnTransfer(db, args[1])
		}
		return dropTransfer(db, args[1])
	case "dump":
		out := filepath.Join(dir, "last-payload.json")
		if len(args) > 1 {
			out = args[1]
		}
		return dump(db, out)
	}
	return fmt.Errorf("unknown command %q (transfers, return <steamId>, drop <steamId>, dump [file])", args[0])
}

func listTransfers(db *sql.DB) error {
	rows, err := db.Query(`SELECT steam_id,from_server,to_server,arrival,created_at,fail_count,COALESCE(last_error,''),length(payload_text)
		FROM transfers WHERE state='pending' ORDER BY created_at`)
	if err != nil {
		return err
	}
	defer rows.Close()
	n := 0
	for rows.Next() {
		var steamID, from, to, arrival, created, lastErr string
		var fails, size int
		if err := rows.Scan(&steamID, &from, &to, &arrival, &created, &fails, &lastErr, &size); err != nil {
			return err
		}
		n++
		fmt.Printf("%s  %s -> %s (%s)  since %s UTC  %d bytes", steamID, from, to, arrival, created, size)
		if fails > 0 {
			fmt.Printf("  FAILED %dx: %s", fails, lastErr)
		}
		fmt.Println()
	}
	if n == 0 {
		fmt.Println("no pending transfers")
	}
	return rows.Err()
}

func pendingOf(tx *sql.Tx, steamID string) (token, from, to string, err error) {
	err = tx.QueryRow(`SELECT token,from_server,to_server FROM transfers WHERE steam_id=? AND state='pending'
		ORDER BY created_at DESC, rowid DESC LIMIT 1`, steamID).Scan(&token, &from, &to)
	if errors.Is(err, sql.ErrNoRows) {
		err = fmt.Errorf("%s has no pending transfer", steamID)
	}
	return
}

// The trip turns around: same snapshot, now travelling to the server he left. Arrival "" = that server's first
// arrival point (the mod falls back to it). He still owns nothing until that server confirms.
func returnTransfer(db *sql.DB, steamID string) error {
	tx, err := db.Begin()
	if err != nil {
		return err
	}
	defer tx.Rollback()
	token, from, to, err := pendingOf(tx, steamID)
	if err != nil {
		return err
	}
	if _, err := tx.Exec(`UPDATE transfers SET from_server=?, to_server=?, arrival='', fail_count=0, last_error=NULL
		WHERE token=?`, to, from, token); err != nil {
		return err
	}
	if err := tx.Commit(); err != nil {
		return err
	}
	fmt.Printf("%s: trip %s -> %s turned around, he arrives on %s (first arrival point) next time he joins\n", steamID, from, to, from)
	return nil
}

func dropTransfer(db *sql.DB, steamID string) error {
	tx, err := db.Begin()
	if err != nil {
		return err
	}
	defer tx.Rollback()
	token, from, to, err := pendingOf(tx, steamID)
	if err != nil {
		return err
	}
	if _, err := tx.Exec(`UPDATE transfers SET state='expired' WHERE token=?`, token); err != nil {
		return err
	}
	if err := tx.Commit(); err != nil {
		return err
	}
	fmt.Printf("%s: trip %s -> %s dropped (snapshot discarded); his next login anywhere is a normal login\n", steamID, from, to)
	return nil
}
