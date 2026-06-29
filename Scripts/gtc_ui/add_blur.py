"""
add_blur.py — add a frosted BackgroundBlur backdrop to the EXISTING kit pause menu.

WHY load_object: UE5.8 made UWidgetBlueprint.WidgetTree a protected property, so
get_editor_property("WidgetTree") / .widget_tree both fail. But the WidgetTree is a
named subobject of the asset, so load_object(None, "<pkg>.<class>:WidgetTree") fetches
it directly by object path — the same path MCP uses to edit CanvasPanelSlots.

Widgets are created with new_object(class, outer=tree) (UWidgetTree.ConstructWidget is a
C++ template, not Python-exposed). Slot geometry is set via the UCanvasPanelSlot setter
methods (set_anchors/set_offsets/set_z_order), not set_editor_property.

Run from the in-editor console:
  py "/Users/ziwenxu/Desktop/Code/GTA6-unreal/GT-caliber_5.8/Scripts/gtc_ui/add_blur.py"

Idempotent: re-running reuses the existing GTC_BackdropBlur and just re-positions it.
Sentinels: GTC_BLUR_START / GTC_BLUR_TREE / GTC_BLUR_FOUND|NEW / GTC_BLUR_POSITIONED / GTC_BLUR_DONE / GTC_BLUR_FAIL
"""
import traceback
import unreal

PKG = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Pause_menu_widget"
OBJ = PKG + ".WB_Pause_menu_widget:WidgetTree"

unreal.log("GTC_BLUR_START")
try:
    tree = unreal.load_object(None, OBJ)
    root = unreal.load_object(None, OBJ + ".Pause_menu_canvas")
    unreal.log("GTC_BLUR_TREE tree=%s root=%s" % (tree is not None, root is not None))
    if tree is None or root is None:
        raise Exception("could not load WidgetTree or root canvas by path")

    blur = None
    for i in range(root.get_children_count()):
        c = root.get_child_at(i)
        if isinstance(c, unreal.BackgroundBlur):
            blur = c
            break

    if blur is not None:
        unreal.log("GTC_BLUR_FOUND " + blur.get_name())
    else:
        blur = unreal.new_object(unreal.BackgroundBlur, tree, "GTC_BackdropBlur")
        root.add_child_to_canvas(blur)
        unreal.log("GTC_BLUR_NEW " + blur.get_name())

    blur.set_editor_property("blur_strength", 14.0)

    slot = blur.get_editor_property("slot")
    a = unreal.Anchors()
    a.set_editor_property("minimum", unreal.Vector2D(0.0, 0.0))
    a.set_editor_property("maximum", unreal.Vector2D(1.0, 1.0))
    slot.set_anchors(a)
    slot.set_offsets(unreal.Margin(0.0, 0.0, 0.0, 0.0))
    slot.set_z_order(-100)
    unreal.log("GTC_BLUR_POSITIONED " + blur.get_name())

    bp = unreal.load_asset(PKG)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    try:
        unreal.EditorAssetLibrary.save_asset(PKG)
    except Exception as se:
        unreal.log("GTC_BLUR_SAVEWARN " + repr(se))
    unreal.log("GTC_BLUR_DONE " + blur.get_name())
except Exception as e:
    unreal.log_error("GTC_BLUR_FAIL " + repr(e))
    unreal.log_error(traceback.format_exc())
