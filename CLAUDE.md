# GT-caliber_5.8 — working rules

Original open-source open-world game on **Unreal Engine 5.8**. Game code is a
single C++ runtime module, **`GTC_UE5`** (`GTC_UE5.uproject`). This file is the
quick contract for working in the repo; the full machine-readable rules live in
`AGENTS.md`, and the deeper docs in `docs/` (`ROADMAP.md` = tasks,
`VISION.md` = quality bar, `ARCHITECTURE.md` = layering, `ASSETS.md` = asset
provenance).

## The single Unreal editor (do not break this)

There is always **one** Unreal editor running for this project. Treat it as a
long-lived shared resource.

- **Never close, kill, or restart it.** No `pkill`/`killall`/`taskkill` on
  `UnrealEditor`, no AppleScript quit, no commands that trigger an editor
  shutdown or restart as a side effect.
- **Never launch a second editor.** Always reuse the already-running one.
  Do not run the `UnrealEditor` / `UnrealEditor-Cmd` binary, `open -a`, or any
  `-game`/`-editor` launch.
- **If no editor appears to be running, stop and ask** — do not start one
  yourself.
- To act on the editor, go **through a live connection** (see below), never by
  spawning a process.

A PreToolUse hook (`.claude/hooks/protect-unreal-editor.sh`, wired in
`.claude/settings.json`) hard-enforces the first two rules — it refuses any Bash
command that would kill or relaunch the editor. The hook is the backstop; follow
the intent above so you never hit it.

## Connecting to the running editor

Act on the live editor through one of these, in order of preference:

- **Unreal MCP** — the `unreal-mcp` server in `.mcp.json`, served by the
  in-editor `ModelContextProtocol` plugin at `http://127.0.0.1:8000/mcp`
  (configured in `Config/DefaultEditorPerProjectUserSettings.ini`:
  `ServerPortNumber=8000`, `ServerUrlPath=/mcp`). This is the primary control
  surface — use its tools to inspect/modify the world and run editor actions.
- **Python inside the editor** — `PythonScriptPlugin` /
  `EditorScriptingUtilities` are enabled; run editor Python through the live
  connection, not a fresh `-run=pythonscript` process.

If the MCP server is unreachable, the editor or its plugin may be down — stop
and ask rather than launching anything.

## Source layout (`Source/GTC_UE5/`)

Feature-foldered C++: `Player/`, `Vehicles/`, `Weapons/` (Core, Firing,
Ballistics, Component), `AI/` (Combat, Emergency, Gps), `NPC/`, `Missions/`
(Core, Coordinators, Activities, StuntScore), `World/` (RoadNetwork, Buildings,
Places, Police, Interaction), `Worldcore/`, `Camera/`, `UI/` (Hud, Minimap,
Phone, Menus), `FrontEnd/`, `Systems/`, `Core/`. Most subsystems carry a local
`Tests/` folder with `GTC.*` automation tests.

Key plugins (from `GTC_UE5.uproject`): `CesiumForUnreal` (3D-tiles world),
`ChaosVehiclesPlugin`, `MassGameplay`, `EnhancedInput`, plus the MCP/Python
toolset plugins above.

## Where things go (repository map)

Content has **two homes by provenance**. Put new work in the right one — full law
in `docs/ASSETS.md` (provenance ledger) and `docs/ASSET_HANDLING.md` (third-party).

**Project-authored — tracked in THIS repo** (original / CC0 / CC-BY only, every new
binary needs a ledger row in `docs/ASSETS.md`, routed through Git LFS):

- **Features / gameplay (C++)** → `Source/GTC_UE5/<feature>/` (see Source layout
  above). New systems are C++ first, with a `GTC.*` automation test.
- **Authored Blueprints** → `Content/Blueprints/` (subfolders: `AI/`, `weapons/`,
  `weapons_pickup/`, `Cameras/`, `ControlRigs/`, `Data/`, `ENUMS/`, `Interfaces/`,
  `MovementModes/`, `SmartObjects/`, `AnimModifiers/`, `AnimNotifies/`). The
  top-level game-mode/pawn/NPC Blueprints live in `Content/Core/`
  (`BP_GTCGameMode`, `BP_GTCPlayerCharacter`, `BP_NPC`).
- **Materials / surfaces / environment** → `Content/Materials/`,
  `Content/Surfaces/`, `Content/Environment/`. **UI / widgets** → `Content/UI/`.
- **Maps:** prototype / feature-test levels → `Content/Levels/` (`DefaultLevel`,
  `LocomotorLevel`, `NPCLevel`). The **shipping world map (CityBeachStrip) is NOT
  here** — it lives in the submodule (below).

**Third-party / licensed / paid — NEVER committed to this (public) repo.** Either
in the asset submodule or kept on disk local-only and **gitignored**:

- **Asset submodule** `Content/GTCaliberAssets/` (~15 GB Git-LFS gitlink → private
  `duolahypercho/GT-Caliber-Asset`). Holds the shipping world (`CityBeachStrip/Maps/`),
  licensed `Characters/`, `ThirdPerson/`, `LevelPrototyping/`, and the canonical
  **Input**. UE path `/Game/GTCaliberAssets/Content/...`. **Licensed/paid art goes
  HERE** (commit to the submodule repo), never into the parent tree.
- **Vendored plugins** → `Plugins/` (gitignored). **Marketplace/Fab packs** kept
  for the editor but gitignored: `CitySampleVehicles/`, `FPS_Weapon_Bundle/`,
  `CesiumSettings/`.
- ⚠ **Leak risk:** packs currently on disk but **not yet gitignored** must never be
  committed and should be added to `.gitignore`: `Realistic_Starter_VFX_Pack_Vol2/`,
  `MarketplaceBlockout/`, `Fab/`, `Locomotor/`, `MetaHumans/`.

**Animations & characters** (mostly third-party, treat as above):

- **Animations** → `Content/Anims/GASP/` (Epic Game-Animation-Sample library),
  `Content/Mixamo/` (Mixamo clips; player retargets in
  `Content/Mixamo/SoldierRifle/`). Retargeted assets → `Content/Mixamo/.../Anims`
  and `Content/Blueprints/RetargetedCharacters/`. The player is on the **soldier
  rig** (`ABP_PlayCharacter`).
- **Characters / skeletons** → `Content/Characters/` (Epic Mannequins, Paragon
  samples), MetaHuman heroes → `Content/MetaHumans/`.

## Enhanced Input — one canonical source (do not duplicate)

The input assets the **game actually loads** live ONLY in the submodule at
`Content/GTCaliberAssets/Content/Input/` (`IMC_Default`, `IMC_MouseLook`,
`Actions/IA_*`, `Touch/`). A CoreRedirect in `Config/DefaultEngine.ini`
(`+PackageRedirects=(OldName="/Game/Input/",NewName="/Game/GTCaliberAssets/Content/Input/",MatchSubstring=true)`)
rewrites **every** `/Game/Input/...` reference into that submodule path.

- **Add new `IA_*` / `IMC_*` to the submodule only.** Anything dropped loose into
  `Content/Input/` is shadowed by the redirect and never loads — that is the
  recurring "duplicate input" trap.
- **One copy per asset.** Before creating an input action/context, check it does
  not already exist in the submodule set. Never keep a same-named copy in two
  places.
- Reference input by `/Game/Input/...` (the redirect resolves it); the loose
  `Content/Input/` `IA_Aim/Crouch/Sprint/...` + `IMC_Sandbox` + `MobileHUD/` files
  are the **Sandbox/mobile WIP**, not gameplay input, and are currently mis-wired
  by that same redirect (fix in-editor by moving them into the submodule or
  narrowing the redirect — never by duplicating).

## Conventions (see AGENTS.md for the full set)

- UE5 C++ to the repo `.clang-format` / `.editorconfig`; `PascalCase` types with
  UE prefixes (`U`/`A`/`F`). Extract testable logic into plain non-UObject
  classes/static functions (pattern: `Source/GTC_UE5/Player/PlayerMotion.*`) and
  cover it with an in-module `GTC.*` automation test.
- **Never** add assets, code, names, or layouts from Grand Theft Auto or any
  commercial game; every new binary asset needs a provenance row per
  `docs/ASSETS.md`.
- Do **not** hand-edit `Plugins/**` (vendored), generated `.uasset`/`.umap`, or
  `Binaries/**` / `Intermediate/**` / `DerivedDataCache/**` (build artifacts).
- `.ini` and `.uproject` are hand-editable text — keep diffs minimal, never
  reformat sections you didn't touch.
- Events up, calls down. Levels are self-contained (streaming-ready): no
  cross-level hard references — use tags/subsystems and delegates. One concern
  per PR, referencing the roadmap line/issue it closes.

## Definition of done

Per `AGENTS.md`, every change must format + lint + build (UnrealBuildTool) +
pass automation tests, and `main` must still boot to a controllable character.
If you cannot run the build or automation tests in this environment, say so
explicitly rather than claiming the checks passed.
