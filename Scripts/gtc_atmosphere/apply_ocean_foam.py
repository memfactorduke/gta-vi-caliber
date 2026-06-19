# Apply shoreline foam to the CityBeachStrip ocean material, then SAVE immediately.
#
# Run inside the live editor (e.g. via the vibeue execute_python_code MCP tool, or
# the editor's Python console). Idempotent: if the foam graph is already present it
# only re-applies the tuned MI_Ocean overrides and saves. Verified visually on
# 2026-06-19 (turquoise water with a natural foam band replacing the hard
# waterline) before an editor crash lost the unsaved in-memory edit — this script
# is the durable recipe; it saves both assets so a crash can't lose it again.
#
# What it builds on MM_OceanWater (master): a DepthFade-based shoreline mask
#   mask = (1 - DepthFade(FadeDistance=ShoreFoamWidth)) ^ 1.6 * ShoreFoamAmount
# lerping the existing BaseColor toward ShoreFoamColor. Three exposed params:
#   ShoreFoamWidth (scalar, world units of the soft band), ShoreFoamAmount
#   (scalar), ShoreFoamColor (vector). MI_Ocean overrides them to the tuned look.
import unreal

MASTER='/Game/GTCaliberAssets/Content/CityBeachStrip/Materials/Master/MM_OceanWater.MM_OceanWater'
INST  ='/Game/GTCaliberAssets/Content/CityBeachStrip/Materials/Water/MI_Ocean.MI_Ocean'
MEL=unreal.MaterialEditingLibrary
MP=unreal.MaterialProperty

mat=unreal.load_asset(MASTER)
have_foam = any(str(n)=='ShoreFoamAmount' for n in MEL.get_scalar_parameter_names(mat))

if not have_foam:
    bc_node=MEL.get_material_property_input_node(mat, MP.MP_BASE_COLOR)
    bc_out =MEL.get_material_property_input_node_output_name(mat, MP.MP_BASE_COLOR)
    def mk(cls,x,y): return MEL.create_material_expression(mat, cls, x, y)
    df  = mk(unreal.MaterialExpressionDepthFade, -1100, 900)
    fdst= mk(unreal.MaterialExpressionScalarParameter, -1350, 980); fdst.set_editor_property('parameter_name','ShoreFoamWidth'); fdst.set_editor_property('default_value',800.0)
    MEL.connect_material_expressions(fdst,'',df,'FadeDistance')
    one = mk(unreal.MaterialExpressionConstant, -1100, 760); one.set_editor_property('r',1.0)
    sub = mk(unreal.MaterialExpressionSubtract, -950, 860)
    MEL.connect_material_expressions(one,'',sub,'A'); MEL.connect_material_expressions(df,'',sub,'B')
    expc= mk(unreal.MaterialExpressionConstant, -950, 1000); expc.set_editor_property('r',1.6)
    pw  = mk(unreal.MaterialExpressionPower, -800, 880)
    MEL.connect_material_expressions(sub,'',pw,'Base'); MEL.connect_material_expressions(expc,'',pw,'Exponent')
    amt = mk(unreal.MaterialExpressionScalarParameter, -800, 1020); amt.set_editor_property('parameter_name','ShoreFoamAmount'); amt.set_editor_property('default_value',1.3)
    mask= mk(unreal.MaterialExpressionMultiply, -650, 900)
    MEL.connect_material_expressions(pw,'',mask,'A'); MEL.connect_material_expressions(amt,'',mask,'B')
    foam= mk(unreal.MaterialExpressionVectorParameter, -650, 1060); foam.set_editor_property('parameter_name','ShoreFoamColor'); foam.set_editor_property('default_value', unreal.LinearColor(0.95,0.97,1.0,1.0))
    lerp= mk(unreal.MaterialExpressionLinearInterpolate, -450, 880)
    MEL.connect_material_expressions(bc_node, bc_out, lerp, 'A')
    MEL.connect_material_expressions(foam,'',lerp,'B')
    MEL.connect_material_expressions(mask,'',lerp,'Alpha')
    MEL.connect_material_property(lerp,'',MP.MP_BASE_COLOR)
    MEL.recompile_material(mat)
    print('built foam graph on MM_OceanWater')
else:
    print('foam graph already present, skipping build')

# Tuned overrides on the instance the ocean planes use.
mi=unreal.load_asset(INST)
MEL.set_material_instance_scalar_parameter_value(mi,'ShoreFoamWidth',800.0)
MEL.set_material_instance_scalar_parameter_value(mi,'ShoreFoamAmount',1.3)
MEL.set_material_instance_vector_parameter_value(mi,'ShoreFoamColor',unreal.LinearColor(0.95,0.97,1.0,1.0))
MEL.update_material_instance(mi)

# SAVE immediately so a crash can't lose it.
unreal.EditorAssetLibrary.save_asset(MASTER, only_if_is_dirty=False)
unreal.EditorAssetLibrary.save_asset(INST,   only_if_is_dirty=False)
print('saved MM_OceanWater + MI_Ocean')
