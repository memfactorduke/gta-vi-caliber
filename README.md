# GTC

**An original, community-driven open-world game project, built on Unreal Engine 5.**
The quality bar is the fidelity of modern AAA open-world games: a dense, living
coastal city with seamless streaming, vehicles, crowds, water, and weather.

> ⚠️ This is an **original, unaffiliated** project. It is **not** associated
> with, endorsed by, or connected to Rockstar Games or Take-Two Interactive,
> and it contains **no** assets, code, trademarks, or content from any
> commercial game. Any "AAA open-world" comparison describes a *quality
> benchmark*, nothing more.

## Licensing (read this first)

- **Game code** (everything in `Source/`, build scripts, and configuration) is
  licensed under the **[MIT License](LICENSE)**.
- **Game assets we author** (our own files under `Content/`, excluding the
  `Content/GTCaliberAssets/` submodule) are licensed under
  **[CC BY 4.0](LICENSE-ASSETS)**, with provenance tracked in
  [docs/ASSETS.md](docs/ASSETS.md). The third-party assets in the
  `Content/GTCaliberAssets/` submodule are **separately licensed and NOT
  CC BY 4.0** (see [docs/ASSET_HANDLING.md](docs/ASSET_HANDLING.md) and the
  "Assets" section below).

**This is not "fully open-source software," and cannot be.** The project runs on
**Unreal Engine 5**, which is **proprietary**: Unreal is provided by Epic Games
under the **[Unreal Engine EULA](https://www.unrealengine.com/eula)** (source-available,
royalty-bearing, **not** an OSI/FSF open-source license). You must **obtain the
engine yourself** from Epic; it is **not redistributed** here, and the engine
cannot be relicensed under MIT. What is open here is our **own game code (MIT)**
and **our own art (CC BY 4.0)**, sitting on top of a proprietary engine. Any
Epic sample content, Fab marketplace assets, or engine plugins added later
remain under their own Epic/third-party terms and are likewise not open source.

In short: **MIT-licensed game code and CC-BY assets, on a proprietary engine.**

## Status

🛠️ **In active development; not yet a playable build.** The project is a
ground-up Unreal Engine 5.7 (C++) port of a prior prototype. The work is staged:

- **Wave 1, tested logic core (complete):** pure gameplay/AI/economy models
  (scoring, ballistics, NPC decision cores, faction/territory, economy, missions,
  weapons, vehicle logic, and more) ported to C++ with **automation-test parity**.
- **Wave 2, subsystems (complete):** self-wiring managers landed as UE
  GameInstance/World Subsystems across four batches (progression, player stats,
  save/persistence, wanted/arrest, NPC dialogue, disguise, roster, mission
  coordinators, and more).
- **Wave 3, engine/feel (in progress):** the C++ framework is merged (player
  pawn, context interaction, HUD base, Enhanced Input, armor reconciliation as
  tested logic); rendering, World Partition streaming, Chaos vehicles, Mass AI
  crowds/traffic, animation, audio, and UMG follow.

The logic core currently passes a full headless automation suite of **1246 tests**.

## Build (contributors)

UE **5.7**, macOS (Apple Silicon verified) / Windows. You provide the engine; the
repo provides the game.

```bash
# 1. Install Unreal Engine 5.7 from the Epic Games Launcher (accept the UE EULA).
# 2. Install Git LFS once per machine:
git lfs install
# 3. Clone WITH submodules (the game art lives in a submodule; see "Assets" below):
git clone --recurse-submodules <repo-url>
#    (already cloned without it? run: git submodule update --init --recursive)
# 4. Pull the Git LFS asset objects for the submodule:
cd Content/GTCaliberAssets && git lfs pull && cd -
# 5. Generate project files and build the editor target
#    (right-click the .uproject, "Generate project files", or use the engine's
#     Build.sh / UnrealBuildTool for GTC_UE5Editor).
```

Headless tests run via the engine's Automation system against the `GTC.*` test
prefixes. See `Source/` for the module layout and the in-module tests under each
system's `Tests/` folder.

## Assets

Most game art is **not** in this repository. It lives in a **private** Git LFS
submodule mounted at `Content/GTCaliberAssets/` (about 15 GB), sourced from:

- **Fab / PropHaus** (Standard License) for the city/environment set,
- **Epic Games** (Unreal Engine EULA) for the Mannequin and template content,
- **Cesium for Unreal** for geospatial config.

These are **separately licensed and are NOT CC BY 4.0**; their handling rules and
per-set provenance live in [docs/ASSET_HANDLING.md](docs/ASSET_HANDLING.md). This
repository tracks only a submodule pointer, never the asset files.

**Access is gated.** Fetching the assets requires read access to the private
asset repository. Without it you can clone and build the **code**, but you will
not get the art, and the project is **not runnable as-is**. This is intentional:
licensed and paid assets are never redistributed here. Contributors who need the
art should request access; the code stays open regardless.

## Repository layout

| Path | What it is |
| --- | --- |
| `Source/` | The UE5 C++ game module (`GTC_UE5`): systems + in-module automation tests |
| `Content/` | Unreal content: project assets (CC BY 4.0) plus the private `GTCaliberAssets` asset submodule (separately licensed) |
| `Config/` | Project / engine / input configuration |
| `Data/` | Data tables and world data |
| `docs/` | Asset policy + provenance, notes, and design docs |
| `tools/` | Build / test gate and helper scripts |

## Contributing

Contributions are welcome: programmers, 3D artists, audio, writers, playtesters.

1. Read [CONTRIBUTING.md](CONTRIBUTING.md).
2. Pick an open task.
3. Open a PR. CI validates headlessly; the build + automation tests must pass.

By contributing **code** you agree to license it under MIT; by contributing
**assets** you agree to CC BY 4.0 and attest you hold the rights (see
[docs/ASSETS.md](docs/ASSETS.md)). AI-generated assets are accepted only when the
generator's terms permit CC BY 4.0 redistribution and that is recorded in the
provenance ledger.
