"""
GT-caliber — author the bullet-mark decals the impact system stamps on hard surfaces.

Creates, under /Game/Surfaces/Decals:
  M_GTC_BulletDecal      a minimal deferred-decal MASTER (hole texture * tint -> BaseColor,
                         texture R -> Opacity). Authored from scratch so it depends on nothing
                         fragile (NOT the broken CitySample vehicle materials).
  MI_GTC_BulletHole      generic dark pit  (concrete/asphalt/brick/ceramic/wood/leather/paper/default)
  MI_GTC_BulletScorch    near-black metal scorch ring (metal)
  MI_GTC_GlassCrack      light radial crack (glass/ice) — uses T_Glass

These are the exact paths SurfaceImpact.cpp's ImpactEffectFor already points DecalPath at, so once
this runs the bursts start leaving marks with no further code change.

CRASH-SAFE: every asset here derives from a clean, self-authored master, so saving never triggers
the broken-vehicle-shader thumbnail assert. Still: run in the LIVE editor only (never spawn one).

RUN (in the live editor, via MCP/Python):
  exec(open(r"<repo>/Scripts/gtc_surfaces/apply_bullet_decals.py").read()); build()

After it runs, eyeball one decal in the editor — the look (tint/opacity) is a first pass meant to
be tuned. T_Impact is the hole source; swap to a sharper bullet-hole texture later if desired.
"""

import unreal

FOLDER = "/Game/Surfaces/Decals"
HOLE_TEX = "/Game/Realistic_Starter_VFX_Pack_Vol2/Textures/T_Impact"
CRACK_TEX = "/Game/Realistic_Starter_VFX_Pack_Vol2/Textures/T_Glass"

_tools = unreal.AssetToolsHelpers.get_asset_tools()
_mel = unreal.MaterialEditingLibrary


def _build_master():
    path = f"{FOLDER}/M_GTC_BulletDecal"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.EditorAssetLibrary.load_asset(path)
    mat = _tools.create_asset("M_GTC_BulletDecal", FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

    hole = unreal.EditorAssetLibrary.load_asset(HOLE_TEX)
    # TextureSampleParameter2D "MarkTex": the per-instance swappable mark art.
    tex = _mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -500, 0)
    tex.set_editor_property("parameter_name", "MarkTex")
    if hole is not None:
        tex.set_editor_property("texture", hole)
    # VectorParameter "Tint": per-instance colour of the mark.
    tint = _mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -500, 260)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(0.05, 0.05, 0.05, 1.0))
    # ScalarParameter "Opacity": global strength multiplier.
    strength = _mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -500, 420)
    strength.set_editor_property("parameter_name", "Opacity")
    strength.set_editor_property("default_value", 1.0)

    # BaseColor = Tint * MarkTex.rgb
    mul = _mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, 80)
    _mel.connect_material_expressions(tint, "", mul, "A")
    _mel.connect_material_expressions(tex, "RGB", mul, "B")
    _mel.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # Opacity = MarkTex.R * Opacity  (T_Impact is a grayscale mask — R carries the shape)
    omul = _mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, 400)
    _mel.connect_material_expressions(tex, "R", omul, "A")
    _mel.connect_material_expressions(strength, "", omul, "B")
    _mel.connect_material_property(omul, "", unreal.MaterialProperty.MP_OPACITY)

    _mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(path)
    unreal.log(f"[gtc-decals] authored master {path}")
    return mat


def _instance(name, master, tex_path, tint):
    path = f"{FOLDER}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        mi = unreal.EditorAssetLibrary.load_asset(path)
    else:
        mi = _tools.create_asset(name, FOLDER, unreal.MaterialInstanceConstant,
                                 unreal.MaterialInstanceConstantFactoryNew())
    _mel.set_material_instance_parent(mi, master)
    tex = unreal.EditorAssetLibrary.load_asset(tex_path)
    if tex is not None:
        _mel.set_material_instance_texture_parameter_value(mi, "MarkTex", tex)
    _mel.set_material_instance_vector_parameter_value(mi, "Tint", tint)
    unreal.EditorAssetLibrary.save_asset(path)
    unreal.log(f"[gtc-decals] {path}")
    return mi


def build():
    master = _build_master()
    _instance("MI_GTC_BulletHole", master, HOLE_TEX, unreal.LinearColor(0.04, 0.04, 0.04, 1.0))
    _instance("MI_GTC_BulletScorch", master, HOLE_TEX, unreal.LinearColor(0.015, 0.015, 0.015, 1.0))
    _instance("MI_GTC_GlassCrack", master, CRACK_TEX, unreal.LinearColor(0.7, 0.75, 0.82, 1.0))
    unreal.log("[gtc-decals] done — 1 master + 3 instances. Paths match SurfaceImpact.cpp DecalPath.")
    return True
