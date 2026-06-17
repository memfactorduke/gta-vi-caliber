# AGENTS.md — repo contract for AI agents

You are contributing to an original open-source open-world game built on
Unreal Engine 5.7 (C++) with the game logic in UE5 C++ modules. Humans and agents
follow the same rules; this file is the machine-readable summary.

## Ground truth

- **Task source:** `docs/ROADMAP.md`. Work top-down within the current
  milestone; an unchecked box is a task. Check the box in the same PR that
  completes it.
- **Quality bar:** `docs/VISION.md`. **Layering rules:** `docs/ARCHITECTURE.md`.
- **Never** add assets, code, names, or map layouts from Grand Theft Auto or
  any commercial game. Asset rules: `docs/ASSETS.md` — every new binary asset
  needs a provenance ledger row in the same change.

## Definition of done (every change)

```bash
tools/check.sh   # format + lint + UnrealBuildTool build + automation tests
```

Must exit 0. If you cannot run UnrealBuildTool or the Unreal automation tests in
your environment, say so explicitly in the PR body: do not claim checks passed.

## Hard rules

1. `main` stays playable: launching the project in the Unreal editor (or a
   packaged build) must boot to a controllable character after your change.
2. UE5 C++ following the project's clang-format / `.editorconfig` conventions;
   `PascalCase` types with the UE prefix convention (`U`/`A`/`F`).
3. Extract testable logic into plain non-UObject classes / static functions
   (pattern: `Source/GTC/Player/PlayerMotion.*`) and add an in-module
   `Tests/` automation test (`GTC.*` prefix) for it.
4. Do not hand-edit `Plugins/**` (vendored), generated `.uasset`/`.umap`
   binaries, or `Binaries/**` / `Intermediate/**` (build artifacts).
5. No new GameInstance/World Subsystems, no new third-party plugins, no
   engine-source patches without an issue approved by a maintainer first.
6. Engine-feel C++ (rendering, Chaos, Mass) lands per `docs/ARCHITECTURE.md`.
7. One concern per PR. Reference the roadmap line or issue it closes.
8. Config files (`.ini`) and `.uproject` are hand-editable text: keep diffs
   minimal and never reformat sections you didn't change.

## Gameplay/wiring conventions (quick reference)

- Events up, calls down. World levels are self-contained (streaming-ready):
  no cross-level hard references; use tags/subsystems and delegates.
- Player spawns are `APlayerStart` (or tagged spawn) actors in the level.
- Input actions are Enhanced Input assets (IMC/IA) referenced from
  `Config/DefaultInput.ini`: move (2D axis), jump, sprint (+ a cancel action
  for mouse release).

## Commit/PR format

- Imperative title ≤ 72 chars, body explains *why*.
- PR template checklist is mandatory; fill it honestly.
- If a change is reverted by CI failure, fix forward only with a root cause;
  otherwise revert cleanly.
