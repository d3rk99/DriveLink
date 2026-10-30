# DriveLink Hub - next release (not released yet)

## Hub updates

- The Hub checks GitHub releases every 6 hours and shows a newer version on the Setup page ("Hub updates", plus an
  "update" mark on the Setup tab). hub.json `autoUpdate`: `"notify"` (default), `"auto"` or `"off"`.
- **Update now** downloads `DriveLinkHub.exe`, checks it against `SHA256SUMS.txt`, runs it once to confirm its
  version, keeps the old exe as `DriveLinkHub.old.exe` and restarts into the new one (console or Windows service).
- `"auto"` installs updates of the same major.minor that speak the same protocol by itself, once no trip started or
  arrived for 5 minutes. Bigger versions are only shown: they go with a mod update.
- If the new version does not start, the old one is put back (the failed one stays as `DriveLinkHub.failed.exe`)
  and that version is never installed by itself again.
- `DriveLinkHub.exe version` prints the version and protocol.

Tested on this PC with a local release server, in a console and as the Windows service: 0.5.0 -> 0.5.1 by itself
(about 1 s down), a broken 0.5.2 rolled back (console: within a second; service: about 30 s, while it waits to see
whether the new one keeps running) and was not retried, and a bad checksum was refused.

# DriveLink Hub 0.5.0

Goes with DriveLink mod 0.5. Older protocol-1 mods still connect; they just don't report cars.

## Travelled cars and the vehicle economy

- New ledger of travelled cars (`POST /api/cars`, reported by each server every minute). Each car keeps a home
  server, so the home map can count it while it is away (mod setting `carEconomy`: `"off"`, `"track"` or `"veto"`).
- Map page: a "Travelled cars" layer shows where each travelled car was last reported. Blue = on its home map,
  yellow = visiting. Click a car for its home server, health, fuel and its last 10 trips.
- Console: `DriveLinkHub.exe cars` lists the ledger.
- Database migrations 007 (cars) and 008 (car positions and trips) run on the first start.

## Upgrade

Stop the Hub, back up its folder and replace the executable while preserving `hub.json`, the database,
certificates and custom maps. Update the DriveLink mod on the servers to 0.5 for the car features.

## Validation

22 Rust tests passed. Two local Chernarus servers: 4 cars sent A -> B, restarts, wrecks and returns, with the network
car total exact in every step, plus one player round trip A -> B -> A by car. Boats and motorbikes are not tested.

# DriveLink Hub 0.4.9-rc1

Release candidate for public testing. Automatic client switching requires the matching DriveLink mod 0.4.9-rc1 or newer, in addition to this Hub.

## Automatic client switching

- Query each server's advertised client mods through its UDP Steam query port.
- Compare client mod identities, ordering and advertised builds to choose direct reconnect or helper restart automatically.
- Exclude server-only mods, signing-key lists and official map/DLC flags from the comparison.
- Distribute policies in hello and layout replies, including replies where the map layout has not changed.
- Show query status and directional Automatic / Keep client / Restart controls in the server editor.
- Hold departure when a report is missing, malformed or stale, instead of incorrectly requesting helper setup.
- Identify conflicting advertised builds of shared mods as a server update problem.

## Arrival editor changes since the last GitHub Hub release

- Optional separate staging positions/directions for land vehicles and boats.
- Staging and convoy spacing controls, with bounded saved values.
- Preserve staging settings through layout saves and server polling. Existing arrivals default to staging disabled.

## Upgrade

Stop the Hub, back up its folder and replace the executable while preserving `hub.json`, the database, certificates and custom maps. Update the DriveLink mod on all participating servers and clients for automatic switching. No manual `routes.json` maintenance is needed with the updated Hub and mod.

Older protocol-1 mods can connect but do not consume the automatic helper policy. The Windows helper binaries are unchanged at 0.4.8-rc3; existing installations of that helper do not need reinstalling for this release.

## Validation and limits

15 Rust tests, 16 isolated Hub API checks and 22 native DayZ script assertions passed during candidate preparation. Checks include matching/different lists, server-only exclusion, stale reports, override persistence and query recovery. The owner reported successful live tests with this Hub; the running Workshop mod was 0.4.8-rc3, so that report is not a complete end-to-end validation of the new 0.4.9 mod policy path.

The parser supports IPv4 DayZ browser protocol v2. Unsupported/compressed or unreachable query reports produce an unknown policy with an explanation. Advertised mod hashes are compatibility hints, not cryptographic file verification. Larger public-network tests remain part of this release candidate.

Protocol reference: [WoozyMasta A2S documentation](https://woozymasta.github.io/a2s/pkg/a3sb/docs/). The Hub parser is an independent implementation with no additional runtime dependency.
