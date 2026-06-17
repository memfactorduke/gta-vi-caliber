# Architecture

## The two-layer model

```
┌────────────────────────────────────────────────────────┐
│  game/        Godot 4.6 project — GDScript, scenes,    │
│               assets, UI, missions, tests              │
│               → 90% of contributions happen here       │
├────────────────────────────────────────────────────────┤
│  engine/      C++ GDExtension modules — streaming,     │
│               crowds, ocean, impostors                 │
│               → compiled into game/bin/*.{so,dylib,…}  │
├────────────────────────────────────────────────────────┤
│  Godot 4.6    Stock upstream editor/runtime.           │
│               Never forked. Never patched casually.    │
└────────────────────────────────────────────────────────┘
```

We "enhance Godot into the engine this game needs" through **GDExtension**,
not a fork. The game always opens in a stock Godot editor; native modules are
optional accelerators that the game detects and uses when present.

### The golden rule for native code

A system earns a C++ module in `engine/` only when **all three** hold:

1. A working GDScript implementation exists (or is provably impossible).
2. A captured profile shows it blowing the
   frame budget — not a hunch, a flame graph.
3. The module boundary is a narrow, stable API (Godot classes in, signals and
   typed data out) so the game layer never needs to know C++ exists.

Anything generically useful (a better streaming primitive, an impostor baker)
gets offered upstream to Godot. If core engine patches ever become
unavoidable, they live in `engine/patches/` as rebased patch files with an
upstream PR link each — see [VISION.md](VISION.md).

## game/ layout

| Path | Contents | Rules |
| --- | --- | --- |
| `scenes/world/` | Sandbox, city tiles, environment scenes | One scene = one streamable concept |
| `scenes/player/` | Player rig | Player logic talks to world via signals/groups only |
| `scenes/ui/` | HUD, menus, map | No game logic — UI observes, emits |
| `scripts/` | GDScript, mirroring `scenes/` (`scripts/player/`, …) | Typed GDScript, tabs, `snake_case` |
| `assets/` | Binary assets via LFS, by category: `characters/`, `buildings/`, `vehicles/`, `props/`, `environment/`, `ui/`, `audio/`, plus shared `materials/` & `textures/` | Provenance ledger in [ASSETS.md](ASSETS.md) |
| `addons/` | Vendored third-party plugins | Never hand-edit; replace wholesale on upgrade |
| `bin/` | Compiled `engine/` libraries + `.gdextension` manifests | Build artifacts, gitignored |
| `tests/unit/` | Unit tests (`test_*.gd`) | Run by `tools/check.sh` |
| `tests/smoke_test.gd` | Headless boot test | Loads main scene, asserts player exists |

### Conventions that keep 100 contributors from colliding

- **Composition over inheritance.** Scenes compose nodes; deep script
  inheritance trees are rejected in review.
- **Signals up, calls down.** A child never reaches up the tree; it emits.
- **Autoloads are rare.** Only true global services (e.g. a future
  `GameClock`, `SaveManager`). Each new autoload needs maintainer sign-off.
- **Logic extracted, scenes thin.** Anything testable lives in a plain class
  with static functions or a `RefCounted` (see
  `scripts/player/player_motion.gd`) so unit tests run headless without
  instantiating scenes.
- **One PR, one concern.** Mixed refactor+feature PRs get split.

## engine/ layout

| Path | Contents |
| --- | --- |
| `SConstruct` | godot-cpp SCons build (all platforms) |
| `godot-cpp/` | Submodule, pinned to the engine version in CI |
| `src/<module>/` | One directory per module |
| `src/register_types.{h,cpp}` | Extension entry point |

Build output lands in `game/bin/` next to its `.gdextension` manifest. The
game must **degrade gracefully** when a native module is absent — feature
flags via `ClassDB.class_exists()`, with a GDScript fallback or a clear
on-screen notice in debug builds. CI builds the game without native modules,
so this rule is enforced by construction.

## Streaming design direction (M3 — the first big engine system)

Decided early because everything in `scenes/world/` must be authored to fit:

- World space is a grid of **tiles** (target 128 m), each a self-contained
  scene with its own LODs, occluders, and navmesh chunk.
- The streamer keeps a **ring of residency** around the camera, prioritized
  by velocity direction; loads happen on worker threads, instancing is
  time-sliced on the main thread.
- Distant tiles collapse to **baked impostors** (the M3 impostor baker).
- A **floating origin** shifts the world when the player passes ~8 km from
  origin so physics never sees big floats.

Authoring implication today: keep every world scene self-contained — no
cross-scene node references, communicate via groups and signals.

## Performance budget (1080p, mid-range GPU = RTX 3060 class)

| System | Budget |
| --- | --- |
| Frame total | 16.6 ms (60 FPS) |
| Rendering | ≤ 10 ms |
| Physics + traffic + crowds | ≤ 3 ms |
| GDScript gameplay | ≤ 1.5 ms |
| Streaming (main-thread share) | ≤ 1 ms |
| Headroom | ~1 ms |

PRs that regress a captured benchmark need either a fix or an argued budget
change in review.
