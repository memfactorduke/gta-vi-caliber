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
