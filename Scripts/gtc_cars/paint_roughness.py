"""Make the parked-car paint glossy (Roughness 0).

BasicShapeMaterial has no Roughness parameter, so build a tiny master
M_GTC_CarPaintSimple (BaseColor / Roughness / Metallic params), reparent the 7
MI_GTC_CarPaint_* instances to it, set Roughness=0, Metallic=0, and re-set each
BaseColor. The placed cars reference these instances, so they update with no
level edit. Run: py "<abs path>"
"""
import unreal

PRE = "GTC_ROUGH: "


def log(m):
    unreal.log_warning(PRE + str(m))


mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

MASTER_DIR = "/Game/GTCaliberAssets/Materials"
MASTER_NAME = "M_GTC_CarPaintSimple"
MASTER_PATH = MASTER_DIR + "/" + MASTER_NAME

COLORS = {
    "Red":    (0.55, 0.02, 0.02),
    "Blue":   (0.02, 0.06, 0.42),
    "White":  (0.78, 0.78, 0.80),
    "Black":  (0.018, 0.018, 0.022),
    "Silver": (0.38, 0.40, 0.43),
    "Green":  (0.02, 0.28, 0.10),
    "Orange": (0.72, 0.20, 0.02),
}


def ensure_master():
    m = unreal.load_asset(MASTER_PATH)
    if m:
        log("master exists")
        return m
    m = tools.create_asset(MASTER_NAME, MASTER_DIR, unreal.Material, unreal.MaterialFactoryNew())
    col = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -450, -120)
    col.set_editor_property("parameter_name", "BaseColor")
    col.set_editor_property("default_value", unreal.LinearColor(0.5, 0.5, 0.5, 1.0))
    mel.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -450, 80)
    rough.set_editor_property("parameter_name", "Roughness")
    rough.set_editor_property("default_value", 0.0)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

    metal = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -450, 220)
    metal.set_editor_property("parameter_name", "Metallic")
    metal.set_editor_property("default_value", 0.0)
    mel.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)

    mel.recompile_material(m)
    log("created master %s" % MASTER_PATH)
    return m


def main():
    master = ensure_master()
    pkgs = [master.get_outermost()]
    for name, (r, g, b) in COLORS.items():
        p = "/Game/GTCaliberAssets/Materials/CarPaint/MI_GTC_CarPaint_" + name
        mic = unreal.load_asset(p)
        if not mic:
            log("MISSING %s" % name)
            continue
        mel.set_material_instance_parent(mic, master)
        mel.set_material_instance_vector_parameter_value(mic, "BaseColor", unreal.LinearColor(r, g, b, 1.0))
        mel.set_material_instance_scalar_parameter_value(mic, "Roughness", 0.0)
        mel.set_material_instance_scalar_parameter_value(mic, "Metallic", 0.0)
        rr = mel.get_material_instance_scalar_parameter_value(mic, "Roughness")
        log("%-7s BaseColor set, Roughness readback=%s" % (name, rr))
        pkgs.append(mic.get_outermost())
    ok = unreal.EditorLoadingAndSavingUtils.save_packages(pkgs, False)
    log("save_packages(%d) -> %s" % (len(pkgs), ok))
    log("=== ROUGH DONE ===")


main()
