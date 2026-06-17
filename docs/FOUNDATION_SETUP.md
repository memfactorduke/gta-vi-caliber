# GTC UE5 — Foundation setup record

What was set up from the FOUNDATION section of `../../the migration plan` (12 CARRIES OVER +
16 NATIVE + the "execute first" scaffold). The `.uproject` C++ baseline already existed; this layer adds
the carried knowledge/tooling and the engine-feature configuration.

## CARRIES OVER (copied in, not re-authored)
- **Design & systems reference, asset policy, quality ledger** → `docs/` (ARCHITECTURE, SYSTEMS, VISION,
  ROADMAP, BUILD_ORDER, PIVOT_TO_MIAMI, ASSETS, ASSET_PIPELINE, QUALITY),
  `CREDITS.md` at root. The provenance ledger in `docs/ASSETS.md` + `CREDITS.md` is load-bearing for
  legal compliance — kept verbatim.
- **Front-door / governance** → `README.md`, `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `SECURITY.md`,
  `reference/README.md`, `AGENTS.md`, `LOOP_AGENT.md`, `.editorconfig`, `.github/ISSUE_TEMPLATE/`,
  `.github/PULL_REQUEST_TEMPLATE.md`, `.codacy/`, `LICENSE`, `LICENSE-ASSETS`.
- **Portable tooling** → `tools/osm/` (OSM pipeline), `tools/three_mara_modeler/` (GLB exporters),
  `tools/worktrees/`.
- **Still need UE5 updates (per the foundation plan):** the install/quickstart + code-style sections of README/
  CONTRIBUTING are Godot/GDScript-specific; `.editorconfig` language sections, the `.github` template
  checklist lines (`.uid`/GDScript), and the Codacy runtime list should be retargeted to UE5 C++. The
  Godot CI workflows were **not** copied (they are DROP/REBUILD, not CARRIES OVER).

## OSM pipeline re-point (INVESTIGATE #7 = keep OSM)
`tools/osm/fetch_district.py` now writes to `Data/World/` (was `game/assets/world/`) and
`build_manifest.py` reads `Data/World/` and emits plain `Data/World/<name>.json` paths (was `res://`).
The district JSON **data files** themselves are migrated under INVESTIGATE #7's follow-on (the UE5
World-Partition/PCG district build), not in this foundation pass — see `Data/World/README.md`.

## NATIVE features (configured, old code not ported)
See `NATIVE_FEATURES.md` for the per-item notes. Concretely applied here:
- **Plugins** (`GTC_UE5.uproject`): `EnhancedInput`, `ChaosVehiclesPlugin`, `MassEntity`, `MassGameplay`.
- **`Config/DefaultEngine.ini`**: Nanite project-wide, ray-traced shadows, Interchange glTF import;
  Lumen/VSM/Substrate/ray-tracing + the WP OpenWorld default map were already present.
- **`Config/DefaultScalability.ini`** (new): the LOW–ULTRA quality tiers (NATIVE #7).
- **`Config/DefaultInput.ini`**: Enhanced Input default player-input/input-component classes.

## "Execute first" scaffold — run in the editor
`tools/ue5/setup_foundation.py` creates the editor-only assets (no running editor in the migration
sandbox, so these `.uasset`s are not committed):
- **`/Game/Materials/MPC_GTCGlobals`** — Material Parameter Collection with `world_night_amount` and
  `world_wetness` (the two shared globals from Godot `project.godot`).
- **`/Game/Input/IMC_Default`** — optional starter Enhanced Input mapping context.

Run it, then commit the generated assets. Verify unreal API names via the VibeUE skills/discovery first
(`UE5_NOTES.md` §0). The `.uproject`/Project Settings/`MPC` together are the bootstrap
prerequisite for Wave 1 C++ and all Wave 3 shader/material work.
