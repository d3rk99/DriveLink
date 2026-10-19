# R3 - Map images for the hub's map editor (2026-10-18)

Goal (Derk): the hub shows an interactive map per server; owners click+drag leaving zones (centre + radius) and
arrival points (point + facing), saved instantly, used in game without a restart.

## Where the map images come from: the owner's OWN game files (nothing redistributed)
dayz.ginfo.gg uses Leaflet (open source) but gives no licence for its map images (Bohemia data, "All Rights
Reserved") -> we copy the technique, not the images. Every DayZ install already has the maps:

| Map | Satellite tiles (`layers\s_XXX_YYY_lco.paa`) | Paper-map base (`usermap\`) |
|---|---|---|
| Chernarus | worlds_chernarusplus_data.pbo, 1024 tiles | gear_navigation.pbo, 1024 tiles |
| Livonia | worlds_enoch_data.pbo, 1024 tiles | worlds_enoch_data.pbo, 1024 tiles |

- Satellite tile = 512x512 px, real colour (roads, fields, forest). USE THIS.
- usermap tile = mostly white base colour; the game draws contours/labels from vector data -> not useful alone.
- Tile placement (from the tile's rvmat, decoded with DayZ Tools CfgConvert -txt): Stage1 `uvSource="worldPos"`,
  `aside[]={1/512,0,0}`, `up[]={0,0,1/512}`, `pos[]={-14.03125,15.03125,0}` for tile 015_016 -> one tile = 512 m at
  1 m/px, 16 px overlap each side, step 480 m; 32 x 480 = 15360 = Chernarus size. Each tile's rvmat gives its exact
  world origin, so stitching is exact.
- Tools that do the conversion (all in DayZ Tools, which server owners have): BankRev (extract PBO), ImageToPAA
  (PAA -> PNG, verified), CfgConvert (rvmat -> text, verified).

## Plan
1. Zones/arrival points move into the hub DB; servers fetch them on boot + every ~30 s (live, no restart);
   config.json stays as fallback.
2. Hub "map build" step: from a DayZ install path, extract + convert + stitch the satellite tiles into a Leaflet
   tile pyramid (256 px, JPEG/WebP) per map, cached next to the hub (once per map, ~minutes).
3. Web admin page in the hub (`/admin`, password): Leaflet CRS.Simple in DayZ metres; draw zones (centre + drag
   radius), arrival points (point + drag facing); x/z only - height is the terrain's (the mod already uses SurfaceY).
4. Fallback for maps without these files: upload any image + map size.
