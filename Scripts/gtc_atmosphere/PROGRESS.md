# M6 Atmosphere — pure-core progress

Driving **docs/ROADMAP.md M6** ("Trailer-grade polish") forward, **headless only**:
ocean v2 (foam, wakes, buoyancy) and weather/lighting that read as trailer-grade
(night lighting worth screenshotting, ocean with real shoreline interaction —
docs/VISION.md "Atmosphere").

Each item below is an **editor-free** simulation/parameter core: a plain
non-UObject `F…` struct of `static` methods (double precision), following the
`FSkyModel` / `FWeatherSystem` pattern in `Source/GTC_UE5/World/Environment/`.
Every core ships with a `GTC.*` automation test **and** a standalone clang
verifier under this folder so it can be proven green without the editor.

## What already landed (baseline, do not redo)

- `FSkyModel` — continuous time-of-day → sun pitch/yaw, daylight, warmth,
  skylight, stars. Tests `GTC.World.Sky.*`.
- `FWeatherSystem` / `FWeatherParams` — weather presets + timed transition blend
  + adjacent-severity director. Tests `GTC.World.Weather.*`.
- `AGTCWeatherController` — actor adapter; drives sun/skylight/fog + an MPC
  (`MPC_GTCWeather`) with CloudCoverage/CloudDensity/RainIntensity/Wetness/Wind/
  Daylight/SunWarmth/StarOpacity/SkyDesaturation. Auto-creates the sky rig.
- **Ocean v1 is material-only** — ROADMAP marks "Gerstner/FFT water with
  shoreline blend" done, but there is **no C++ ocean code**. Everything in the
  Ocean section below is net-new pure-core, the foundation for v2.

## Headless test harness

`Scripts/gtc_atmosphere/run_checks.sh` compiles each verifier against a tiny
`FMath`/`CoreMinimal.h` shim (it `#include`s the **real** core `.cpp`), so the
actual shipping math is what runs. The in-editor UE automation suite
(`UnrealEditor-Cmd … Automation RunTests GTC.World`) is **not** launched here —
the editor-protect hook + one-editor rule forbid it; the user/CI runs that.

## Pure-core checklist

### Ocean v2
- [x] **OceanSurface** — Gerstner wave sum: height + horizontal displacement +
  surface normal at a world (X,Y) and time, from a set of `FGerstnerWave`.
  Foundation everything else samples. `GTC.World.Ocean.Surface`.
- [x] **Buoyancy** — sample N hull points against the surface height → Archimedes
  buoyant force + restoring torque + linear/angular drag at the waterline.
  `GTC.World.Ocean.Buoyancy`.
- [x] **Wake & foam** — from boat speed/heading + hull, Kelvin wake half-angle,
  wake strength, foam spawn rate; whitecap foam from wave steepness/Jacobian.
  `GTC.World.Ocean.Wake`.
- [x] **Shoreline** — shore blend + wetness band from wave height vs terrain
  depth: shallow-water whitening, run-up wet line that follows the swash.
  `GTC.World.Ocean.Shore`.

### Weather / lighting
- [x] **WeatherFront** — spatial+temporal clear→overcast→rain front profile
  (a moving front line over the map) and the **MPC_GTCGlobals** scalar values it
  drives (`world_wetness`, `world_night_amount`). `GTC.World.WeatherFront`.
- [x] **SurfaceWetness** — rain → accumulation/drying curve so wetness *lags*
  rain (puddles build, streets dry slowly). Feeds MPC `world_wetness`.
  `GTC.World.Wetness`.
- [x] **NightLights** — streetlight/lit-window activation vs time-of-day:
  hysteretic dusk-on/dawn-off threshold + per-light jitter so the city doesn't
  pop on in unison. Drives `world_night_amount`. `GTC.World.NightLights`.

> **Pure-core checklist exhausted (2026-06-19).** All 7 cores landed with
> `GTC.World.*` tests + headless verifiers, all green via `run_checks.sh`. The
> only remaining M6 atmosphere work is the **visual hookup**, which needs the
> live editor — see below. Stop-and-ask reached.

## Live-editor visual hookup

### Done (2026-06-19, live editor)
- [x] **Ocean shoreline foam** — added a `DepthFade`-based foam term to the
  CityBeachStrip ocean master `MM_OceanWater` (mask `(1-DepthFade(ShoreFoamWidth))^1.6
  * ShoreFoamAmount` lerps base colour → `ShoreFoamColor`); tuned on `MI_Ocean`
  (width 800, amount 1.3, near-white). Replaces the hard waterline with a natural
  foam band. **Saved** to both `.uasset`s. Recipe: `apply_ocean_foam.py`
  (idempotent, self-saving). Verified in daylight via a temp sun rig.

> **Editor visual loop that works here** (the prior session said capture needs
> the user): `unreal_capture_viewport` returns STALE frames — instead drive the
> camera with `UnrealEditorSubsystem.set_level_viewport_camera_info`, fire
> `HighResShot <w>x<h>` via `execute_console_command`, and Read the PNG from
> `Saved/Screenshots/MacEditor/`. `LevelEditorSubsystem.editor_set_viewport_realtime(True)`
> exists. Editor-static has NO sun (the weather controller's sun is PIE-only), so
> spawn a temp movable DirectionalLight + SkyAtmosphere + realtime SkyLight
> (tagged, then destroy) to grade in daylight. vibeue `execute_python_code` has a
> 30s timeout — `recompile_material` can exceed it (edits still apply); MI param
> sets + post-compile saves are fast. The ocean = 6 planes at X≈69000, Z=-500
> using `MI_Ocean`; `MPC_GTCGlobals` is currently UNUSED by any material.

### Still to do (needs the live editor)
- [ ] Lumen GI / volumetric fog tuning for trailer-grade night lighting.
- [ ] Daytime turquoise re-grade of `MI_Ocean` (DeepWater/ShallowWater) + optional
  wave-modulated (non-uniform) foam.
- [ ] Wire `MPC_GTCGlobals` (`world_wetness`/`world_night_amount`) into the
  facade/road/water materials (currently unreferenced) for the wet-neon look.
- [ ] Spawn the buoyant boat pawn + emitter hookup for wake/foam.
- [ ] Apply the parameter cores onto real components (needs the C++ adapter
  wiring built into the editor → module rebuild + relaunch, user-gated).
