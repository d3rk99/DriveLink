# R1 - How other travel mods save and rebuild a player (2026-10-13)

Derk: "look at other map link mods to find out, do the research before we waste time with trial and error."
Sources read (code read only, NOT copied - MapLink + UAPI are AGPL-3.0):

## 1. DaemonForge MapLink + Universal API (github DaemonForge/DayZ-MapLink, DayZ-UniveralApi)
- Player = PlayerDataStore (MapLink 4_World/MapLink/entities/PlayerDataStore.c). Gear = a FLAT top list
  m_Attachments of UEntityStore/UApiEntityStore, one per worn item + hands (PlayerDataStore.c:374-388).
- Each item store saves ITSELF and its children by asking the engine for the whole subtree in one call:
  `item.GetInventory().EnumerateInventory(InventoryTraversalType.LEVELORDER, items)` (UAPI
  _UniversalApi/.../ItemStore.c:18; engine proto 3_game/systems/inventory/inventory.c:127), then keeps only DIRECT
  children (HasEntityInCargo || HasAttachment) and breaks at the first deeper one (LEVELORDER = level by level).
  Each child is `new UApiEntityStore(child)`, whose constructor runs SaveEntity again -> recursion happens through
  CONSTRUCTORS, and no function adds up its own return value.
- Saved per item (ItemStore.c:3-112): type, health, quantity, wet, temperature, energy + on/off, liquid type,
  agents, cleanness, inventory location (slot, cargo idx/row/col/flip), in hands, quickbar slot, magazine rounds
  ONE BY ONE (ammo type + damage), weapon: chambered rounds per muzzle, fire modes, current muzzle; damage zones;
  free-form metadata for other mods (OnUApiSave/OnUApiLoad hooks).
- Rebuild (ItemStore.c:117-195): Create(parent) puts the item at its EXACT saved place: cargo ->
  CreateEntityInCargoEx(type, idx, row, col, flip); hands -> CreateInHands; attachment -> CreateAttachmentEx(type, slot).
  If that fails: CreateObject on the ground next to the parent (item is never lost silently). Then LoadEntity sets
  the state and calls Create(item) for every saved child. Magazines going into a weapon -> weap.AttachMagazine.
- Player stats (PlayerDataStore.c:330-460): health/blood/shock global AND per damage zone, blood type (+visible),
  bleeding sources, modifiers (diseases, etc.) + their time, agents, stomach contents, stats (playtime, kills,
  distance), shave state, bloody hands, broken legs, arrows stuck in the body, persistent flags.
- JSON in/out: UApiJSONHandler<T> = JsonSerializer on the CONCRETE type (_UAPIBase/.../JsonHandler.c:5-11).
- Arrival: target asks the API in OnClientPrepareEvent, builds in ClientNew via g_Game.CreatePlayer + SelectPlayer
  + InvokeOnConnect, then SetupPlayer (MissionServer.c:57-140). Source: SetAllowDamage(false), redirect RPC, kill
  350 ms later and the body is deleted in HandleBody (PlayerBase.c:420-428, MissionServer.c:144-157).
- The Workshop repack "MapLink Hive" (3478983598) and both UAPI Workshop items were removed by Steam; repack broke
  on 1.29 (comments). Source on GitHub is the newest (MapLink last commit 2026-04-18).

## 2. DayZed.GG "Nexus" (https://dayzed.gg/wiki/guides/travel/nexus-travel) - mod not public
- One character over 5 maps. Station needs a 9V battery; you are dropped to the main menu with a server list.
- **Vehicles: only PACKED.** "Pack the car before you call ... comes out the other side as a Vehicle Package to
  unpack". Needs a pipe wrench, engine off, NOBODY in it, all containers and storage EMPTY.
  -> Nobody has public car travel WITH people inside / with cargo. DriveLink's drive-in-with-crew is new ground.
- Arrival protection 5 s within 5 m; full server -> you land in its queue; can't join another map directly while
  alive (same anti-dupe rule as ours).

## 3. DayZ Original (dayzoriginal.com) - 3-server hive, characters + gear travel; vehicles: no evidence.

## What this means for our bug (items lost on arrival, selftest 3/11)
Our CountItems/RestoreList recursion lost everything after the first element (index loop didn't help). UAPI never
does that: recursion only through constructors/methods that return nothing, and the item list comes from the
engine (EnumerateInventory). 009e0d8 already replaced our recursion with a work list (no self-calls) - same
outcome as UAPI's shape. Things UAPI does that we don't, to adopt:
1. Save cargo POSITION (idx/row/col/flip) and rebuild with CreateEntityInCargoEx - our CreateEntityInCargo picks any
   free spot and can FAIL when the free space is fragmented.
2. If an item can't be placed: drop it on the ground at the player instead of losing it.
3. Magazine: rounds one by one (ammo type + damage), weapons: chambered round, attached mag, muzzle, fire mode.
4. Temperature, energy (+ on/off), agents, cleanness, per-zone health, blood type, bleeding, modifiers, stomach.
5. Hooks for other mods (OnSave/OnLoad metadata).
Car travel: no public mod to learn from; DayZed's "pack first" proves the item path (vehicle -> item -> vehicle)
works, but we want the car itself. Next research (R2): vanilla CarScript save/restore (fluids, parts as attachments,
engine/part health, cargo) before writing M2.
