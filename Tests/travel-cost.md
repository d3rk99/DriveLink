# Travel cost test (Research/travel-cost-design.md)

## 2026-10-19 14:32-14:40, build 3268189, Chernarus (refuse mode, 80 km zone, time off) -> livonia2 -> Chernarus, OffroadHatchback
| Check | Result |
|---|---|
| Price on departure | 80 km: 32% food, 40% water each, 40 L fuel |
| Eat on the road | PASS: ate 2 mushrooms + apple + WaterBottle 545 ml + 1207 ml water from a CanisterGasoline (radiator water from the debug kit, counted as a drink, not fuel); bars paid 1253 food / 0 water |
| Car charged | PASS: fuel 42 -> 2 L, engine 100 -> 84%, 5 wheels -24% (incl. the mounted spare) |
| Remote server on an older build | kicked: "Client has a more recent version (@DriveLink)"; trip stayed safe in the hub; after the update it arrived |
| Arrival keeps the charges (back on Chernarus) | PASS: 33 items (36 - 3 eaten foods), stomach 0, fuel 0.042 (about 2 L), engine running |
Not tested yet: refusal message (short on supplies), spare-wheel rule, travel time (time was off on Chernarus).
