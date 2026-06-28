"""
universal_retarget.py — GT-Caliber universal animation pipeline.

ONE WAY to give ANY 3D humanoid model the WHOLE animation library.

Architecture ("Manny is the hub"):
  * Every animation in the project lives on ONE canonical skeleton, the UE5
    Mannequin (SK_Mannequin) — GASP, ALS, Lyra and the aim offsets are all
    authored there (1,748 clips under /Game/_AnimationLibrary + the submodule).
  * A new model never gets hand-rigged. Instead it gets, automatically:
        1) an IK Rig (auto-characterized from its skeleton), and
        2) an IK Retargeter from Manny -> the model.
    Then it inherits the entire library — either BAKED (per-model copies) or
    RETARGETED AT RUNTIME (one shared Manny AnimBP, zero duplication).

Why it is built this way (hard-won, UE 5.7 specifics):
  * apply_auto_generated_retarget_definition() recognizes humanoid skeletons
    (verified on Mixamo) and emits STANDARD chain names (Spine, LeftArm, ...).
  * Creating a retargeter from a blank factory does NOT bind its chain mapping
    headless (the 5.7 op-stack only initializes in-editor). So we DUPLICATE a
    known-good TEMPLATE retargeter and swap its target rig — because mappings
    are stored BY CHAIN NAME and auto-characterize always uses the same names,
    the mapping carries over 1:1 for any model. (Validated: 19/19.)

Run inside the live editor (Unreal MCP / vibeue execute_python_code), e.g.:
    import universal_retarget as ur
    ur.onboard_model('/Game/Characters/MyGuy/SKM_MyGuy', 'MyGuy')
    ur.bake_aim_and_locomotion('MyGuy')          # optional: bake a curated set
"""
import unreal

# ---- canonical hub config -------------------------------------------------
HUB_SOURCE_MESH   = '/Game/GTCaliberAssets/Content/Characters/Mannequins/Meshes/SKM_Manny_Simple'
TEMPLATE_RTG      = '/Game/Mixamo/SoldierRifle/RTG_Manny_to_Soldier'   # Manny-source, fully-mapped, 6-op stack
ANIM_LIBRARY      = '/Game/_AnimationLibrary'                          # GASP + ALS + Lyra (all on Manny)
PIPELINE_ROOT     = '/Game/_AnimPipeline'                              # where per-model rigs/retargeters land

EAL = unreal.EditorAssetLibrary
AT  = unreal.AssetToolsHelpers.get_asset_tools()
SRC = unreal.RetargetSourceOrTarget.SOURCE
TGT = unreal.RetargetSourceOrTarget.TARGET


def _chain_names(ik_rig):
    return [str(c.get_editor_property('chain_name'))
            for c in unreal.IKRigController.get_controller(ik_rig).get_retarget_chains()]


def onboard_model(model_mesh_path, model_name, out_dir=None, align=True):
    """Give `model_mesh_path` an IK Rig + a Manny->model IK Retargeter.

    Returns dict(ik_rig, retargeter, chains, mapped, auto). Fully headless.
    """
    out = out_dir or f'{PIPELINE_ROOT}/{model_name}'
    mesh = EAL.load_asset(model_mesh_path)
    assert mesh, f'mesh not found: {model_mesh_path}'

    # 1) IK Rig + auto-characterize chains -------------------------------
    ik_path = f'{out}/IK_{model_name}'
    if EAL.does_asset_exist(ik_path):
        EAL.delete_asset(ik_path)
    rig = unreal.IKRigDefinitionFactory().create_new_ik_rig_asset(out, f'IK_{model_name}')
    rc  = unreal.IKRigController.get_controller(rig)
    if not rc.set_skeletal_mesh(mesh):
        raise RuntimeError(f'{model_name}: skeleton incompatible with IK Rig')
    auto = rc.apply_auto_generated_retarget_definition()   # humanoid auto-characterize
    chains = _chain_names(rig)
    if not auto or not chains:
        # Non-standard skeleton: caller must supply chains via add_chains() below.
        unreal.log_warning(f'{model_name}: auto-characterize FAILED ({len(chains)} chains) '
                           f'- skeleton not recognized; add chains manually.')

    # 2) Retargeter = duplicate template + swap target rig ----------------
    rtg_path = f'{out}/RTG_Manny_to_{model_name}'
    if EAL.does_asset_exist(rtg_path):
        EAL.delete_asset(rtg_path)
    rtg = AT.duplicate_asset(f'RTG_Manny_to_{model_name}', out, EAL.load_asset(TEMPLATE_RTG))
    rtg.set_editor_property('target_ik_rig_asset', rig)
    trc = unreal.IKRetargeterController.get_controller(rtg)

    # mapping carries over by name; map any extra chains the template lacked
    src_chains = set(_chain_names(trc.get_ik_rig(SRC)))
    for c in chains:
        if str(trc.get_source_chain(c)) in (None, 'None', '') and c in src_chains:
            trc.set_source_chain(c, c)
    mapped = sum(1 for c in chains if str(trc.get_source_chain(c)) not in (None, 'None', ''))

    # 3) align the rest pose to the model's proportions, save -------------
    if align:
        trc.auto_align_all_bones(TGT, unreal.RetargetAutoAlignMethod.CHAIN_TO_CHAIN)
    EAL.save_asset(ik_path); EAL.save_asset(rtg_path)
    unreal.log(f'{model_name}: IK Rig + Retargeter ready ({mapped}/{len(chains)} chains mapped).')
    return dict(ik_rig=ik_path, retargeter=rtg_path, chains=chains, mapped=mapped, auto=auto)


def bake_onto_model(model_name, anim_paths, out_subdir='Anims', out_dir=None,
                    source_mesh=HUB_SOURCE_MESH):
    """Batch-retarget a list of Manny animations onto an onboarded model.

    Use for hero models / curated sets. For MANY models prefer runtime retarget
    (one shared Manny AnimBP + the per-model retargeter) — see README.
    """
    out = out_dir or f'{PIPELINE_ROOT}/{model_name}'
    rtg = EAL.load_asset(f'{out}/RTG_Manny_to_{model_name}')
    src = EAL.load_asset(source_mesh)
    tgt = unreal.IKRigController.get_controller(EAL.load_asset(f'{out}/IK_{model_name}')).get_skeletal_mesh()
    ar  = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = []
    for p in anim_paths:
        ad = ar.get_asset_by_object_path(p)
        if ad and ad.is_valid():
            assets.append(ad)
    op = unreal.IKRetargetBatchOperation()
    new = op.duplicate_and_retarget(assets, src, tgt, rtg, prefix=f'{model_name}_',
                                    include_referenced_assets=False, overwrite_existing_files=True)
    unreal.log(f'{model_name}: baked {len(new)} clips.')
    return [str(a.get_full_name()) for a in new]


def library_clips(folder=ANIM_LIBRARY, classes=('AnimSequence',)):
    """All animation object-paths under a library folder (default: the whole hub)."""
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    return [str(a.get_object_path_string()) if hasattr(a, 'get_object_path_string')
            else f'{a.package_name}.{a.asset_name}'
            for a in ar.get_assets_by_path(folder, recursive=True)
            if str(a.asset_class_path.asset_name) in classes]


# ===========================================================================
#  STANDARD CHARACTER SET — bake a full locomotion+aim+jump+crouch kit
# ===========================================================================
DIRS      = ('Fwd', 'Bwd', 'Left', 'Right')
_DIR_DEG  = {'Fwd': 0.0, 'Right': 90.0, 'Bwd': 180.0, 'Left': -90.0}
_AO_YAW   = {'C': 0.0, 'L': -90.0, 'R': 90.0, 'LB': -180.0, 'RB': 180.0}
_AO_PITCH = {'C': 0.0, 'D': -90.0, 'U': 90.0}
_AO_SUFFIX = [h + v for h in _AO_YAW for v in _AO_PITCH]   # CC,CD,CU,LC,...,RBU (15)

# Per-stance clip families (Lyra naming on Manny). Rifle is the full default.
# Pistol/Unarmed share the shape: swap 'Rifle'->'Pistol'/'Unarmed', and the aim
# offsets differ (Pistol: ADS only; Unarmed: Idle_Ready instead of Hipfire/ADS).
STANCE = {
    'Rifle': dict(
        locomotion   = ['MF_Rifle_Walk', 'MF_Rifle_Jog'],   # -> 8-dir blendspaces
        crouch_walk  = 'MM_Rifle_Crouch_Walk',
        sequences    = ['MM_Rifle_Idle_Hipfire', 'MM_Rifle_Crouch_Idle',
                        'MM_Rifle_Crouch_Entry', 'MM_Rifle_Crouch_Exit',
                        'MM_Rifle_Jump_Start', 'MM_Rifle_Jump_Fall_Loop',
                        'MM_Rifle_Jump_Fall_Land'],
        aim_offsets  = {'Hipfire': 'MM_Rifle_Idle_Hipfire', 'ADS': 'MM_Rifle_Idle_ADS'},
    ),
    'Pistol': dict(
        locomotion   = ['MF_Pistol_Walk', 'MF_Pistol_Jog'],
        crouch_walk  = None,
        sequences    = ['MM_Pistol_Jump_Start', 'MM_Pistol_Jump_Fall_Loop', 'MM_Pistol_Jump_Fall_Land'],
        aim_offsets  = {'ADS': 'MM_Pistol_Idle_ADS'},
    ),
    'Unarmed': dict(
        locomotion   = ['MF_Unarmed_Walk', 'MF_Unarmed_Jog'],
        crouch_walk  = 'MM_Unarmed_Crouch_Walk',
        sequences    = ['MM_Unarmed_Idle_Ready', 'M_Neutral_Stand_Idle_Loop',
                        'MM_Unarmed_Crouch_Idle', 'MM_Unarmed_Crouch_Entry', 'MM_Unarmed_Crouch_Exit',
                        'MM_Unarmed_Jump_Start', 'MM_Unarmed_Jump_Fall_Loop', 'MM_Unarmed_Jump_Fall_Land'],
        aim_offsets  = {'Ready': 'MM_Unarmed_Idle_Ready'},   # look-around offset (no weapon)
    ),
}


def _axis(name, lo, hi, grid, wrap=False):
    b = unreal.BlendParameter()
    b.set_editor_property('display_name', name); b.set_editor_property('min', float(lo))
    b.set_editor_property('max', float(hi));     b.set_editor_property('grid_num', grid)
    b.set_editor_property('wrap_input', wrap)
    return b


def _bake_clips(model_name, clip_names, out_dir=None, source_mesh=HUB_SOURCE_MESH):
    """Retarget Manny clips (by name, found anywhere under ANIM_LIBRARY) onto a
    model, prefixed `<Model>_`, into <out>/Anims/. Returns the Anims dir path."""
    out  = out_dir or f'{PIPELINE_ROOT}/{model_name}'
    ar   = unreal.AssetRegistryHelpers.get_asset_registry()
    rtg  = EAL.load_asset(f'{out}/RTG_Manny_to_{model_name}')
    src  = EAL.load_asset(source_mesh)
    tgt  = unreal.IKRigController.get_controller(EAL.load_asset(f'{out}/IK_{model_name}')).get_skeletal_mesh()
    want = set(clip_names)
    assets = [a for a in ar.get_assets_by_path(ANIM_LIBRARY, recursive=True) if str(a.asset_name) in want]
    pfx  = f'{model_name}_'
    unreal.IKRetargetBatchOperation().duplicate_and_retarget(
        assets, src, tgt, rtg, prefix=pfx, include_referenced_assets=False, overwrite_existing_files=True)
    dest = f'{out}/Anims/'
    for ad in ar.get_assets_by_path('/Game', recursive=False):
        n = str(ad.asset_name)
        if n.startswith(pfx):
            d = dest + n
            if EAL.does_asset_exist(d): EAL.delete_asset(d)
            EAL.rename_asset('/Game/' + n, d)
    return dest


def _model_skeleton(model_name, out_dir=None):
    out = out_dir or f'{PIPELINE_ROOT}/{model_name}'
    return unreal.IKRigController.get_controller(
        EAL.load_asset(f'{out}/IK_{model_name}')).get_skeletal_mesh().get_editor_property('skeleton')


def build_aim_offset(model_name, ao_name, pose_prefix, out_dir=None):
    """2D Yaw x Pitch AimOffset from the 15 baked `<Model>_<pose_prefix>_AO_*` poses."""
    out = out_dir or f'{PIPELINE_ROOT}/{model_name}'; sk = _model_skeleton(model_name, out)
    if EAL.does_asset_exist(f'{out}/{ao_name}'): EAL.delete_asset(f'{out}/{ao_name}')
    f = unreal.AimOffsetBlendSpaceFactoryNew(); f.set_editor_property('target_skeleton', sk)
    ao = AT.create_asset(ao_name, out, unreal.AimOffsetBlendSpace, f)
    ao.set_editor_property('blend_parameters', [_axis('Yaw', -180, 180, 4), _axis('Pitch', -90, 90, 2), unreal.BlendParameter()])
    samples = []
    for suf in _AO_SUFFIX:
        h, v = suf[:-1], suf[-1]
        a = EAL.load_asset(f'{out}/Anims/{model_name}_{pose_prefix}_AO_{suf}')
        if a:
            bs = unreal.BlendSample(); bs.set_editor_property('animation', a)
            bs.set_editor_property('sample_value', unreal.Vector(_AO_YAW[h], _AO_PITCH[v], 0)); samples.append(bs)
    ao.set_editor_property('sample_data', samples); EAL.save_asset(f'{out}/{ao_name}')
    return len(samples)


def build_dir_blendspace(model_name, bs_name, clip_family, out_dir=None):
    """1D Direction (wrap) blendspace from baked `<Model>_<clip_family>_<dir>` clips (8-way)."""
    out = out_dir or f'{PIPELINE_ROOT}/{model_name}'; sk = _model_skeleton(model_name, out)
    if EAL.does_asset_exist(f'{out}/{bs_name}'): EAL.delete_asset(f'{out}/{bs_name}')
    f = unreal.BlendSpaceFactory1D(); f.set_editor_property('target_skeleton', sk)
    bs = AT.create_asset(bs_name, out, unreal.BlendSpace1D, f)
    bs.set_editor_property('blend_parameters', [_axis('Direction', -180, 180, 8, wrap=True), unreal.BlendParameter(), unreal.BlendParameter()])
    samples = []
    for d, deg in _DIR_DEG.items():
        a = EAL.load_asset(f'{out}/Anims/{model_name}_{clip_family}_{d}')
        if not a:
            continue
        s = unreal.BlendSample(); s.set_editor_property('animation', a); s.set_editor_property('sample_value', unreal.Vector(deg, 0, 0)); samples.append(s)
        if d == 'Bwd':   # wrap seam: duplicate at -180
            w = unreal.BlendSample(); w.set_editor_property('animation', a); w.set_editor_property('sample_value', unreal.Vector(-180.0, 0, 0)); samples.append(w)
    bs.set_editor_property('sample_data', samples); EAL.save_asset(f'{out}/{bs_name}')
    return len(samples)


def bake_standard_set(model_name, stance='Rifle', out_dir=None):
    """ALL animations for a model: bake locomotion + jump + crouch + aim source
    clips, then build the 8-dir Walk/Jog/CrouchWalk blendspaces and the aim
    offsets. Run AFTER onboard_model(). Returns a manifest of created assets."""
    cfg = STANCE[stance]
    out = out_dir or f'{PIPELINE_ROOT}/{model_name}'
    # 1) collect every source clip name we need
    clips = []
    for fam in cfg['locomotion']:
        clips += [f'{fam}_{d}' for d in DIRS]
    if cfg['crouch_walk']:
        clips += [f"{cfg['crouch_walk']}_{d}" for d in DIRS]
    clips += list(cfg['sequences'])
    for pose_prefix in cfg['aim_offsets'].values():
        clips += [pose_prefix] + [f'{pose_prefix}_AO_{s}' for s in _AO_SUFFIX]
    # 2) bake them all through the model's retargeter
    _bake_clips(model_name, clips, out)
    # 3) build the blendspaces + aim offsets
    # blendspaces are tagged by stance so e.g. Rifle + Unarmed coexist on one model
    made = {'blendspaces': {}, 'aim_offsets': {}}
    for fam in cfg['locomotion']:
        kind = 'Walk' if 'Walk' in fam else ('Jog' if 'Jog' in fam else fam.split('_')[-1])
        made['blendspaces'][kind] = build_dir_blendspace(model_name, f'BS_{model_name}_{stance}_{kind}', fam, out)
    if cfg['crouch_walk']:
        made['blendspaces']['CrouchWalk'] = build_dir_blendspace(model_name, f'BS_{model_name}_{stance}_CrouchWalk', cfg['crouch_walk'], out)
    for label, pose_prefix in cfg['aim_offsets'].items():
        made['aim_offsets'][label] = build_aim_offset(model_name, f'AO_{model_name}_{stance}_{label}', pose_prefix, out)
    unreal.log(f'{model_name}: standard {stance} set baked - '
               f'{sum(made["blendspaces"].values())} loco samples, '
               f'{sum(made["aim_offsets"].values())} aim samples.')
    return made


def animate_model(model_mesh_path, model_name, stance='Rifle', fbik=True, out_dir=None):
    """ONE call: onboard + full-body IK + the entire standard animation set."""
    r = onboard_model(model_mesh_path, model_name, out_dir)
    if fbik:
        ik = EAL.load_asset(r['ik_rig']); c = unreal.IKRigController.get_controller(ik)
        c.apply_auto_fbik(); EAL.save_asset(r['ik_rig'])
    r['standard_set'] = bake_standard_set(model_name, stance, out_dir)
    return r


if __name__ == '__main__':
    # One-shot: rig + IK + every animation for a model.
    print(animate_model('/Game/Mixamo/SoldierRifle/Rifle_Aiming_Idle-3', 'SoldierDemo'))
