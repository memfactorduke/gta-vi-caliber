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
- [ ] **Frame-budget scheduler** — one bounded step per frame: admit at most N
  tile ops (or M ms of work) per tick so a burst of newly-needed tiles spreads
  across frames instead of hitching.
- [ ] **Residency / VRAM accounting** — track resident-set bytes against a
  budget; pick eviction victims (lowest priority / farthest) when over budget.
- [ ] **Residency planner** — ties grid + priority + hysteresis + budget into a
  deterministic per-frame *(load list, unload list)* decision, fed by a priority
  queue ordering. The headless brain the runtime adapter will drive.

## Needs the LIVE editor (STOP and ask — do not attempt headless)

- [ ] World Partition runtime cell wiring / actual async cell load+unload.
- [ ] Nanite / HLOD **impostor baking** for distant buildings (Impostor Baker).
- [ ] `UWorldSubsystem` adapter that drives the planner each tick and the
  streaming debug HUD readout (tiles resident, VRAM, frame budget).

## Verification notes (this environment)

- One UE 5.7 editor is always running and must not be killed/relaunched, and the
  protect hook + project rules forbid spawning `UnrealEditor-Cmd`, so the headless
  `Automation RunTests` runner cannot be launched here. Each piece is **compiled**
  via `Build.sh GTC_UE5Editor` (allowed; does not touch the running editor) and
  the math is covered by `GTC.World.Streaming.*` automation tests for a maintainer
  to run. Commits/PR state this honestly per `AGENTS.md`.
