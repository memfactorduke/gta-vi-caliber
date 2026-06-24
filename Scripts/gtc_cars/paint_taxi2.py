"""BP_vehicle12_Car body is a baked M_Veh_FullTexture (yellow + baked dirt = the
blotches), not the CarPaint slot. Override its FullTexture slots with the solid
Yellow instance; log every slot's base material so we can see the breakdown.
Run: py "<abs path>"
"""
import unreal

PRE = "GTC_TAXI2: "


def log(m):
    unreal.log_warning(PRE + str(m))


def level_actors():
    try:
        return unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    except Exception:
        return unreal.EditorLevelLibrary.get_all_level_actors()


yellow = unreal.load_asset("/Game/GTCaliberAssets/Materials/CarPaint/MI_GTC_CarPaint_Yellow")

taxi = None
for a in level_actors():
    if a and a.get_actor_label() == "BP_vehicle12_Car":
        taxi = a
        break
log("taxi=%s" % (taxi.get_actor_label() if taxi else None))

fixed = 0
for c in taxi.get_components_by_class(unreal.StaticMeshComponent):
    for i in range(c.get_num_materials()):
        m = c.get_material(i)
        if not m:
            continue
        try:
            bp = m.get_base_material().get_path_name()
        except Exception:
            bp = ""
        short = bp.split(".")[-1]
        if "FullTexture" in bp:
            log("OVERRIDE %s[%d] base=%s" % (c.get_name(), i, short))
            c.set_material(i, yellow)
            fixed += 1
        else:
            log("keep     %s[%d] base=%s" % (c.get_name(), i, short))

taxi.modify()
unreal.EditorLoadingAndSavingUtils.save_packages([taxi.get_outermost()], False)
log("fixed=%d" % fixed)
log("=== TAXI2 DONE ===")
