# GTC City Generator — Ocean Drive slice progress

**Goal:** Build a NEW Miami map procedurally. First deliverable = the South Beach /
Ocean Drive district as a vertical slice, proving a generator that scales to the full
Caliber City spec. Method = pure-Python solver + apply into the live editor via Unreal
MCP. World base = **Cesium** (real Miami terrain + sun/sky) with the **full** stylized
CityBeachStrip building palette placed on top.

**Editor rule:** ONE shared live UE 5.8 editor. Never kill/relaunch/second-instance.
Act only through Unreal MCP (port 8000). Leave new actors UNSAVED for review on the
first apply; never auto-commit.

## Architecture (three-stage seam)
- **(A) Pure-Python solver** (`generate_slice.py` + `layout.py` + `validate_overlap.py`),
  NO `import unreal`. Roads -> ROW rects -> blocks -> plots -> buildings -> sidewalk/
  crosswalk graph -> furniture-lane dressing, all axis-aligned integer rects on the 800
  grid. No-overlap holds by construction (building ⊆ plot ⊆ block ⊆ land−ROW−water).
- **build_graph.py** — ports Legacy_GTC/tools/build_lane_graph.py (same constants) to bake
  `sidewalk_graph.json` -> `lane_graph.json` for the slice dir.
- **(B) Apply** — `apply_slice.tooljson` (ProgrammaticToolset.execute_tool_script payload),
  driven page-by-page by Claude over MCP. Pure executor: purge-by-namespace then spawn.

## Outputs (canonical, beside the future map)
`Content/GTCaliberAssets/Content/CityGen/`:
  placement_plan.json · sidewalk_graph.json · lane_graph.json · validation_report.json

## DONE — offline, verified (editor OFF)
- [x] M1 spec/config/assets. Full building palette wired (27 LI_Building + 3 LI_Resort +
  7 background + 4 skyscraper reserved for Downtown). Asset paths verified on disk;
  fairy-light palms corrected to 01/02/03/06; awning = SM_Awning_01_3m.
- [x] M2 solver + validator. Latest run: **validation PASS**, 160 actors — 43 buildings
  (2 resort anchors at the strip ends + a party-wall midrise Deco row + background),
  24 blocks, 13 roads, 36 crosswalks, 16 dressing props, 1672 sidewalk nodes.
- [x] M3 tests green (19/19): grid-snap, spacing invariant, containment chain,
  side-of-street, furniture-lane-clear, party-wall row, hero ends, determinism
  (placement_plan.json byte-identical on rerun). Overlap validator BITES on injected
  overlaps and accepts party-wall touches. lane_graph: one connected component
  (1708/1708 nodes), all 36 crosswalks bridge >=2 kerbs.
- [x] Cesium foundation wired into config (`foundation: cesium`): the solver no longer
  emits a flat ground plane or basic sky rig — those come from the Cesium step.

## Cesium foundation (decided: "use cesium + all asset buildings")
- Use `Content/GTCaliberAssets/Content/Python/build_miami.py` to set up the live
  CesiumGeoreference (South Beach 25.7907, -80.1300), CesiumSunSky, and a fly cam.
  It currently streams **Google Photorealistic 3D Tiles** (ion asset 2275207). For
  "stylized city on real terrain" without photoreal buildings clashing, switch its
  tileset to **Cesium World Terrain** (ion asset 1) — config field
  `cesium.tileset = "world_terrain"`. (build_miami.py edit is a one-liner; not yet made.)
- RESOLVED — coordinate reconciliation: `recenter_to_origin = true`. The solver keeps true
  spec coords internally (overlap is translation-invariant, so the slice still composes into
  the full city later), but the emitted placement_plan + graphs are shifted by
  `header.world_offset` (= [-284800, 0]) so the slice center sits at the Cesium georeference
  point (real South Beach land), not 3 km offshore. Buildings now span ~X[-16.7k,+18.8k],
  Y[-55.2k,+56k]; PlayerStart (26200,0). For the flat barrier island, building Z ~ sea level,
  so terrain-snap is minor.

## Apply path (IMPORTANT)
The Unreal MCP client is declared in `.mcp.json` (unreal-mcp @ :8000/mcp) but is NOT wired
into the coding-agent session, so the agent can't push actors over MCP. **Primary apply =
`apply_in_editor.py`** run in the editor's Python console (uses the `unreal` module + real
file IO, like build_miami.py / enhance_citybeachstrip_nav.py). It purges only our namespace,
spawns the 160 actors, leaves them UNSAVED. `apply_slice.tooljson` remains for the MCP path
if it gets rewired.

## TODO — needs the live editor (handoff; user runs apply_in_editor.py)
- [ ] **M0** Create the new WP map once in-editor (File > New Level > Open World/WP),
  e.g. `…/CityGen/Maps/OceanDriveSlice`. OpenCity.umap is NOT World Partition, so don't
  clone it. (We cannot relaunch the editor, so this is a human step.)
- [ ] **M4** Pilot apply (ONE block, unsaved): run `check_mcp.py` (GO/NO-GO) → confirm
  PIE off → execute_tool_script with `apply_slice.tooljson`, page 0, PURGE=True, ~20
  actors. FocusOnActors + CaptureViewport to verify; iterate frontage/yaw. **Verify the
  LI_ level-instance spawn path here** (add_to_scene_from_asset on a .umap may need a
  dedicated LevelInstance API).
- [ ] **M5** Full slice apply (page <=50). NavMeshBoundsVolume, PlayerStart, and POI
  markers are ALREADY in placement_plan.json (spawned by apply_slice), so the only extra
  step is one `EditorLevelLibrary.editor_build_paths()` MCP call after the last page, plus
  setting `Kind` on the POI markers (carried in each POI actor's `props`). Capture top-down + 3/4.
- [ ] **M6** Save dirty packages for review; branch off `main`; commit only on approval.

## Open MCP questions to settle during M4 (be honest, verify live)
- LI_ level-instance spawn API via the toolset (vs static-mesh add).
- Exact destroy/delete tool name (`ActorTools.destroy_actor` assumed) and whether
  `get_actor_folder` exists (else filter purge by label prefix only).
- execute_tool_script file IO is unproven → we inject actor pages inline (current design).

## FULL CALIBER CITY buildout (looped, /loop dynamic — "block by block, road by road, no rush")
Goal: generate + validate the WHOLE city offline, then apply when the editor/MCP is healthy.
Pipeline (spec's strict order, guarantees global no-overlap): **roads -> blockmask -> per-district
fill -> coastline -> dressing**. Building independent districts would overlap at boundaries
(Downtown & Wynwood bboxes already intersect), so blocks are carved from the global road network
and each block is filled by whichever district contains it.

Plan of record: `city_config.json` (all 5 grid districts + Ocean Drive ref + road network),
synthesized from workflow w5zzw3h00 (6 agents, grounded in caliber_city_spec.json + verified assets).
Outputs land in Content/GTCaliberAssets/Content/CityGen/.

Progress:
- [x] city_config.json — 5 districts (Downtown rot18, Wynwood rot12, Little Havana, Port, Suburban)
  each with center/extent/rotation/road-spacing/columns/verified building_assets; + road network.
- [x] ROADS layer — `generate_city_roads.py` -> road_network.json (centerlines) + city_roads_plan.json
  (300 tiles: beltway, ring, 8 arteries, I-95, 3 causeways), full ~5km footprint.
- [x] MASKS / blockmask — `generate_city_blocks.py` rasterizes road ROWs + water (ocean/bay/river/
  port) + coastline radius -> global free mask (590k cells). Primary roads carve ~17 big district
  regions; the fine blocks come from each district's own local grid in the fill step.
- [WIP] DISTRICT FILL — `generate_city_district.py` lays each district's rotated local grid, gated by
  the global free mask + per-cell district OWNERSHIP (nearest-center), so neighbours (overlapping
  bboxes like Downtown/Wynwood) never place overlapping buildings; no-overlap by construction.
  Accumulates into city_buildings.json (idempotent per district). DONE: Downtown = 123 buildings.
  Next (one per wake): Wynwood, Little Havana, Port/Marina, Outer suburban. (Fix applied: configs'
  column frontage_cm is lot WIDTH, not a setback — fill uses plot_module_cm + party-derived setback.)
- [x] ALL 5 districts filled: Downtown 123, Wynwood 100, Little Havana 142, Port 20, Suburban 350
  = 735 district buildings (city_buildings.json).
- [x] COMBINED CITY PLAN — `generate_city.py` -> city_plan.json: **1197 actors (389 LI level
  instances)**, ~5.8 x 5.8 km = roads(300) + districts(735) + Ocean Drive at spec coords(160) +
  city nav/PlayerStart. The whole city is generated + validated offline.
- [ ] POLISH (later): organic coastline refinement, per-district local-street meshes, more dressing.
- [ ] APPLY — MCP verified working (PIE off; tools respond). The LI level-instance path is SOLVED:
  SceneTools.create_level_instance(level_path, name, xform) spawns LI_ buildings; static meshes via
  add_to_scene_from_asset; engine actors via add_to_scene_from_class. BLOCKED: get_current_level returns
  CityBeachStrip (the production map) — must NOT apply there. NEXT: user opens/creates the new WP map
  (File > New Level > Open World, save as CityGen/Maps/CaliberCity), then apply via MCP (stage by folder:
  roads -> districts -> ocean drive; heavy 389 LI). apply_in_editor.py (PLAN_FILE=city_plan.json) is the
  editor-Python fallback. No SceneTools "new empty level" tool exists, so map creation is a user step.
Apply note: the LI_ level-instance buildings are heavyweight (thousands of sub-actors each); a full-city
plan will be large — lean on background static meshes + stage the apply / HLOD. Roads are static meshes.
