# Main menu "on the road" scene - research (2026-10-30)

Goal: a player sent back to the main menu during travel time ("You are still on the road ...", `DL_Kick`) sees
himself in the seat of the vehicle he travels in, and with a party his crew / the whole convoy, in their gear.

## How the vanilla menu scene works (DayZ 1.30 scripts, P:\scripts)

- `MissionMainMenu` creates `DayZIntroScenePC` (`5_mission/dayzintroscenepc.c`, base `5_mission/dayzintroscene.c`)
  unless `m_NoCutscene` is set (DriveLink sets it only while a redirect is pending, `DL_MainMenu.c`, so a travel-time
  kick does show the scene).
- `DayZIntroScene.SetScene` picks a random entry of `cfgCharacterScenes <world>` (camera position, target, date,
  weather), creates a `staticcamera` and puts the character 3.7 m in front of it.
- `IntroSceneCharacter` (`4_world/classes/introscenecharacter.c`) builds the character from the client's local
  character cache (`MenuData.CreateCharacterPerson`), a local `PlayerBase` with his last known gear.
- Everything is a **local, offline world on the client**: script can `CreateObjectEx` any vehicle / character class
  and attach items (`LocalTakeEntityToTargetAttachmentEx`, as `SetAttachment` does). Other mods replace the scene by
  modding these two classes (camera spot, props, animation) - the same route is open to us.

## What it takes

1. **Data to the client before the kick.** The server already builds the trip's film (`DL_Film`: crew actors with
   clothes layers + weapons, cars, which actor rides in which car). For the menu we need real classes instead of the
   film's sprite names: per member `charType` + worn item classes (from `DL_PlayerData.items`, slot + type), per car
   its class and seat of each member. A new `RPC_MENU_SCENE` (JSON, in parts like `RPC_FILM`) sent right before
   `DL_Kick` for a "wait" login, and before the first redirect of a timed trip. The client keeps it in `DayZGame`
   (survives the mission change, like `m_DL_Notice`) with the arrival time.
2. **modded DayZIntroScenePC**: when DriveLink scene data is present and the trip is not over yet:
   - pick our own spot instead of `cfgCharacterScenes` (a straight road on the menu world; a few hard-coded spots per
     vanilla map, falling back to the vanilla scene for unknown maps); boats: a shore spot, camera over the water;
   - create the vehicle(s) locally, in a line for a convoy, with their parts (wheels, doors) so they look whole;
   - create each crew member (`CreateObjectEx(charType)` + clothes) and put him in his seat; the menu's own character
     takes his own seat instead of standing in front of the camera;
   - camera framing the lead vehicle (or the whole convoy), a "on the road to X: 1 h 12 min left" line in the menu.
3. **Seating is the unknown.** In the menu world there is no server and no simulation of `HumanCommandVehicle`;
   `StartCommand_Vehicle` on a local character may or may not work offline. Fallbacks, in order:
   (a) `StartCommand_Vehicle(transport, seatIdx, seatPos)` on the local character (what the arrival seating does);
   (b) place the character at the seat's proxy position (`Transport.CrewPositionIndex`/`GetCrewMemberPosition`-like
       memory points) and play the vehicle sit animation through `DayZPlayer` animation commands;
   (c) a static seated pose.
   A spike (dev build, menu only) decides which one works before anything else is built.

## Feasibility

- Own character in his vehicle: **likely** (local objects in the menu world are normal; the risk is only the seat pose).
- Crew with their gear: **likely**, same mechanism, gear from the snapshot classes (no textures to ship).
- Whole convoy: **yes** once one car works; the cost is camera framing and performance (4 cars x 4 crew = 16 local
  characters, fine).
- Limits: the menu world is the client's menu map, not where the trip is; modded vehicle classes only show if the
  player has that mod loaded at the menu (Workshop mods are loaded at the menu when the launcher started with them),
  otherwise fall back to a vanilla look-alike (`DL_FilmCast.CarSprite` already maps classes to kinds).

## Spike result (2026-10-30, dev build, Chernarus menu)

`DL_MenuSceneSpike.c` + `DL_MenuSeatSpike.c` (both `#ifdef DL_DEV`), client started with `-dlMenuSpike=a`:

- An `OffroadHatchback` made with `CreateObjectEx` in the menu world shows with every part created by
  `GetInventory().CreateAttachment` (wheels, doors, hood, trunk).
- **Seating (a) works**: `StartCommand_Vehicle` from the local character's command handler puts both the menu
  character (seat 0) and a second spawned character with clothes (seat 1) into their seats in 2 ticks; they stay
  seated and animated (log: "after 6 s seat 0 = SurvivorM_Rolf, seat 1 = SurvivorM_Mirek"). (b) and (c) were not
  needed.
- Moving the menu's camera (`m_Camera.SetPosition` / `LookAt`) after `super.SetScene()` sticks.
- Screenshot: `main-menu-spike-a.png` (the passenger with the red cap faces the camera; the driver is on the far side,
  so the real scene should put the camera on the driver's side).
- Note: `GetPosition()` of a seated character reports a meaningless position; use the vehicle's.

## Built (2026-10-30)

- Server (`DL_MissionServer.c`, travel time on only): `DL_RememberScene` captures the party as it leaves (each
  vehicle with its attached parts, each member's character class, worn items and seat; boats are left out, their crew
  stands); `DL_SendScene` sends it (`RPC_MENU_SCENE`, in parts) right before the "on the road" kick, with the member
  index of the receiver. Steam ids stay on the server.
- Client (`DL_Client.c`): puts the parts together and saves `$profile:DriveLink/menuscene.json` with the time the trip
  ends (`DL_MenuScene`, UTC clock, survives a game restart). Deleted 10 min after the trip is over.
- Menu (`DL_MenuSceneBuilder.c`, `DL_MenuSeat.c`): in the vanilla scene's spot the vehicles stand in a line, driver's
  side to the camera, everyone seated in his clothes, the menu's own character in his own seat; walkers in a row. The
  camera backs off for a convoy, then steps closer until no scene prop is between it and the first / last vehicle
  (`DL_ClearView`), and looks a little right so the party sits clear of the menu's panels. Classes from mods the
  client hasn't loaded at the menu are skipped (their crew stands).
- Main menu line (`DL_MainMenu.c`): "On the road to X: about N min left", then "You have arrived: join X".
- Fixed spots per map (`DL_MenuSpots` in `DL_MenuSceneBuilder.c`): Chernarus road 13307.43 / 10829.09, Chernarus sea
  14851.60 / 10906.85 (Derk's picks). Maps without spots use the vanilla scene's spot (boats are then left out,
  their crew stands). A road spot carries a route (points down the road's middle, read off the Hub's satellite tiles);
  script can't see roads, and fitting the terrain surface around a road (cp_gravel) proved unreliable (wrong heading,
  tracing drifted into grass). Boats are made at sea level (`ECE_KEEPHEIGHT`) and float.
- Cars under way: after 2.5 s on their own wheels (ride height measured then), each car's physics body goes kinematic
  (`dBodyDynamic(false)`) and it is placed on the route every frame at 8 m/s, facing along it with the slope; forcing a
  dynamic car's position made it flip and sink. The camera keeps its place relative to the lead car in the road's
  frame. At the route's end everything starts over (Chernarus: ~700 m, ~90 s). Wheels don't turn (kinematic).
- The main menu re-aims the camera at the menu character when it opens (`MainMenu` -> `ResetIntroCamera`); the
  override keeps it on the party.
- Framing: a low three-quarter shot from ahead of the lead vehicle, from the side that puts the lead on the left of
  the screen and the line trailing right (over the menu's panels). Boats sail in an echelon (11 m back, 5 m out
  each) so the formation recedes. Scene fog over 0.1 is held at 0.08 (re-applied every 2 s: the weather drifts).
- Boats are under way: 5 m/s along the heading (`SetVelocity` every frame, the water physics keeps them bobbing),
  the camera rides along at its start offset from a smoothed lead position at sea level. The run is measured first
  (open sea ahead of every boat, at most 400 m); at its end all boats jump back to their start, still moving.
  Tested: 25 m per 5 s, a straight course, crews seated after the jump.
- Tested with hand-written scene files: 2 cars / 4 people on the road (`main-menu-road.png`), 4 boats / 10 people at
  sea (`main-menu-boats.png`).
  NOT tested yet: a real timed trip sending the scene (server capture + RPC).

## On foot (2026-10-31)

- A party with no vehicles walks the recorded Chernarus route: two abreast, the first alone in front, 1.8 m
  between rows. Each walker is a real character walking: `PlayerBase.HeadingModel` turns him toward a point 3.5 m
  ahead on his line (heading in radians, same sign as the yaw; negated, the group walked off 46 degrees),
  `HumanInputController.OverrideMovementSpeed(ENABLED, 1)` walks him (0 idle, 1 walk, 2 jog), angle 0.
- Whoever drops back from his place walks faster (up to a jog), whoever is ahead slows; more than 6 m off his
  line or 12 m out of place he is put back. Each sets off up to 1.5 s after the others, so their steps aren't in
  sync. The camera follows the group's front like the convoy's lead; at the route's end all start over.
- Tested with a 6-person scene (dev build).

## Open

- The countdown line's font is small; the default TextWidget font doesn't grow with its box here.
- The menu character carousel (statistics arrows) makes a new character at the old spot, not in the seat.
