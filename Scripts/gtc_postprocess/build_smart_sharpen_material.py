# Build M_GTC_SmartSharpen — the post-process material that drives AGTCSmartSharpen.
#
# WHY A SCRIPT (and not live MCP authoring): creating a Post-Process-domain
# material references the engine DefaultPostProcessMaterial, and doing that from
# a Python tool while the editor is busy with other package loads can deadlock
# the game thread (the editor's own startup log warns about recursive-load
# deadlocks on DefaultPostProcessMaterial). Compiling the real sharpen shader is
# also a multi-second, game-thread-blocking step. So author this in a DEDICATED
# editor session where nothing else is racing:
#
#   1. Open the GTC editor (the normal Launch-GTC-with-MCP.command).
#   2. Tools > Execute Python Script... -> pick this file
#      (or paste into the editor's Python console; press Enter twice).
#   3. Wait for "OK: M_GTC_SmartSharpen built + saved" — shader compile may take
#      a minute. Do not run other heavy ops meanwhile.
#
# The math here is the HLSL twin of FSmartSharpen (Source/GTC_UE5/Camera/
# SmartSharpen.h) — distance falloff, shadow-safe luminance gate, foliage blend,
# resolution-independent kernel. AGTCSmartSharpen feeds the same constants into a
# dynamic instance of this material at runtime.

import unreal

PKG = "/Game/Materials/PostProcess"
NAME = "M_GTC_SmartSharpen"
FULL = f"{PKG}/{NAME}"

MEL = unreal.MaterialEditingLibrary

# SceneTextureId constants used inside the HLSL (verified against this engine).
ID_POST_PROCESS_INPUT0 = 14
ID_SCENE_DEPTH = 1
ID_CUSTOM_STENCIL = 25

HLSL = r"""
// Resolution-independent texel step: PixelRadius pixels over the viewport size.
float2 px = View.ViewSizeAndInvSize.zw * PixelRadius;

// 5-tap cross unsharp mask on the prior post-process output (PPI 14).
float3 c  = SceneTextureLookup(UV, 14, false).rgb;
float3 sl = SceneTextureLookup(UV + float2(-px.x, 0.0), 14, false).rgb;
float3 sr = SceneTextureLookup(UV + float2( px.x, 0.0), 14, false).rgb;
float3 stp= SceneTextureLookup(UV + float2(0.0, -px.y), 14, false).rgb;
float3 sb = SceneTextureLookup(UV + float2(0.0,  px.y), 14, false).rgb;
float3 blur = (sl + sr + stp + sb) * 0.25;

// Distance control: lerp Amount -> FarAmount across DistanceNear..DistanceFar (cm).
float depth = SceneTextureLookup(UV, 1, false).r;
float farA  = lerp(Amount, FarAmount, saturate((depth - DistanceNear) / max(1.0, DistanceFar - DistanceNear)));
float distAmt = lerp(Amount, farA, saturate(DistanceEnable));

// Foliage control: custom-stencil-tagged pixels use FoliageAmount.
float stencil = SceneTextureLookup(UV, 25, false).r;
float foliageMask = (FoliageEnable > 0.5 && stencil > 0.5) ? 1.0 : 0.0;
float amt = lerp(distAmt, FoliageAmount, foliageMask);

// Shadow safety: fade sharpening out in near-black to avoid lifting noise.
float luma = dot(c, float3(0.2126, 0.7152, 0.0722));
float gate = (ShadowFloor > 0.0) ? smoothstep(0.0, ShadowFloor, luma) : 1.0;
amt = clamp(amt * gate, 0.0, 5.0);

float3 sharp = c + (c - blur) * amt;
// Keep the scene-texture inputs referenced so their bindings are emitted.
sharp += 0.0 * (Scene0.rgb + Scene1.rgb + Scene2.rgb);
return max(sharp, 0.0);
"""

PARAMS = [
    ("Amount", 1.0), ("PixelRadius", 1.0), ("ShadowFloor", 0.05),
    ("DistanceEnable", 0.0), ("FarAmount", 0.4), ("DistanceNear", 500.0), ("DistanceFar", 8000.0),
    ("FoliageEnable", 0.0), ("FoliageAmount", 0.5),
]

INPUT_NAMES = ["UV", "Amount", "PixelRadius", "ShadowFloor", "DistanceEnable", "FarAmount",
               "DistanceNear", "DistanceFar", "FoliageEnable", "FoliageAmount",
               "Scene0", "Scene1", "Scene2"]


def build():
    if unreal.EditorAssetLibrary.does_asset_exist(FULL):
        # Don't delete in-place (delete of an in-use material pops a modal and can
        # corrupt the package). Remove it manually in the Content Browser first.
        unreal.log_warning(f"{FULL} already exists — delete it in the Content Browser, then re-run.")
        return False

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = tools.create_asset(NAME, PKG, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)

    def scalar(name, default, x, y):
        n = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
        n.set_editor_property("parameter_name", name)
        n.set_editor_property("default_value", default)
        n.set_editor_property("group", "Smart Sharpen")
        return n

    pn = {}
    y = -400
    for nm, dv in PARAMS:
        pn[nm] = scalar(nm, dv, -800, y); y += 90

    uv = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -800, 480)

    def scenetex(tid, x, y):
        n = MEL.create_material_expression(mat, unreal.MaterialExpressionSceneTexture, x, y)
        n.set_editor_property("scene_texture_id", unreal.SceneTextureId.cast(tid))
        return n
    st_color = scenetex(ID_POST_PROCESS_INPUT0, -800, 560)
    st_depth = scenetex(ID_SCENE_DEPTH, -800, 640)
    st_sten = scenetex(ID_CUSTOM_STENCIL, -800, 720)

    custom = MEL.create_material_expression(mat, unreal.MaterialExpressionCustom, -300, 0)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    custom.set_editor_property("description", "GTC Smart Sharpen")
    ci = []
    for n in INPUT_NAMES:
        c = unreal.CustomInput()
        c.set_editor_property("input_name", n)
        ci.append(c)
    custom.set_editor_property("inputs", ci)
    custom.set_editor_property("code", HLSL)

    MEL.connect_material_expressions(uv, "", custom, "UV")
    for nm, _ in PARAMS:
        MEL.connect_material_expressions(pn[nm], "", custom, nm)
    MEL.connect_material_expressions(st_color, "Color", custom, "Scene0")
    MEL.connect_material_expressions(st_depth, "Color", custom, "Scene1")
    MEL.connect_material_expressions(st_sten, "Color", custom, "Scene2")
    MEL.connect_material_property(custom, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    MEL.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(FULL)
    unreal.log(f"OK: {NAME} built + saved at {FULL}")
    return True


if __name__ == "__main__":
    build()
