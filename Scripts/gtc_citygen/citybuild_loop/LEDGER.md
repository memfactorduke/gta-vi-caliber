# Caliber City build loop — LEDGER (the /loop workflow)

Driver: `/loop` dynamic mode — "build the entire city slowly, block by block,
building by building, road by road. No rush." Each wake advances ONE stage (or one
district/page within a stage), verifies it in the live editor, updates this ledger,
then schedules the next wake.

## Foundations (verified 2026-06-18, this session)
- **Unreal MCP IS wired into the agent this session** (editor_toolset.* tools work).
  This removes the long-standing blocker in PROGRESS.md ("MCP not wired → can't push
  actors"). We can now apply over MCP, staged.
- The WHOLE city is already generated + validated OFFLINE:
  `Content/GTCaliberAssets/Content/CityGen/city_plan.json` = 1093 actors
  (roads 300, district_buildings 631, ocean_drive 160; level_instances 337),
  ~5.1 x 5.8 km. Roads-only layer: `city_roads_plan.json` (300 tiles).
- Macro layout (see city_preview.png): circular **beltway = city boundary**, vertical
  **I-95 spine**, **ring road**, **8 radial arteries**, **3 causeways** to the barrier
  island (Ocean Drive), water east (ocean) + bay.
- Road meshes resolve in the asset registry: SM_Road01_8x8, SM_Road01_8m,
  SM_Street_8m_02 (all under .../CityBeachStrip/Meshes/Street).
- Current live level = CityBeachStrip (production) and it is NOT dirty. NEVER apply
  city actors into CityBeachStrip. Editor rule: one shared editor; never kill/relaunch.

## Apply-over-MCP recipe (how each stage pushes actors)
Plan actor -> MCP scene tool:
- static mesh (`is_li`=false / spawn="asset") -> `SceneTools.add_to_scene_from_asset(asset_path=ref, name=label, xform=...)`
- level instance (`is_li`=true / spawn="level_instance") -> `SceneTools.create_level_instance(level_path=ref, name=label, xform=...)`
- engine class (spawn="class") -> `SceneTools.add_to_scene_from_class(actor_type=ref, ...)`
Then `SceneTools.set_actor_folder(actor, folder)`. Batch many per call with
`ProgrammaticToolset.execute_tool_script` (define `run()`, loop, call `execute_tool`).
Idempotent: purge our namespace first (folder prefix `GTC_CityGen/` or label prefix
`GTC_CG`). Leave UNSAVED until a stage is reviewed.

## Stages (advance one per wake; "no rush")
- [ ] **S0 — New map.** Open World (World Partition) level at
  `/Game/GTCaliberAssets/Content/CityGen/Maps/CaliberCity`.
  Can't be done via MCP (no new-level tool). HANDOFF: user runs
  `py ".../Scripts/gtc_citygen/create_caliber_city_map.py"` once.
  FALLBACK (if still absent after a couple of wakes): MCP-duplicate a minimal map
  (e.g. /CesiumForUnreal/Tests/Maps/SingleCube) -> CaliberCity, load, strip strays
  (non-WP; convert to World Partition later in-editor).
  Gate before any apply: `SceneTools.get_current_level()` must be CaliberCity, NOT CityBeachStrip.
- [ ] **S1 — Boundaries + highways.** Apply the 300 road tiles (`city_roads_plan.json`:
  Beltway 100, RingRoad 47, Artery 96, I95 38, Causeway 19). Pilot ~10 first (incl. an
  SM_Street_8m_02 tile) -> verify spawn via find_actors -> then the rest, paged ~50.
  Verify top-down capture. THIS IS "draw out the boundaries, highways first".
- [ ] **S2 — District street grids / blocks** (local streets are a generator polish TODO;
  for now blocks are implied by the district fills).
- [ ] **S3 — District buildings, district by district** (337 level instances + static
  backgrounds): Downtown 123 -> Wynwood 100 -> Little Havana 142 -> Port 20 ->
  Suburban 350. Heavy LIs -> one district (or page) per wake, verify each.
- [ ] **S4 — Ocean Drive slice** (160 actors at spec coords; placement_plan.json).
- [ ] **S5 — Nav + gameplay**: NavMeshBoundsVolume, PlayerStart, POI marker Kinds,
  `editor_build_paths`; optional Cesium georeference via build_miami.py.
- [ ] **S6 — Review + save**: save dirty packages; branch; commit only on approval.

## Current pointer
**S0 (map creation) — handed off to user; S1 (highways) is READY to apply the moment
CaliberCity exists.** Next wake: check `AssetTools.exists(/Game/.../CityGen/Maps/CaliberCity)`;
if present -> load it + run S1 (pilot then full). If absent after ~2 wakes -> use the S0 fallback.

## Per-wake checklist
1. `get_current_level()` — confirm CaliberCity (or do S0).
2. Do the next unchecked stage/page; purge-our-namespace then spawn; leave UNSAVED.
3. Verify: find_actors count + a viewport capture.
4. Tick this ledger; note counts. Schedule next wake (dynamic, ~20-30 min fallback).
