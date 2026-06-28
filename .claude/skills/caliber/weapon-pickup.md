# Adding a weapon to the pickup system (GT-caliber)

> Part of the `caliber` skill. Connect through the live editor per
> `editor-mcp.md` (Unreal MCP / editor Python console). Never launch or kill an
> editor.

## How the system actually works

A weapon pickup does **not** contain a weapon. It holds a single class
reference and spawns it on interact. The whole flow is three hops:

```
Player presses E on pickup
  -> BP_BaseWeaponPickup :: Event Interact (from BPI_Interction)
       -> calls BP_ThirdPersonCharacter :: SpawnWeapon(Weapon)   // Weapon = pickup's class slot
       -> Destroy Actor (the pickup removes itself)
  -> SpawnWeapon (Custom Event on the player) spawns the class and
     stores it in the player's `Current Weapon` (BP_BaseWeapons_C*)
```

So a pickup works **only if its `Weapon` slot points at a real weapon class.**
An empty slot means `SpawnWeapon(None)` -> nothing spawns -> the pickup
silently destroys itself, which reads as "it won't pick up."

### The asset layout

Weapons (`/Game/Blueprints/weapons/`):
- `BP_BaseWeapons` — root weapon actor, holds the gun mesh + shared logic.
- `BP_BaseSemiAutoWeapon` — child of base; semi-auto firing. (the "AR" in the
  default level inherits its mesh from `BP_BaseWeapons`.)
- `BP_BaseFullAutoWeapons` — child of base; full-auto firing. Its gun mesh is
  overridden to `SK_KA47` (this is the "KA74").

Pickups (`/Game/Blueprints/weapons_pickup/`):
- `BP_BaseWeaponPickup` — base pickup. **One instance-editable variable:
  `Weapon`** (a `BP_BaseWeapons` subclass). This is the slot that matters.
- `BP_FullAutoWeaponPickup1`, `BP_SemiAutoWeaponPickup` — placeable children.
  Their class-default `Weapon` should be set to the matching weapon class.

Weapon art lives under `/Game/FPS_Weapon_Bundle/Weapons/` (e.g.
`Meshes/Ka47/SK_KA47`, `Meshes/KA74U/SK_KA74U_X`, plus per-weapon
Textures/Materials).

## The #1 gotcha (check this first)

If a placed pickup does nothing, open it in the Details panel (or read it) and
confirm its **`Weapon` slot is not empty**. An unset `Weapon` is the cause of
"the KA74 won't pick up but the AR does" — the working pickup had its slot
filled, the broken one was `None`.

Read every pickup's slot (in-editor Python console):

```python
import unreal
les = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in les.get_all_level_actors():
    if 'Pickup' in a.get_name():
        print(a.get_name(), '-> Weapon =', a.get_editor_property('Weapon'))
```

Note: the placed instance usually *inherits* the pickup Blueprint's class
default, so the fix belongs on the Blueprint default, not the instance. (Setting
`Weapon` on a placed instance via Python fails with "cannot be edited on
instances" — set the class default and the instance inherits it.)

## Recipe A — make an existing weapon class grant on a pickup

Use when the weapon BP already exists (e.g. `BP_BaseFullAutoWeapons`) and you
just need a pickup to hand it out.

```python
import unreal
weapon_cls = unreal.load_object(None,
    '/Game/Blueprints/weapons/BP_BaseFullAutoWeapons.BP_BaseFullAutoWeapons_C')
pickup_cls = unreal.load_object(None,
    '/Game/Blueprints/weapons_pickup/BP_FullAutoWeaponPickup1.BP_FullAutoWeaponPickup1_C')
unreal.get_default_object(pickup_cls).set_editor_property('Weapon', weapon_cls)
unreal.EditorAssetLibrary.save_asset(
    '/Game/Blueprints/weapons_pickup/BP_FullAutoWeaponPickup1', only_if_is_dirty=False)
```

In the editor instead: open the pickup BP -> Class Defaults -> set `Weapon` ->
Compile + Save. Every placed instance that hasn't overridden the slot updates.

## Recipe B — add a brand-new weapon (e.g. a second rifle)

1. **Pick a base by fire mode.** Duplicate `BP_BaseFullAutoWeapons` (full-auto)
   or `BP_BaseSemiAutoWeapon` (semi-auto) to a new BP, e.g.
   `BP_Rifle_<Name>` in `/Game/Blueprints/weapons/`. Duplicating a base keeps
   the firing logic; you only restyle the mesh.
2. **Set the gun mesh.** Open the new BP -> select the gun **SkeletalMesh
   component** -> set its Skeletal Mesh to your weapon (e.g.
   `/Game/FPS_Weapon_Bundle/Weapons/Meshes/KA74U/SK_KA74U_X`). Adjust the
   relative transform so it sits right in the hand if needed.
3. **Make a pickup for it.** Either reuse `BP_FullAutoWeaponPickup1` /
   `BP_SemiAutoWeaponPickup`, or duplicate one to `BP_<Name>Pickup`. Set its
   Class-Defaults `Weapon` to your new `BP_Rifle_<Name>_C` (Recipe A). Give the
   pickup a visible display mesh if it doesn't already have one.
4. **Place it.** Drag the pickup into the level (`CityBeachStrip`), put it on
   the ground (check Z so it isn't buried), Save the level.
5. **Provenance.** Any new binary weapon asset needs a row in `docs/ASSETS.md`
   per the repo rules. Never import assets/names from a commercial game.

## Make sure the weapon is actually *usable* after pickup

`SpawnWeapon` stores the spawned actor as the player's `Current Weapon`. For the
new gun to aim/fire, it must follow the same pattern as the working bases —
which is why **duplicating a base weapon BP (Recipe B step 1) is the safe path**:
the firing graph, ammo, and recoil come along. If you build a weapon from
scratch instead, it must derive from `BP_BaseWeapons` (or a `BP_Base*AutoWeapon`)
and keep the gun mesh component, or the player won't be able to use it.

If you swap fire mode, pick the matching base: full-auto holds fire to keep
shooting, semi-auto fires per trigger pull — that behavior lives in the base
class, not the pickup.

## Verify

1. `Play` in the editor, walk onto the pickup, press the interact key (E).
2. The pickup disappears and the gun appears in the player's hands.
3. Aim + fire. Confirm full-auto vs semi-auto matches the base you chose.
4. Re-run the read-back snippet above to confirm no pickup still has
   `Weapon = None`.

If nothing happens on E: 99% of the time the pickup's `Weapon` slot is still
empty (Recipe A), or the weapon BP doesn't derive from `BP_BaseWeapons` so the
player can't equip it.
