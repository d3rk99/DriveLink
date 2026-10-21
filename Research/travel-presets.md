# Travel presets: vehicle fuel, food and water (2026-10-20)

The hub's Travel settings have four difficulty presets (Servers page > Travel). This note is where their numbers come from.

## 1. Vehicles: in-game vs real

In-game values are from the DayZ 1.30 PBOs (`vehicles_wheeled`, `vehicles_singletrack`, `vehicles_water`, config.bin
converted with CfgConvert). `fuelConsumption` is the config value; for every car it is the real model's published
L/100 km, so DriveLink reads it as **litres per 100 km**. (The engine's real burn in game also depends on RPM and
throttle; not measured in game.)

| In game | Class | Tank (L) | Game L/100 km | Real model | Real tank (L) | Real L/100 km | Game range | Real range |
|---|---|---|---|---|---|---|---|---|
| Ada 4x4 | OffroadHatchback | 42 | 11 | Lada Niva 1.6 | 42 | 10-12 | 380 km | 380 km |
| Olga 24 | CivilianSedan | 55 | 13 | GAZ-24 Volga | 55 | 13-14 | 420 km | 410 km |
| Gunter 2 | Hatchback_02 | 55 | 6.5 | VW Golf Mk2 | 55 | 6.5-8 | 850 km | 750 km |
| Sarka 120 | Sedan_02 | 38 | 7 | Skoda 120 | 38 | 7-8 | 540 km | 510 km |
| M3S | Truck_01 | 120 | 30 | Praga V3S | 120 | 28-32 | 400 km | 400 km |
| M1025 | Offroad_02 | 115 | 30 | AM General M1025 HMMWV | 95 | 24-30 (8-10 mpg) | 380 km | 350 km |
| Moped | Motorbike_01 | 3.5 | 1.8 | 50 cc moped | 3-5 | 2-2.5 | 190 km | 160 km |
| Dirt bike | Motorbike_02 | 13.5 | 3.5 | 250-400 cc enduro | 9-14 | 4-5 | 390 km | 270 km |
| Rubber boat | Boat_01 | 25 | 5.5 | 4-5 m RIB, 25-40 hp outboard | 25 | 25-35 (8-12 L/h at 30-40 km/h) | 450 km | 80 km |

Reading: the cars and motorbikes are close to real life. **The boat is the outlier**: real outboards burn about 5x what
the game says. Hence `fuelBoatScale` (Realism 5, Hard 2).

Containers: jerry can (CanisterGasoline) 20 L. Old DriveLink default (flat 5 L per 10 km, motorbikes 0.4, boats 1.0):
a boat ran 50 km on its 25 L tank; 100 km = tank + 1.25 cans.

100 km by preset (per-vehicle fuel):

| | Ada | Olga | Gunter | Sarka | M3S | M1025 | Moped | Dirt bike | Boat |
|---|---|---|---|---|---|---|---|---|---|
| Medium (x0.5) | 5.5 L | 6.5 | 3.3 | 3.5 | 15 | 15 | 0.9 | 1.8 | 2.8 |
| Hard (x1, boats x2) | 11 L | 13 | 6.5 | 7 | 30 | 30 | 1.8 | 3.5 | 11 |
| Realism (x1, boats x5) | 11 L | 13 | 6.5 | 7 | 30 | 30 | 1.8 | 3.5 | 27.5 (tank + a can) |

## 2. Food and water

DayZ bars: energy max 5000 (the game calls it kcal), water max 5000 (ml). `PlayerConstants` burn per second:
basal 0.01, walk 0.1, jog 0.3, sprint 0.6 (energy and water alike).

Per 10 km, % of a full bar:

| | Game: walking (~1.6 m/s, 1.7 h) | Game: jogging (~5 m/s, 33 min) | Game: driving 60 km/h (10 min) | Real: hiking with a pack (2 h) | Real: sitting in a car |
|---|---|---|---|---|---|
| Food | 13.7 % (690) | 12.4 % (620) | 0.1 % (6) | ~19 % (~950 kcal: ~80 kcal/km + basal) | ~0.4 % (~100 kcal/h) |
| Water | 13.7 % | 12.4 % | 0.1 % | ~30 % (0.5-1 L/h, ~1.5 L) | ~0.4 % (~0.1 L/h) |

Swimming (real): ~500-700 kcal/h at 2-3 km/h, so 10 km is ~4 h and ~2400 kcal, about 2.5x walking it; you sweat
less in water than walking, so food and water together are taken as ~2x (`swimFoodX`). Open-water swims of 10+ km
are marathon-swimmer territory: drowning risk grows with every 10 km (`drownPct`, x1, x2, x3 ...).

Settings are in kcal and ml per 10 km (since 2026-10-20; older % settings were converted at x50). In items: DayZ
nutrition is per 100 g, so a 400 g can of baked beans = 800 kcal (+100 ml water), tactical bacon 1200 kcal, sardines
~250 kcal; a water bottle or canteen = 1000 ml. 40 km on foot under Realism = 3800 kcal (~5 cans) + 6 L (6 bottles).

## 3. The presets

| Setting | Easy | Medium | Hard | Realism |
|---|---|---|---|---|
| When short | refuse | refuse | arrive weakened | arrive weakened |
| Travel time | off | off | on | on |
| Speed foot / vehicle (km/h) | - | - | 10 / 50 | 5 / 50 |
| Food / water on foot (per 10 km) | 0 / 0 | 300 kcal / 300 ml | 650 kcal / 650 ml (game, jogging) | 950 kcal / 1500 ml (real hiker) |
| Food / water in a vehicle | 0 / 0 | 25 kcal / 25 ml | 50 kcal / 50 ml | 20 kcal / 20 ml (real) |
| Fuel | none | per vehicle x0.5 | per vehicle x1, boats x2 | per vehicle x1, boats x5 (real) |
| Engine / wheel wear (%/10 km) | 0 / 0 | 1 / 1 | 2 / 3 | 3 / 4 |
| Find fuel when out | 100 % | 50 % | 25 % | 10 % |
| Infected attack / kills | 0 / 0 | 3 / 0 | 5 / 10 | 8 / 15 |
| Starvation kills | 0 | 0 | 25 | 40 |
| Flat tyre (%/10 km) | 0 | 1 | 3 | 4 |
| Swimming food x / drowning % | 1 / 0 | 1.5 / 2 | 2 / 6 | 2 / 10 |

Medium and Easy refuse a trip the player can't pay, so the road chances (attacks, flats, drowning) don't come into play
there; the numbers are set for a server that switches Medium to "arrive weakened".
Wear and road risks are gameplay, not statistics. Realism's speeds: walking pace and Chernarus country roads.
