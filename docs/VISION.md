# Vision

## What we are building

An original, fully open-source open-world game whose quality bar is the
fidelity of modern AAA open-world trailers: a dense, living coastal city you
can drive, walk, and cause trouble in — seamless streaming, convincing crowds,
water you want to look at, weather that changes how the city feels.

We will not get there by wishing. We get there by shipping a playable build
every single week and never letting the repo regress below "clone it, open it,
it runs."

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

## Why Unreal Engine 5, and why we fork nothing

We build **on top of** Unreal Engine 5.7:

1. **The game layer:** UE5 C++ in `Source/`, with levels, Blueprints, UMG,
   and assets in `Content/`. Anyone can contribute here (C++ or Blueprint).
2. **The engine-feel layer:** custom C++ modules plus built-in UE5 systems
   (World Partition streaming, Mass crowd/traffic, Nanite/HLOD, ocean) for the
   places where stock features need extension.

We extend UE5 with C++ modules and engine features **without forking the engine
source**. Engine upgrades stay clean, contributors use the stock Unreal editor,
and reusable pieces stay as documented modules. Engine-source patches are a last
resort kept isolated and minimal: a permanent private fork is a failure state,
not a goal.

## How we work

- **Playable trunk.** `main` always runs. CI enforces it headlessly.
- **Vertical slices over horizontal layers.** One drivable block of city with
  working streetlights beats ten systems at 40%.
- **Built-in features first, custom C++ when profiled.** Custom native code
  must justify itself with a captured profile (see
  [ARCHITECTURE.md](ARCHITECTURE.md)).
- **Original assets only.** Provenance for every file ([ASSETS.md](ASSETS.md)).
  We are studying the look of AAA trailers, not copying their content.
- **Humans and AI agents contribute under the same contract**
  ([../AGENTS.md](../AGENTS.md)). The roadmap is the shared steering wheel.
