# NATIVE features — what UE5 provides in place of the old Godot modules

Per the FOUNDATION section of `../../the migration plan`, these 16 capabilities were custom
code in the Godot project and are **provided natively by Unreal Engine 5** — so the old C++/GDScript is
**not ported**; we configure the engine feature instead. Each note records the replacement and where it
is set up in this repo. (Asset-backed items — the MPC, Input assets, Landscape, materials — are content
that `tools/ue5/setup_foundation.py` and later editor work create; this sandbox has no running editor,
so those `.uasset`s are not committed here. See `FOUNDATION_SETUP.md`.)

1. **SpatialHash** (`engine/src/worldcore/spatial_hash.*`) → **Mass `FHashGrid2D`** / engine spatial
   acceleration (Octree, physics broadphase). The uniform XZ neighbour-grid becomes a Mass hash grid;
   the `MassEntity` + `MassGameplay` plugins are enabled in `GTC_UE5.uproject`. No custom grid module.

2. **Impostor** (octahedral LOD, `engine/src/worldcore/impostor.*`) → **Nanite** (automatic geometry
   LOD) + **HLOD** for distant proxies + the built-in **Impostor Baker** for billboard atlases. Nanite
   is enabled project-wide (`r.Nanite.ProjectEnabled=True` in `DefaultEngine.ini`); the bespoke
   screen-size/octahedral swap logic goes away.

3. **TileStreamer** (`engine/src/worldcore/tile_streamer.*`) → **World Partition** (+ Level Streaming,
   Data Layers, HLOD). Residency, load/unload and distance LOD are built in; the project's default map
   is the WP-enabled OpenWorld template (`GameDefaultMap` in `DefaultEngine.ini`).

4. **District streaming & residency** (`world/{district_streamer,streaming,tile_math}.gd`) → **World
   Partition** grid residency + Data Layers. The manifest-driven district set maps onto WP cells. (The
   travel-direction lookahead, if its feel matters, is a small companion that reorders the WP load
   queue — that lives in Wave 3, not here.)

5. **Floating-origin rebasing** (`world/{floating_origin,origin_math}.gd`) → **Large World
   Coordinates** (double-precision world). Rebasing is automatic in UE5.7; the custom origin-shift hack
   and its `origin_shifted` signal are deleted (consumers re-point to WP streaming events).

6. **District materials** (procedural, `world/{ground_material,ground_material_bindings,pbr_material}.gd`)
   → **UE5 Materials / Material Instances** + a material-parameter DataTable, with noise via Material
   expression nodes. Runtime material-building code goes away; texture sets migrate to the Content
   Browser (mind the SamplerType/compression rule in `../UE5_NOTES.md` §1.16).

7. **Quality & post-process** (`world/{color_grade_lut,graphics_quality,world_quality,lod_util}.gd`) →
   **Post-Process Volume** (color-grading LUT import) + the **Scalability** system + Nanite/HLOD for
   LOD. The LOW–ULTRA tiers are authored in `Config/DefaultScalability.ini` (@0..@3, +@4 cinematic);
   the old NPC/traffic density multipliers feed **Mass LOD** rather than a Scalability group.

8. **Facade material bindings** (`world/facade_material_bindings.gd`) → **Material Instance** + a
   **Material Parameter Collection**. The shared globals `world_night_amount` and `world_wetness` live
   in the MPC `MPC_GTCGlobals` (created by `tools/ue5/setup_foundation.py`); facade/road/neon materials
   read it instead of Godot global shader params.

9. **Pathfinding grid & smoothing** (`ai/{nav_grid,path_smoother}.gd`) → **Recast NavMesh** + its
   built-in string-pull path simplification. The custom uniform-grid A* and smoother are dropped; add a
   `NavMeshBoundsVolume` + `RecastNavMesh` per streamed area (NavigationSystem is core, no plugin).

10. **Neighbor grid** (`ai/neighbor_grid.gd`) → **Mass `FHashGrid2D`** / engine spatial acceleration —
    the same capability as #1 (`MassEntity` enabled). The native-vs-GDScript dispatch collapses to one
    Mass path.

11. **Vehicle physics layers Chaos covers** (`vehicles/{traction,weight_transfer,aerodynamics}.gd`) →
    **Chaos Vehicles** suspension/tire model + air-resistance/downforce. The `ChaosVehiclesPlugin` is
    enabled in `GTC_UE5.uproject`; the friction-circle/load-shift/drag layers retire (kept only as
    feel reference — verify Chaos defaults match intended handling, else expose as tuning).

12. **Camera shake (trauma)** (`camera/camera_shake.gd`) → **UE5 CameraShake**
    (`UPerlinNoiseCameraShakePattern`, or legacy MatineeCameraShake). Perlin trauma shake is provided
    out of the box; drive trauma from impact speed via a CameraShake source. Core engine, no plugin.

13. **Cinematic camera & spline** (`camera/{cinematic_camera,camera_path}.gd`) → **Sequencer** + **Cine
    Camera Actor** (+ **Spline Component** for arc-length). Scripted dolly moves and constant-speed
    arc-length playback come from Sequencer; the custom Catmull-Rom dolly retires.

14. **Traffic-lens shader** (`shaders/traffic_lens.gdshader`) → a standard **unlit material with Vertex
    Color → Emissive** on an **Instanced Static Mesh**. Per-instance lens colour drives emission; no
    custom shader. (Master material authored in Wave 3; this note records the approach.)

15. **Binary asset library** (`game/assets/**`, 407 files) → **re-import into UE5**: GLB → Static/
    Skeletal Meshes (Interchange glTF path, `bUseInterchangeFramework=True`), textures → Material
    Instances, ogg/wav → SoundWaves/MetaSounds, video → MP4. Data carries over; mind normal-map tangent
    space (OpenGL→DirectX Y-flip), ORM packing, and the `docs/ASSETS.md` provenance ledger.

16. **App icon (re-import)** (`game/icon.svg`) → raster **PNG/ICO** in the UE5 platform config
    (Project Settings → Platforms; macOS `.icns` / Windows `.ico`). The SVG is rasterised at the
    required sizes; the Godot export icon is not ported.
