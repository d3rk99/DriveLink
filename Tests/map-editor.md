# Map editor + live layout test

Zones/arrival points drawn in the hub's /admin editor (y = 0, the mod puts them on the terrain), servers poll /api/layout every 30 s.

## Results 2026-10-19 (Derk, OffroadHatchback, 102504-byte build, commit d3d74d9)
| Check | Result |
|---|---|
| Servers load the hub layout at hello (chernarus rev 19, livonia rev 5) | PASS |
| Editor save picked up with no restart (chernarus rev 20) | PASS: 30 s later |
| Chernarus -> Livonia through an editor zone (01:45) | PASS: 36/36 items, car 29/29, fluids exact, seated, arrival height from the terrain (192.5) |
| Livonia -> Chernarus (01:46), Chernarus -> Livonia (01:55) | PASS: same, health carried (81) |
| Zone moved in the editor while in game (livonia rev 6, 02:00) | PASS: picked up in < 30 s, drove through the new spot (02:01) -> Chernarus 36/36, car 29/29, seated |
| Source car removed once seats empty | PASS every trip (13 s), no crash |
| SCRIPT (E) lines | none |

Notes: the hub's payload size can differ by a few bytes from the depart size (numfix rewrites numbers); all items still arrive.
Keep arrival points well outside their own server's leaving zone (Chernarus arrival_1 is ~11 m from the "To Livonia" edge).
