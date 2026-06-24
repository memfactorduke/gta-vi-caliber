"""GTC parked-car paint fix (durable).

Gives each hand-placed GTC_ParkedCar_* a real matte body color via saveable
MaterialInstanceConstant assets (children of the engine BasicShapeMaterial),
replacing the colorless CitySample car-paint that renders as chrome. Leaves the
default taxi (BP_vehicle12_Car) untouched. Saves only the assets/levels touched.

Run inside the live editor:  py "<abs path>"
"""
import unreal

PRE = "GTC_PAINT: "


def log(m):
    unreal.log_warning(PRE + str(m))


def level_actors():
    try:
        return unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    except Exception:
        return unreal.EditorLevelLibrary.get_all_level_actors()


# Varied matte car colors (linear RGB).
PALETTE = [
    ("Red",    (0.55, 0.02, 0.02)),
    ("Blue",   (0.02, 0.06, 0.42)),
    ("White",  (0.78, 0.78, 0.80)),
    ("Black",  (0.018, 0.018, 0.022)),
    ("Silver", (0.38, 0.40, 0.43)),
    ("Green",  (0.02, 0.28, 0.10)),
    ("Orange", (0.72, 0.20, 0.02)),
    ("Teal",   (0.02, 0.33, 0.36)),
    ("Maroon", (0.28, 0.03, 0.07)),
    ("Sand",   (0.55, 0.46, 0.28)),
]

DEST = "/Game/GTCaliberAssets/Materials/CarPaint"
base = unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial")
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def make_mic(cname, color):
    pkg = DEST + "/MI_GTC_CarPaint_" + cname
    mic = unreal.load_asset(pkg)
    if not mic:
        mic = tools.create_asset("MI_GTC_CarPaint_" + cname, DEST,
                                 unreal.MaterialInstanceConstant,
                                 unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mic, base)
    mel.set_material_instance_vector_parameter_value(
        mic, "Color", unreal.LinearColor(color[0], color[1], color[2], 1.0))
    return mic


def main():
    log("base=%s" % (base.get_path_name() if base else None))
    cars = [a for a in level_actors() if a and a.get_actor_label().startswith("GTC_ParkedCar_")]
    cars.sort(key=lambda a: a.get_actor_label())
    log("parked cars: %d" % len(cars))

    pkgs = set()
    for idx, a in enumerate(cars):
        cname, col = PALETTE[idx % len(PALETTE)]
        try:
            mic = make_mic(cname, col)
        except Exception as e:
            log("  MIC err for %s: %s" % (cname, e))
            continue
        pkgs.add(mic.get_outermost())
        slots = 0
        for c in a.get_components_by_class(unreal.StaticMeshComponent):
            for i in range(c.get_num_materials()):
                m = c.get_material(i)
                if not m:
                    continue
                try:
                    bp = m.get_base_material().get_path_name()
                except Exception:
                    bp = ""
                if "CarPaint" in bp:
                    c.set_material(i, mic)
                    slots += 1
        a.modify()
        pkgs.add(a.get_outermost())
        loc = a.get_actor_location()
        log("  %-26s -> %-7s slots=%d  loc=(%.0f,%.0f,%.0f)" % (
            a.get_actor_label(), cname, slots, loc.x, loc.y, loc.z))

    # Frame the camera on the first car for a verification screenshot.
    if cars:
        loc = cars[0].get_actor_location()
        cam = unreal.Vector(loc.x - 650.0, loc.y - 650.0, loc.z + 350.0)
        rot = unreal.MathLibrary.find_look_at_rotation(cam, loc)
        try:
            unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(cam, rot)
            unreal.get_editor_subsystem(unreal.EditorActorSubsystem).set_selected_level_actors([cars[0]])
        except Exception as e:
            log("cam err %s" % e)

    pkg_list = [p for p in pkgs if p]
    log("saving %d packages: %s" % (len(pkg_list), [p.get_name() for p in pkg_list]))
    try:
        ok = unreal.EditorLoadingAndSavingUtils.save_packages(pkg_list, False)
        log("save_packages -> %s" % ok)
    except Exception as e:
        log("save err: %s" % e)
    log("=== PAINT APPLY DONE ===")


main()
