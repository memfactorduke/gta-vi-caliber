# Where does a change belong — C++ vs Blueprint vs ABP

> Part of the `caliber` skill. Read this to decide which layer to put a change in
> before you start. Asset specifics (which BP/map/ABP) are in `asset-map.md`.

This project is **C++-first** (one runtime module, `GTC_UE5`), with Blueprints as
the glue/content layer on top. Pick the layer by what the change *is*, not by
what's easiest to reach.

## The decision shortcut

*behavior / rules* → **C++** · *composition & tunables* → **Blueprint** ·
*on-screen display* → **UMG widget** · *how a skeleton moves* → **Anim Blueprint**

## The layers

- **C++ (`Source/GTC_UE5/<feature>/`)** — all new *systems, rules, and logic*:
  gameplay subsystems, components, actors with real behavior, math, AI, economy,
  data structures. New systems are **C++ first** and ship with a `GTC.*`
  automation test. Pull the testable core into a plain non-`UObject` class/static
  fn (pattern: `Player/PlayerMotion.*`) so it can be unit-tested headless. C++
  changes need a **build** (UnrealBuildTool) and **cannot be hot-authored over
  MCP** — so on this Mac, when a build isn't available, prefer wiring through an
  existing C++ surface from a Blueprint rather than adding new C++ you can't
  compile.
- **Blueprint (`Content/Blueprints/…`, `Content/Core/…`)** — *content, wiring, and
  designer-facing tunables* on top of C++: GameMode/Controller/Pawn composition,
  pickups, spawners, per-actor config, glue that calls down into C++ functions.
  Subclass C++ classes in BP to set meshes/defaults; **keep logic thin** (call
  down into C++, don't reimplement systems in BP). This is the layer the **live
  MCP** edits well (DSL for function graphs, imperative tools for the EventGraph —
  see `editor-mcp.md`).
- **UMG Widget Blueprint (`WBP_*` / `WB_*`, `Content/UI/`)** — HUD and menus. Data
  comes from C++ subsystems/components via getters or `BlueprintAssignable`
  delegates (e.g. `UWantedSubsystem::OnStarsChanged`); the widget only *displays*.
  Restyle/re-wire existing widgets via MCP `ObjectTools`/`BlueprintTools`;
  **creating new widgets** needs the editor (MCP can't add tree nodes).
- **Anim Blueprint (`ABP_*`)** — character animation graphs **only**. Never put
  gameplay logic in an ABP. And per house rule: **never author animation from
  scratch** — retarget an existing clip, then wire it (no source clip → ask). To
  give a new model the whole library, see `animate-model.md`. Note AnimGraph nodes
  are **not API-editable** (neither MCP nor Python) — wiring a node is a manual
  in-editor step.

## Conventions per layer (see `AGENTS.md` for the full set)

- UE5 C++ to the repo `.clang-format` / `.editorconfig`; `PascalCase` types with
  UE prefixes (`U` / `A` / `F`).
- **Events up, calls down.** Levels are self-contained (streaming-ready): no
  cross-level hard references — use tags/subsystems and delegates.
- **Never** add assets, code, names, or layouts from Grand Theft Auto or any
  commercial game; every new binary asset needs a provenance row per
  `docs/ASSETS.md`.
- Do **not** hand-edit `Plugins/**` (vendored), generated `.uasset`/`.umap`, or
  `Binaries/**` / `Intermediate/**` / `DerivedDataCache/**`.
- `.ini` / `.uproject` are hand-editable text — keep diffs minimal, never
  reformat sections you didn't touch.

## Definition of done

Every change must format + lint + build (UnrealBuildTool) + pass automation
tests, and `main` must still boot to a controllable character. If you cannot run
the build or tests in this environment, **say so explicitly** rather than
claiming the checks passed.
