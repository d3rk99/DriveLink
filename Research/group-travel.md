# Group travel: groups on foot and convoys

Derk, 2026-10-20: besides a solo traveller and a crew in one car, players who enter a zone together travel together.

## Forming (source server, `DL_MissionServer.DL_CheckZones` / `DL_TendParties`)

- A **party** forms in a zone. The first walker, or the first driver of a car, starts the countdown.
- Anyone of the same kind who comes into the same zone while it runs joins the party, and **the countdown starts again**:
  - groups on foot take up to 8 walkers;
  - convoys take up to 4 cars, each with its whole crew.
- The countdown notification says "group of N" or "convoy of N cars".
- Leaving the zone, getting out of the car, or switching between foot and car drops you out of the party. The others keep the time they had left.
- When the countdown runs out with a single unit, it is the normal trip, unchanged.
- In refuse mode each unit must pay its own way. A unit that can't stays behind and is told so; the others go.

## The road (`DL_Journey.RunParty`, weakened mode)

- One journey for the whole party, played in 10 km legs.
- **Fuel:** a car that runs dry first takes jerry cans from the other cars' trunks and the other travellers' gear. Then comes the luck roll, then the car is left.
- **Flats:** the spare must fit that car's wheels. The search order is:
  1. that car's spare slot and trunk;
  2. its crew;
  3. the other cars;
  4. the other travellers.

  A repair kit can come from anyone in the party.
- **A car left behind:** its crew climb into free seats of the remaining cars. Whoever doesn't fit walks.
- **Pace:** if anyone walks, the whole party goes at walking pace. Travel time counts those legs as walked.
- **Food and water:** each person eats from their own gear first, then from the cars' trunks, their own car's first.
- **Infected:** only people on foot get bitten.
- **Dead driver:** another occupant takes the wheel. If there is none, a walker climbs in. If nobody is left, the car stays.

## Arrival (target server)

- One waiting room per car, as for a crew today. Cars park one behind the other, 10 m apart (`convoyIdx`).
- A group on foot arrives side by side, 1.5 m apart (`partySlot`).
- Everyone carries the same travel time and the same film.

## The film (`DL_FilmPlayer`)

- `DL_Film.cars` lists every car's look. `DL_FilmActor.car` says which car each person starts in.
- Event fields:
  - `car`: the car the event is about;
  - `donor`: the car a spare or jerry can comes from.
- New event types:
  - `move`: a person changes car, or walks on;
  - `fuel` with detail `shared`: a jerry can from another car.
- A convoy gets a tall window (80 % of the screen height), with the cars in a line.
- During each car's event the others wait. A spare or can is carried over from the donor car.
- When a car is left behind, the line closes up. People on foot walk beside the convoy on the left lane.
- At the end every car drives out of the bottom, and walkers walk out.
- F9 cycles through five demos: crew, solo, truck, convoy, group on foot.
