# Characters — the Character Forge

Goal: a creator drops **one skeleton** (a `USkeletalMesh` / `USkeleton`) into the
side editor and the backend **auto-connects every gameplay action the rig can
support** — locomotion, sprint, crouch, jump, swim, weapon hold/aim/reload,
melee, hit reactions, ragdoll death, facial lip-sync, eye look, finger pose —
then drops a fully playable pawn into the live game. No per-character hand-wiring.

This mirrors the module-wide split used everywhere else (see `NpcArchetypes`,
`Missions/Editor`): a **scene-free, headless-tested pure-core decision layer**
under a thin **UCLASS adapter** that touches engine state.

## Layers

- `Wiring/CharacterWiring.{h,cpp}` — **pure core (done).** Bone-name →
  `FSkeletonProbe` capability inference across rig conventions (UE5 Mannequin,
  Mixamo, MetaHuman, generic), and `FCharacterWiring::Plan(probe, role)` →
  `FCharacterWiringPlan`: which actions to wire for a `ECharacterRole`
  (Player / Pedestrian / Combatant / VehicleOccupant), with per-action skip
  reasons and a one-line `Summary()`. Headless tests in `Wiring/Tests/`
  (prefix `GTC.Characters.Wiring`).

- `Forge/GTCCharacterForge.{h,cpp}` — **adapter (Wave 2, done; compiles green).**
  A `UWorldSubsystem` bridging a real rig into the brain:
  `BoneNamesOf(USkeleton)` → `ProbeMesh`/`ProbeSkeleton` → `PlanForMesh`, and
  `ApplyToCharacter(ACharacter*, USkeletalMesh*, role)` which assigns the body
  mesh and attaches a `UGTCWeaponComponent` when `WeaponHold` is wired
  (idempotent, null-guarded). Drives the console **side editor**:
  `GTC.Character.Inspect <SkeletalMeshPath> [role]` prints the plan + per-action
  checklist + skip reasons; `GTC.Character.Apply <SkeletalMeshPath> [role]` wires
  the live player pawn from that skeleton on the spot.

- `Forge/GTCCharacterForge` **Wave 3 (done; compiles green):**
  `SpawnForged(Body, role, where, seed, &outPlan)` deferred-spawns an
  `AGTCCitizen` (the crowd's configured `CitizenClass` when set, else the raw
  class), stamps `InitializeFromSeed`, finishes spawning, then overrides the body
  mesh and wires the role via `ApplyToCharacter`. Console:
  `GTC.Character.Spawn <SkeletalMeshPath> [role] [seed]` drops a fresh wired pawn
  in front of the player — a dropped-in skeleton becomes a walking NPC, not just
  a re-skin.

- `Wiring/CharacterWiring` **action binding table (Wave 4, done; compiles green):**
  `GTCActionBinding(action)` → `FActionBinding {Driver, MontageSlot, AnimNode,
  ComponentTag}` mapping every action to its *real* driver — locomotion graph
  nodes (`Move`/`JumpStart`), full/upper-body montage slots
  (`DefaultSlot`/`UpperBody`), component tags (`Weapon`→`UGTCWeaponComponent`,
  `Health`→`UPlayerHealthComponent`), or a Blueprint seam (lip-sync/eye-look).
  `ApplyToCharacter` now consumes it to attach the weapon *and* health components
  (idempotent). Covered by `GTC.Characters.Wiring` tests.

- `Wiring/RigProfile.{h,cpp}` **rig identification (Wave 5, done; compiles green):**
  `GTCDetectRigConvention(bones)` → `ERigConvention` (UE5Mannequin / Mixamo /
  MetaHuman / Generic / Unknown) and `GTCResolveCanonicalBone(slot, bones)` →
  the rig's verbatim bone name for a canonical joint (Pelvis, RightHand, Jaw, …).
  This is the cross-rig retargeting/socket-binding intelligence — so the shared
  anim graph and weapon sockets bind correctly no matter how the skeleton names
  its joints. Surfaced via `GTC.Character.Rig <SkeletalMeshPath>`. Tested under
  `GTC.Characters.Rig`.

- `Forge/GTCCharacterForge` **cross-rig socket binding (Wave 6, done; compiles
  green):** `ApplyToCharacter` now resolves the inserted rig's actual hand bone
  via `GTCResolveCanonicalBone(RightHand→LeftHand)` and sets the weapon
  component's `AttachSocketName` before it attaches — so weapons bind to the
  correct bone on Mixamo/MetaHuman/generic rigs, not just UE's `hand_r`.

- `Editor/SGTCForgePanel.{h,cpp}` **on-screen side editor (Wave 7, done;
  compiles green):** a Slate panel (`SCompoundWidget`) with a skeleton-path
  field, role field, Inspect / Apply-to-Player / Spawn buttons, and a live plan
  checklist readout — wired to the world's `UGTCCharacterForge` via an
  `FGTCForgeServices` callback bridge. Toggle with `GTC.Character.Editor`.
  (Compile-verified; visual/runtime behaviour needs an in-editor check.)

- `Forge/CharacterRecipe.{h,cpp}` **recipe persistence (Wave 8, done; compiles
  green):** save/load an authored character (skeleton path + role + seed) as a
  hand-editable `.character.json` (own `gtc_character` envelope, reuses the
  in-module `Systems/Save/SaveJson` model like `GtcMissionJson`). Round-trip +
  garbage-safe, tested under `GTC.Characters.Recipe`. Console
  `GTC.Character.SaveRecipe` / `LoadRecipe` (load re-spawns the wired character).

- `Forge/CharacterRecipe` **recipe bank (Wave 9, done; compiles green):**
  `BankToJson`/`BankFromJson` (+ file IO) persist a whole roster as a
  `gtc_character_bank` file (malformed entries skipped, not fatal). The Forge
  holds an in-memory `AuthoredRoster`; console `GTC.Character.Add` / `Roster` /
  `SaveBank` / `LoadBank` / `SpawnAll` author a cast and drop it all into the
  world at once. Bank round-trip tested under `GTC.Characters.Recipe`.

- `Wiring/CharacterWiring` **creature roles + auto-role (Wave 10, done; compiles
  green):** `ECharacterRole::Creature` (moves/reacts/vocalises, no hands→no
  weapons) for dogs/animals, and `GTCSuggestRole(probe)` so the backend auto-picks
  the role from the rig (handless+feet+spine → Creature, else Pedestrian).
  `GTC.Character.Inspect`/`Spawn` with no role arg now self-select. Tested.

- `Wiring/RigAudit.{h,cpp}` **rig quality audit (Wave 11, done; compiles green):**
  `GTCAuditRig(probe, bones)` → `FRigAudit {Score 0-100, Issues, Notes,
  bProductionReady}` — grades a skeleton for AAA suitability beyond yes/no
  capability: spine depth, twist/roll bones, l/r symmetry, toe/finger detail,
  bone-budget sanity. Surfaced via `GTC.Character.Audit`. Tested under
  `GTC.Characters.Audit`.

> Remaining items below need the **live editor / content** — under the
> single-editor rules, if no editor is running, stop and ask rather than launch.

## Roadmap (next waves)

1. **ABP retargeting** — build/assign an IK Retargeter so the shared locomotion
   graph drives the inserted rig (needs an editor/content step, not pure C++).
2. **Author the reserved montages** — hit-react flinch, reload, crouch/swim
   poses so the bound slots actually play.
3. **Panel polish** — role as a dropdown/segmented control; a "browse asset"
   picker; persist the last-used path. (Needs in-editor visual iteration.)

Definition of done per wave: format + lint + build (UnrealBuildTool) + the
`GTC.Characters.*` automation tests pass, and `main` still boots to a
controllable character.
