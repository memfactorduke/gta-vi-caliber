# Roadmap

> **Where are we right now?** This file is the forward task board, and its
> checkboxes lag the code. For the authoritative current state and the
> dependency-ordered build order, read [`BUILD_ORDER.md`](BUILD_ORDER.md). For
> which gameplay systems are wired versus still on the shelf,
> [`SYSTEMS.md`](SYSTEMS.md) is the source of truth.

The path from empty repo to trailer-grade open world, as shippable milestones.
**Every unchecked box is an invitation** — comment on or open an issue before
starting so work isn't duplicated. Boxes only get checked when the feature is
on `main`, passing CI, and playable.

Maintainers also point autonomous agent loops at this file: editing it is how
humans steer the machines. Keep tasks small, concrete, and verifiable.

---

## M0 — Bootstrap ✅ (current)

Goal: every clone runs instantly; contribution pipeline works end to end.

- [x] Repo structure, licenses, contribution docs, agent contract
- [x] UE5 project with playable sandbox (ground, sky, sun)
- [x] Third-person character: walk, sprint, jump, mouse-look camera
- [x] `tools/check.sh` local gate = CI (format, lint, UnrealBuildTool build, boot/automation tests)
- [x] GitHub Actions CI, issue templates, PR template
- [x] Stand up the in-module `Tests/` UE5 Automation test harness (`GTC.*` prefixes) and the test runner
- [x] First exported build artifacts (Linux/Windows/macOS) uploaded by CI on tag
- [x] Stable menu-to-world startup with bounded district and crowd loading

## M1 — Locomotion & camera feel

Goal: moving around is *fun* before there is anything to do.

- [x] Character model + run/walk/idle/jump animations (original or CC0; see `art` issues)
- [x] Acceleration/deceleration curves, air control, coyote time (+ jump buffering)
- [x] Camera: collision probe, shoulder offset, FOV kick on sprint
- [x] Footstep audio hooked to surface type
- [x] Greybox "movement playground" scene with stairs, slopes, gaps, ladders
- [x] Gamepad support + rebindable input
  - landed: gamepad default bindings, `InputRemap` save/load persistence, and
    settings-screen binding controls with reset-to-defaults.

## M2 — Vehicles

Goal: get in a car, drive it, crash it, get out.

- [x] Chaos Vehicles-based car with tuned suspension (greybox body)
- [x] Seamless enter/exit interaction
- [x] Chase camera with speed-based FOV and look-behind
- [x] Engine/tire/impact audio loops
- [x] Damage model v1 (visual deformation can wait; mechanical state first)
- [x] Motorbike + boat prototypes

## M3 — Streaming world foundation *(engine track begins)*

Goal: walk or drive 4 km in any direction with no loading screen.

- [ ] World partitioned into tiles with seam-free LOD terrain
- [x] C++ district residency with threaded 128 m tile preparation, near-ring
  collision/navigation, HLOD/occluders, and one bounded main-thread step per frame
- [ ] **async tile streaming on World Partition / a C++ module** (load/unload around camera, priority by velocity vector)
- [ ] **runtime impostor baker (Nanite/HLOD + Impostor Baker)** for distant buildings
- [x] Large World Coordinates / floating-origin to dodge float precision at distance
- [x] Streaming debug HUD (tiles resident, VRAM, frame budget)
- [x] Benchmark scene + captured profile

## M4 — A living district

Goal: one city district that feels inhabited.

- [x] Blockout of a coastal district: streets, sidewalks, shore, 30+ building footprints (VeniceBeach.umap: 1332 real footprints, sand shore + Ocean v1, golden-hour sky)
- [ ] Road network graph + traffic system (**native-module candidate after profiling**)
  - landed: `Source/GTC/AI/NavGrid` A* grid (tested) + `Source/GTC/AI/Traffic*` streaming kinematic
    cars that route on a world-baked navmesh and car-follow (queue, don't overlap).
  - landed (UE): ambient traffic now drives a TRUE routable road graph — `FRoadNetwork`
    (A*) + `FRoadRoute`/`FLanePath` + `FTurnChoice` + `FTrafficModel` composed by
    `FTrafficAgent` (cars route to a destination, turn at junctions, queue per lane).
    `UGTCTrafficSubsystem` streams them on an `FCityGrid` graph and feeds the pedestrian
    look-before-you-cross reflex. Still TODO: replace the `FCityGrid` block source with
    real OSM/citygen polylines into the same `FRoadNetwork` seam; junction traffic lights.
- [ ] Pedestrian crowds: navmesh flows, reactions (flee/gawk), spawn/despawn invisible to player
  - landed: `Source/GTC/NPC/CrowdDirector` streams palette/stature/gait-varied premium
    humans using imported rigged man/woman visuals with shared runtime animation
    retargeting; spawn/despawn around the player, ground-snapped, kept out of
    buildings via the baked navmesh. Reactions (flee) in `Pedestrian`. TODO:
    route peds *through* `NavGrid.find_path` (now that buildings are solid)
    instead of straight wander; gawk reaction. Demo:
    the LivingCity level (`Content/.../LivingCity.umap`) (crowd + traffic).
- [x] Time-of-day cycle driving sun, streetlights, building windows
- [ ] Weather fronts: clear → overcast → rain, wet-surface materials
- [x] Ocean v1: Gerstner/FFT water with shoreline blend (**native-module candidate**)

## M5 — Play

Goal: it is a *game* now.

- [ ] Mission framework (triggers, objectives, fail/retry) + 3 sample missions
- [ ] Wanted/heat system with police response escalation
- [ ] Minimap + full map UI
- [ ] Radio: streaming music channels in vehicles (CC-licensed tracks)
- [ ] Save/load of world + player state
- [ ] NPC dialogue barks

## M6 — Trailer-grade polish

Goal: the acceptance test — a 90-second in-engine trailer from a release build.

- [ ] Lighting pass: GI (Lumen tuning or a native solution), volumetrics
- [ ] Ocean v2: foam, wakes, buoyancy
- [ ] Crowd density pass (**GPU-driven crowd rendering (Mass + ISM/Nanite)**)
- [x] Cinematic camera tooling for capture
- [ ] Performance lockdown: 60 FPS @ 1080p mid-range GPU, captured profiles
  - [x] Phase 0 deterministic release harness, strict runtime gate, and working
    distance/visibility culling
  - [ ] Capture the Phase 0 baseline on RTX 3060-class target hardware
- [ ] Cut, score, and publish the trailer

---

## Engine track (parallel, ongoing)

Native C++ modules; rules in [ARCHITECTURE.md](ARCHITECTURE.md). Reusable pieces
stay as documented modules.

- [x] UnrealBuildTool module scaffolding + first compiled C++ module on all 3 platforms
- [x] CI job building the native modules and running their C++ automation tests
- [ ] Streaming module (M3)
- [ ] Impostor baker (M3)
- [ ] Crowd/traffic simulation core (M4+, only with profile evidence)
- [ ] Ocean simulation (M4+)
- [ ] Module change log in `docs/MODULES.md` (create with first module)
