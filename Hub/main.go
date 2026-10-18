// DriveLink Hub - the one program every DayZ server talks to (M1: players on foot).
//
// Plug-and-play: run it once -> it writes hub.json with a random server key and stops, telling you what to
// fill in (your servers). Run it again -> it creates/updates its database (drivelink.db, SQLite, next to the
// exe) itself and serves the API. Nothing to install: no Postgres, no Node, no Mongo, no certificate on a LAN.
//
// DayZ's script RestApi can only set the Content-Type header (vanilla 3_game/http/restapi.c:136-141,
// RestContext.SetHeader "Set Content-Type header"), so the key travels in the JSON body, not a header.
package main

import (
	"context"
	"crypto/rand"
	"crypto/subtle"
	"database/sql"
	"embed"
	"encoding/hex"
	"encoding/json"
	"errors"
	"flag"
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"time"

	_ "modernc.org/sqlite" // pure Go SQLite: the hub stays one exe, no C compiler, no DLL
)

//go:embed migrations/*.sql
var migrationFS embed.FS

const maxBody = 4 << 20 // 4 MB per request (a player snapshot is far smaller)

const defaultDatabase = "drivelink.db"

type ServerEntry struct {
	ID         string `json:"id"`         // short id, also set in that DayZ server's DriveLink config
	Name       string `json:"name"`       // shown to players
	LanHost    string `json:"lanHost"`    // address players on the same network use (e.g. 192.168.1.133)
	PublicHost string `json:"publicHost"` // address players from the internet use
	Port       int    `json:"port"`       // game port
	Password   string `json:"password"`   // server password, "" = none
}

type Config struct {
	Listen    string        `json:"listen"`    // e.g. "0.0.0.0:7290"
	Database  string        `json:"database"`  // SQLite file, relative to hub.json's folder (default drivelink.db)
	ServerKey string        `json:"serverKey"` // shared secret every DayZ server sends
	Servers   []ServerEntry `json:"servers"`
}

func randomKey() string {
	b := make([]byte, 24)
	if _, err := rand.Read(b); err != nil {
		log.Fatal(err)
	}
	return hex.EncodeToString(b)
}

func saveConfig(path string, c *Config) error {
	out, _ := json.MarshalIndent(c, "", "  ")
	return os.WriteFile(path, out, 0o600)
}

func loadOrCreateConfig(path string) (*Config, bool, error) {
	data, err := os.ReadFile(path)
	if errors.Is(err, os.ErrNotExist) {
		c := &Config{
			Listen:    "0.0.0.0:7290",
			Database:  defaultDatabase,
			ServerKey: randomKey(),
			Servers: []ServerEntry{
				{ID: "chernarus", Name: "Chernarus", LanHost: "192.168.1.133", PublicHost: "", Port: 2302},
				{ID: "livonia", Name: "Livonia", LanHost: "192.168.1.133", PublicHost: "", Port: 2402},
			},
		}
		if err := saveConfig(path, c); err != nil {
			return nil, false, err
		}
		return c, true, nil
	}
	if err != nil {
		return nil, false, err
	}
	var c Config
	if err := json.Unmarshal(data, &c); err != nil {
		return nil, false, fmt.Errorf("%s is not valid JSON: %w", path, err)
	}
	// hub.json from before the SQLite switch (v0.1/v0.2 used PostgreSQL): drop the old URL (it held a password)
	if c.Database == "" || strings.HasPrefix(c.Database, "postgres://") || strings.HasPrefix(c.Database, "postgresql://") {
		if c.Database != "" {
			log.Printf("hub.json: the hub no longer uses PostgreSQL; database is now %s (SQLite, created automatically)", defaultDatabase)
		}
		c.Database = defaultDatabase
		if err := saveConfig(path, &c); err != nil {
			return nil, false, err
		}
	}
	return &c, false, nil
}

func (c *Config) validate() error {
	if len(c.ServerKey) < 16 {
		return errors.New(`"serverKey" is too short (16+ characters)`)
	}
	seen := map[string]bool{}
	for _, s := range c.Servers {
		if s.ID == "" || s.Port == 0 || (s.LanHost == "" && s.PublicHost == "") {
			return fmt.Errorf("server %q needs id, port and lanHost or publicHost", s.ID)
		}
		if seen[s.ID] {
			return fmt.Errorf("server id %q is listed twice", s.ID)
		}
		seen[s.ID] = true
	}
	return nil
}

func (c *Config) server(id string) *ServerEntry {
	for i := range c.Servers {
		if c.Servers[i].ID == id {
			return &c.Servers[i]
		}
	}
	return nil
}

// ---------------- database ----------------

// openDB opens (or creates) the SQLite file. WAL = readers never block the writer and a crash can't leave a
// half-written file; busy_timeout = wait instead of failing if the file is briefly locked (e.g. by a backup).
// ONE connection: every transaction runs one after the other, so "read owner, then change it" can never race
// (what SELECT ... FOR UPDATE did in PostgreSQL). Hub traffic is a few requests per login/trip; that's plenty.
func openDB(path string) (*sql.DB, error) {
	dsn := "file:" + filepath.ToSlash(path) + "?_pragma=busy_timeout(5000)&_pragma=journal_mode(WAL)&_pragma=synchronous(FULL)"
	db, err := sql.Open("sqlite", dsn)
	if err != nil {
		return nil, err
	}
	db.SetMaxOpenConns(1)
	if err := db.Ping(); err != nil {
		db.Close()
		return nil, err
	}
	return db, nil
}

func migrate(ctx context.Context, db *sql.DB) error {
	if _, err := db.ExecContext(ctx, `CREATE TABLE IF NOT EXISTS schema_version (version INTEGER PRIMARY KEY)`); err != nil {
		return err
	}
	var have int
	if err := db.QueryRowContext(ctx, `SELECT COALESCE(MAX(version),0) FROM schema_version`).Scan(&have); err != nil {
		return err
	}
	names, err := migrationFS.ReadDir("migrations")
	if err != nil {
		return err
	}
	sort.Slice(names, func(i, j int) bool { return names[i].Name() < names[j].Name() })
	for _, n := range names {
		var v int
		if _, err := fmt.Sscanf(n.Name(), "%03d_", &v); err != nil || v <= have {
			continue
		}
		sqlText, _ := migrationFS.ReadFile("migrations/" + n.Name())
		tx, err := db.BeginTx(ctx, nil)
		if err != nil {
			return err
		}
		if _, err := tx.ExecContext(ctx, string(sqlText)); err != nil {
			tx.Rollback()
			return fmt.Errorf("migration %s: %w", n.Name(), err)
		}
		if _, err := tx.ExecContext(ctx, `INSERT INTO schema_version(version) VALUES (?)`, v); err != nil {
			tx.Rollback()
			return err
		}
		if err := tx.Commit(); err != nil {
			return err
		}
		log.Printf("database: applied %s", n.Name())
	}
	return nil
}

// ---------------- API ----------------

type hub struct {
	cfg *Config
	db  *sql.DB
}

type baseReq struct {
	ServerID string `json:"serverId"`
	Key      string `json:"key"`
}

type serverOut struct {
	ID         string `json:"id"`
	Name       string `json:"name"`
	LanHost    string `json:"lanHost"`
	PublicHost string `json:"publicHost"`
	Port       int    `json:"port"`
	Password   string `json:"password"`
}

func toOut(s *ServerEntry) *serverOut {
	if s == nil {
		return nil
	}
	return &serverOut{s.ID, s.Name, s.LanHost, s.PublicHost, s.Port, s.Password}
}

type statusWriter struct {
	http.ResponseWriter
	code int
}

func (s *statusWriter) WriteHeader(c int) { s.code = c; s.ResponseWriter.WriteHeader(c) }

// Every request gets one log line (method, path WITHOUT the query - it can hold the key - status, time), so a
// DayZ "error 5" can always be matched to what the hub saw, or to nothing (= never arrived).
func logRequests(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		start := time.Now()
		sw := &statusWriter{ResponseWriter: w, code: 200}
		next.ServeHTTP(sw, r)
		log.Printf("%s %s %s -> %d (%d ms, %d bytes in, content-type %q)", r.RemoteAddr, r.Method, r.URL.Path, sw.code, time.Since(start).Milliseconds(), r.ContentLength, r.Header.Get("Content-Type"))
	})
}

// First 160 bytes of a request body for the log, with any "key" value masked.
func preview(b []byte) string {
	s := string(b)
	if i := strings.Index(s, `"key":"`); i >= 0 {
		j := strings.Index(s[i+7:], `"`)
		if j >= 0 {
			s = s[:i+7] + "***" + s[i+7+j:]
		}
	}
	if len(s) > 160 {
		s = s[:160] + "..."
	}
	return s
}

func writeJSON(w http.ResponseWriter, code int, v any) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(code)
	json.NewEncoder(w).Encode(v)
}

func dbError(w http.ResponseWriter, err error) {
	log.Printf("database error: %v", err)
	writeJSON(w, 500, map[string]any{"ok": false, "error": "database: " + err.Error()})
}

// decode reads the body into v and checks the key + server id. Returns false (and has answered) on failure.
func (h *hub) decode(w http.ResponseWriter, r *http.Request, v any, base *baseReq) bool {
	if r.Method != http.MethodPost {
		log.Printf("refused %s: method %s", r.URL.Path, r.Method)
		writeJSON(w, 405, map[string]any{"ok": false, "error": "POST only"})
		return false
	}
	body, err := io.ReadAll(io.LimitReader(r.Body, maxBody+1))
	if err != nil || len(body) > maxBody {
		writeJSON(w, 413, map[string]any{"ok": false, "error": "body too large or unreadable"})
		return false
	}
	if err := json.Unmarshal(body, base); err != nil {
		log.Printf("refused %s from %s: bad json (%v), body starts %q", r.URL.Path, r.RemoteAddr, err, preview(body))
		writeJSON(w, 400, map[string]any{"ok": false, "error": "bad json"})
		return false
	}
	// The DayZ mod also puts serverId + key in the URL (M1 retest 2026-10-13: the body's key never matched).
	if base.Key == "" {
		base.Key = r.URL.Query().Get("key")
	}
	if base.ServerID == "" {
		base.ServerID = r.URL.Query().Get("serverId")
	}
	if subtle.ConstantTimeCompare([]byte(base.Key), []byte(h.cfg.ServerKey)) != 1 {
		// never print the key itself, only enough to see what arrived
		log.Printf("refused %s from %s: wrong key (got %d chars, serverId %q), body starts %q", r.URL.Path, r.RemoteAddr, len(base.Key), base.ServerID, preview(body))
		writeJSON(w, 401, map[string]any{"ok": false, "error": "wrong key"})
		return false
	}
	if h.cfg.server(base.ServerID) == nil {
		log.Printf("refused %s: unknown serverId %q", r.URL.Path, base.ServerID)
		writeJSON(w, 400, map[string]any{"ok": false, "error": "unknown serverId " + base.ServerID + " (add it to hub.json)"})
		return false
	}
	if v != nil {
		if err := json.Unmarshal(body, v); err != nil {
			log.Printf("refused %s: body does not fit (%v), starts %q", r.URL.Path, err, preview(body))
			writeJSON(w, 400, map[string]any{"ok": false, "error": "bad json: " + err.Error()})
			return false
		}
	}
	h.db.ExecContext(r.Context(), `INSERT INTO servers(id,name,last_seen) VALUES(?,?,CURRENT_TIMESTAMP)
		ON CONFLICT (id) DO UPDATE SET last_seen = CURRENT_TIMESTAMP`, base.ServerID, h.cfg.server(base.ServerID).Name)
	return true
}

// POST /api/hello  {serverId,key} -> the server list (so DayZ configs don't repeat addresses)
func (h *hub) hello(w http.ResponseWriter, r *http.Request) {
	var b baseReq
	if !h.decode(w, r, nil, &b) {
		return
	}
	out := []*serverOut{}
	for i := range h.cfg.Servers {
		out = append(out, toOut(&h.cfg.Servers[i]))
	}
	log.Printf("hello from %s", b.ServerID)
	writeJSON(w, 200, map[string]any{"ok": true, "servers": out, "time": time.Now().UTC().Format(time.RFC3339)})
}

type departReq struct {
	baseReq
	SteamID string          `json:"steamId"`
	To      string          `json:"to"`
	Arrival string          `json:"arrival"`
	Payload json.RawMessage `json:"payload"`
}

// setOwner records which server holds the live character (nil = in transit).
func setOwner(ctx context.Context, tx *sql.Tx, steamID string, owner *string) error {
	_, err := tx.ExecContext(ctx, `INSERT INTO characters(steam_id,owner_server,updated_at) VALUES(?,?,CURRENT_TIMESTAMP)
		ON CONFLICT (steam_id) DO UPDATE SET owner_server=excluded.owner_server, updated_at=CURRENT_TIMESTAMP`, steamID, owner)
	return err
}

// POST /api/depart  source server hands the player over.
// Only the current owner (or a server for a player the hub has never seen) may send a player away.
func (h *hub) depart(w http.ResponseWriter, r *http.Request) {
	var b baseReq
	var q departReq
	if !h.decode(w, r, &q, &b) {
		return
	}
	target := h.cfg.server(q.To)
	if q.SteamID == "" || target == nil || q.To == b.ServerID || len(q.Payload) == 0 || !json.Valid(q.Payload) {
		log.Printf("depart refused: steamId %q, to %q (known %v), from %q, payload %d bytes", q.SteamID, q.To, target != nil, b.ServerID, len(q.Payload))
		writeJSON(w, 400, map[string]any{"ok": false, "error": "need steamId, a different known target server and a JSON payload"})
		return
	}
	ctx := r.Context()
	tx, err := h.db.BeginTx(ctx, nil)
	if err != nil {
		dbError(w, err)
		return
	}
	defer tx.Rollback()
	var owner sql.NullString
	err = tx.QueryRowContext(ctx, `SELECT owner_server FROM characters WHERE steam_id=?`, q.SteamID).Scan(&owner)
	switch {
	case errors.Is(err, sql.ErrNoRows):
		// first time the hub sees this player: the server he stands on owns him
	case err != nil:
		dbError(w, err)
		return
	case !owner.Valid || owner.String != b.ServerID:
		cur := "nobody (in transit)"
		if owner.Valid {
			cur = owner.String
		}
		log.Printf("depart REFUSED %s from %s: owned by %s", q.SteamID, b.ServerID, cur)
		writeJSON(w, 409, map[string]any{"ok": false, "error": "this character is owned by " + cur})
		return
	}
	token := randomKey()
	if _, err := tx.ExecContext(ctx, `UPDATE transfers SET state='superseded' WHERE steam_id=? AND state='pending'`, q.SteamID); err != nil {
		dbError(w, err)
		return
	}
	if _, err := tx.ExecContext(ctx, `INSERT INTO transfers(token,steam_id,from_server,to_server,arrival,payload_text,state)
		VALUES(?,?,?,?,?,?,'pending')`, token, q.SteamID, b.ServerID, q.To, q.Arrival, string(q.Payload)); err != nil {
		dbError(w, err)
		return
	}
	if err := setOwner(ctx, tx, q.SteamID, nil); err != nil {
		dbError(w, err)
		return
	}
	if err := tx.Commit(); err != nil {
		dbError(w, err)
		return
	}
	log.Printf("depart %s: %s -> %s (%s), %d bytes, token %s..", q.SteamID, b.ServerID, q.To, q.Arrival, len(q.Payload), token[:8])
	writeJSON(w, 200, map[string]any{"ok": true, "token": token, "target": toOut(target)})
}

type arriveReq struct {
	baseReq
	SteamID string `json:"steamId"`
}

// POST /api/arrive  called by every server when a player connects.
//
//	{"action":"transfer", payload, arrival, from}  -> build this player from the payload (claimed, single use)
//	{"action":"redirect", target}                  -> this player belongs to / is travelling to another server
//	{"action":"normal"}                            -> normal login; this server owns the player from now on
func (h *hub) arrive(w http.ResponseWriter, r *http.Request) {
	var b baseReq
	var q arriveReq
	if !h.decode(w, r, &q, &b) {
		return
	}
	if q.SteamID == "" {
		log.Printf("arrive refused: no steamId in the body")
		writeJSON(w, 400, map[string]any{"ok": false, "error": "need steamId"})
		return
	}
	ctx := r.Context()
	tx, err := h.db.BeginTx(ctx, nil)
	if err != nil {
		dbError(w, err)
		return
	}
	defer tx.Rollback()

	var token, from, to, arrival, payloadText string
	err = tx.QueryRowContext(ctx, `SELECT token,from_server,to_server,arrival,payload_text FROM transfers
		WHERE steam_id=? AND state='pending' ORDER BY created_at DESC, rowid DESC LIMIT 1`, q.SteamID).
		Scan(&token, &from, &to, &arrival, &payloadText)
	if err == nil {
		if to != b.ServerID {
			log.Printf("arrive %s on %s: travelling to %s, redirect", q.SteamID, b.ServerID, to)
			writeJSON(w, 200, map[string]any{"ok": true, "action": "redirect", "target": toOut(h.cfg.server(to))})
			return
		}
		// Two-phase (M1 test 2026-10-13: the hub claimed before the character existed). The transfer stays
		// pending until the target server confirms it BUILT the character (/api/confirm). Asking again (reconnect)
		// returns the same transfer.
		log.Printf("arrive %s on %s from %s (%s): transfer offered, waiting for confirm", q.SteamID, b.ServerID, from, arrival)
		// payloadJson = the exact text the source server sent, as a string (the mod parses it with the same
		// serializer that wrote it).
		writeJSON(w, 200, map[string]any{"ok": true, "action": "transfer", "token": token, "from": from, "arrival": arrival, "payloadJson": plainNumbers(payloadText)})
		return
	}
	if !errors.Is(err, sql.ErrNoRows) {
		dbError(w, err)
		return
	}
	var owner sql.NullString
	err = tx.QueryRowContext(ctx, `SELECT owner_server FROM characters WHERE steam_id=?`, q.SteamID).Scan(&owner)
	if err == nil && owner.Valid && owner.String != b.ServerID && h.cfg.server(owner.String) != nil {
		log.Printf("arrive %s on %s: owned by %s, redirect", q.SteamID, b.ServerID, owner.String)
		writeJSON(w, 200, map[string]any{"ok": true, "action": "redirect", "target": toOut(h.cfg.server(owner.String))})
		return
	}
	if err != nil && !errors.Is(err, sql.ErrNoRows) {
		dbError(w, err)
		return
	}
	if err := setOwner(ctx, tx, q.SteamID, &b.ServerID); err != nil {
		dbError(w, err)
		return
	}
	if err := tx.Commit(); err != nil {
		dbError(w, err)
		return
	}
	log.Printf("arrive %s on %s: normal login, owner set", q.SteamID, b.ServerID)
	writeJSON(w, 200, map[string]any{"ok": true, "action": "normal"})
}

type confirmReq struct {
	baseReq
	SteamID string `json:"steamId"`
	Token   string `json:"token"`
}

// POST /api/confirm  the target server built the character from this transfer: claim it, it owns him now.
func (h *hub) confirm(w http.ResponseWriter, r *http.Request) {
	var b baseReq
	var q confirmReq
	if !h.decode(w, r, &q, &b) {
		return
	}
	ctx := r.Context()
	tx, err := h.db.BeginTx(ctx, nil)
	if err != nil {
		dbError(w, err)
		return
	}
	defer tx.Rollback()
	res, err := tx.ExecContext(ctx, `UPDATE transfers SET state='claimed', claimed_at=CURRENT_TIMESTAMP
		WHERE token=? AND steam_id=? AND to_server=? AND state='pending'`, q.Token, q.SteamID, b.ServerID)
	if err != nil {
		dbError(w, err)
		return
	}
	if n, _ := res.RowsAffected(); n != 1 {
		log.Printf("confirm REFUSED %s on %s: no pending transfer with that token", q.SteamID, b.ServerID)
		writeJSON(w, 409, map[string]any{"ok": false, "error": "no pending transfer with that token"})
		return
	}
	if err := setOwner(ctx, tx, q.SteamID, &b.ServerID); err != nil {
		dbError(w, err)
		return
	}
	if err := tx.Commit(); err != nil {
		dbError(w, err)
		return
	}
	log.Printf("confirm %s on %s: transfer claimed, owner set", q.SteamID, b.ServerID)
	writeJSON(w, 200, map[string]any{"ok": true})
}

func (h *hub) routes() http.Handler {
	mux := http.NewServeMux()
	mux.HandleFunc("/api/hello", h.hello)
	mux.HandleFunc("/api/depart", h.depart)
	mux.HandleFunc("/api/arrive", h.arrive)
	mux.HandleFunc("/api/confirm", h.confirm)
	mux.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
		if err := h.db.PingContext(r.Context()); err != nil {
			writeJSON(w, 503, map[string]any{"ok": false, "error": "database down"})
			return
		}
		writeJSON(w, 200, map[string]any{"ok": true})
	})
	return logRequests(mux)
}

// "DriveLinkHub.exe dump [file]": write the newest pending transfer's snapshot to a file (default
// last-payload.json next to the exe) for debugging. Replaces Tools\dump-payload.ps1 (psql).
func dump(db *sql.DB, out string) error {
	var steamID, from, to, text string
	err := db.QueryRow(`SELECT steam_id,from_server,to_server,payload_text FROM transfers WHERE state='pending'
		ORDER BY created_at DESC, rowid DESC LIMIT 1`).Scan(&steamID, &from, &to, &text)
	if errors.Is(err, sql.ErrNoRows) {
		fmt.Println("no pending transfer")
		return nil
	}
	if err != nil {
		return err
	}
	if err := os.WriteFile(out, []byte(text), 0o644); err != nil {
		return err
	}
	fmt.Printf("pending %s: %s -> %s, wrote %s (%d bytes)\n", steamID, from, to, out, len(text))
	return nil
}

func main() {
	cfgPath := flag.String("config", "hub.json", "config file")
	flag.Parse()
	exe, _ := os.Executable()
	if !filepath.IsAbs(*cfgPath) {
		*cfgPath = filepath.Join(filepath.Dir(exe), *cfgPath)
	}
	dir := filepath.Dir(*cfgPath)
	if lf, err := os.OpenFile(filepath.Join(dir, "hub.log"), os.O_CREATE|os.O_APPEND|os.O_WRONLY, 0o644); err == nil {
		log.SetOutput(io.MultiWriter(os.Stdout, lf))
	}
	cfg, created, err := loadOrCreateConfig(*cfgPath)
	if err != nil {
		log.Fatal(err)
	}
	if created {
		fmt.Printf("\nFirst start: wrote %s\n", *cfgPath)
		fmt.Println("1. Check the \"servers\" list (id, lanHost, publicHost, port).")
		fmt.Println("2. Put this in EVERY DayZ server's DriveLink config as hubKey:")
		fmt.Printf("     %s\n", cfg.ServerKey)
		fmt.Println("Then start the hub again.")
		return
	}
	if err := cfg.validate(); err != nil {
		log.Fatalf("hub.json: %v", err)
	}
	dbPath := cfg.Database
	if !filepath.IsAbs(dbPath) {
		dbPath = filepath.Join(dir, dbPath)
	}
	db, err := openDB(dbPath)
	if err != nil {
		log.Fatalf("cannot open the database %s: %v", dbPath, err)
	}
	ctx, cancel := context.WithTimeout(context.Background(), 15*time.Second)
	if err := migrate(ctx, db); err != nil {
		cancel()
		log.Fatalf("database setup failed: %v", err)
	}
	cancel()

	if flag.Arg(0) == "dump" {
		out := flag.Arg(1)
		if out == "" {
			out = filepath.Join(dir, "last-payload.json")
		}
		if err := dump(db, out); err != nil {
			log.Fatal(err)
		}
		return
	}

	h := &hub{cfg: cfg, db: db}
	srv := &http.Server{Addr: cfg.Listen, Handler: h.routes(), ReadHeaderTimeout: 10 * time.Second, ReadTimeout: 30 * time.Second}
	log.Printf("DriveLink Hub listening on %s, %d servers, database %s", cfg.Listen, len(cfg.Servers), dbPath)
	log.Fatal(srv.ListenAndServe())
}
