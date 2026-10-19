#!/usr/bin/env bash
# Black-box check of a DriveLink hub exe (Rust or Go): starts it on a TEMP hub.json + database (ports 17290/17291),
# runs the whole M1 trip, anti-dupe, refusals, fail/return/drop and HTTPS, then stops it.
# Usage: bash Tools/hub-api-check.sh <path to hub exe>
set -u
EXE="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
T="$(mktemp -d)"
cp "$EXE" "$T/hub.exe"
KEY=0123456789abcdef0123456789abcdef
cat > "$T/hub.json" <<EOF
{"listen":"127.0.0.1:17290","listenTls":"127.0.0.1:17291","database":"test.db","serverKey":"$KEY",
 "servers":[{"id":"chernarus","name":"Chernarus","lanHost":"127.0.0.1","port":2302},{"id":"livonia","name":"Livonia","lanHost":"127.0.0.1","port":2402}]}
EOF
"$T/hub.exe" -config "$T/hub.json" > "$T/out.log" 2>&1 &
PID=$!
trap 'kill $PID 2>/dev/null; sleep 0.5; rm -rf "$T"' EXIT
for i in $(seq 1 50); do curl -s -m 1 http://127.0.0.1:17290/health >/dev/null && break; sleep 0.2; done
U=http://127.0.0.1:17290
FAIL=0
SID=76561198000000001
post() { curl -s -m 5 -o "$T/body" -w "%{http_code}" -H "Content-Type: application/octet-stream" --data-binary "$2" "$U$1"; }
body() { cat "$T/body"; }
expect() { # name, got, want
  if [ "$2" == "$3" ]; then echo "ok   $1"; else echo "FAIL $1: got [$2] want [$3]"; FAIL=1; fi
}
has() { if grep -q -- "$2" "$T/body"; then echo "ok   $1"; else echo "FAIL $1: body $(body)"; FAIL=1; fi; }
req() { echo "{\"serverId\":\"$1\",\"key\":\"$KEY\"$2}"; }

PAY='{"version":2,"charType":"SurvivorM_Boris","wet":1e-05,"items":[{"id":1,"parent":0,"type":"Apple"}]}'
WANT='{"version":2,"charType":"SurvivorM_Boris","wet":0.00001,"items":[{"id":1,"parent":0,"type":"Apple"}]}'

expect "first login" "$(post /api/arrive "$(req chernarus ",\"steamId\":\"$SID\"")")" 200; has "  = normal" '"action":"normal"'
expect "depart" "$(post /api/depart "$(req chernarus ",\"steamId\":\"$SID\",\"to\":\"livonia\",\"arrival\":\"west_road\",\"payload\":$PAY")")" 200
TOKEN=$(sed -E 's/.*"token":"([0-9a-f]+)".*/\1/' "$T/body")
expect "depart again while in transit" "$(post /api/depart "$(req chernarus ",\"steamId\":\"$SID\",\"to\":\"livonia\",\"payload\":$PAY")")" 409
post /api/arrive "$(req chernarus ",\"steamId\":\"$SID\"")" >/dev/null; has "join source while in transit = redirect" '"action":"redirect"'
for n in 1 2; do
  post /api/arrive "$(req livonia ",\"steamId\":\"$SID\"")" >/dev/null; has "arrive on target #$n = transfer" '"action":"transfer"'
  has "  bytes = ${#WANT}" "\"bytes\":${#WANT}"
  expect "payload #$n" "$(post /api/payload "$(req livonia ",\"steamId\":\"$SID\",\"token\":\"$TOKEN\"")")" 200
  expect "  exact text, numbers plain" "$(body)" "$WANT"
done
expect "payload for the wrong server" "$(post /api/payload "$(req chernarus ",\"steamId\":\"$SID\",\"token\":\"$TOKEN\"")")" 409
expect "confirm wrong token" "$(post /api/confirm "$(req livonia ",\"steamId\":\"$SID\",\"token\":\"nope\"")")" 409
expect "confirm wrong server" "$(post /api/confirm "$(req chernarus ",\"steamId\":\"$SID\",\"token\":\"$TOKEN\"")")" 409
expect "confirm" "$(post /api/confirm "$(req livonia ",\"steamId\":\"$SID\",\"token\":\"$TOKEN\"")")" 200
expect "double confirm" "$(post /api/confirm "$(req livonia ",\"steamId\":\"$SID\",\"token\":\"$TOKEN\"")")" 409
post /api/arrive "$(req chernarus ",\"steamId\":\"$SID\"")" >/dev/null; has "join old server after trip = redirect" '"action":"redirect"'
post /api/arrive "$(req livonia ",\"steamId\":\"$SID\"")" >/dev/null; has "rejoin target = normal" '"action":"normal"'

# fail / return / drop
expect "depart 2" "$(post /api/depart "$(req livonia ",\"steamId\":\"$SID\",\"to\":\"chernarus\",\"arrival\":\"east_road\",\"payload\":$PAY")")" 200
TOKEN=$(sed -E 's/.*"token":"([0-9a-f]+)".*/\1/' "$T/body")
expect "fail" "$(post /api/fail "$(req chernarus ",\"steamId\":\"$SID\",\"token\":\"$TOKEN\",\"reason\":\"no arrival points\"")")" 200
post /api/arrive "$(req chernarus ",\"steamId\":\"$SID\"")" >/dev/null; has "after fail still offered" '"action":"transfer"'
"$T/hub.exe" -config "$T/hub.json" transfers > "$T/body" 2>&1; has "transfers lists FAILED 1x" "FAILED 1x: no arrival points"
"$T/hub.exe" -config "$T/hub.json" return $SID > "$T/body" 2>&1; has "return" "turned around"
post /api/arrive "$(req livonia ",\"steamId\":\"$SID\"")" >/dev/null; has "returned trip offered to livonia" '"arrival":""'
"$T/hub.exe" -config "$T/hub.json" drop $SID > "$T/body" 2>&1; has "drop" "dropped"
post /api/arrive "$(req chernarus ",\"steamId\":\"$SID\"")" >/dev/null; has "after drop = normal" '"action":"normal"'

# refusals
expect "wrong key" "$(post /api/arrive "{\"serverId\":\"chernarus\",\"key\":\"wrong-key-wrong-key\",\"steamId\":\"$SID\"}")" 401
expect "unknown server" "$(post /api/arrive "$(req sakhal ",\"steamId\":\"$SID\"")")" 400
expect "depart to self" "$(post /api/depart "$(req chernarus ",\"steamId\":\"$SID\",\"to\":\"chernarus\",\"payload\":{}")")" 400
expect "key in the URL" "$(post "/api/hello?serverId=chernarus&key=$KEY" '{}')" 200

# joining from the web app: a new server asks with its claim, is approved on the Servers page, learns its id
apost() { curl -s -m 5 -o "$T/body" -w "%{http_code}" -H "Content-Type: application/json" --data-binary "$2" "$U$1"; }
expect "join: hello without id" "$(post /api/hello "{\"serverId\":\"\",\"key\":\"$KEY\",\"claim\":\"c-123\",\"port\":2502,\"queryPort\":27020,\"world\":\"Sakhal\"}")" 200
has "join: answer is pending" '"pending":true'
curl -s -m 5 "$U/admin/api/state" > "$T/body"; has "join: listed on the Servers page" '"claim":"c-123"'
has "join: suggests this PC's LAN address" '"suggestLan":"'
expect "join: approve" "$(apost /admin/api/pending/c-123/approve '{"id":"sakhal","name":"Sakhal","lanHost":"127.0.0.1","port":2502}')" 200
grep -q '"claim": "c-123"' "$T/hub.json" && echo "ok   join: saved in hub.json" || { echo "FAIL join: not in hub.json"; FAIL=1; }
expect "join: hello by claim" "$(post /api/hello "{\"serverId\":\"\",\"key\":\"$KEY\",\"claim\":\"c-123\",\"port\":2502}")" 200
has "join: learns its id" '"serverId":"sakhal"'
expect "join: works by id now" "$(post /api/arrive "$(req sakhal ",\"steamId\":\"76561198000000009\"")")" 200
expect "link: hello from a reinstalled livonia" "$(post /api/hello "{\"serverId\":\"\",\"key\":\"$KEY\",\"claim\":\"c-456\",\"port\":2402}")" 200
expect "link: to the existing livonia" "$(apost /admin/api/pending/c-456/approve '{"id":"livonia","name":"x","port":0,"link":true}')" 200
post /api/hello "{\"serverId\":\"\",\"key\":\"$KEY\",\"claim\":\"c-456\"}" >/dev/null; has "link: learns it is livonia" '"serverId":"livonia"'
expect "servers page: add" "$(apost /admin/api/servers '{"isNew":true,"id":"deerisle","name":"Deer Isle","lanHost":"10.0.0.5","port":2602}')" 200
expect "servers page: add twice refused" "$(apost /admin/api/servers '{"isNew":true,"id":"deerisle","name":"Deer Isle","lanHost":"10.0.0.5","port":2602}')" 400
expect "servers page: edit" "$(apost /admin/api/servers '{"id":"deerisle","name":"Deer Isle 2","lanHost":"10.0.0.6","port":2602}')" 200
grep -q 'Deer Isle 2' "$T/hub.json" && echo "ok   servers page: edit saved" || { echo "FAIL edit not saved"; FAIL=1; }
expect "servers page: remove" "$(apost /admin/api/servers/deerisle/delete '')" 200
expect "removed server refused" "$(post /api/arrive "$(req deerisle ",\"steamId\":\"$SID\"")")" 400
expect "hello reports a new port" "$(post /api/hello "{\"serverId\":\"livonia\",\"key\":\"$KEY\",\"port\":2412,\"queryPort\":27022}")" 200
grep -q '"port": 2412' "$T/hub.json" && echo "ok   reported port saved" || { echo "FAIL reported port not saved"; FAIL=1; }
expect "settings: public address" "$(apost /admin/api/settings '{"publicHost":"hub.example.com"}')" 200
grep -q 'hub.example.com' "$T/hub.json" && echo "ok   settings saved" || { echo "FAIL settings not saved"; FAIL=1; }
expect "settings: short password refused" "$(apost /admin/api/settings '{"adminPassword":"abc"}')" 400
code=$(curl -s -m 20 -o "$T/body" -w "%{http_code}" "$U/admin/api/firewall"); expect "firewall check" "$code" 200
has "firewall: hub rule listed" '"DriveLink Hub"'

# HTTPS (self-signed, so -k)
code=$(curl -s -k -m 5 -o "$T/body" -w "%{http_code}" --data-binary "$(req chernarus "")" https://127.0.0.1:17291/api/hello)
expect "HTTPS hello (self-signed)" "$code" 200
[ -f "$T/hub-cert.pem" ] && echo "ok   self-signed certificate written" || { echo "FAIL no hub-cert.pem"; FAIL=1; }

echo "--- hub log ---"; tail -3 "$T/out.log"
[ $FAIL == 0 ] && echo "ALL PASSED" || { echo "SOME FAILED"; exit 1; }
