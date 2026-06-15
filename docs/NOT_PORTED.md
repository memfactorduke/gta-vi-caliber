# NOT PORTED (DROP) — Godot code/infra not carried to UE5

From the DROP section of `../../the migration plan`. Each is provided natively by UE5 or no
longer needed; recorded here with a one-line reason (paths are the old `GT-caliber` Godot repo).

> **Count note:** the brief said "25"; the live plan lists **26** — the INVESTIGATE #4 decision
> (deprecate the upstream-to-Godot policy) added entry 26 after that framing. All 26 are listed.

1. **WorldCore version probe** — `engine/src/worldcore/worldcore.{cpp,h}`, `worldcore_version.h` — runtime "is the native module present?" check is unnecessary in C++-native UE5 (version → `.uplugin`).
2. **NativeBench** — `engine/src/native_bench/*` — a "prove the C++ pipeline works" shim is pointless in a C++-native engine.
3. **register_types** — `engine/src/register_types.{cpp,h}` — replaced wholesale by `IModuleInterface` / `.uplugin` registration.
4. **SConstruct build** — `engine/SConstruct` — UE5 uses UnrealBuildTool (`.uproject`/`.Build.cs`/`.uplugin`); no SCons.
5. **GDExtension manifest** — `engine/worldcore.gdextension.example` — the GDExtension manifest format has no UE5 equivalent.
6. **engine/README.md (build doc)** — `engine/README.md` — Godot/GDExtension build doc; only the design *principles* survive as UE5 plugin reference.
7. **Dev/demo scripts** — `game/scripts/world/{mission_demo,debug_inspector}.gd` — diagnostics only; the UE5 editor supplies wireframe/collision debug, the mission demo retires with the framework.
8. **Group-cache perf utility** — `game/scripts/systems/group_cache.gd` — a Godot SceneTree-group caching hack; UE5 Subsystem queries + cached component refs make it unnecessary.
9. **Legacy procedural animator** — `game/scripts/player/character_animator.gd` — superseded by the skeletal AnimatedRig; skeletal animation covers all poses.
10. **In-vehicle radio synth placeholder** — `game/scripts/vehicles/{radio,radio_model}.gd` — stopgap arpeggio synth; real radio = the §2e radio subsystem + MetaSounds (M5).
11. **Night window emission driver** — `game/scripts/props/night_window_emission.gd` — superseded by the global day/night Material Parameter Collection; duplicates BuildingFacade emission.
12. **Benchmark & perf tooling** — `game/scripts/performance/{benchmark_config,benchmark_metrics,visibility_range}.gd` — UE5 `stat` + Unreal Insights replace the harness; Nanite/HLOD replace manual visibility-range.
13. **Benchmark/test scene** — `game/scenes/tests/benchmark_runner.tscn` (+ `game/tests/asset_gauntlet.tscn`) — headless QA harness scenes; UE5 Insights/Automation replace them.
14. **Screenshot/asset captures (~38)** — `game/tests/*_capture.gd` — Godot-rendering-specific; UE5 High-Res Screenshot / Sequencer / Movie Render Queue replace them.
15. **Test runners & benchmark glue** — `game/tests/{run_tests,run_legacy_tests,benchmark}.gd` — gdUnit4/legacy framework glue; the UE5 Automation runner + Insights replace them.
16. **Player/asset validation probes (~5)** — `game/tests/player_mara_*.gd`, `build_npc_variants.gd` — one-off rig/variant validation; obsolete after the UE5 Skeletal Mesh + IK Retargeter character port.
17. **gdUnit4 testing addon (265 files)** — `game/addons/gdUnit4/` — Godot-only test framework; UE5 has its own Automation/Functional framework (the tests themselves re-author).
18. **godot-cpp submodule** — `engine/godot-cpp @ 3a7edf0` — the Godot C++ binding layer; UE5 native code uses UnrealBuildTool + the UE5 C++ API — no binding layer exists.
19. **Godot native build script** — `tools/build_engine.sh` — invokes SCons to build GDExtension; replaced by UnrealBuildTool/UAT.
20. **Godot build documentation** — `docs/BUILDING.md` — Godot 4.6 + GDExtension/SCons steps; rewrite from scratch for UE5 editor + UBT + `.uplugin`.
21. **Godot lint/format config** — `.gdlintrc` — gdtoolkit (GDScript-only); UE5 C++ uses clang-format/clang-tidy.
22. **Git submodules** — `.gitmodules` — declares only `engine/godot-cpp`; the submodule is retired (see #18).
23. **One-command installers** — `install.sh`, `install.ps1` — bootstrap the Godot engine; UE5 assumes the engine via Epic Launcher/source.
24. **Greybox tile streamer + placeholder tiles** — `game/scripts/world/{tile_streamer,world_tile}.gd` — greybox playground retired; the OSM path uses the manifest-driven `DistrictStreamer` (INVESTIGATE #1).
25. **Procedural terrain (greybox)** — `game/scripts/world/{terrain_model,terrain}.gd` — greybox playground retired; OSM-district world (INVESTIGATE #1). `terrain_model`'s pure FBM may optionally return as a dormant Wave 1 utility if procedural wilderness is ever wanted.
26. **Upstream-to-Godot policy** — `docs/UPSTREAM.md` — deprecated; its premise evaporates under UE5 (SpatialHash → Mass, Impostor → Impostor Baker, TileStreamer → World Partition are exactly what UE5 provides NATIVE) (INVESTIGATE #4).
