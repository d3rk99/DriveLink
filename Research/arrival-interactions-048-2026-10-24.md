# 0.4.8 arrival interaction investigation — 2026-10-24

Player report on 0.4.7: restored truck shows no fuel; doors/refuel/exit unavailable, seat switching still available. Server logs record successful final-point construction, inventory restoration and seating. Numeric root/seat agreement alone does not prove native client interaction works.

The staged server-only client release did not explicitly re-enable simulation. Final construction also used holdArrival, transiently disabling simulation before clients streamed the vehicle. 0.4.8 separates surface placement from the hold and explicitly resumes simulation/body activity/synchronization on server and client. Client positioning remains observation-only for staged arrivals. Diagnostics now include disabled/targetable/fuel state and native seating-transition/door-exit conditions.

EngineCheck-20261024-142847: both Chernarus and Namalsk compile and pass 24 checks; one native refuel check is inconclusive. A known saved 60% fuel value restores correctly. Subsequent native Fill leaves both the reconstructed truck and an untouched control truck at 60% in the no-client fixture. Switching damage permission on or activating the body did not resolve the fixture limitation. Previous failed probes were retained. Do not claim actual refueling, door actions, exiting or driving have passed.

Current candidate needs a connected-player test covering doors, exit from each seat, refuel, driving, then a round trip with seat switching. No arbitrary fuel top-up or saved-data reset was applied. Helper executables remain 0.4.6.

Local deployment: C:\DayZMods\Mods\@DriveLink, 0.4.8-rc1. Backup: C:\DayZMods\DemoServers\Backups\before-local048-20261024-143001. Three PBO signatures verified with Derk only. Hub was kept running at port 7290. No Workshop publication.