"""
add_minimap_frame.py — add a rounded bezel frame behind the HUD minimap (AAA radar look).
Uses the load_object + new_object technique (UE5.8 protected WidgetTree). Brush styling is
applied separately via MCP. Run: py "<this path>"
Sentinels: GTC_MMF_*
"""
import traceback
import unreal

PKG = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Game_HUD_widget"
OBJ = PKG + ".WB_Game_HUD_widget:WidgetTree"

unreal.log("GTC_MMF_START")
try:
    tree = unreal.load_object(None, OBJ)
    root = unreal.load_object(None, OBJ + ".Main canvas")
    unreal.log("GTC_MMF_LOAD tree=%s root=%s" % (tree is not None, root is not None))
    if tree is None or root is None:
        raise Exception("could not load WidgetTree / Main canvas")

    frame = None
    for i in range(root.get_children_count()):
        c = root.get_child_at(i)
        if c.get_name() == "GTC_MinimapFrame":
            frame = c
            break

    if frame is None:
        frame = unreal.new_object(unreal.Image, tree, "GTC_MinimapFrame")
        root.add_child_to_canvas(frame)
        unreal.log("GTC_MMF_NEW")
    else:
        unreal.log("GTC_MMF_FOUND")

    slot = frame.get_editor_property("slot")
    a = unreal.Anchors()
    a.set_editor_property("minimum", unreal.Vector2D(0.0, 1.0))
    a.set_editor_property("maximum", unreal.Vector2D(0.0, 1.0))
    slot.set_anchors(a)
    slot.set_alignment(unreal.Vector2D(0.0, 1.0))
    # point-anchor offsets are (X, Y, SizeX, SizeY); 12px bezel margin around the 280x200 map
    slot.set_offsets(unreal.Margin(12.0, -38.0, 304.0, 224.0))
    slot.set_z_order(-1)
    unreal.log("GTC_MMF_POSITIONED")

    bp = unreal.load_asset(PKG)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.log("GTC_MMF_DONE")
except Exception as e:
    unreal.log_error("GTC_MMF_FAIL " + repr(e))
    unreal.log_error(traceback.format_exc())
