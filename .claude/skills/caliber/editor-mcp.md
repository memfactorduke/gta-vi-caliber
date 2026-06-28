# Editor control surface — `unreal-mcp` (the live MCP)

> Part of the `caliber` skill. Read this first for any task that drives the
> editor. Never launch / close / restart the editor; reuse the running one.

## What the live MCP is

`unreal-mcp` is an **HTTP** MCP at `http://127.0.0.1:8000/mcp` (registered in
`~/.claude.json`, served by the in-editor `ModelContextProtocol` plugin; port and
path come from `Config/DefaultEditorPerProjectUserSettings.ini` —
`ServerPortNumber=8000`, `ServerUrlPath=/mcp`). It is the **only** control
surface in use.

If it seems down, probe with `mcp__unreal-mcp__list_toolsets`. If that fails the
editor or its plugin is down — **stop and ask**, don't launch anything.

### `vibe-ue` / VibeUE and `unrealclaude` are legacy — do not use

Older memories, some `docs/`, and parts of `animate-model.md` mention **VibeUE**
(`unreal.WidgetService`, `load_object`, `execute_python_code`). That was the
*previous* MCP with a completely different tool surface; the panel may still be
docked but it is **not** being driven. There is also a vendored `unrealclaude`
bridge in `Plugins/UnrealClaude/.mcp.json` (node → `localhost:3000`) — also not
wired up. When a legacy doc says "vibeue `execute_python_code`", the modern
equivalent is the **in-editor Python console** for raw `import unreal` work, or
the matching `unreal-mcp` toolset for everything else.

## How to call it — three meta-tools

You don't call tools by a flat name. Go through:

- `mcp__unreal-mcp__list_toolsets` — list the toolsets (do this first if unsure).
- `mcp__unreal-mcp__describe_toolset` — list tools + schemas in one toolset. ⚠ Its
  output is large and often overflows the token cap (spills to a `.txt`);
  describe **one** toolset at a time.
- `mcp__unreal-mcp__call_tool` — run a tool. Pass the **bare** `tool_name` (no
  toolset prefix) **plus** the separate `toolset_name`, and the tool's args.

## The toolsets you'll actually use

- **`EditorToolset.EditorAppToolset`** — editor/PIE control: `IsPIERunning`,
  `StartPIE` (options `{bSimulate, playMode:"PlayMode_InViewPort",
  warmupSeconds}`), `StopPIE`, **`CaptureEditorImage`** (base64 PNG of the WHOLE
  editor incl. the live PIE viewport **and** the UMG HUD overlay — this is how
  you actually *see* HP/minimap/ammo, unlike a plain viewport capture which
  misses UMG), `CaptureViewport`, `SearchCVars`, `OpenEditorForAsset`. There is
  **no console-exec / no `py`** tool here.
- **`editor_toolset.toolsets.object.ObjectTools`** — `get_properties` /
  `set_properties` (values are JSON strings; **deep-merges** nested structs, so
  partial edits are safe & cheap) / `list_properties` / `get_class` /
  `search_subclasses`. Works on ANY `UObject`: actor components, widgets-in-tree
  (address children by path `<WBP>.<WBP>:WidgetTree.<WidgetName>`), material
  expressions, BP CDOs.
- **`editor_toolset.toolsets.blueprint.BlueprintTools`** — graph authoring. Two
  styles: **DSL** (`get_graph_dsl_docs`, `read_graph_dsl`, `write_graph_dsl` =
  populate+compile) which works on **function graphs only, NOT the EventGraph
  (ubergraph)**; and **imperative** (`add_variable`, `add_event`, `create_node`,
  `connect_pins`, `break_pins`, `compile_blueprint`, `create`,
  `add_function_graph`, `add_function_param`) for the EventGraph.
- **`editor_toolset.toolsets.asset.AssetTools`** — `save_assets` (`[]` = save all
  dirty; otherwise package paths, NO `.Object` suffix), `find_assets`,
  `load_asset`, `exists`, `duplicate`, `delete`, `is_dirty`.
- **`editor_toolset.toolsets.material.MaterialTools`** /
  **`material_instance.MaterialInstanceTools`** — author materials & MICs
  (`add_expression`, `connect_to_output`, `connect_expressions`, `recompile`, …).
- **`editor_toolset.toolsets.scene.SceneTools`** / **`actor.ActorTools`** — load
  levels, place/remove actors, transforms, outliner, level camera.
- Data-driven asset editors: **`data_table` / `data_asset` / `curve_table` /
  `string_table` / `texture` / `static_mesh` / `skeletal_mesh` / `primitive`**.
- **`editor_toolset.toolsets.programmatic.ProgrammaticToolset`** —
  `execute_tool_script`: a **sandboxed** Python that *orchestrates other MCP
  tools* in one round-trip (define `run()->dict`; helper `execute_tool(name,
  json_str)`). Sandbox allows **only** stdlib `json/math/datetime/copy/re/time` —
  it **cannot `import unreal`**. Great for batching many get/set calls. ⚠
  `execute_tool` raises `RuntimeError` on the called tool's failure and it
  propagates fatally **even inside `try/except`**, so only batch calls you expect
  to succeed (a probe loop over maybe-missing objects aborts the whole script).
- **`ToolsetRegistry.AgentSkillToolset`** — list/read/create skills (rarely
  needed from here).
- **`EditorToolset.LogsToolset`** — read the output log, set category verbosity.

## What MCP can NOT do (use the in-editor Python console / manual editor)

- Import binary assets (`.ttf` fonts, FBX, textures) — no import hook.
- **Create new widgets** in a WidgetTree — you can only restyle/re-wire existing
  named widgets, not add tree nodes.
- Edit `IMC_*` Enhanced-Input mapping arrays — they read back `[]` and
  blind-writing wipes ALL controls. Do input rebinds in-editor or via Python.
- Run any `r.*` / console command — there is no exec tool; `SearchCVars` only
  *reads*. (cvars set via Python `execute_console_command` are **session-only**
  and don't persist.)
- `unreal_open_level` to switch the live level **crashes** — don't.

## Gotchas worth remembering

- Big returns (`CaptureEditorImage`, `describe_toolset`) overflow the token cap
  and spill to a `.txt` — decode the base64 to a PNG and `Read` it.
- `StartPIE` may surface accumulated BP runtime warnings as an ERROR even though
  PIE started fine — confirm with `IsPIERunning` (true) and proceed.
- Some licensed-kit assets (e.g. `WB_Pause_menu_widget`) report "Asset does not
  exist" to `exists()` / `save_assets([path])` even though `load/get/set` work —
  persist them with `save_assets([])` (save-all) or Save All in-editor.
- `save_assets([])` can abort on a kit asset's "Accessed None
  PlayerCharacterReference" spam — save **specific package paths** instead.
- **Never** write widget **CDOs** from Python (desyncs the registry).
- DSL `write_graph_dsl`/`read_graph_dsl` raise on the `:EventGraph` — use the
  imperative tools there. A bare widget-var word is not auto a variable in DSL;
  use the getter `Variables|<BlueprintName>|Get<Name>`.
- The in-editor Python console (not the MCP sandbox) is the path for raw
  `import unreal` work — it has no 30s reply window, unlike MCP calls.
