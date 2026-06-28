# Which BP / map / ABP is which (the live set)

> Part of the `caliber` skill. Reference for "what actually boots / which asset
> do I touch." Pair with `coding-layers.md` (where logic belongs) and
> `editor-mcp.md` (how to edit it live).

## GameMode → player chain (what boots `CityBeachStrip`)

- **`BP_GTCShooterGameMode`** (`/Game/GTCShooter/`) — the GTA-loop shooter mode.
- **`BP_GTCGameMode`** (`/Game/Core/`) — base GTC mode → GTC controller.
- The player pawn in play is the **TPS-Kit Manny motion-match pawn**,
  `BP_PlayerCharacterBase` (`/Game/ThirdPersonKit/Blueprints/…`).
- **`BP_GTCPlayerCharacter`** (`/Game/Core/`) — the C++-backed pawn.
- **`BP_ThirdPersonGameMode`** (submodule) — plain Epic template mode for
  prototype levels.

## Weapons

- **`BP_BaseWeapons`** (`/Game/Blueprints/weapons/`) — root weapon actor (gun
  mesh + shared logic). Children by fire mode: **`BP_BaseFullAutoWeapons`**,
  **`BP_BaseSemiAutoWeapon`**, **`BP_BaseBurstWeapons`**,
  **`BP_BaseFullSMGWeapons`**.
- Pickups are `BP_*WeaponPickup` (`/Game/Blueprints/weapons_pickup/`); each
  pickup's `Weapon` class slot decides what gun it grants. Full recipe in
  `weapon-pickup.md`.

## Maps

There is **no shipping map in this repo's `Content/Levels/`** (it's empty). The
real world is in the submodule (`Content/GTCaliberAssets/Content/…`):

- **`CityBeachStrip`** (`CityBeachStrip/Maps/`) — the shipping World-Partition
  city (sublevels `_Beach` / `_Streets` / `_Resorts`, plus `AssetZoo`). The
  gameplay map.
- **`L_FrontEnd`** (`FrontEnd/Maps/`) — intro→menu front-end, driven by C++
  Slate (`AGTCFrontEndController`). Currently **off** because the menu→city
  transition crashes; `GameDefaultMap` points at the ThirdPerson template
  instead.
- **`Lvl_ThirdPerson`** (`ThirdPerson/`) — Epic template, default/prototype boot.
- Feature/prototype levels named in docs (`DefaultLevel`, `LocomotorLevel`,
  `NPCLevel`) belong in `Content/Levels/` when authored here.

⚠ Switching the live level via MCP `unreal_open_level` **crashes** — don't.

## Anim Blueprints

The project's own ABPs are few; most `ABP_*` are third-party kit.

- **`ABP_PlayCharacter`**
  (`/Game/Mixamo/SoldierRifle/Anims/AnimationBluePrint/`) — the **player**
  locomotion graph on the **soldier rig**. Touch this for player
  movement / aim-offset work.
- **`ABP_GTC_Citizen`** — NPC / citizen graph. **`ABP_WoodenChar`** — wooden NPC.
- **`ABP_TPSKit_InstanceCustomPlayer`** + the large `*_UE5Manny` family
  (`/Game/ThirdPersonKit/…`) — the **licensed kit** anim graphs (motion-matching
  / IK stack). Don't author from scratch.
- **`ABP_Manny`** / **`ABP_Quinn`** — Epic samples.

## HUD widgets (`/Game/UI/`)

- **`WB_Game_HUD_widget`** (kit) — the main in-game HUD.
- The project's own overlays, each driven by a C++ subsystem/component:
  **`WBP_Money`**, **`WBP_AmmoCount`**, **`WBP_WantedStars`**,
  **`WBP_ArrestMeter`**, **`WBP_Busted`**, **`WBP_Wasted`**.

## Where new work goes (provenance — full law in `docs/ASSETS.md`)

- Project-authored / original / CC0 / CC-BY → tracked in **this** repo
  (`Source/`, `Content/Blueprints/`, `Content/Core/`, `Content/Materials/`,
  `Content/UI/`, prototype `Content/Levels/`).
- Third-party / licensed / paid art → the **asset submodule**
  `Content/GTCaliberAssets/` (or local-only + gitignored). Never commit licensed
  art to the public parent repo.
- The canonical **Enhanced Input** assets live only in the submodule
  (`…/Content/Input/`); a CoreRedirect rewrites every `/Game/Input/...` reference
  there. Add new `IA_*` / `IMC_*` to the submodule only — loose copies in
  `Content/Input/` are shadowed and never load.
