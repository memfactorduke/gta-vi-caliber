# Animate any model — the universal pipeline (GT-caliber)

> Part of the `caliber` skill. Work through the **live running editor** per
> `editor-mcp.md`. The pipeline runs raw `import unreal` code, so use the
> **in-editor Python console** (legacy notes call this "vibeue
> `execute_python_code`" — same idea, current path is the editor console; the MCP
> `ProgrammaticToolset` sandbox cannot `import unreal`). Never
> launch/close/restart an editor.

## Mental model: Manny is the animation hub

Every animation in the project lives on ONE skeleton — the UE5 Mannequin
(`SK_Mannequin`): GASP + ALS + Lyra + the aim offsets, **1,748 clips** under
`/Game/_AnimationLibrary` plus the submodule. A new model is **never hand-rigged**.
It gets, automatically:

1. an **IK Rig** (auto-characterized from its own skeleton), and
2. an **IK Retargeter** Manny → the model,

then it inherits the whole library. The engine is `Scripts/gtc_anim/universal_retarget.py`
(full notes in `Scripts/gtc_anim/README.md`). This file is the how/when.

## Recipe — one call creates ALL animations

Run inside the live editor (Python console):

```python
import sys; sys.path.append('/Users/ziwenxu/Desktop/Code/GTA6-unreal/GT-caliber_5.8/Scripts/gtc_anim')
import importlib, universal_retarget as ur; importlib.reload(ur)

ur.animate_model('/Game/Characters/MyGuy/SKM_MyGuy', 'MyGuy')   # stance='Rifle' (default)
```

`animate_model` runs the whole pipeline and gives the model the **full set**:
1. **onboard** — auto IK Rig + Manny→model retargeter (duplicate template + swap
   rig, mapping carries over by chain name), rest pose auto-aligned.
2. **full-body IK** — `apply_auto_fbik` adds hand/foot goals (the fix that took
   the Mixamo soldier from awkward to good — pins grips/feet that FK alone drifts).
3. **standard set** — every animation, retargeted + assembled.

Everything lands in `/Game/_AnimPipeline/MyGuy/`:
- `AO_MyGuy_Rifle_Hipfire`, `AO_MyGuy_Rifle_ADS` — **aim offsets** (8-dir look, 5×3 Yaw×Pitch grid)
- `BS_MyGuy_Walk`, `BS_MyGuy_Jog`, `BS_MyGuy_CrouchWalk` — **8-directional** blendspaces (walk / run / crouch-walk)
- `Anims/` — baked source clips incl. **jump** (Start/Fall_Loop/Fall_Land), **crouch** (idle/entry/exit), idle

> **Timeout:** the full bake is ~45s — over the MCP 30s reply window. Either run
> from the editor's Python console (no limit), or split into two calls that each
> finish in time:
> ```python
> r = ur.onboard_model(mesh, 'MyGuy')
> unreal.IKRigController.get_controller(unreal.EditorAssetLibrary.load_asset(r['ik_rig'])).apply_auto_fbik()
> unreal.EditorAssetLibrary.save_asset(r['ik_rig'])
> ur.bake_standard_set('MyGuy')        # ~13s
> ```
> If `animate_model` reports a timeout, the editor still finished — just verify.

### Stances & granular pieces
```python
ur.animate_model(mesh, 'MyGuy', stance='Pistol')      # or 'Unarmed'
ur.bake_standard_set('MyGuy', stance='Rifle')         # just the animation set
ur.build_dir_blendspace('MyGuy','BS_MyGuy_Walk','MF_Rifle_Walk')      # one blendspace
ur.build_aim_offset('MyGuy','AO_MyGuy_Rifle_ADS','MM_Rifle_Idle_ADS') # one aim offset
```
Stances live in `STANCE` in `universal_retarget.py`: **Rifle** (full — walk/jog/
crouch/jump + hipfire & ADS aim), **Pistol** (ADS aim), **Unarmed** (idle-ready
aim). Add new stances/clip families there. Check `r['auto']` — `False` means the
skeleton wasn't recognized (see "Non-humanoid").

## Wiring it onto the character — two modes

The standard set gives the model ready blendspaces + aim offsets. Hook them into a
character one of two ways:

**A) Bake (default — what `animate_model` does):** the model owns native
`<Model>_*` clips + `BS_<Model>_*` / `AO_<Model>_*` assets. Drive them from the
model's own AnimBP state machine (Idle → `BS_Walk`/`BS_Jog` by Speed; the aim
offset layered on the upper body via LayeredBlendPerBone at `spine_01`; jump &
crouch states from the baked clips). For extra/curated clips beyond the standard
set: `ur.bake_onto_model('MyGuy', [clip object-paths])`.

**B) Runtime retarget (many models, zero duplication):** author the locomotion/aim
AnimBP **once** on Manny; each model uses a tiny follower AnimBP = a single
*Retarget Pose From Mesh* node + that model's `RTG_Manny_to_<model>`. The node is
placed by hand **once** (UE scripting can't edit AnimGraphs), then reused for every
model.

> Worked example to copy: `/Game/_AnimPipeline/SoldierUni/` is the Mixamo soldier
> with the full kit built this way — `AO_SoldierUni_Rifle_Hipfire`/`_ADS`,
> `BS_SoldierUni_Walk`/`_Jog`/`_CrouchWalk`, plus jump + crouch clips in `Anims/`.

## Verify

```python
unreal.get_editor_subsystem(unreal.AssetEditorSubsystem).open_editor_for_assets(
    [unreal.EditorAssetLibrary.load_asset('/Game/_AnimPipeline/MyGuy/AO_MyGuy_Rifle_Hipfire')])
```
Open a baked aim offset or anim and confirm it plays clean on the model (drag the
green dot on an aim offset). Then preview locomotion in PIE if the model is the
player.

## Gotchas (don't re-discover — discovered on UE 5.7, pipeline still applies)

- **Auto-characterize works on non-UE skeletons** (Mixamo `mixamorig9:*`) and
  emits standard chain names — that naming is what makes mapping universal.
- **A from-scratch factory retargeter won't bind its chain mapping headless** —
  the op-stack only initializes in-editor, so `auto_map_chains`/`set_source_chain`
  silently no-op. That's why `onboard_model` **duplicates the template** instead.
- **AnimGraphs are not API-editable** (neither MCP nor Python) — any AnimBP node
  (e.g. Retarget Pose From Mesh, or wiring an aim offset in) is a manual step.
- **Provenance:** the Mega library + Manny meshes are Epic-licensed and gitignored
  (`/Content/_AnimationLibrary/`, `/Content/_WeaponsPacks/`, `/Content/Audio/`,
  `/Content/Characters/Mannequins/`, `/Content/_MegaSample/`). Per-model rigs in
  `/Game/_AnimPipeline/` are project-authored config and may be tracked.

## Non-humanoid / unrecognized skeletons

If `r['auto']` is `False`, add the standard chains manually on the model's IK Rig
(`add_retarget_chain('Spine', start, end, 'None')`, etc. — names MUST match
Manny's: Root, Spine, Neck, Head, Left/RightClavicle, Left/RightArm,
Left/RightLeg, the finger chains), then re-run the duplicate/swap. Quadrupeds need
their own template retargeter with a matching chain vocabulary.
