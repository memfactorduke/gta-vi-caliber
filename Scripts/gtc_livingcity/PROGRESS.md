# M4 — A living district: pure-core progress

Goal (ROADMAP M4): one city district that feels inhabited — **real road-graph
traffic** (cars route true roads with spacing, intersections, turns) and
**navmesh crowds with reactions** (peds path through walkable space, flee/gawk
on a rising edge). Today cars route a baked walkability grid and peds
straight-wander; this loop closes those gaps one editor-free pure-core piece at
a time.

## Working method

Every iteration adds ONE plain non-UObject `F…` struct (double precision, XZ
plane / Y-up, the repo's pure-core convention) under `Source/GTC_UE5/…`, paired
with an in-module `GTC.*` automation test in a sibling `Tests/` folder. One
concern per commit; tick a box here on green; PR into `main` when the slice is
coherent.

**Verification in this environment.** A live UE 5.7 editor is running (it must
never be closed or have a second instance launched — see `CLAUDE.md`), so the
in-tree UnrealBuildTool build and the `UnrealEditor-Cmd` headless automation run
cannot be exercised here without risking that editor. Following the repo's own
precedent (`docs/NPC_LIVING_CITY.md`: pure-core "validated out-of-tree"), each
piece is compiled and run **out-of-tree** with `clang++` against a minimal
`CoreMinimal.h` shim (`oracle/ue_shim/`), executing the *real* `.h/.cpp`
unmodified and asserting the same cases the `GTC.*` UE test asserts. Run all
oracles with `Scripts/gtc_livingcity/oracle/run.sh`. The in-editor build +
automation pass remains the outstanding **live-editor** check (stop-and-ask).

## What already exists (read before adding)

- `World/RoadNetwork/FRoadNetwork` — OSM polylines → junction nodes + two-way
  directed segments + A* node path (`FindPath`), nearest-point, spatial index.
  This IS the true road graph. Missing: lanes (lateral offset / side of road),
  and the glue from a node path to a *drivable* lane a car follows.
- `Worldcore/FTrafficModel` — IDM longitudinal accel (car-following), given a
  gap + leader speed. Missing: selecting the leader / gap on a lane.
- `Worldcore/FCrowdSteering`, `Worldcore/FFlowField` — boids + Dijkstra flow.
- `NPC/Steering/FNpcSteering` (seek/arrive/separation/`AdvanceWaypoint`),
  `FPedestrianTraffic` (curb-safety, dodge, `NearestThreat`).
- `AI/Gps/FGpsNavigation` — route progress for a player-driven polyline (not
  arc-length sampling, not lane geometry).
- `NPC/Decision/FNpcReaction` — per-tick verb (ignore/gawk/flee). Missing: a
  *stateful* FSM with rising-edge triggers + cooldowns (no per-frame flicker).
- `NPC/Agent/UGTCCrowdSubsystem` — streams the crowd around the player; does
  spawn/despawn inline. Missing: extracted, testable budgeting math.

## Pure-core checklist (cars route real roads, peds path + react)

- [x] **Lane geometry** (`FLanePath`): a drivable lane built by offsetting a
      centerline polyline to the side of travel; arc-length parameterised; pose
      (pos+heading) at distance `s`; advance `s` clamped to the lane. The glue a
      car rides. `GTC.World.LanePath`. ✅ oracle 28/28.
- [x] **Node path → centerline** (`FRoadRoute`): turn an `FRoadNetwork` A* node
      path into the ordered centerline polyline a car drives (+ thread an arbitrary
      spawn/destination onto the ends), so cars route the real graph instead of a
      grid. `GTC.World.RoadRoute`. ✅ oracle 11/11.
- [x] **Car-follow leader/gap** (`FTrafficLane`): given cars by arc-length
      position on one lane, find each car's leader + bumper gap (feeds
      `FTrafficModel`), so a queue forms without overlap. `GTC.AI.Traffic.Lane`.
      ✅ oracle 11/11.
- [x] **Intersection right-of-way / stop arbitration** (`FIntersection`):
      which approach proceeds, who yields/stops, deterministic tie-break, plus a
      stop-line-as-stopped-leader gap for FTrafficModel, so cars don't pile
      through a junction. `GTC.AI.Traffic.Intersection`. ✅ oracle 11/11.
- [ ] **Turn / lane choice at junctions** (`FTurnChoice`): pick the next lane
      at a node (continue / turn), respecting the planned route. `GTC.AI.Traffic.Turn`.
- [ ] **Pedestrian path request** (grid A* `FNavGrid`): point-to-point path over
      a walkability grid so peds route *through* walkable space (around solid
      buildings) instead of straight-wandering; waypoints feed
      `FNpcSteering::AdvanceWaypoint`. `GTC.NPC.NavGrid`.
- [ ] **Flee/gawk reaction FSM** (`FReactionState`): rising-edge triggers +
      cooldowns + hysteresis over `FNpcReaction::Decide`, mirroring the repo's
      bark/flinch idiom, so a citizen commits to a reaction instead of
      flickering. `GTC.NPC.Decision.ReactionState`.
- [ ] **Crowd spawn/despawn budget** (`FCrowdBudget`): from live count, target,
      ring radii, per-pass cap and per-citizen distances, decide how many to
      spawn and which to retire — invisible to the player. `GTC.NPC.CrowdBudget`.

## Done log

- `FLanePath` (`World/RoadNetwork/LanePath.{h,cpp}` + `Tests/LanePathTest.cpp`,
  `GTC.World.LanePath`). Lane offset geometry + arc-length pose/advance. Out-of-tree
  oracle 28/28 (`oracle/run.sh`). In-editor build + automation run still pending
  (live editor up).
- `FTrafficLane` (`AI/Traffic/TrafficLane.{h,cpp}` + `Tests/TrafficLaneTest.cpp`,
  `GTC.AI.Traffic.Lane`). Leader/gap selection that feeds FTrafficModel → spaced
  queue, no overlap. Out-of-tree oracle 11/11. In-editor run pending.
- `FRoadRoute` (`World/RoadNetwork/RoadRoute.{h,cpp}` + `Tests/RoadRouteTest.cpp`,
  `GTC.World.RoadRoute`). Node path → drivable centerline (+ end threading);
  completes FRoadNetwork A* → FRoadRoute → FLanePath → FTrafficModel pipeline.
  Out-of-tree oracle 11/11. In-editor run pending.
- `FIntersection` (`AI/Traffic/Intersection.{h,cpp}` + `Tests/IntersectionTest.cpp`,
  `GTC.AI.Traffic.Intersection`). Right-of-way arbitration (priority → FCFS →
  yield-to-right) + stop-line gap. Out-of-tree oracle 11/11. In-editor run pending.
