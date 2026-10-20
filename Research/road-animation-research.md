# Road animation: research (2026-10-20, no code yet)

Goal: replace the arrival story text with a short animated scene built from what happened on the road. The car
drives along a scrolling road; it stops for a flat or for fuel; the crew gets out, searches, fixes, or walks on.

## 1. Can DayZ play it? Yes, as a script-driven 2-D animation (verified in the vanilla 1.29 scripts)
| Need | DayZ engine API | Notes |
|---|---|---|
| Draw sprites | `ImageWidget.LoadImageFile(n, "path.edds")`, up to 8 images per widget, `SetImage(n)` | the black screen is already code-made widgets (DL_ClientHud) |
| Sprite sheets (frames) | `WidgetFlags.CUSTOMUV` + `ImageWidget.SetUV(float uv[4][2])` | pick one frame out of a sheet, no cutting needed |
| Move / turn / fade | `SetPos`, `SetSize`, `SetRotation`, `SetAlpha` every frame in `MissionGameplay.OnUpdate` | runs at the game's frame rate |
| Scrolling road | `CUSTOMUV` again: slide the UV of a tiling road strip | endless road from one texture |
| Video instead? | `VideoWidget.Load/Play` exists (main-menu DLC promos, files under `video\`) | can't be built per trip; at most pre-made clips per event; whether it loads from a mod PBO is unproven |
| Sound | config `CfgSoundShaders` / `CfgSoundSets` (+ `SEffectManager`) with `.ogg` | engine, doors, refuel sounds exist in the pack |
| Textures | PNG -> `.paa`/`.edds` with DayZ Tools ImageToPAA (already used for the hub's map tiles); power-of-two sizes | packed into the DriveLink PBO; players download them with the mod |

**Recommendation: script-driven sprites, not video.** Each trip is different, and the journey engine already
knows every event. The client gets the event list and plays one short scene per event.

## 2. The assets (`C:\DayZMods\_assets\minidayz`, 287 PNG + 76 OGG)
- **Cars:** UAZ, Volga and Humvee animation sheets (256x256, several angles, **side views included**). Wrecks for
  the roadside: sedans, hatchbacks, vans, police, truck, BTR, bus. Effects: explosion, smoke and tyre-dirt
  particles, headlight. Fuel-tank item.
- **People:** 20 player skins, each 3 sheets of 32x32 frames in 4x4 grids (walk/idle in 8 directions), plus a
  dead pose. Bandit and soldier NPCs.
- **Infected:** normal/fast/army/shooter/screamer/tank/jumper, live and dead poses.
- **Sounds:** engine start/idle/stop, doors, crashes, refuel, footsteps per surface, eating, drinking, bandaging,
  infected attacks.
- **Missing: the world.** There is no road, grass, tree or sky art. The scenery has to be made: simple pixel-art
  strips, or drawn in code from coloured rectangles plus the wreck sprites as roadside props.
- Frame boundaries are in the original game's `data.js`, which is not in the pack. Most sheets are a regular grid
  (players 4x4 of 32x32), so they can be cut by measuring.

## 3. Licence: needs a decision before anything is published
The pack's README: *"Artwork and audio are the property of Bohemia Interactive ... Personal reference only."*
MiniDayZ being Bohemia's IP does not by itself let a DayZ mod redistribute it. The DayZ modding licence covers
**DayZ's** game data. MiniDayZ is a separate game whose files don't ship with DayZ.
- Fine now: private prototyping on the test servers.
- Before publishing on the Workshop: get Bohemia's written OK, or replace the art with original sprites in the
  same style. The animation code doesn't care which art it plays.

## 4. How it would fit DriveLink
1. DL_Journey (source server) also records **events**, not only story lines: `drive km`, `flat (spare|kit|none,
   who)`, `fuel out (found|lost, who)`, `car left`, `walk km`, `attack (who, died)`, `ration`, `arrive`.
2. The events travel in the snapshot like `journey` does today.
3. The target (or the source, when travel time is on) sends them to the player's client.
4. The client plays the scenes back to back (about 3-6 s each). Players can skip. The text story stays as a
   caption, and as the fallback when the textures aren't there.
5. Crews: everyone sees the same film, with their own names and skins.

## 5. Decisions (Derk, 2026-10-20)
- **Art:** prototype with the MiniDayZ sprites on the test servers only. Before publishing, get Bohemia's written
  OK or swap in original art.
- **When:** on arrival only, on the arrival black screen, before the car and crew appear.
- **View:** side view. The car sits in the middle facing right, and the road and countryside scroll past.
- **Length:** 20-40 s, skippable (Space/Esc). The text story stays as a caption.
- **Scenery:** simple pixel art drawn for DriveLink (sky, tree line, grass, road strips), with pack wrecks as
  roadside props.
- **Car sprite:** closest match. Hatchback and sedans use the Volga; Offroad_02, trucks and anything unknown use
  the UAZ.
- **Skins:** fixed per player, one of the 20 picked from the steam id.
- **Sound:** yes, from the pack (engine idle, doors, refuel, footsteps, infected attack), at low volume.

## 6. Build order
1. **Engine spike:** one converted sprite sheet (UAZ) shown with `CUSTOMUV` frame picking, plus one scrolling road
   strip, on the black screen. Proves texture conversion, packing, UV frames and smooth scrolling in game.
2. **Event list:** DL_Journey records structured events; the snapshot carries them; the target sends them to the
   client.
3. **Scene player:** a timeline of scenes (drive, stop, crew out, search/fix/refuel, crew in, walk, attack,
   arrive), skip key, caption.
4. **Art and sound:** scenery strips, car and skin mapping, sound sets.
5. **Room timing:** the waiting room holds the reveal until the film ends or is skipped.
