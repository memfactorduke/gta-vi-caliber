# Vision

## What we are building

An original, fully open-source open-world game whose quality bar is the
fidelity of modern AAA open-world trailers: a dense, living coastal city you
can drive, walk, and cause trouble in — seamless streaming, convincing crowds,
water you want to look at, weather that changes how the city feels.

We will not get there by wishing. We get there by shipping a playable build
every single week and never letting the repo regress below "clone it, press
F5, it runs."

## The quality bar, made concrete

"Trailer-level" is not a vibe, it is a checklist. Each item graduates from the
roadmap into the build:

| Pillar | Bar |
| --- | --- |
| **World** | A coastal city district ≥ 4 km² streamed with no loading screens, stable 60 FPS on a mid-range GPU |
| **Traversal** | On-foot, vehicles (car + bike + boat), seamless enter/exit, camera that never fights you |
| **Life** | Crowds and traffic that react: flee, honk, gawk, call for help |
| **Atmosphere** | Time-of-day, weather fronts, ocean with real shoreline interaction, night lighting worth screenshotting |
| **Play** | Mission framework, wanted/heat system, radio, persistence |
| **Polish** | A 90-second in-engine trailer captured from a release build is the final acceptance test |

## Why Godot, and why we fork nothing

We build **on top of** Godot 4.x, upstream-first:

1. **Game layer (`game/`)** — typed GDScript, scenes, assets. Anyone can
   contribute here with zero C++ knowledge.
2. **Engine layer (`engine/`)** — GDExtension C++ modules for the systems
   where stock Godot runs out of headroom: world streaming, crowd/traffic
   simulation, impostor generation, GPU-driven foliage, ocean.

GDExtension means we extend the engine **without forking it**. Engine updates
stay one `brew upgrade` away, contributors don't need a custom editor build,
and anything generically useful gets offered upstream to Godot itself. If we
ever hit a wall that genuinely requires patching the engine core, the patch
lives in `engine/patches/` with an upstream PR opened the same week — a
permanent private fork is a failure state, not a goal.

## How we work

- **Playable trunk.** `main` always runs. CI enforces it headlessly.
- **Vertical slices over horizontal layers.** One drivable block of city with
  working streetlights beats ten systems at 40%.
- **GDScript first, C++ when profiled.** Native code must justify itself with
  a captured profile (see [ARCHITECTURE.md](ARCHITECTURE.md)).
- **Original assets only.** Provenance for every file ([ASSETS.md](ASSETS.md)).
  We are studying the look of AAA trailers, not copying their content.
- **Humans and AI agents contribute under the same contract**
  ([../AGENTS.md](../AGENTS.md)). The roadmap is the shared steering wheel.
