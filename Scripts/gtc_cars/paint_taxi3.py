"""M_Veh_FullTexture is shared by body + wheels + brakes. Keep the body
(SM_Frame) yellow, but revert the wheel/brake components to their mesh-default
materials so they aren't yellow. Run: py "<abs path>"
"""
import unreal

PRE = "GTC_TAXI3: "


def log(m):
    unreal.log_warning(PRE + str(m))


def level_actors():
    try:
        return unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    except Exception:
        return unreal.EditorLevelLibrary.get_all_level_actors()


taxi = None
for a in level_actors():
    if a and a.get_actor_label() == "BP_vehicle12_Car":
        taxi = a
        break
log("taxi=%s" % (taxi.get_actor_label() if taxi else None))

reverted = 0
for c in taxi.get_components_by_class(unreal.StaticMeshComponent):
    n = c.get_name()
    if n.startswith("SM_Wheel") or n.startswith("SM_Brake"):
        sm = c.get_static_mesh()
        for i in range(c.get_num_materials()):
            d = sm.get_material(i) if sm else None
            c.set_material(i, d)
        reverted += 1
        log("reverted %s" % n)

taxi.modify()
unreal.EditorLoadingAndSavingUtils.save_packages([taxi.get_outermost()], False)
log("reverted=%d" % reverted)
log("=== TAXI3 DONE ===")
