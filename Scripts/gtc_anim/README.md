# Universal animation pipeline — one way for every model

Goal: drop in **any** 3D humanoid model and have it play the **entire** animation
library with one repeatable step — no per-model hand-rigging.

## The architecture: Manny is the hub

Every animation in the project lives on **one** canonical skeleton, the UE5
Mannequin (`SK_Mannequin`):

- `/Game/_AnimationLibrary/` — GASP (923) + ALS (152) + Lyra (673) = 1,748 clips
- the aim offsets (`AO_*`) and the project's other Manny clips

A new model is never rigged by hand. It gets, **automatically**:

1. an **IK Rig** — auto-characterized from its own skeleton, and
2. an **IK Retargeter** Manny → model.

Then it inherits the whole library. The model keeps its own mesh/skeleton; only
the *animation* is mapped across.

## Onboard a new model (the one call)

In the live editor (Unreal MCP / vibeue `execute_python_code`):

```python
import universal_retarget as ur

# EVERYTHING: rig + full-body IK + the whole standard animation set
ur.animate_model('/Game/Characters/MyGuy/SKM_MyGuy', 'MyGuy')   # stance='Rifle'

# ...or the pieces:
r = ur.onboard_model('/Game/Characters/MyGuy/SKM_MyGuy', 'MyGuy')  # just IK Rig + retargeter
ur.bake_standard_set('MyGuy', stance='Rifle')                      # just the animations
```

`animate_model` creates, under `/Game/_AnimPipeline/MyGuy/`:
`IK_MyGuy` + `RTG_Manny_to_MyGuy`, the 8-dir blendspaces `BS_MyGuy_Walk`/`_Jog`/
`_CrouchWalk`, the aim offsets `AO_MyGuy_Rifle_Hipfire`/`_ADS`, and baked jump /
crouch / idle clips in `Anims/`. `r['auto']` is False if the skeleton wasn't
recognized (add chains manually — see below). NB: the full bake is ~45s — run it
from the editor console, or split onboard + `bake_standard_set` under the MCP 30s
window. Stances (`Rifle`/`Pistol`/`Unarmed`) live in `STANCE` in the script.

## Delivering the animations: two modes

**A) Runtime retarget (preferred for MANY models — zero duplication).**
Author the locomotion/aim AnimBP **once** on Manny. Each model uses a tiny
"follower" AnimBP whose AnimGraph is a single **Retarget Pose From Mesh** node
pointing at the Manny leader pose + that model's `RTG_Manny_to_<model>`. One
AnimBP authored once, every model reuses it (set the retargeter per model).
*(The Retarget-Pose-From-Mesh node must be placed by hand once — UE's scripting
API cannot edit AnimGraphs; after that it's reused for all models.)*

**B) Bake (for hero models / a curated set).**
```python
ur.bake_onto_model('MyGuy', ur.library_clips('/Game/_AnimationLibrary/Lyra'))
# or a hand-picked list of clip object-paths
```
Creates `MyGuy_*` copies on the model's skeleton. Avoid baking all 1,748 onto
many models (storage); bake a curated locomotion+aim set, or use mode A.

## Why it's built the way it is (UE 5.7 gotchas, don't re-discover)

- `apply_auto_generated_retarget_definition()` recognizes humanoid skeletons
  (verified on Mixamo `mixamorig9:*`) and emits **standard** chain names
  (`Spine`, `LeftArm`, `LeftIndex`, …). That naming is what makes the rest
  universal.
- A retargeter built from a **blank factory does not bind its chain mapping
  headless** — the 5.7 op-stack (Pelvis Motion → FK Chains → IK Goals → Run IK
  Rig → Root Motion → Curves) only initializes in-editor, so `auto_map_chains`
  / `set_source_chain` silently no-op. Workaround used here: **duplicate the
  template retargeter** (`RTG_Manny_to_Soldier`, already mapped) and **swap its
  target rig**. Mappings are stored by chain *name*, and auto-characterize gives
  every model the same names → the mapping transfers 1:1 (validated 19/19).
- `auto_align_all_bones` re-aligns the rest pose per model. If source and target
  already match directionally it legitimately produces zero offsets.

## Non-humanoid / unrecognized skeletons

If `onboard_model` reports `auto=False`, the skeleton didn't match a template.
Add the standard chains explicitly on the model's IK Rig controller, e.g.:

```python
c = unreal.IKRigController.get_controller(ik_rig)
c.add_retarget_chain('Spine',    'spine_start_bone',    'spine_end_bone',    'None')
c.add_retarget_chain('LeftArm',  'upperarm_l',          'hand_l',            'None')
# ... root, neck, head, clavicles, arms, legs, fingers — names MUST match Manny's
```
then re-run the duplicate/swap step. Quadrupeds/creatures need their own template
retargeter with a matching chain vocabulary.

## Limits (honest)

Retargeting is rotation-driven; a model whose proportions differ a lot from Manny
(e.g. the Mixamo soldier) can still look slightly off on precise weapon grips,
because FK retargeting preserves angles, not hand *positions*. For pixel-perfect
results either add IK goals (hand/foot) to the model rig, or skin the model to
`SK_Mannequin` directly (then it's native, no retarget). For most models the
auto pipeline above is the right default.
