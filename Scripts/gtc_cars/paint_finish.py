"""Finish step: fix GTC_ParkedCar_MarlinGT (which kept a transient test material)
by assigning it the saved Green instance, then frame the camera on a car out in
the open street for a verification screenshot. Run: py "<abs path>"
"""
import unreal

PRE = "GTC_PAINT2: "


def log(m):
    unreal.log_warning(PRE + str(m))


def level_actors():
    try:
        return unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    except Exception:
        return unreal.EditorLevelLibrary.get_all_level_actors()


acts = {a.get_actor_label(): a for a in level_actors()
        if a and a.get_actor_label().startswith("GTC_ParkedCar_")}

green = unreal.load_asset("/Game/GTCaliberAssets/Materials/CarPaint/MI_GTC_CarPaint_Green")
m = acts.get("GTC_ParkedCar_MarlinGT")
fixed = 0
if m and green:
    for c in m.get_components_by_class(unreal.StaticMeshComponent):
        for i in range(c.get_num_materials()):
            mat = c.get_material(i)
            if not mat:
                continue
            try:
                bp = mat.get_base_material().get_path_name()
            except Exception:
                bp = ""
            if "BasicShapeMaterial" in bp:   # the leftover transient test override
                c.set_material(i, green)
                fixed += 1
    m.modify()
    unreal.EditorLoadingAndSavingUtils.save_packages([m.get_outermost()], False)
log("MarlinGT fixed slots=%d" % fixed)

# Frame a car out in the bright street (Surfline) for verification.
t = acts.get("GTC_ParkedCar_Surfline") or acts.get("GTC_ParkedCar_Esplanade") or m
if t:
    loc = t.get_actor_location()
    cam = unreal.Vector(loc.x + 420.0, loc.y + 420.0, loc.z + 300.0)
    rot = unreal.MathLibrary.find_look_at_rotation(cam, loc)
    unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(cam, rot)
    unreal.get_editor_subsystem(unreal.EditorActorSubsystem).set_selected_level_actors([t])
    log("framed %s at (%.0f,%.0f,%.0f)" % (t.get_actor_label(), loc.x, loc.y, loc.z))
log("=== DONE ===")
