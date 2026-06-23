# M3 — Seamless streaming world: pure-core checklist

Goal (ROADMAP M3 / VISION): drive or walk **4 km in any direction with no
loading screen**, stable 60 FPS on a mid-range GPU. This file tracks the
**pure-core, editor-free** pieces an autonomous `/loop` can build and unit-test
headlessly — plain non-`UObject` structs / static functions in the
`FPlayerMotion` pattern, each paired with a `GTC.*` automation test.

## What already exists (do NOT duplicate)

ROADMAP M3 marks these `[x]`, but the checked items reference the **old
`Source/GTC/` layout** and were *not* ported into the current `GTC_UE5` module —
verified by search on 2026-06-19: there is **no** tile-streaming / residency /
priority C++ in `Source/GTC_UE5/` today. The `Worldcore/` folder holds only
`FlowField`, `TrafficModel`, `CrowdSteering` (crowd/traffic cores), not streaming.

So in this module the streaming core is greenfield. Build it fresh under
`Source/GTC_UE5/World/Streaming/`, pure-core first.

## Pure-core pieces (headless; build these in the loop)

- [x] **Tile grid addressing** — `FStreamingGrid`: world⇄tile, tile
  center/bounds, Chebyshev tile distance, square residency-window enumeration.
  The coordinate substrate every other piece sits on.
  `World/Streaming/StreamingGrid.{h,cpp}`, tests `GTC.World.Streaming.Grid.*`.
- [x] **Load/unload priority from camera position + velocity vector** —
  `FTileStreamPriority`: anticipatory look-ahead distance so tiles *ahead* of the
  camera's motion load first; behind-and-far are least urgent.
  `World/Streaming/TileStreamPriority.{h,cpp}`, tests `GTC.World.Streaming.Priority.*`.
- [x] **Boundary hysteresis** — separate load vs. unload radii (+ optional dwell)
  so tiles straddling a ring boundary don't thrash load/unload every frame.
  `World/Streaming/TileHysteresis.{h,cpp}`, tests `GTC.World.Streaming.Hysteresis.*`.
- [x] **Per-tile LOD / impostor distance selection** — choose a tile's detail
  band (full / HLOD / impostor / unloaded) from camera distance with hysteresis.
  `World/Streaming/TileLodSelect.{h,cpp}`, tests `GTC.World.Streaming.Lod.*`.
- [x] **Frame-budget scheduler** — one bounded step per frame: admit at most N
  tile ops (or M ms of work) per tick so a burst of newly-needed tiles spreads
  across frames instead of hitching.
  `World/Streaming/StreamBudget.{h,cpp}`, tests `GTC.World.Streaming.Budget.*`.
- [x] **Residency / VRAM accounting** — track resident-set bytes against a
  budget; pick eviction victims (lowest priority / farthest) when over budget.
  `World/Streaming/ResidencyBudget.{h,cpp}`, tests `GTC.World.Streaming.Residency.*`.
- [x] **Residency planner** — ties grid + priority + hysteresis + budget into a
  deterministic per-frame *(load list, unload list)* decision, fed by a priority
  queue ordering. The headless brain the runtime adapter will drive.
  `World/Streaming/ResidencyPlanner.{h,cpp}`, tests `GTC.World.Streaming.Planner.*`.

> **Pure-core checklist COMPLETE (2026-06-19).** All seven editor-free pieces are
> implemented, compiled green together (`Build.sh GTC_UE5Editor` → Result:
> Succeeded), and covered by `GTC.World.Streaming.*` automation tests.

## Runtime adapter — headless half DONE (2026-06-22)

The `UWorldSubsystem` adapter was split: its planner-driving half is **headless-clean**
(it talks to World Partition only through an empty virtual seam, so it pulls no
WorldPartition module dependency), so it builds and is testable without the editor.
The WP cell hop *behind* the seam stays live-editor work (below).

- [x] **Stateful streaming brain** — `FStreamingResidency` (`World/Streaming/
  StreamingResidency.{h,cpp}`): owns the resident set + per-tile LOD band across
  frames, applies each `FResidencyPlanner::Plan` and re-bands with hysteresis.
- [x] **Runtime adapter** — `UWorldStreamingSubsystem` (`World/Streaming/
  WorldStreamingSubsystem.{h,cpp}`): a tickable `UWorldSubsystem` (the
  `UGTCTrafficSubsystem` shape) that samples player pos+velocity each tick, drives
  the brain, and forwards load/unload/LOD events to virtual WP hooks
  (`OnTileLoad`/`OnTileUnload`/`OnTileLodChanged`, no-ops by default). Exposes
  resident-count / last-load / player-tile getters for the debug HUD.
- [x] **4 km traverse acceptance probe** — `GTC.World.Streaming.Traverse.FourKmNoHitch`
  (`World/Streaming/Tests/StreamingTraverseTest.cpp`): drives the brain over a 4 km
  straight + 1 km past a 90° corner, asserts ring-stays-ahead, no thrash (loads
  inside / unloads outside the hysteresis band), budget respected, residency bounded.
  Verified on host clang against the real cores (834 steps, max-residency 47/169,
  every tile loaded exactly once — zero thrash through the corner). UObject glue +
  the automation wrapper compile-pending a maintainer `GTC_UE5Editor` build (not run
  here: rebuilding the editor module's dylib while the shared editor holds it open is
  forbidden — stated honestly per `AGENTS.md`).

## Needs the LIVE editor (STOP and ask — do not attempt headless)

- [ ] World Partition runtime cell wiring behind the adapter seam: a live-wired
  subclass overrides `OnTileLoad`/`OnTileUnload`/`OnTileLodChanged` to drive actual
  WP cell load/unload + the HLOD/impostor swap, on a WP-authored canonical map.
- [ ] Nanite / HLOD **impostor baking** for distant buildings (Impostor Baker).
- [ ] Streaming debug HUD readout (tiles resident, VRAM, frame budget) reading the
  adapter getters.
- [ ] Capture the 4 km live profile (streaming ≤ 1 ms main-thread); it gates whether
  the custom worldcore async streamer / impostor baker are ever promoted.

## Verification notes (this environment)

- One UE 5.7 editor is always running and must not be killed/relaunched, and the
  protect hook + project rules forbid spawning `UnrealEditor-Cmd`, so the headless
  `Automation RunTests` runner cannot be launched here. Each piece is **compiled**
  via `Build.sh GTC_UE5Editor` (allowed; does not touch the running editor) and
  the math is covered by `GTC.World.Streaming.*` automation tests for a maintainer
  to run. Commits/PR state this honestly per `AGENTS.md`.
