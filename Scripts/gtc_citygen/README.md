# gtc_citygen — procedural Ocean Drive slice

Generates a stylized South Beach / Ocean Drive district for a new World Partition map
on a **Cesium** Miami base, using the full CityBeachStrip building palette. Pure-Python
solver offline; apply into the live editor via Unreal MCP. See `PROGRESS.md` for status
and the `caliber_city_spec.json` authority spec.

## Run offline (no editor needed)
```bash
cd Scripts/gtc_citygen
python3 generate_slice.py          # -> Content/GTCaliberAssets/Content/CityGen/{placement_plan,sidewalk_graph,validation_report}.json
python3 build_graph.py             # -> .../CityGen/lane_graph.json
python3 -m unittest discover -s tests   # 19 tests: layout invariants, overlap validator, graph connectivity
```
Tune the slice via `slice_config.json` (seed, extent, roads, columns, setbacks,
`foundation`, `cesium` block) and re-run. Same seed => byte-identical `placement_plan.json`.

## Files
- `caliber_city_spec.json` — authority spec (the whole Caliber City; this slice realizes its Ocean Drive district).
- `slice_config.json` — slice knobs.
- `assets.py` — verified asset catalog + selection.
- `layout.py` — geometry solver (rects on the 800 grid).
- `validate_overlap.py` — independent no-overlap validator.
- `generate_slice.py` — orchestrates + emits the artifacts.
- `build_graph.py` — sidewalk_graph -> lane_graph (port of Legacy_GTC/tools/build_lane_graph.py).
- `check_mcp.py` — pre-apply GO/NO-GO gate (validation PASS + MCP :8000 reachable).
- `apply_slice.tooljson` — editor-side apply payload (paged spawn, idempotent purge).
- `tests/` — unittest suite.

## Apply into the editor — RECOMMENDED: run in the editor's Python console
The reliable path (no MCP client needed; handles file IO + level instances):
1. Create the new WP map once in-editor (File > New Level > Open World). OpenCity is NOT WP.
2. Cesium base: run `Content/GTCaliberAssets/Content/Python/build_miami.py` in the editor's
   Python console (switch its tileset to Cesium World Terrain for stylized-on-terrain).
3. With the NEW map open and PIE off, in Output Log (switch Cmd -> Python) run:
   ```
   py "<repo>/Scripts/gtc_citygen/apply_in_editor.py"
   ```
   It purges only our `GTC_CityGen/OceanDrive` namespace, then spawns the 160 actors,
   left UNSAVED for review. For a quick pilot, set `LIMIT=25` at the top of the script.

## Alternative: Unreal MCP (.mcp.json declares `unreal-mcp` at :8000/mcp)
If the MCP client is wired into your agent session, drive `apply_slice.tooljson` page-by-page
(PURGE=True on page 0) after `python3 check_mcp.py` returns GO. Same idempotent namespacing.
