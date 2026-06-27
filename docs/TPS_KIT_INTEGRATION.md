# Third Person Shooter Kit → GTA6 (GTC) integration

Tracking the integration of the paid **ThirdPersonShooterKit** (Fab, ~$200, ~12GB,
UE 5.5, Blueprint-only) into GTC_UE5 (UE 5.7), targeting the **CityBeachStrip**
shipping map.

## Source

- Project dropped at `Content/ThirdPersonShooterKit/` (its own `.uproject` +
  `Config/` + `Content/ThirdPersonKit/`). **Gitignored** — paid asset, must NEVER
  hit the public repo.
- It's **GASP + Motion Matching** on the **UE4/UE5 Mannequin** (and MetaHuman)
  skeletons. Default showcase map = `FeaturesMap_v2` (`BP_TPS_Game_Mode`), with
  per-feature rooms (Weapons, AI, Cover, Melee, MotionMatching, Turret,
  HitReaction, SecurityCamera).

## Done (config + prep, this session)

All committable text — no assets moved yet.

1. **Leak closed** — `.gitignore` now ignores `/Content/ThirdPersonShooterKit/**`
   and `/Content/ThirdPersonKit/**` (the migration landing zone).
2. **Plugins** — added `RawInput`, `LiveLinkControlRig`, `AppleARKitFaceSupport`
   to `GTC_UE5.uproject` (optional; the core motion-matching stack — PoseSearch,
   MotionTrajectory, MotionWarping, AnimationWarping, Chooser,
   AnimationLocomotionLibrary — was **already enabled** via GASP). Takes effect on
   next editor launch.
3. **Collision** (`Config/DefaultEngine.ini`, `[/Script/Engine.CollisionProfile]`):
   added the kit's 11 custom trace channels **verbatim** — GTC had defined NONE,
   so `GameTraceChannel1..11` (Bullet, Player_Mesh, Player_Cover_Back_Wall,
   Human_AI_Capsule, Ladder, Human_AI_mesh, Enemy_Component_Add_On, Vault_Trigger,
   PawnHelper, Water, Trigger) line up with the indices baked into the kit BPs.
   Added the `DamageArea` profile + the kit's channel-rename redirects.
4. **Gameplay tags** — created `Config/DefaultGameplayTags.ini` (GTC had none) with
   the kit's Foley.* / MotionMatching.* / Abilities.* tags.
5. **Input** — appended the kit's legacy ActionMappings/AxisMappings (Shoot, Reload,
   Aiming, weapon slots, melee, grenade, cover) to `Config/DefaultInput.ini`. The
   kit BPs fire legacy input nodes; GTC's Enhanced Input is a separate system, so
   additive.

### Known cosmetic mismatch (NOT broken)

**Physical surfaces were deliberately NOT merged.** GTC owns `SurfaceType1..14`
(Wood/Metal/Glass/…/Water, load-bearing order, 330 `PM_GTC_*` materials). The kit
uses a different index order (its `SurfaceType4=Metal` = GTC's `Concrete`). The
kit's gun impact FX will pick the wrong surface burst until its PhysicalMaterials
are remapped to GTC's table. Cosmetic only.

## Next — asset migration (in-editor, user-run; MCP is status-only, can't Migrate)

1. Open `Content/ThirdPersonShooterKit/ThirdPersonShooterKit.uproject` → **"Open a
   copy"** (5.5→5.7 upgrade; never open-in-place). This is a separate editor from
   the shared GTC editor — fine, different project.
2. Content Browser → right-click **`ThirdPersonKit`** → Asset Actions → **Migrate**
   → target `…/GT-caliber_5.8/Content`. Preserves `/Game/ThirdPersonKit/…` paths so
   internal refs don't break; the gitignore above keeps it out of the public repo.
3. Close the kit editor. The GTC editor picks the assets up (content-browser
   refresh; no forced restart).

## Then — player rig decision (the one real fork)

To get the kit's shooter *feel*, the GTC **player** becomes the kit's
motion-matching Manny character (`BP_PlayerCharacterMannequinUE5_MotionMatching`),
wired into `BP_GTCGameMode`, keeping GTC's controller/HUD/world systems. This
**retires the soldier-rig 8-way for the player** (soldier rig stays for NPCs).
Alternative (not chosen): keep soldier player and retarget the kit's motion-match
DB to soldier — painful, DBs are skeleton-bound.

## Then — CityBeachStrip

Goal: the kit's gameplay running in the shipping world, not just the feature rooms.
- Player pawn swap above → drop into CityBeachStrip and verify locomotion/aim/fire.
- Place kit systems into the map (cover points, ladders/vaults, weapon stands,
  pickups, AI encounters/spawners) via MCP `save_current_level()` (NOT `open_level`
  — that crashes the editor, see memory).
- Reconcile with GTC's existing C++ weapon/police/AI so they don't double up.

## Gotchas

- Save levels with `save_current_level()`; `open_level`/`unreal_open_level` to
  switch live level **crashes** the shared editor.
- Don't relaunch/kill the shared GTC editor (hook-enforced).
- Migrated assets + the kit project are gitignored; never `git add -A` without the
  protective `.gitignore` present.
</content>
</invoke>
