# Architecture

## The two-layer model

```
┌────────────────────────────────────────────────────────┐
│  game/content  UE5 project: C++ gameplay in Source/,   │
│                levels/Blueprints/UMG in Content/,      │
│                config in Config/                       │
│                -> 90% of contributions happen here     │
├────────────────────────────────────────────────────────┤
│  engine-feel   C++ modules atop Chaos/Mass/rendering   │
│                (streaming, crowds, ocean, impostors)   │
│                -> compiled into the project binaries   │
├────────────────────────────────────────────────────────┤
│  Unreal 5.7    Stock engine. Never forked.             │
│                Never patched casually.                 │
└────────────────────────────────────────────────────────┘
```

We build **on Unreal Engine 5.7** and extend it with C++ modules and built-in
engine features (Mass, Chaos, World Partition) into the engine this game needs,
not an engine fork. The project always opens in a stock Unreal editor; our
modules are accelerators layered on top of stock systems.

### The golden rule for native code

A system earns a bespoke C++ module only when **all three** hold:

1. A working pure-C++/Blueprint implementation exists (or is provably
   impossible).
2. A captured profile shows it blowing the frame budget, not a hunch, a flame
   graph.
3. The module boundary is a narrow, stable API (engine types in, delegates and
   typed data out) so the game layer never needs to know the internals.

Anything generically useful (a better streaming primitive, an impostor baker)
is kept as a reusable, documented module behind a clean plugin/module boundary.
We do not fork or patch engine source casually; if a deeper change ever becomes
unavoidable it is isolated to its own plugin with a rationale, see
[VISION.md](VISION.md).

## Project layout (Source/ + Content/)

| Path | Contents | Rules |
| --- | --- | --- |
| `Content/Maps` (world levels) | Sandbox, city cells, environment levels | One level = one streamable concept |
| Player pawn/character (`Source/` + `Content/`) | Player rig | Player logic talks to the world via delegates/tags only |
| `Content/UI` (UMG widgets) | HUD, menus, map | No game logic, UI observes and emits |
| `Source/GTC/` | C++ modules, mirroring the content areas (`Source/GTC/Player/`, ...) | Typed C++, Unreal naming conventions |
| `Content/` assets | Binary assets via LFS, by category: `characters/`, `buildings/`, `vehicles/`, `props/`, `environment/`, `ui/`, `audio/`, plus shared `materials/` & `textures/` | Provenance ledger in [ASSETS.md](ASSETS.md) |
| `Plugins/` | Vendored third-party plugins | Never hand-edit; replace wholesale on upgrade |
| `Binaries/`, `Intermediate/` | Compiled modules + build outputs | Build artifacts, gitignored |
| in-module `Tests/` | UE5 automation tests (`GTC.*`) | Run by `tools/check.sh` |
| boot automation test | Headless boot test | Loads the main level, asserts the player exists |

### Conventions that keep 100 contributors from colliding

- **Composition over inheritance.** Compose actor components; deep `AActor`
  hierarchies are rejected in review.
- **Events/delegates up, calls down.** A child never reaches up the tree; it
  broadcasts.
- **Subsystems are added sparingly.** Only true global services (e.g. a future
  `GameClock`, `SaveManager`) become GameInstance/World Subsystems. Each new
  one needs maintainer sign-off.
- **Logic extracted, actors thin.** Anything testable lives in a plain,
  non-`UObject` C++ class (see `Source/GTC/Player/PlayerMotion.*`) so
  automation tests run without spawning actors.
- **One PR, one concern.** Mixed refactor+feature PRs get split.

## Module layout (Source/<Module>/)

| Path | Contents |
| --- | --- |
| `<Module>.Build.cs` | UnrealBuildTool module rules (all platforms) |
| Engine version | Pinned to UE 5.7 in CI |
| `Source/<Module>/` | One directory per module |
| `IModuleInterface` (`StartupModule`/`ShutdownModule`) | Module entry point |

Modules compile into the project binaries. The game must **degrade
gracefully** when an optional feature is unavailable: gate on feature
availability checks, with a fallback path or a clear on-screen notice in debug
builds when a plugin is disabled. CI builds the project with the standard
module set, so this rule is enforced by construction.

## Streaming design direction (M3, the first big engine system)

Decided early because every world level must be authored to fit:

- World space is a set of **World Partition cells / streaming levels** (target
  128 m), each self-contained with its own LODs, occluders, and navmesh chunk.
- The streamer keeps a **ring of residency** around the camera, prioritized
  by velocity direction; loads happen on worker threads, instancing is
  time-sliced on the main thread.
- Distant cells collapse to **baked impostors** (the M3 impostor baker; pairs
  with Nanite and HLOD).
- **Large World Coordinates** (double precision) keep physics and transforms
  accurate as the player travels far from origin, so we never see big floats.

Authoring implication today: keep every world level self-contained, no
cross-level hard references; communicate via tags, subsystems, and delegates.

## Performance budget (1080p, mid-range GPU = RTX 3060 class)

| System | Budget |
| --- | --- |
| Frame total | 16.6 ms (60 FPS) |
| Rendering | ≤ 10 ms |
| Physics + traffic + crowds | ≤ 3 ms |
| C++ gameplay | ≤ 1.5 ms |
| Streaming (main-thread share) | ≤ 1 ms |
| Headroom | ~1 ms |

PRs that regress a captured benchmark need either a fix or an argued budget
change in review.
