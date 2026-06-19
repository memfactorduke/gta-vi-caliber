"""Rig the wooden character mesh to the UE5 Mannequin skeleton with automatic
weights, so it animates with the project's existing ABP_Unarmed and can become the
shared body for the player + the whole NPC crowd via DA_PlayerAppearance.

Run headless (NOT inside the editor):
    blender --background --python rig_wooden_to_mannequin.py -- <mannequin.fbx> <wooden.glb> <out.fbx>

INPUTS:
  mannequin.fbx : SK_Mannequin exported from UE (right-click the asset ->
                  Asset Actions -> Export). Carries the exact skeleton + bone names.
  wooden.glb    : the provided wooden character mesh.
OUTPUT:
  out.fbx       : wooden mesh skinned to the mannequin skeleton, ready to import
                  back into UE *onto the existing SK_Mannequin skeleton*.

The wooden mesh is auto-scaled to the skeleton's height and dropped so its feet sit
at the skeleton's base, then bound with automatic (heat) weights; if heat weighting
fails on the rough geometry it falls back to envelope weights so it still binds.
"""

import bpy
import sys
import mathutils


def args():
    a = sys.argv
    return a[a.index("--") + 1:]


def bbox(o):
    cs = [o.matrix_world @ mathutils.Vector(c) for c in o.bound_box]
    mn = mathutils.Vector((min(c.x for c in cs), min(c.y for c in cs), min(c.z for c in cs)))
    mx = mathutils.Vector((max(c.x for c in cs), max(c.y for c in cs), max(c.z for c in cs)))
    return mn, mx


def main():
    MANN, GLB, OUT = args()[:3]

    bpy.ops.wm.read_factory_settings(use_empty=True)

    # 1) mannequin: keep its armature, discard its reference mesh
    bpy.ops.import_scene.fbx(filepath=MANN)
    arm = next((o for o in bpy.context.scene.objects if o.type == 'ARMATURE'), None)
    if arm is None:
        print("ERROR: no armature in mannequin FBX"); return
    for m in [o for o in bpy.context.scene.objects if o.type == 'MESH']:
        bpy.data.objects.remove(m, do_unlink=True)

    # 2) wooden mesh
    bpy.ops.import_scene.gltf(filepath=GLB)
    wood = next((o for o in bpy.context.scene.objects if o.type == 'MESH'), None)
    if wood is None:
        print("ERROR: no mesh in wooden GLB"); return

    # 3) scale wooden to skeleton height, feet on the deck, centred under the root
    zs = []
    for b in arm.data.bones:
        zs.append((arm.matrix_world @ b.head_local).z)
        zs.append((arm.matrix_world @ b.tail_local).z)
    arm_h = max(zs) - min(zs)
    arm_floor = min(zs)

    wmn, wmx = bbox(wood)
    wood_h = wmx.z - wmn.z
    if wood_h > 1e-6:
        s = arm_h / wood_h
        wood.scale = (s, s, s)
    bpy.context.view_layer.update()

    wmn, wmx = bbox(wood)
    wood.location.x -= (wmn.x + wmx.x) / 2.0
    wood.location.y -= (wmn.y + wmx.y) / 2.0
    wood.location.z += (arm_floor - wmn.z)
    bpy.context.view_layer.update()
    # bake transform into the mesh so weights/export are clean
    bpy.ops.object.select_all(action='DESELECT')
    wood.select_set(True)
    bpy.context.view_layer.objects.active = wood
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

    # 4) bind: automatic (heat) weights, envelope fallback for rough meshes
    bpy.ops.object.select_all(action='DESELECT')
    wood.select_set(True)
    arm.select_set(True)
    bpy.context.view_layer.objects.active = arm
    try:
        bpy.ops.object.parent_set(type='ARMATURE_AUTO')
        print("bound: automatic weights")
    except RuntimeError as e:
        print("heat weighting failed (%s); falling back to envelope" % e)
        bpy.ops.object.select_all(action='DESELECT')
        wood.select_set(True); arm.select_set(True)
        bpy.context.view_layer.objects.active = arm
        bpy.ops.object.parent_set(type='ARMATURE_ENVELOPE')

    # 5) export armature + skinned mesh for UE
    bpy.ops.object.select_all(action='DESELECT')
    arm.select_set(True); wood.select_set(True)
    bpy.ops.export_scene.fbx(
        filepath=OUT, use_selection=True, add_leaf_bones=False,
        mesh_smooth_type='FACE', primary_bone_axis='Y', secondary_bone_axis='X',
        apply_unit_scale=True, bake_space_transform=False,
        object_types={'ARMATURE', 'MESH'})
    print("RIG DONE ->", OUT)


main()
