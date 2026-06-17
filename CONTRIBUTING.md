# Contributing to GTC

Thanks for being here. GTC is an original, community-driven open-world game built
on Unreal Engine 5.7 (C++). The game code is MIT-licensed; the art is licensed
content on top of a proprietary engine you supply yourself. This is an original,
unaffiliated project and contains no assets, code, or trademarks from any
commercial game.

There is meaningful work for programmers, 3D artists, audio designers, writers,
and playtesters. You do not need to be an expert.

## Setup

You provide the engine; the repository provides the game.

1. **Install Unreal Engine 5.7** from the Epic Games Launcher and accept the
   Unreal Engine EULA. The engine is not redistributed here.

2. **Install Git LFS** once per machine:
   ```bash
   git lfs install
   ```

3. **Clone with submodules.** Most of the art lives in a private asset submodule:
   ```bash
   git clone --recurse-submodules <repo-url>
   # already cloned without it?  git submodule update --init --recursive
   ```

4. **Get the assets.** The art is in the private `duolahypercho/GT-Caliber-Asset`
   submodule (about 15 GB, Git LFS).
   - You need **read access** to that private repository. Without it, you can
     clone and build the **code**, but you will not get the art and the project
     is **not runnable as-is**. This is intentional: licensed and paid assets are
     never redistributed. Ask a maintainer for access.
   - Pull the LFS objects:
     ```bash
     cd Content/GTCaliberAssets && git lfs pull && cd -
     ```

5. **CoreRedirects.** Marketplace packs reference their own assets by `/Game/...`
   root paths, but the submodule mounts one level deep, so `Config/DefaultEngine.ini`
   contains package-path redirects that remap them. They are already committed.
   You do not need to do anything, except: CoreRedirects load at engine startup,
   so if you ever edit them, **restart the editor**. See `docs/ASSET_HANDLING.md`.

6. **Build** the editor target with UnrealBuildTool (or right-click the
   `.uproject` and "Generate project files", then build `GTC_UE5Editor`).

## How to work

### Branches and pull requests
- Branch from `main` with a descriptive name (`feat/...`, `fix/...`).
- Keep each PR scoped to one thing.
- Open a PR; describe what changed and how you verified it.

### Tests must stay green
- The automation suite under the `GTC.*` prefixes is the gate. **It must stay
  green.** The build plus the full `GTC.*` suite must pass before a change lands.
- Pure gameplay/logic belongs in tested C++ models with automation-test parity.
  Do not drop automated coverage just because logic now sits behind an actor.

### Engine-coupled work: the verification standard
- For engine-coupled features (actors, UMG, vehicles, rendering) there is no unit
  oracle. Each such change must show: the full `GTC.*` suite still green, the
  editor compiles and PIE launches clean on the relevant map, and **one
  continuous recording of the full interaction-to-effect sequence including edge
  behavior** (a single happy-path frame does not count), framed and non-black.

### Assets and AI: hard rule
- **Never feed asset file contents into any AI model.** Do not read, parse,
  describe, transform, generate variations of, or train on the mesh/texture/
  material/animation files. Referencing an asset by its **path** in a spawn, a
  DataTable row, a Blueprint, or config is fine. The line is: referencing a path
  is fine; ingesting the contents is not. See `docs/ASSET_HANDLING.md`.
- Assets are never committed to this repository; they live only in the private
  submodule. The repository tracks a pointer, not the files.

### House style
- **No em dashes.** Use commas, colons, parentheses, or separate sentences.
- Keep prose and comments clean and typed; match the surrounding code's density
  and naming.

## Pointers
- `docs/ASSET_HANDLING.md` - asset submodule, CoreRedirects, the no-AI rule.
- `UE5_NOTES.md` - hard-won UE5 / editor / MCP gotchas; append new fixes.
- `ROADMAP.md` and `PROJECT_CHECKLIST.md` - what is built, what is left, by phase.
- `the migration plan` and `docs/SYSTEMS.md` - the ported gameplay logic
  behind each feature.

By contributing **code** you agree to license it under MIT; by contributing
**assets** you agree to the asset terms recorded in `docs/ASSETS.md` and attest
you hold the rights.
