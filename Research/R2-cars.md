# R2 - Cars: what to save and how to rebuild (vanilla 1.29 scripts, 2026-10-13)

No public mod moves a car between servers with people in it (R1: DayZed packs empty cars into an item). Everything
below is from the vanilla scripts (BohemiaInteractive/DayZ-Script-Diff).

## The car itself
- Class tree: Transport (3_game/vehicles/transport.c) -> Car (3_game/vehicles/car.c) -> CarScript
  (4_world/entities/vehicles/carscript.c) -> e.g. CivilianSedan, OffroadHatchback, Hatchback_02, Sedan_02,
  Offroad_02, Truck_01_*, Van_01 (4_world/entities/vehicles/inheritedcars).
- Fluids: Car.GetFluidCapacity(CarFluid) / GetFluidFraction(CarFluid) / Fill(CarFluid, amount) / LeakAll(CarFluid)
  (car.c:359-376, re-checked 2026-10-18). CarFluid = FUEL, OIL, BRAKE, COOLANT, USER1, USER2 (car.c:18-26). Rebuild: LeakAll then Fill(fraction * capacity).
- Health: the car has damage ZONES (Engine, FuelTank, Radiator, Front/Back etc.): DamageSystem.GetDamageZoneMap(car) +
  GetHealth(zone,"Health") / SetHealth - same as the player zones in v0.2.
- Parts are ATTACHMENTS: wheels (incl. spare), doors, hood, trunk, battery (CarBattery/TruckBattery), radiator
  (slot "CarRadiator", carscript.c:772), spark plug (slot "SparkPlug") / glow plug ("GlowPlug"), headlights, etc.
  -> the v0.2 item code handles them unchanged (car = parent 0 instead of the player): type, slot, health.
  CarScript.EEItemAttached tracks battery/plug/radiator (carscript.c:740-790), so attaching them normally keeps it in sync.
- Cargo (trunk) = the car's own cargo -> same item list (cargo idx/row/col/flip).
- Doors: open/closed is an animation phase (CarDoorState, carscript.c:1-6, GetCarDoorsState :2796). Close all doors on
  arrival (simplest, safe).
- Engine: Car.EngineIsOn/EngineStart/EngineStop (car.c:241-247). Gear: GetGear/ShiftTo (car.c:259-271). Lights:
  Transport.LightIsOn/LightOn/LightOff (transport.c:164-170). Handbrake: SetHandbrake (car.c:220).
- Spawn: g_Game.CreateObjectEx(type, pos, ECE_SETUP | ECE_CREATEPHYSICS | ECE_KEEPHEIGHT ...) (centraleconomy.c:9-37);
  orientation via SetOrientation / Transport.SetWorldTransform (transport.c:13); zero velocity (SetLinearVelocity).
  Vanilla car persistence normally comes from the game's own storage; ours is a NEW object on the target server,
  so the target server's storage saves it from then on (lifetime from types.xml, like any car).

## Crew
- Transport.CrewSize / CrewMember(posIdx) / CrewDriver / CrewMemberIndex(human) (transport.c:112-128).
- Put a player in a seat (server): same as the get-in action: seat = trans.GetSeatAnimationType(crewIdx) (transport.c:475),
  vehCommand = player.StartCommand_Vehicle(trans, crewIdx, seat) (human.c:1492) + vehCommand.SetVehicleType(trans.GetAnimInstance())
  (actiongetintransport.c:87-96). The engine also has Transport.CrewGetIn(human, posIdx) (transport.c:143).
- Unconscious passengers: dayzplayerimplement.c:2370-2378 re-seats them with StartCommand_Vehicle(..., fromUnconscious=true).

## Plan for M2 (driver only) -> M3 (crew)
1. Zone check also looks at vehicles: if a car's center is in the zone and its DRIVER is a player, countdown for the
   whole crew (engine speed doesn't matter; the countdown cancels if the car leaves the zone).
2. Depart: snapshot the car (type, fluids, zone health, all items flat - reuse v0.2 code with the car as root), and every
   crew member (player snapshot + crewIdx). ONE hub transfer with car + crew list; each player row is locked in transit.
3. Arrival: the FIRST crew member to load spawns the car at the arrival point (stopped, doors closed, engine off, handbrake
   on), restores parts/fluids/cargo, and is seated in their crewIdx. Later crew members are seated in theirs. The car is
   protected (no damage) until everyone arrived or 60 s passed. A crew member who never arrives: his seat stays empty, his
   character is still in transit and arrives at the arrival point (no car) next time he joins.
4. Source: car deleted only after the hub confirms the transfer (same two-phase rule as players).
5. Anti-dupe for cars: the car snapshot exists only inside the transfer; source deletes it on hand-over; target spawns it once
   per token (hub tracks a car_spawned flag on the transfer).

## M2 spikes, in game 2026-10-18 (DayZ 1.30 servers, debug.json on Chernarus)
1. Seat a player from script - PASS 5/5, also right after a character was built from a transfer.
   Server sends a sync juncture to the player (PlayerBase.SendSyncJuncture); it runs on the server AND the owning
   client, both set a member and the next CommandHandler tick calls StartCommand_Vehicle(car, crewIdx,
   GetSeatAnimationType) + SetVehicleType(GetAnimInstance) - the vanilla debug spawner's path (DL_PlayerSeat.c).
   Gotcha: juncture id 4810 never arrived; 233 works -> the id must stay below 256 (vanilla uses 0..26 and 200).
2. Car snapshot -> delete -> JSON text -> rebuild - PASS: OffroadHatchback 28/28 parts/items, health and 15 damage
   zones, 9.7 kB JSON; the rebuilt car had its fuel and started. Parts and trunk are just DL_Snapshot.CaptureTree /
   ApplyTree with the car as root (no car-specific item code).
   Gotchas: CarFluid has 8 values in 1.30 (USER1..USER4), not 6. GetFluidFraction reads 0 right after Fill on a
   brand-new car although the fill worked - read it later. GetObjectsAtPosition searches a SPHERE (centre it on the
   ground, then filter by 2-D distance).
Not yet: engine on/off, lights, doors open, ruined parts, a car with people in it being deleted.
