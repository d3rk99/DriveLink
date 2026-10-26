# Namalsk in a DriveLink server network

Research date: 2026-10-22. Scope: effects on Chernarus, Livonia, Sakhal and Nasdara, and the feasibility of changing mods without exiting DayZ. This is a static inspection and feasibility report, not an in-game compatibility certification.

## Findings

Both Namalsk Island and Namalsk Survival are installed locally. Loading them throughout a network is **not behavior-neutral** on the official maps. Survival has substantial global gameplay overrides; Island has smaller global presentation overrides. Many Namalsk-specific world features are correctly restricted to Namalsk, but that restriction does not cover the whole package.

There is no supported complete Workshop-mod hot-swap mechanism identified in DayZ's public modding interface. A new PBO can implement map-aware behavior for already loaded code; it cannot be assumed to mount/unmount arbitrary PBOs, rebuild merged configs, undo inherited class overrides, and renegotiate network schemas. The practical design is a common client content set with a separate compatibility addon that restores the intended behavior on each world.

## Local installation and evidence

- Island: `E:\Steam\steamapps\workshop\content\221100\2289456201`.
- Survival: `E:\Steam\steamapps\workshop\content\221100\2289461232`.
- They are present through the DayZ client's Workshop installation. I did not find dedicated `@Namalsk...` links in the inspected DayZServer root.
- The two running local DayZ servers had CF, VPPAdminTools and the DriveLink film prototype in their mod arguments, without Namalsk. The demo launch script also omits Namalsk.
- Existing server processes, launch files, missions, keys and mod binaries were not changed or restarted.

Bohemia's installed BankRev extracted `namalsk_scripts.pbo` and `ns_dayz.pbo` to `C:\DayZMods\_assets\NamalskAudit-2026-10-22`. CfgConvert decoded the Survival config files there. This folder also holds `installed-pbos.csv` with sizes, timestamps and SHA-256 hashes of the two inspected PBOs, plus `modded-class-inventory.txt`.

Paths in the following evidence tables are relative to that audit folder. The installed `ns_dayz.pbo` timestamp was October 20, 2026; Workshop pages showed later activity. These findings describe the installed snapshot, not a claim that every installed package is the latest release. The remaining Island asset/config PBOs and Survival music PBO were not exhaustively audited.

## Cross-map effects confirmed in installed scripts

| Package / behavior | Evidence | Consequence on official maps |
| --- | --- | --- |
| Island: daylight lights | `namalsk_scripts/4_world/namalskterrain/entities/itembase/dzn_lights.c` | Constructors force `SetVisibleDuringDaylight(true)` on gas-lamp, torch and chemlight lights, without a world check. Island alone is therefore not completely inert elsewhere. |
| Survival: heat constants | `ns_dayz/scripts/3_game/namalsksurvival/gameconstants.c` | Heat-buffer decrease becomes `0.025` versus `0.01` in the local vanilla scripts; the environmental-temperature divisor becomes `56` versus `40`. Both definitions are global. |
| Survival: heat comfort and cold resistance | `ns_dayz/scripts/4_world/namalsksurvival/classes/environment/environment.c` | Replaces ordinary heat-comfort processing, accumulates cold resistance, and caps the ordinary target at `0.14`. No world check surrounds this logic. |
| Survival: frostbite | `.../classes/playermodifiers/modifiersmanager.c` and `.../modifiers/frostbite.c` | Frostbite is registered on every world, always activates, and evaluates cold exposure and clothing. It is not limited to Namalsk. Whether a player develops frostbite depends on conditions. |
| Survival: health recovery | `.../modifiers/healthregen.c` | Replaces health-regeneration logic and imposes a maximum based on the player's frostbite-effect stat. The override applies on every map. |
| Survival: temperature damage | `.../modifiers/heatcomfortmdfr.c` | Replaces temperature-related energy, water and health processing, without world isolation. |
| Survival: torch heat/drying/hazard | `.../entities/itembase/flammablebase.c` and `.../classes/environment/environment.c` | Changes ground heat radius/cap, drying, and adds a delayed damage area around dropped lit torches. Player clothing drying also uses a minimum source distance for flammable heat sources. |
| Survival: cooking fuel requirement | `.../entities/itembase/fireplacebase.c` | Kindling-only fires cannot cook; cooking requires real fuel or residual heat from a fire that burned fuel. Applies across worlds. |
| Survival: stove toggling | `.../entities/itembase/portablegasstove.c` | Skips the immediate work tick following ignition, changing stove heat/cooking timing on every map. |
| Survival: vanilla gas-mask speech | `.../entities/itembase/gasmasks.c` | AirborneMask, GasMask and GP5GasMask return false for voice obstruction and zero voice effect, without a world check. |
| Survival: inventory UI | `ns_dayz/scripts/5_mission/namalsksurvival/gui/inventorynew/inventory.c` and adjacent container scripts | Uses Namalsk inventory layouts and cold-resistance/frostbite UI without checking the world. |
| Survival: additional player state | `.../classes/playerstats/playerstats.c` and `.../entities/manbase/playerbase.c` | Adds cold-resistance/frostbite stats and network variables across maps. Server/client schema consistency remains important even if gameplay effects are later gated. |

`...` in this table means `ns_dayz/scripts/4_world/namalsksurvival`. These are demonstrated non-isolated overrides, not an exhaustive list of every changed method. Other shared classes are also extended; some only affect Namalsk-specific equipment or states.

### Map implications

- **Chernarus and Livonia:** these changes remain present even during warm weather. Cooking, torches, voice effects and UI are immediately relevant; frostbite becomes relevant during sufficiently cold exposure.
- **Sakhal:** its cold environment makes frostbite, heat-buffer changes, drying and temperature-damage changes especially important. Loading Survival must not be treated as simply adding Namalsk assets.
- **Nasdara:** the ordinary heat-comfort target cap is a priority concern. The local vanilla scripts start the warm warning at `0.15`, above Survival's `0.14` cap. This suggests the ordinary path could suppress intended overheating and associated survival pressure. Special heat-comfort paths and actual Nasdara gameplay still require measurement; this is a source-supported risk, not a completed runtime test.

## What does remain Namalsk-specific or mission-controlled?

- Survival's `5_mission/.../mission/missionbase.c` selects `NamalskData` and its dynamic-music registry only when the world name is `namalsk`; other worlds call the inherited initializer.
- The monthly cold temperature arrays live in `4_world/.../classes/worlds/namalsk.c`. They do not, by themselves, replace each official world's monthly temperature table.
- Island's `5_mission/.../mission/missionbase.c` guards creation of its special structures with `m_IsOnNamalskIsland`. Its underground VFX initialization is also world-guarded.
- Survival's MissionServer guards Lantia/Phoenix setup and update paths with `m_IsOnNamalsk`; the player command handler also guards its Lantia kill switch. The presence of coordinates in these methods alone is not evidence that they execute on Chernarus.
- The random event manager is constructed generally, but its scheduling is enabled by mission setup. The supplied Regular mission explicitly calls `OnInitServer` and registers blizzard, extreme cold, snowfall, EVR, aurora and fog events. Simply loading the PBO does not establish that these storms start on every map.
- Namalsk loot, spawns, mission weather, difficulty and economy files are mission content. Loading the addon does not automatically import the Namalsk mission's economy into another map. Preserve each map's mission and do not copy the Namalsk init/economy wholesale.
- Island's local particle updater is created generally but checks Namalsk surface types. It is not sufficient evidence that official maps will receive blanket snow effects.

Disabling EVR scheduling alone will **not** remove the global survival changes listed above.

## Can one PBO load and unload the other mods at runtime?

Bohemia documents ordinary mods through the `-mod=` launch parameter and organizes script additions into game, world and mission modules. The local Namalsk config registers its scripts in those modules. Those modules, config inheritance, asset resolution and network-visible classes are more than a list that a mission can toggle.

The local API has potentially misleading names:

| API | What inspection establishes | What it does not establish |
| --- | --- | --- |
| `ModLoader.LoadMods()` in `PDrive/scripts/3_game/client/mods/modloader.c` | Enumerates existing `CfgMods` children into display metadata objects. | It does not mount new Workshop content. |
| `ScriptModule.LoadScript()` / `Release()` in `PDrive/scripts/1_core/proto/enscript.c` | Provides loading and release of a script module. | No demonstrated complete mod/config/asset hot-swap or removal of existing `modded class` inheritance. |
| `DayZGame.ReloadMission()` in `PDrive/scripts/3_game/dayzgame.c` | The inspected implementation recreates a mission under `ENABLE_LOGGING`. | It is not a general retail Workshop-mod reload implementation. |
| `RequestReload()` in `PDrive/scripts/2_gamelib/gamelib.c` | Follow-up inspection found this is inside `GAME_TEMPLATE`, on `Game`, not DayZ's active `CGame` interface. | It is not an exposed DayZ client reload mechanism. |

Conclusion: **no supported, demonstrated way to implement arbitrary full-mod load/unload as an ordinary PBO was found.** This is not a proof that engine developers could never add it. Treat it as engine-level work requiring support beyond the public PBO interface, rather than a practical DriveLink compatibility feature.

Follow-up on October 22: the standalone `C:\DayZMods\ClientModLoaderLab` prototype tested the installed retail client. It explicitly rejects `ScriptModule.LoadScript` and `ExecuteEnforceScript`; dynamic script modules worked on an isolated dedicated server. The retail client and server did not automatically mount a PBO added after startup. A separate `RequestRestart(0)` experiment recognized the new PBO only after the client relaunched with a different PID. See that project's `Research\feasibility.md` and `Evidence\results.json` for details. Bohemia's [official 1.24 notes](https://forums.dayz.com/topic/259072-stable-update-124/) document the retail script-loader restriction.

Changing active worlds and reconnecting to another server with an already compatible content set is a different operation. It can avoid exiting the client, although the destination world still needs loading.

## Must every server have literally the same mods?

For this network, standardize the client-required content and dependency order/versions on all participating servers. This gives the running client all destination assets and keeps gameplay scripts, network variables and item classes consistent. Downloaded/subscribed is not the same as loaded.

Literal equality of every server launch argument is not a universal engine rule: server-only logic can vary through `-serverMod`, and some intentionally client-only addons can be accepted under suitable server verification rules. But accepting an extra client addon does not unload its overrides or prove client/server simulation compatibility. A client superset with Namalsk active while the official-map server lacks it is therefore an **unproven alternative**, particularly for Survival's player-state extensions. Do not adopt it as the default network design or relax verification as a substitute for compatibility.

## Recommended implementation direction, before DriveLink-specific work

1. Freeze and record a tested Workshop snapshot and a common client mod order. Recheck the current installed-versus-Workshop versions first.
2. Use a separate addon loaded after Namalsk to isolate effects by actual world name and explicit network settings. Do not use `#ifdef NAMALSK_SURVIVAL` as a map check: it indicates loaded code, not the current terrain.
3. Preserve Namalsk behavior there while restoring the official maps' heat, frostbite policy, cooking, torch, voice and UI behavior. Reset per-mission flags/state on travel and verify both client and server paths.
4. Recognize that a later `super` call still reaches Namalsk's override. It does not automatically bypass it and reach vanilla. Some replacements and global constants need careful refactoring or maintained compatibility implementations; a single generic enable/disable boolean will not solve everything.
5. Keep class definitions/network schemas consistent while gating gameplay. Explicitly decide whether frostbite and cold resistance earned on Namalsk should persist, pause, heal or reset elsewhere. Merely loading a common mod list does not ensure the transfer system preserves those stats.
6. Keep each map's world data, mission economy, weather and event initialization separate.

The author permits Island and Survival to be used independently, so an Island-only network is another viable scope if Namalsk's custom survival gameplay is not required. It still needs the smaller Island effects audited. The author disallows repacking; design a separate compatibility addon rather than redistributing altered Namalsk packages.

## Proposed verification matrix (not executed)

For each official map, compare the same mission/date/weather/loadout under: baseline network mods; baseline plus Island; baseline plus Island and Survival; then the compatibility addon. Repeat on Namalsk to confirm its intended behavior survives the patch.

Measure: heat comfort and actual temperature damage in cold/hot conditions; heat-buffer decay; frostbite and maximum health; torch carried/dropped heat, damage and drying; kindling-only and fueled cooking; stove cycling; gas-mask speech; inventory layout; world lighting; event activation; economy spawning; client/server script errors.

Then test both travel directions between Namalsk and each official map in one client process, including return trips. Check missing/extra PBO rejection, stale UI/effects/timers, correct world data, transferred mod items, player-state policy, reconnects, deaths, passengers and vehicles. Do not label this compatible based on compilation alone.

## Primary references

- [Namalsk author's server files and FAQ](https://github.com/SumrakDZN/Namalsk-Server): independent use of the two mods, mission configuration, optimized dedicated-server packages, detection defines, temperature scope and repacking policy.
- [Namalsk Island Workshop entry](https://steamcommunity.com/sharedfiles/filedetails/?id=2289456201).
- [Namalsk Survival Workshop entry](https://steamcommunity.com/sharedfiles/filedetails/?id=2289461232).
- [Bohemia: DayZ Modding Structure](https://community.bistudio.com/wiki/DayZ:Modding_Structure): launch parameter and module structure. Search-index text was available; direct page fetch returned HTTP 403 during this research.
- [Bohemia: DayZ Modding Basics](https://community.bistudio.com/wiki/DayZ:Modding_Basics): module order and diagnostic/file-patching workflow. Same direct-fetch limitation.
- [Bohemia: DayZ Server Configuration](https://community.bohemia.net/wiki/DayZ:Server_Configuration): server-only mod parameter. Search-index text was available; direct fetch returned HTTP 403.

The detailed behavioral findings above come from the installed PBO scripts, rather than general server-hosting guides or community anecdotes.
