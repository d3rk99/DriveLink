# Travel cost test (Research/travel-cost-design.md)

## 2026-10-19 14:32-14:40, build 3268189, Chernarus (refuse mode, 80 km zone, time off) -> livonia2 -> Chernarus, OffroadHatchback
| Check | Result |
|---|---|
| Price on departure | 80 km: 32% food, 40% water each, 40 L fuel |
| Eat on the road | PASS: ate 2 mushrooms + apple + WaterBottle 545 ml + 1207 ml water from a CanisterGasoline (radiator water from the debug kit, counted as a drink, not fuel); bars paid 1253 food / 0 water |
| Car charged | PASS: fuel 42 -> 2 L, engine 100 -> 84%, 5 wheels -24% (incl. the mounted spare) |
| Remote server on an older build | kicked: "Client has a more recent version (@DriveLink)"; trip stayed safe in the hub; after the update it arrived |
| Arrival keeps the charges (back on Chernarus) | PASS: 33 items (36 - 3 eaten foods), stomach 0, fuel 0.042 (about 2 L), engine running |

## 2026-10-19 14:42-14:54, same build: refusal, supplies only, travel time
| Check | Result |
|---|---|
| Refuse when short (Chernarus, 80 km) | PASS 14:42: "car: fuel 21 L of 40 L (tank + gasoline canisters)", no countdown |
| Paid from carried supplies only | PASS 14:44: 2 canteens + 2 2/3 marmalade jars, bars 0/0; car fuel 42 -> 2 L |
| Weakened mode, short of fuel (Livonia, 10 km, 5 L) | PASS 14:46: tank 2 -> 0 L + 3 L from a gasoline canister |
| Travel time (Livonia rules: on, 100 km/h, 10 km = 6 min) | PASS: "On the road to Chernarus: about 6 min" in the main menu; hub "on the road 360 s" |
| Early login refused, on the SOURCE too | PASS 14:47 (Livonia, 294 s left) and 14:50 (2 min left): back to the menu with the time left, placeholder character removed |
| After the time | PASS 14:53: Livonia -> redirect -> Chernarus built 30/30 items, car 29/29, fuel 0 (engine doesn't start: correct, empty tank) |
Not tested yet: the spare-wheel rule, a crew on the road together, the Players page "Arrive now" in game.

## Two-player crew test 2026-10-19/20 (Derk + OrionKaelin), build 6e97efb
| Check | Result |
|---|---|
| Crew of 2, weakened road, car lost (23:58) | PASS: 2 flats fixed with spares by different crew members, 3rd = car left; both walked 60 km, arrived on foot rationed (health 67 / 73) |
| Crew of 2 + travel time (00:05) | PASS: both "on the road 6 min"; passenger's early login refused (5 min left) |
| Crew waiting room 2/2 after the time (00:07) | PASS: Offroad_02 built 28/28, driver seat 0 + passenger seat 1, engine running |
| Travel time on foot (10 km at 5 km/h = 2 h) | PASS: refusals with time left; Players page Arrive now |
| Story grammar | FIX: solo "You was attacked" -> "You were" (next build) |
