# Changelog

## Mod 0.5 (2026-10-30)

### New
- **Vehicle economy** (server setting `carEconomy`, default `"off"`). A car keeps the slot on the map it came
  from while it is away, so moving cars between servers no longer adds cars to the network.
  - `"track"`: the Hub records every travelled car (its home map, the map it is on, its trips).
  - `"veto"`: tracking plus the home map's Central Economy skips a respawn while that car is still around
    somewhere else.
- **Travelled cars on the Hub's Map page** (needs the updated Hub). Shows where each travelled car was last
  reported, refreshed every minute. Blue cars are on their home map, yellow ones are visiting. Click a car for
  its home server, health, fuel and its last 10 trips.

### Fixed
- A vehicle that arrived with a trip could vanish about 30 s later when no player was near: the game's cleanup
  treated it as expired. Arriving vehicles now get a 45-day lifetime.

### Updating
- Update the Hub as well to get the car map (database migration 008 runs on its first start).
- Servers on 0.4.9.2 still connect to the updated Hub. They just don't report cars.
