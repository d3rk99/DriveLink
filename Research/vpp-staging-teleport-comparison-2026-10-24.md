# VPP teleport comparison — installed Workshop build

Inspected the actual installed `221100/1828439124/Addons/VPPAdminTools.pbo`, SHA256 `BC72882461BF661471CA481949C016999D6EF7E102FC3CB6B97D236D099EAA64`, freshly extracted into `C:/DayZMods/DriveLinkRestartPrototype/Build/VPP-Teleport-Audit-20261024/VPPAdminTools`. Its TeleportManager source matches the earlier local inspection exactly.

## Verified paths

Source paths below are relative to that extraction.

- `5_mission/vppadmintools/missiongameplay.c:557`: teleport-to-cursor sends a reliable RPC to TeleportManager.TeleportToPosition.
- `5_mission/vppadmintools/gui/submenus/menuteleportmanager/menuteleportmanager.c:460`: the map/location UI sends RemoteTeleportPlayers.
- `4_world/vppadmintools/plugins/pluginbase/teleportmanager/teleportmanager.c:266`: TeleportToPosition performs the transform change only in its CallType.Server branch, after permission checks.
- `teleportmanager.c:335`: RemoteTeleportPlayers also dispatches only in the server branch, into TeleportToPoint or TeleportToTown.
- `teleportmanager.c:203`: TeleportToPoint obtains the caller's current HumanCommandVehicle transport. For a vehicle, it reads the existing 4x3 transform, replaces its translation, then calls SetTransform once. For a walker, it calls SetPosition on the player.
- GotoPlayer, BringPlayer and TeleportToTown use the same vehicle-root SetTransform pattern. They do not independently teleport each seated occupant.
- `teleportmanager.c:485`: the separate ESP entity teleport path additionally calls SetPosition and applies a small upward impulse. That extra impulse is not part of the normal player-in-vehicle teleport path and is not evidence it is needed for arrivals.

The inspected TeleportManager does not freeze vehicle simulation, repeat transform writes each second, manually reposition the owning client's vehicle, replace a vehicle, or unload/reload it. No Transport/CarScript override in the extracted mod was found that adds a hidden teleport synchronization step. These findings concern the inspected paths, not a claim about every VPP feature.

## Comparison with DriveLink

DriveLink 0.4.3 restores a new vehicle at staging, disables simulation, seats the crew, teleports the vehicle over a large distance and repeatedly sets the final transform on both server and owning client while waiting for acknowledgements. Its ACK checked the root GetPosition only. The reported seat/model split occurred even though those checks passed.

The unpublished 0.4.4 candidate changes seating order and commits the model hierarchy, but still has paused simulation and client-side placement. It is therefore not a reproduction of VPP's simpler server-only teleport path.

VPP's success establishes that a normal, already-running vehicle can teleport correctly in this installation. It does not establish that the same operation works on a newly restored, paused vehicle during first possession. Nevertheless, it makes the paused state and competing server/client placement writers important suspects. That is an inference, not a confirmed engine diagnosis.

## Useful controlled comparison

Use disposable vehicles and profiles, keep the tested move/heading/crew fixed, and compare:

1. Standard VPP teleport on a normally running vehicle (baseline).
2. The same server-only root-transform operation on a newly restored unoccupied vehicle, then seat only at the destination.
3. The same operation while simulation is disabled, with no owning-client transform writes.
4. The existing paused/client-write handoff.

Keep damage protection independent of simulation pausing. Clients should observe root, physical body and local/world seat transforms for the baseline path rather than changing the transform being measured. Then switch seats, exit and compare the same vehicle from a nearby observer. No re-streaming workaround should be used before capturing those measurements.

A VPP-style path can be implemented directly through DayZ's engine APIs; it need not call VPP's permission-gated admin RPCs or add a VPP dependency to DriveLink. Do not treat a small upward physics impulse as a general fix for truck bounce.

No live server, candidate package or Workshop content was changed during this audit. A live comparison has not been run.
