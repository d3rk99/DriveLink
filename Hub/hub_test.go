package main

import (
	"context"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"path/filepath"
	"strings"
	"testing"
)

const testKey = "0123456789abcdef0123"

func newTestHub(t *testing.T) (*httptest.Server, *hub) {
	t.Helper()
	db, err := openDB(filepath.Join(t.TempDir(), "test.db"))
	if err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() { db.Close() })
	if err := migrate(context.Background(), db); err != nil {
		t.Fatal(err)
	}
	// migrations must be safe to run on every start
	if err := migrate(context.Background(), db); err != nil {
		t.Fatal("second migrate:", err)
	}
	h := &hub{cfg: &Config{ServerKey: testKey, Servers: []ServerEntry{
		{ID: "chernarus", Name: "Chernarus", LanHost: "192.168.1.133", Port: 2302},
		{ID: "livonia", Name: "Livonia", LanHost: "192.168.1.133", Port: 2402},
	}}, db: db}
	srv := httptest.NewServer(h.routes())
	t.Cleanup(srv.Close)
	return srv, h
}

func post(t *testing.T, srv *httptest.Server, path string, body map[string]any) (int, map[string]any) {
	t.Helper()
	b, _ := json.Marshal(body)
	resp, err := http.Post(srv.URL+path, "application/octet-stream", strings.NewReader(string(b)))
	if err != nil {
		t.Fatal(err)
	}
	defer resp.Body.Close()
	var out map[string]any
	json.NewDecoder(resp.Body).Decode(&out)
	return resp.StatusCode, out
}

func req(server string, extra map[string]any) map[string]any {
	m := map[string]any{"serverId": server, "key": testKey}
	for k, v := range extra {
		m[k] = v
	}
	return m
}

const sid = "76561198000000001"

// The whole M1 trip: login, depart, anti-dupe, offer, confirm, single use.
func TestTrip(t *testing.T) {
	srv, _ := newTestHub(t)

	if code, out := post(t, srv, "/api/arrive", req("chernarus", map[string]any{"steamId": sid})); code != 200 || out["action"] != "normal" {
		t.Fatalf("first login: %d %v", code, out)
	}
	payload := `{"version":2,"charType":"SurvivorM_Boris","wet":1e-05,"items":[{"id":1,"parent":0,"type":"Apple"}]}`
	code, out := post(t, srv, "/api/depart", req("chernarus", map[string]any{"steamId": sid, "to": "livonia", "arrival": "west_road", "payload": json.RawMessage(payload)}))
	if code != 200 || out["token"] == nil {
		t.Fatalf("depart: %d %v", code, out)
	}
	token := out["token"].(string)

	// in transit: the source can't send him again, and joining the source redirects him to the target
	if code, _ := post(t, srv, "/api/depart", req("chernarus", map[string]any{"steamId": sid, "to": "livonia", "payload": json.RawMessage(payload)})); code != 409 {
		t.Fatalf("second depart while in transit: want 409, got %d", code)
	}
	if _, out := post(t, srv, "/api/arrive", req("chernarus", map[string]any{"steamId": sid})); out["action"] != "redirect" {
		t.Fatalf("join source while in transit: %v", out)
	}

	// target: offered (twice, reconnect-safe) with the exact text, numbers made plain
	for i := 0; i < 2; i++ {
		_, out = post(t, srv, "/api/arrive", req("livonia", map[string]any{"steamId": sid}))
		if out["action"] != "transfer" || out["token"] != token {
			t.Fatalf("arrive on target #%d: %v", i, out)
		}
		if want := strings.Replace(payload, "1e-05", "0.00001", 1); out["payloadJson"] != want {
			t.Fatalf("payloadJson\n got %v\nwant %s", out["payloadJson"], want)
		}
	}

	// confirm: wrong token / wrong server refused, right one once
	if code, _ := post(t, srv, "/api/confirm", req("livonia", map[string]any{"steamId": sid, "token": "nope"})); code != 409 {
		t.Fatalf("wrong token: %d", code)
	}
	if code, _ := post(t, srv, "/api/confirm", req("chernarus", map[string]any{"steamId": sid, "token": token})); code != 409 {
		t.Fatalf("wrong server: %d", code)
	}
	if code, out := post(t, srv, "/api/confirm", req("livonia", map[string]any{"steamId": sid, "token": token})); code != 200 {
		t.Fatalf("confirm: %d %v", code, out)
	}
	if code, _ := post(t, srv, "/api/confirm", req("livonia", map[string]any{"steamId": sid, "token": token})); code != 409 {
		t.Fatalf("double confirm: %d", code)
	}

	// owned by livonia now
	if _, out := post(t, srv, "/api/arrive", req("chernarus", map[string]any{"steamId": sid})); out["action"] != "redirect" {
		t.Fatalf("join chernarus after trip: %v", out)
	}
	if _, out := post(t, srv, "/api/arrive", req("livonia", map[string]any{"steamId": sid})); out["action"] != "normal" {
		t.Fatalf("rejoin livonia after trip: %v", out)
	}
	// and livonia may send him back
	if code, out := post(t, srv, "/api/depart", req("livonia", map[string]any{"steamId": sid, "to": "chernarus", "arrival": "east_road", "payload": json.RawMessage(payload)})); code != 200 {
		t.Fatalf("return trip depart: %d %v", code, out)
	}
}

func TestRefusals(t *testing.T) {
	srv, _ := newTestHub(t)
	m := req("chernarus", map[string]any{"steamId": sid})
	m["key"] = "wrong-key-wrong-key"
	if code, _ := post(t, srv, "/api/arrive", m); code != 401 {
		t.Fatalf("wrong key: %d", code)
	}
	if code, _ := post(t, srv, "/api/arrive", req("sakhal", map[string]any{"steamId": sid})); code != 400 {
		t.Fatalf("unknown server: %d", code)
	}
	if code, _ := post(t, srv, "/api/depart", req("chernarus", map[string]any{"steamId": sid, "to": "chernarus", "payload": json.RawMessage(`{}`)})); code != 400 {
		t.Fatalf("depart to self: %d", code)
	}
	// key in the URL instead of the body (what the DayZ mod also sends)
	resp, err := http.Post(srv.URL+"/api/hello?serverId=chernarus&key="+testKey, "application/octet-stream", strings.NewReader(`{}`))
	if err != nil || resp.StatusCode != 200 {
		t.Fatalf("hello with URL key: %v %v", err, resp.StatusCode)
	}
}

func TestLegacyPostgresConfig(t *testing.T) {
	p := filepath.Join(t.TempDir(), "hub.json")
	old := &Config{Listen: "0.0.0.0:7290", Database: "postgres://drivelink:secret@localhost:5432/drivelink", ServerKey: testKey}
	if err := saveConfig(p, old); err != nil {
		t.Fatal(err)
	}
	c, created, err := loadOrCreateConfig(p)
	if err != nil || created || c.Database != defaultDatabase {
		t.Fatalf("got %+v created=%v err=%v", c, created, err)
	}
	again, _, _ := loadOrCreateConfig(p)
	if again.Database != defaultDatabase {
		t.Fatalf("hub.json not rewritten: %q", again.Database)
	}
}
