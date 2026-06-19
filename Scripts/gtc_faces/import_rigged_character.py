"""Import a Mixamo-rigged character FBX (skeletal mesh + animations) into the
project, then optionally add the body to the player/NPC wardrobe.

USE THIS AFTER you have auto-rigged the wooden model on Mixamo and downloaded it.
See the docstring of create_player_appearance.py and the chat for the Mixamo steps.

WHAT TO DOWNLOAD FROM MIXAMO (so this script has everything it needs):
  1. With the character selected (after Auto-Rig), pick the "T-Pose" animation,
     Download -> Format: FBX Binary, Skin: WITH SKIN, 30fps, No keyframe reduction.
     Save it as:  ~/Downloads/wooden_character_rigged.fbx
  2. (optional) Add the "Walking" + "Idle" animations (In Place), download each as
     FBX, Skin: WITHOUT SKIN, and save as wooden_walk.fbx / wooden_idle.fbx.

CONFIG: set MAIN_FBX (the WITH-SKIN file) and ANIM_FBXS (without-skin anim files)
to the paths you actually saved, then run IN the already-running editor:
    py "<repo>/Scripts/gtc_faces/import_rigged_character.py"

CAVEAT — SKELETON: a Mixamo skeleton is NOT the UE5 Mannequin skeleton, so the
project's ABP_Unarmed will not drive this mesh directly. This script imports it on
its OWN skeleton and (if anims are provided) you can play those directly. To make it
the player/NPC body that walks with the shared locomotion, an IK Retargeter
(Mannequin -> Mixamo) is the follow-up step; ADD_TO_WARDROBE below just makes the
mesh selectable (it will stand in bind pose until retargeted).
"""

import os
import unreal

# ---- CONFIG ----------------------------------------------------------------
MAIN_FBX = os.path.expanduser("~/Downloads/wooden_character_rigged.fbx")  # WITH skin
ANIM_FBXS = [
    # os.path.expanduser("~/Downloads/wooden_walk.fbx"),
    # os.path.expanduser("~/Downloads/wooden_idle.fbx"),
]
DEST = "/Game/GTCaliberAssets/Content/Characters/Wooden"
UNIFORM_SCALE = 1.0          # bump to ~100 if the mesh imports tiny (m vs cm)
ADD_TO_WARDROBE = False      # True -> append the skeletal mesh to DA_PlayerAppearance BodyMeshes
WARDROBE = "/Game/GTCaliberAssets/Content/Characters/DA_PlayerAppearance"
# ----------------------------------------------------------------------------


def _make_task(filename, as_skeletal, with_anim, skeleton=None):
    task = unreal.AssetImportTask()
    task.filename = filename
    task.destination_path = DEST
    task.automated = True
    task.replace_existing = True
    task.save = True

    opts = unreal.FbxImportUI()
    opts.import_mesh = as_skeletal
    opts.import_as_skeletal = as_skeletal
    opts.import_animations = with_anim
    opts.create_physics_asset = as_skeletal
    if skeleton is not None:
        opts.skeleton = skeleton
        opts.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION
    opts.skeletal_mesh_import_data.set_editor_property("import_uniform_scale", float(UNIFORM_SCALE))
    opts.skeletal_mesh_import_data.set_editor_property("import_morph_targets", True)
    opts.skeletal_mesh_import_data.set_editor_property("convert_scene", True)
    task.options = opts
    return task


def main():
    if not os.path.exists(MAIN_FBX):
        unreal.log_error("MAIN_FBX not found: %s  -- rig on Mixamo, download WITH "
                         "SKIN, save to that path (or edit CONFIG)." % MAIN_FBX)
        return

    at = unreal.AssetToolsHelpers.get_asset_tools()

    # 1) skin + skeleton + (any embedded) anim
    main_task = _make_task(MAIN_FBX, as_skeletal=True, with_anim=True)
    at.import_asset_tasks([main_task])
    imported = list(main_task.get_objects_or_redirectors())
    unreal.log("Imported from skin FBX: %s" % [o.get_name() for o in imported])

    skel_mesh = next((o for o in imported if isinstance(o, unreal.SkeletalMesh)), None)
    skeleton = skel_mesh.skeleton if skel_mesh else None

    # 2) extra animation-only FBXs onto the same skeleton
    for af in ANIM_FBXS:
        if not os.path.exists(af):
            unreal.log_warning("anim FBX missing (skipped): %s" % af)
            continue
        if skeleton is None:
            unreal.log_warning("no skeleton from skin import; cannot import anim %s" % af)
            continue
        t = _make_task(af, as_skeletal=False, with_anim=True, skeleton=skeleton)
        at.import_asset_tasks([t])
        unreal.log("Imported anim: %s" % [o.get_name() for o in t.get_objects_or_redirectors()])

    # 3) optionally make it selectable in the creator / crowd wardrobe
    if ADD_TO_WARDROBE and skel_mesh is not None and unreal.EditorAssetLibrary.does_asset_exist(WARDROBE):
        da = unreal.EditorAssetLibrary.load_asset(WARDROBE)
        bodies = list(da.get_editor_property("body_meshes"))
        if skel_mesh not in bodies:
            bodies.append(skel_mesh)
            da.set_editor_property("body_meshes", bodies)
            unreal.EditorAssetLibrary.save_asset(WARDROBE)
            unreal.log("Added %s to DA_PlayerAppearance BodyMeshes (index %d). "
                       "NOTE: needs a Mannequin->Mixamo IK Retargeter to animate."
                       % (skel_mesh.get_name(), len(bodies) - 1))

    unreal.log("Done. Skeletal mesh under %s" % DEST)


main()
