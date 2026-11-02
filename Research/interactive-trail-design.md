# Interactive trail (design, 2026-11-01)

The road film becomes an Oregon Trail style trip that plays on the **source** server before the party disconnects.
The party votes on choices that change trip time, members, cars and gear. Status: first version built
(2026-11-01), see "Built so far" at the end.

## Server owner settings

| Setting | Values | Default |
|---|---|---|
| `filmMode` | `interactive` (votes) · `watch` (same film, choices resolve themselves) · `off` | `interactive` |
| `readySeconds` | ready-up window | 60 |
| `voteSeconds` | time to lock in a choice | 30 |
| `maxChoices` | cap on choices per trip | 5 |
| Hub travel mode (existing) | travel time · instant arrival | as set on the hub |

In `watch` mode each choice picks the option the party's items suit best.

## Flow

1. **Zone entry.** Everyone in the zone sees the same resources table, refused or not. A refused player sees the reason and can still ready up ("travel anyway"). The party is everyone in the zone; a solo player is a party of one.
2. **Ready-up.** Everyone sees everyone's ready state. If anyone isn't ready when `readySeconds` runs out, nobody travels, and the party must leave the zone and come back.
3. **Reserve.** The hub reserves slots on the target before anyone moves. If that fails, the refusal shows now, not after the film.
4. **Staging.** Screens fade to black and the party and its cars are moved to a hidden staging spot set per map. There they are frozen and invulnerable, with hunger and thirst paused and other players hidden. Two convoys at once get separate spots.
5. **Film.** All the logic runs on the server, and clients only draw what it sends, so everyone sees the same film. In a loop:
   - a chapter of general events (about 30 s, condensed as now);
   - a choice card with up to `voteSeconds` to vote;
   - the outcome is rolled and applied, and a short outcome scene plays (about 6 s).
6. **Apply.** The party is still on the source server, so changes go to the real characters before the snapshot: used items are removed, wounds applied, lost cars deleted and deaths carried out.
7. **End.** The hub settles the trip (time, or instant arrival). Everyone sees an Oregon Trail style summary card ("Arrived in 2 h 40 m. Lost: the sedan. Mika is wounded."), and then the party disconnects. The arrival black screen shows the summary only; the film no longer plays there.

Choice count scales with distance: 1 under 20 km, up to `maxChoices` on long trips (5 choices take about 5 to 6 minutes).

## Voting

- Each card has 2 to 4 options plus **Turn back**. Each voter's name shows on their pick. Votes can change until locked in, and the vote ends early once everyone has locked in.
- **Winner:**
  - the option with the most votes wins;
  - a tie goes to the lead driver's pick;
  - if nobody votes, the careful option wins (never Turn back).
- **Turn back as a group:** needs more than half the party.
- **Hints:** each option shows a risk hint (safe / risky / dangerous), and item lines show what helps, e.g. "Epoxy putty (Derk)". The exact odds stay hidden.

## Turning back alone (any time a card is up)

- **Any player** can press "Turn back alone". A confirmation shows the risk first ("You'll be on your own. Are you sure?").
- **A driver** turns back with their car. The return trip is rolled for risk: fuel, damage and, rarely, an attack.
- **A non-driver walking back** faces a higher risk roll: wounds, lost items, or death. The roll grows the further into the trip they are.
- **A whole car crew** can vote among themselves to split from the convoy and go back together, car included.
- **If the driver leaves,** each passenger chooses to go back with them or stay with the convoy. Those who stay take empty seats in the other cars; if there are none, they walk with the convoy. A walking member speeds up walking events and slows the convoy.
- **When they arrive back,** they are put at the zone exit as they are: anything used stays used, damage stays, fuel burned so far is gone.
- **Disconnecting mid-film** counts as turning back alone, with no risk roll: the player is put back at the zone.

## Death

- A member killed in the film **really dies**. They respawn fresh on the source server, or the starting server. Their gear is lost: it does not go to the rest of the convoy.
- DriveLink only carries out the death. Server rules on top (bans, cooldowns, such as Derk's server via DeathWatcher) stay the owner's own.
- **Instant arrival hub:** options that would add time instead carry a higher risk of death or loss. The trip is short in time but more dangerous.

## Items

Passive item effects, applied without asking:

| Item | Effect |
|---|---|
| GPS or compass | Trip time about −10 to 15 % (instant arrival: lower risk instead). Adds a sure option to "lost your way" cards |
| Epoxy putty or duct tape | Radiator card: "Patch it" always works and uses the item |
| Rifle + ammo + knife | Hunting: more food, costs time, small risk of wounds |
| Fishing rod or hooks | Fishing near water: safer but slower than hunting |
| Matches, lighter or hand drill kit | Meat and fish are cooked. Without one, food is eaten raw: a chance of sickness that slows the party. Charcoal or tetracycline cures it |
| Rope | Tow a broken car; safer river rafting |
| Shovel | Dig out of mud or snow |
| Bandage or rags | A deadly wound becomes a light one |
| Gas mask | Cross a toxic zone |
| Headtorch or working headlights | Keep moving at night |

No prompts tell players to pack these. The cards show which items helped and who carried them.

## Cards (first set; more to come)

| Card | Options (plus Turn back) |
|---|---|
| River crossing | Ford it (risk to car or items) · Find a bridge (+time) · Raft across on foot (rope helps) |
| Bandit roadblock | Pay the toll (lose food or ammo) · Fight (weapons matter, risk of wounds) · Detour (+km, +fuel) |
| Horde on the road | Plow through (car damage) · Sneak around on foot · Wait it out (+time) |
| Fork in the road | Left · Right · Compass or GPS if carried (wrong way: +km) |
| Radiator boiling | Patch it (epoxy or duct tape) · Let it cool (+time) · Push on (risk the engine) |
| Deer spotted / a lake | Hunt or fish (gear needed, +time, +food) · Keep going |
| Stranger asks to join | Take them (eats food; might find things or might steal) · Refuse · Rob them |
| Wounded survivor | Help (uses a bandage; they tell you a shortcut, −time) · Leave them |
| Heli crash smoke | Investigate (risky: infected, military loot) · Ignore |
| Stuck in mud or snow | Dig (shovel) · Push (+time) · Tow (rope + a second car) |
| Party member sick | Rest (+time) · Medicine (charcoal, tetracycline) · Push on (they may die) |
| Night falls | Drive on (lights needed, risk) · Camp (+time; a fire with matches makes it safer) |
| Blizzard (snow) | Shelter (+time) · Push on (cold, risk of getting lost) |
| Toxic zone (Sakhal, Namalsk) | Gas masks on · Long detour |
| Out of fuel near a town | Send a scout (risk losing them, gain fuel) · Siphon from wrecks (+time) · Walk |
| Collapsed bridge | Go around (+km) · Leave the car, cross on foot |

Which cards come up depends on the biome, whether the party is driving or walking, and what it carries. Each option has hidden odds; items, weapons, party size and car count push them up or down.

## Safety cases

- **Login inside a staging area** (after a server restart): move the player back to the zone exit.
- **Hub down at the end:** everyone goes back to the zone and nothing is applied.
- **The last member leaves** (turns back or disconnects): the trip ends and nothing goes to the hub.

## Build order (draft)

1. Ready-up table for the whole zone and the 60 s timer.
2. Staging area: teleport, freeze, restore, and recovery on restart.
3. Server-driven film in chapters; clients render the chapters they're sent (reuses `DL_FilmPlayer`).
4. Choice cards and the voting UI and RPCs, plus Turn back as a group.
5. Outcomes applied to the real characters, the summary card and the hub settlement.
6. Turning back alone, car crews splitting off, re-seating passengers.
7. Item effects and the full card set; `watch` mode.

## Built so far (2026-11-01)

| Part | Where |
|---|---|
| Cards as data, the rules (draw, hints, votes, outcomes, items, summary), chapters | `Scripts/3_Game/DriveLink/DL_Trail.c` |
| The card, the votes, the summary on screen | `Scripts/3_Game/DriveLink/DL_TrailView.c` |
| The film in chapters: it rolls on while a card is up (`Append`, `SetHold`, `note` scenes) | `DL_FilmPlayer.c`, `DL_FilmData.c` |
| Ready-up, freezing, the trip on the server, turning back, deaths, departure | `Scripts/5_Mission/DriveLink/DL_MissionServer.c` (trail section) |
| The trail's results into the snapshots | `DL_Journey.AfterTrail` |
| Client: ready table, film, card menu, RPCs | `Scripts/5_Mission/DriveLink/DL_TrailClient.c` |
| DriveLinkRestart: the client-switch hold also runs before the trail | `Addons/DriveLinkRestart/.../dt_server.c` (`DL_TrailGo`) |
| Dev tests: `-dlTrailTest` (offline, made-up voters), `-dlTrailAuto` + debug.json `trailTest` (live) | `DL_TrailTest.c`, `DL_MissionServerTrailTest.c` |

Settings live in the server's `config.json`, section `trail` (written in on first start): `filmMode`, `readySeconds`,
`voteSeconds`, `maxChoices`, `instantRiskPerHour`, `stagingPos`. Cards: `$profile:DriveLink/cards.json` (15 default
cards, written on first use; `off: true` turns one off). Not in the hub web app yet.

Added the same day:

- Cars move to `stagingPos` with everyone in them (one beside the other, 8 m apart; another party at the same time
  gets its own row 40 m on) and come back to where they stood when they turn back or the hub refuses. A restart
  puts them back too (`trail_cars.json`).
- A passenger can turn back alone (he gets out, walks back, the same risk as on foot). When a driver turns back,
  each passenger gets his own question for 20 s: go back with him, or stay (a free seat in another car, else on
  foot). The get-out runs on the passenger's own client (`RPC_TRAIL_GETOUT`).
- The ready table shows the hub's survival odds (yours, each person's, each vehicle's); asked again when someone
  joins or leaves.
- Before the film the hub is asked whether it would take every one of them (`/api/precheck`, hub build after
  0.5.1); a refusal shows now. An older hub without it: the trail starts as before.
- Hunger and thirst don't tick while they watch.
- A car lost on the trail: the rest of the way counts at walking speed for the time on the road.

## Card scenes (animations)

While the party votes, the film stops and sets the card's scene on the road ahead (the cars move up the window so
it shows above the card). After the vote the outcome plays out before its line, then the road goes on. Both are
plain text in `cards.json`, so a new card can reuse them without a new build:

- **`scene`** on a card: words, any mix: `barrier`, `bandits:N`, `horde:N`, `river`, `steam` (from the front car's
  bonnet), `deer`, `stranger`, `survivor`, `fork` (a side road and a signpost), `lake`, `helicrash` (a burning wreck),
  `mud` (under the front car, wheels spinning), `sick` (the card's member crouched with a fever bubble), `night`,
  `blizzard`, `gas`.
- **`anim`** on an outcome: steps that run one after another. `{who}` of the outcome is the one who acts.

| Step | What happens |
|---|---|
| `out` / `all` | he gets out / everyone gets out |
| `go:npc` `go:deer` `go:front` | he walks to the stranger/bandit, the deer, the car's bonnet |
| `shoot:npc` `shoot:deer` `shoot:zed` | he aims and fires; the target falls |
| `miss:deer` | he fires, the deer runs off |
| `aim:npc` | he raises his weapon |
| `npcfire` / `npcflee` | the bandits (or the stranger) shoot at him / run off |
| `hit` | he blinks: hurt |
| `work:S` | he crouches and works for S seconds |
| `fix` | he walks to the bonnet and fixes it; the steam stops |
| `fire` | a campfire with meat on it beside him |
| `carry` / `drop` | he carries a crate / puts it down (the toll) |
| `join` | the stranger walks to the car and gets in |
| `back` | everyone back into the cars (walkers to their places) |
| `drive` / `slow` / `stop` | the road goes on at speed / walking pace / stops |
| `plow` | drive on; the infected in the car's way go down |
| `aside` / `pass` / `zed` | the horde steps off the road / walks off / one comes for him |
| `ford` / `stall` / `wade` / `splash` | through the river by car / stuck in it / on foot / a splash |
| `steam:off` `steam:black` | the steam stops / black smoke (a dead engine) |
| `clear` | the stage fades away (a detour) |
| `go:lake` `go:wreck` `go:sick` | to the lake's shore, the wreck, the sick member |
| `fish:S` | he fishes for S seconds; a fish comes up |
| `zeds:N` | N infected turn up (by the wreck, else beside him) |
| `push` / `push:fail` | everyone pushes the front car out of the mud / it won't move |
| `unstick` / `cure` | the wheels stop spinning / the fever is gone |
| `think` `alert` `ill` | a "?", "!" or fever bubble over him (or over the car) |
| `mask` | everyone out puts a gas mask on |
| `lights` / `dawn` / `calm` | headlights on / the night lifts / the blizzard dies down |
| `wait:S` | a pause |

Every default card has its scene and an animation for each outcome. An older `cards.json` is upgraded on start
(version 3): the default cards it still has get their scenes; your own cards and edited texts are left alone. Art:
`Tools/anim_trail.py` (deer, barrier, sandbags, campfire, steam, splash, meat, the river, signpost, side road, lake,
helicopter wreck, mud, headlights, gas clouds, fishing rod, fish, bubbles, blowing snow).

Still not done:

- Hunting or fishing food and sickness change the snapshot's food, water and health only (no real meat items, no
  sickness agent yet).
- Arrival shows the summary text on the black screen, no film.
- The settings and cards are not in the hub web app.

With travel time on, the main menu's road scene shows the party as the trail left it (lost cars gone, their crews
walking or in a convoy car that had room, the dead and the ones who turned back left out) and the summary under the
"on the road" banner (`DL_MenuScene.title` / `lines`, `DL_MainMenu.DL_ShowSummary`).

Tested 2026-11-01: offline (`-dlTrailTest`, two trips, all 15 cards drawn over several runs, no script errors) and live
on TestServer (Chernarus -> Livonia, one player on foot, 1 km: ready-up, one card, vote, departure with 180 s on the
road, main menu scene + summary). Then with a car: Livonia -> Chernarus (precheck, car to the staging spot, the car
lost on the card, the menu scene shows him walking with "Lost: the Ada 4x4") and Chernarus (driver turns back alone,
the car comes back from the staging spot with him in it). Not tested live yet (needs a second player): two or more
players voting, convoys, a passenger's choice or turn back, a death on the trail.
