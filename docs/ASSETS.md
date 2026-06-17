# Asset policy & provenance ledger

One illegitimate asset can poison the entire project. These rules are strict
because they have to be.

## The rules

1. **Original work, CC0, or CC-BY only** for assets this project authors and
   commits to this repository. No exceptions, no "temporary" rips, no AI
   generations prompted to imitate a specific copyrighted work. Content from any
   Grand Theft Auto product (or any other commercial game) is banned outright:
   models, textures, audio, fonts, logos, map layouts.
2. **Every project-authored asset has a ledger row** (below), added in the same
   PR that adds the file. PRs with unledgered binaries are closed.
3. **CC-BY attribution** is collected in this file and shipped in the game's
   credits screen.
4. **Size:** nothing over 50 MB per file for project-authored assets. Source
   files (`.blend`, `.psd`) stay out of the repo unless they are the canonical
   editable original and under the cap.
5. Binary formats route through **Git LFS** via `.gitattributes`; verify with
   `git lfs status` before pushing.
6. **Third-party / licensed asset packs are NOT committed to this repository.**
   They live in the private `Content/GTCaliberAssets/` submodule under their own
   licenses and are governed by [ASSET_HANDLING.md](ASSET_HANDLING.md), not by
   this CC-BY ledger. See the "Third-party assets" section below.

## Third-party assets (the bulk of the game art)

Most game art is **not** in this repository and is **not** CC BY 4.0. It lives in
the private Git LFS submodule at `Content/GTCaliberAssets/` (about 15 GB), under a
mix of licenses:

| Set | Source | License |
| --- | --- | --- |
| `CityBeachStrip/` | Fab / PropHaus | Standard License |
| `Characters/` (Mannequins + anim sets), `ThirdPerson/`, `LevelPrototyping/`, `Input/` | Epic Games | Unreal Engine EULA |
| `CesiumSettings/` | Cesium for Unreal | per Cesium terms |

These are tracked per-set, with handling rules and the hard "no asset contents
into any AI model" line, in [ASSET_HANDLING.md](ASSET_HANDLING.md). They are not
relicensed by inclusion, and the repository tracks only a submodule pointer to
them. The CC-BY ledger below covers **only** the project's own committed assets.

## AI-generated assets

Allowed for project-authored assets only if: the generator's terms permit
redistribution under CC BY 4.0, the prompt did not target a specific
artist/game/franchise, and the ledger row marks it `AI (<tool>)`. Maintainers may
reject anything that looks too close to existing commercial work regardless of
provenance.

## Provenance ledger (project-authored assets in this repo)

These are the only binary assets this repository tracks directly. All are
original project work, CC BY 4.0.

| Path | Description | Author | Source | License |
| --- | --- | --- | --- | --- |
| `Content/Materials/MPC_GTCGlobals.uasset` | Material Parameter Collection for global world params (night amount, wetness) driving the day-night and neon-wet-night look | project contributors | original | CC BY 4.0 |
| `Content/Core/BP_GTCGameMode.uasset` | GameMode Blueprint wiring the C++ pawn / PlayerController / PlayerState / HUD classes | project contributors | original | CC BY 4.0 |
| `Content/UI/WBP_GTCHud.uasset` | HUD widget Blueprint parented to the C++ `UGTCHudWidget` (health/armor/money bindings) | project contributors | original | CC BY 4.0 |
| `Content/Input/IMC_Default.uasset` | Enhanced Input Mapping Context (WASD/mouse/gamepad bindings) | project contributors | original | CC BY 4.0 |
| `Content/Input/IA_Move.uasset` | Enhanced Input Action: move (Axis2D) | project contributors | original | CC BY 4.0 |
| `Content/Input/IA_Look.uasset` | Enhanced Input Action: look (Axis2D) | project contributors | original | CC BY 4.0 |
| `Content/Input/IA_Jump.uasset` | Enhanced Input Action: jump (bool) | project contributors | original | CC BY 4.0 |
| `Content/Input/IA_Interact.uasset` | Enhanced Input Action: interact (bool) | project contributors | original | CC BY 4.0 |
| `Content/Input/IA_Fire.uasset` | Enhanced Input Action: fire (bool) | project contributors | original | CC BY 4.0 |

*(Append one row per project-authored asset. Path relative to repo root. "Source"
is `original` or a URL. License must be CC0, CC-BY-4.0, or CC-BY-4.0-compatible.
Third-party / licensed packs do NOT go here; they belong in the
`Content/GTCaliberAssets/` submodule and are tracked in ASSET_HANDLING.md.)*
