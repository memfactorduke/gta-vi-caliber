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
| `Content/Environment/Sky/M_GTCStars.uasset` | Additive unlit night-sky star material; procedural (grid-hashed view direction, no texture), driven by `AGTCWeatherController` scalar params StarOpacity/StarBrightness | project contributors | original | CC BY 4.0 |
| `Content/Surfaces/Physical/PM_GTC_*.uasset` | 13 physical materials (Wood/Metal/Glass/Concrete/Asphalt/Brick/Ceramic/Rubber/Vegetation/Ice/Leather/Paper/Water) tagging SurfaceType1..14 for the weapon/melee impact-FX system; no art, each just sets a surface-type byte | project contributors | original | CC BY 4.0 |
| `Content/Surfaces/Decals/M_GTC_BulletDecal.uasset` | Deferred-decal master for bullet marks (MarkTex*Tint -> BaseColor, MarkTex.R -> Opacity); references VFX-pack textures by path only, embeds none | project contributors | original | CC BY 4.0 |
| `Content/Surfaces/Decals/MI_GTC_BulletHole.uasset`, `MI_GTC_BulletScorch.uasset`, `MI_GTC_GlassCrack.uasset` | 3 bullet-mark decal instances (tints of `M_GTC_BulletDecal`) for hole / metal scorch / glass crack | project contributors | original | CC BY 4.0 |
| `Content/Surfaces/FX/M_GTC_Tracer.uasset` | Unlit additive tracer-streak material (Color*Brightness -> Emissive, no texture); the faint muzzle->impact "ammo line" | project contributors | original | CC BY 4.0 |
| `Content/Blueprints/AI/*.uasset` (8) | AI StateTree assets + AI-controller Blueprints driving police/NPC combat, dispatch and navigation behavior | project contributors | original | CC BY 4.0 |
| `Content/Blueprints/Cameras/**/*.uasset` (20) | Camera rig prefabs + camera-behavior Blueprints for the player/gameplay cameras (config; reference only engine GameplayCameras types, embed no art) | project contributors | original | CC BY 4.0 |
| `Content/Blueprints/ControlRigs/*.uasset` (1) | Control Rig asset for the player/NPC skeleton | project contributors | original | CC BY 4.0 |
| `Content/Blueprints/Data/*.uasset` (34) | Primary data assets / data tables for weapon, NPC, appearance and gameplay configuration | project contributors | original | CC BY 4.0 |
| `Content/Blueprints/ENUMS/*.uasset` (1) | Blueprint enumeration used by the gameplay systems | project contributors | original | CC BY 4.0 |
| `Content/Blueprints/Interfaces/*.uasset` (1) | Blueprint interaction interface (`BPI_Interction`) for the press-E system | project contributors | original | CC BY 4.0 |
| `Content/Blueprints/Magazines/*.uasset` (4) | Weapon magazine Blueprints | project contributors | original | CC BY 4.0 |
| `Content/Blueprints/MovementModes/*.uasset` (5) | Custom movement-mode Blueprints (Mover plugin) | project contributors | original | CC BY 4.0 |
| `Content/Blueprints/SmartObjects/*.uasset` (22) | SmartObject definition Blueprints for NPC interaction/occupation points | project contributors | original | CC BY 4.0 |
| `Content/Blueprints/weapons/*.uasset` (5) | Base weapon Blueprints (full-auto / semi / burst / SMG) parented to the C++ weapon classes | project contributors | original | CC BY 4.0 |
| `Content/Blueprints/weapons_pickup/*.uasset` (8) | Weapon-pickup actor Blueprints | project contributors | original | CC BY 4.0 |
| `Content/Blueprints/BP_NPC.uasset`, `Content/Core/BP_NPC.uasset` (2) | NPC pawn Blueprint(s) | project contributors | original | CC BY 4.0 |
| `Content/Core/BP_ThirdPersonCharacter.uasset` (1) | Player pawn Blueprint — original project logic; structure derived from the Epic UE third-person template (UE EULA), wiring the project Mixamo soldier ABP + Enhanced Input; references only project/engine assets, embeds no licensed art | project contributors | original (Epic template-derived) | CC BY 4.0 |
| `Content/Misc/SandboxAnimCurveCompressionSettings.uasset` (1) | Animation curve compression settings asset | project contributors | original | CC BY 4.0 |
| `Content/Characters/Wooden/*.uasset` (4), `Content/Tripo/{SK_WoodenChar*,M_WoodenChar,T_WoodenChar_BaseColor,IK_WoodenChar,ABP_WoodenChar}.uasset` | AI-generated "Wooden" playable character — skeletal mesh, skeleton, physics asset, material, base-color texture, IK rig and anim blueprint (original-art replacement for licensed humans) | project contributors | AI (Tripo) | CC BY 4.0 |
| `Content/Tripo/RTG_Soldier_to_WoodenChar.uasset`, `Content/Tripo/Anims/WC_*.uasset` (5) | IK retargeter + clips retargeted from the project's existing library animations onto the Wooden rig | project contributors | original (project retarget) | CC BY 4.0 |

*(Append one row per project-authored asset. Path relative to repo root. "Source"
is `original` or a URL. License must be CC0, CC-BY-4.0, or CC-BY-4.0-compatible.
Third-party / licensed packs do NOT go here; they belong in the
`Content/GTCaliberAssets/` submodule and are tracked in ASSET_HANDLING.md.)*

## Known exception — third-party kept in-tree by project decision

The asset below is **not** CC BY 4.0 and, by Rules 1 and 6, would normally live in
the private `Content/GTCaliberAssets/` submodule rather than in this repository. It
is retained directly here as a deliberate, documented project decision, governed by
its upstream license and **not** relicensed by inclusion. It is recorded separately
so the CC-BY ledger above remains accurate.

| Path | Description | Source | License |
| --- | --- | --- | --- |
| `Content/Mixamo/SoldierRifle/**` (52) | Adobe Mixamo soldier-rifle character (mesh / skeleton / textures), Mixamo motion clips, project-retargeted `SR_*` clips, retarget + IK rigs, the player anim blueprint `ABP_PlayCharacter`, and the locomotion blendspace | Adobe Mixamo (mixamo.com) | Mixamo License — free use within projects; redistribution of the raw asset files is restricted |
