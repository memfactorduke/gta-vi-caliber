# GT-caliber_5.8 — working rules

Original open-source open-world game on **Unreal Engine 5.7.4**. Game code is a
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

Act on the live editor through the **UnrealClaude plugin** — the in-editor
"Claude Assistant" panel plus the `mcp__unrealclaude__*` tools. It serves an HTTP
server (`FUnrealClaudeMCPServer`) on **`http://127.0.0.1:3000`** (default port
3000). This is the primary control surface — use its tools to inspect/modify the
world and run editor actions, and `execute_script` to run editor Python through
the live connection, not a fresh `-run=pythonscript` process (`PythonScriptPlugin`
/ `EditorScriptingUtilities` are enabled).

- **The `unreal-mcp` / port-8000 entry in `.mcp.json` is NOT wired up.** It
  expects a `ModelContextProtocol` plugin that isn't installed (not listed in
  `GTC_UE5.uproject`), so nothing listens on 8000. Ignore it — use UnrealClaude
  on 3000.
- **Hidden tools.** The bridge
  (`Plugins/UnrealClaude/UnrealClaude/Resources/mcp-bridge/tool-router.js`) hides
  the `scripting` + `task_*` groups by default to save tokens, so only ~15 of 28
  tools list. To enable all ~24 (needed for FBX import / `execute_script`), add
  those names to `SIMPLE_TOOL_NAMES` and restart the editor. This is a **local
  patch** — the plugin is a vendored repo (`Natfii/UnrealClaude`), so it never
  comes from `git pull`; each dev applies it themselves.
- **Restart race.** After restarting the editor the server occasionally fails to
  bind 3000 if the previous instance hasn't released the port. If `unreal_status`
  reports not-connected, restart the editor once more.

If the MCP server is unreachable, the editor or its plugin may be down — stop and
ask rather than launching anything.

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
