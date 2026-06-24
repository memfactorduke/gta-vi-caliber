"""Fix the one untouched car: BP_vehicle12_Car (yellow wagon) still uses the
original metallic CitySample car-paint, which reflects the dark surroundings as
blotches. Give it a solid glossy yellow via a child of M_GTC_CarPaintSimple,
matching the other 7. Run: py "<abs path>"
"""
import unreal

PRE = "GTC_TAXI: "


def log(m):
    unreal.log_warning(PRE + str(m))


def level_actors():
    try:
        return unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    except Exception:
        return unreal.EditorLevelLibrary.get_all_level_actors()


mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
master = unreal.load_asset("/Game/GTCaliberAssets/Materials/M_GTC_CarPaintSimple")

ypath = "/Game/GTCaliberAssets/Materials/CarPaint/MI_GTC_CarPaint_Yellow"
mic = unreal.load_asset(ypath)
if not mic:
    mic = tools.create_asset("MI_GTC_CarPaint_Yellow",
                             "/Game/GTCaliberAssets/Materials/CarPaint",
                             unreal.MaterialInstanceConstant,
                             unreal.MaterialInstanceConstantFactoryNew())
mel.set_material_instance_parent(mic, master)
mel.set_material_instance_vector_parameter_value(mic, "BaseColor", unreal.LinearColor(0.85, 0.58, 0.02, 1.0))
mel.set_material_instance_scalar_parameter_value(mic, "Roughness", 0.0)
mel.set_material_instance_scalar_parameter_value(mic, "Metallic", 0.0)

taxi = None
for a in level_actors():
    if a and a.get_actor_label() == "BP_vehicle12_Car":
        taxi = a
        break
log("taxi found=%s" % (taxi.get_actor_label() if taxi else None))

fixed = 0
if taxi:
    for c in taxi.get_components_by_class(unreal.StaticMeshComponent):
        for i in range(c.get_num_materials()):
            m = c.get_material(i)
            if not m:
                continue
            try:
                bp = m.get_base_material().get_path_name()
            except Exception:
                bp = ""
            if "CarPaint" in bp:
                log("  override %s[%d] was %s" % (c.get_name(), i, m.get_path_name()))
                c.set_material(i, mic)
                fixed += 1
    taxi.modify()
    ok = unreal.EditorLoadingAndSavingUtils.save_packages([mic.get_outermost(), taxi.get_outermost()], False)
    log("save -> %s" % ok)
log("taxi fixed slots=%d" % fixed)
log("=== TAXI DONE ===")
