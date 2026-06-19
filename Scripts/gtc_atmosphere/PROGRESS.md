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
- [ ] **Shoreline** — shore blend + wetness band from wave height vs terrain
  depth: shallow-water whitening, run-up wet line that follows the swash.
  `GTC.World.Ocean.Shore`.

### Weather / lighting
- [ ] **WeatherFront** — spatial+temporal clear→overcast→rain front profile
  (a moving front line over the map) and the **MPC_GTCGlobals** scalar values it
  drives (`world_wetness`, `world_night_amount`). `GTC.World.WeatherFront`.
- [ ] **SurfaceWetness** — rain → accumulation/drying curve so wetness *lags*
  rain (puddles build, streets dry slowly). Feeds MPC `world_wetness`.
  `GTC.World.Wetness`.
- [ ] **NightLights** — streetlight/lit-window activation vs time-of-day:
  hysteretic dusk-on/dawn-off threshold + per-light jitter so the city doesn't
  pop on in unison. Drives `world_night_amount`. `GTC.World.NightLights`.

## Blocked on the live editor (stop-and-ask when reached)

The visual hookup is **not** pure-core and needs the running editor:
- Lumen GI / volumetric fog tuning for trailer-grade night lighting.
- The ocean **material graph** (Gerstner verts, foam mask, depth blend) and
  wiring `MPC_GTCGlobals` / `MPC_GTCWeather` into facade/road/water materials.
- Spawning/placing the buoyant boat pawn + emitter hookup for wake/foam.
- Applying any of the above parameter cores onto real components in-editor.

When the next useful step is one of these, **stop and ask** for the live editor.
