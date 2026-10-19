# Travel cost + travel time: design (agreed with Derk 2026-10-19, no code yet)

Long trips between servers cost food, water, fuel and car wear, and on hardcore servers they take real time.
Everything is OFF by default (distance 0 = today's free, instant trip) and is set in the hub web app.

## 1. Where it is set
- **Per leaving zone (Map page): distance in km** (0 = free and instant). One field next to the zone's target.
- **Per server (new "Travel" section on the Servers page)**: the rules for trips that START on that server:

| Setting | Default | Meaning |
|---|---|---|
| Shortfall mode | Refuse | **Refuse**: not enough = trip doesn't start. **Arrive weakened**: trip goes, the road decides (section 4) |
| Food / water on foot | 15% / 15% per 10 km | % of a full stomach / thirst bar (DayZ max 5000 each; walking burns ~7%/h) |
| Food / water by car | 4% / 5% per 10 km | per person in the car |
| Fuel | 5 L per 10 km | from the car's tank |
| Car wear | engine 2%, wheels 3% per 10 km | health taken from the engine and every wheel |
| Travel time | off | on = the character is on the road (section 3) |
| Speed on foot / by car | 5 / 40 km/h | travel time = distance / speed (real minutes) |
| Road events (weakened mode only) | see section 4 | chances for luck and danger |

## 2. Cost check + charge (source server, when the countdown starts and again at departure)
- Entering a zone shows the price: "Livonia, 60 km: 24% food, 30% water, 30 L fuel, about 1 h 30 min".
- **Refuse mode**: short on anything means no countdown. The message lists what is missing, per person for a crew
  ("Britney: water 12% of 30%"). Nothing is spent.
- **Charge = edit the snapshot before it leaves**: food/water/fuel/wear come off the travelling copy. A trip that fails
  (hub down, refused) costs nothing, because the player keeps the character he has.
- Crew: each person pays their own food and water; the car pays fuel and wear. One short person in refuse mode
  stops the whole car.

## 3. Travel time ("on the road")
- The source works out the trip time (car part at car speed, walked part at walking speed) and the hub stores
  **ready at** on the trip. The whole crew gets the same time.
- The player goes to the main menu with the arrival black screen text: "On the road to Livonia: 1 h 30 min".
- **Joining ANY DriveLink server before ready** is refused with a dialog: "Your convoy reaches Livonia in 47 min".
  This works on every server because each one asks the hub at login. It is the "pseudo-ban".
- After ready: join the target as now. The crew waiting room works as today. Anyone not in within 120 s is left
  behind; the rest go on. A latecomer arrives on foot at the arrival point.
- Admin: the Players page shows "on the road, 47 min left" with **Arrive now** (and the existing send / drop).

## 4. "Arrive weakened" mode: the road decides (Oregon Trail style)
The source server plays the journey once for the whole crew, with random rolls. The result (who pays what, what
happened, and the story text) goes into each traveller's snapshot. The target shows the story on the black screen.

| Shortfall | What can happen (chances set per server) |
|---|---|
| Food or water short | Rationing: arrive with 0 food/water and the hungry/thirsty states. A big shortfall also hurts health and blood. Past a limit there is a chance to **die on the road** |
| Fuel short (car) | Chance the car is **lost** = the share of the route left without fuel, minus a **luck** roll ("found a gas station"). Lost: the crew walks the rest (more food/water, more time at walking speed). Kept: the car arrives with almost no fuel |
| Walking parts | Per 10 km walked, a small chance of an **infected attack**: bleeding/injury, rarely death |
| Car wear | Always applied. A part worn to ruined stays ruined on arrival |

- **Death on the road = character and gear are LOST.** He spawns fresh on the target (a new character), and the black
  screen tells how he died. The hub marks the trip "died on the road" for the Players page.
- **Story lines** use player names, e.g.
  "Ran out of gas on the road. Luckily, Britney found a gas station."
  "Food ran low: everyone had to ration." (arrive with no food, hungry)
  "Ran out of gas on the road, had to abandon the vehicle. Britney died from a bite."
- **Refuse mode has no random events.** It only charges the costs and the wear.

## 5. Who does what
- **Hub**: stores the zone distances and per-server travel rules (sent to the servers with the layout, live
  reload), stores the trip's ready-at and journey result, refuses early logins, and runs the Players page.
- **Source DayZ server**: shows the price, checks it, plays the journey for the crew, edits the snapshots, and
  sends the travel time.
- **Target DayZ server**: shows the story on the waiting-room black screen, builds a dead traveller as a fresh
  spawn, and builds a lost car as no car.
- **Old snapshots** (no journey) arrive as today.

## 6. Build order (once this is approved)
1. Zone distance + per-server rules in the hub and web app (stored, sent to the servers, not used yet).
2. Refuse mode: price on the countdown, check, charge food/water/fuel/wear in the snapshot.
3. Travel time: ready-at, the login refusal dialog on every server, Arrive now on the Players page.
4. Weakened mode: journey rolls, story text on the black screen, death and lost car.

## Still to tune by playing
The default numbers, the event chances, the wear levels, and how often the luck rolls should win.
