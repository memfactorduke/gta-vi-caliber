"""
add_pause_header.py — add a GTA-style top-left header to the live pause menu:
GTC_PauseKicker ("PAUSED") above GTC_PauseTitle ("GT-CALIBER"). Passive TextBlocks (allowed
via the protected-WidgetTree bypass). Creates the widgets + sets their CanvasPanelSlot
geometry via the proven slot METHODS; text/font/color are applied afterwards over MCP.
Idempotent. Compiles + saves.

Run: py "/Users/ziwenxu/Desktop/Code/GTA6-unreal/GT-caliber_5.8/Scripts/gtc_ui/add_pause_header.py"
Sentinels: GTC_HDR_*
"""
import traceback
import unreal

PKG = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Pause_menu_widget"
OBJ = PKG + ".WB_Pause_menu_widget:WidgetTree"

unreal.log("GTC_HDR_START")
try:
    tree = unreal.load_object(None, OBJ)
    anchor = unreal.load_object(None, OBJ + ".Resume_button")
    if tree is None or anchor is None:
        raise Exception("tree/Resume_button missing")

    node = anchor
    top = node
    while node is not None:
        top = node
        try:
            node = node.get_parent()
        except Exception:
            node = None

    def find_canvas(w):
        if isinstance(w, unreal.CanvasPanel):
            return w
        try:
            n = w.get_children_count()
        except Exception:
            return None
        for i in range(n):
            r = find_canvas(w.get_child_at(i))
            if r:
                return r
        return None

    canvas = top if isinstance(top, unreal.CanvasPanel) else find_canvas(top)
    if canvas is None:
        raise Exception("no canvas")

    def ensure(name):
        for i in range(canvas.get_children_count()):
            c = canvas.get_child_at(i)
            if c.get_name() == name:
                return c, False
        w = unreal.new_object(unreal.TextBlock, tree, name)
        canvas.add_child_to_canvas(w)
        return w, True

    def topleft(slot, pos, size):
        a = unreal.Anchors()
        a.set_editor_property("minimum", unreal.Vector2D(0.0, 0.0))
        a.set_editor_property("maximum", unreal.Vector2D(0.0, 0.0))
        slot.set_anchors(a)
        slot.set_alignment(unreal.Vector2D(0.0, 0.0))
        slot.set_size(unreal.Vector2D(size[0], size[1]))
        slot.set_position(unreal.Vector2D(pos[0], pos[1]))
        slot.set_z_order(50)

    kicker, kn = ensure("GTC_PauseKicker")
    title, tn = ensure("GTC_PauseTitle")
    topleft(kicker.get_editor_property("slot"), (98.0, 44.0), (400.0, 22.0))
    topleft(title.get_editor_property("slot"), (96.0, 64.0), (600.0, 64.0))
    unreal.log("GTC_HDR_SET kicker_new=%s title_new=%s" % (kn, tn))

    bp = unreal.load_asset(PKG)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.EditorLoadingAndSavingUtils.save_packages([PKG], True)
    unreal.log("GTC_HDR_DONE")
except Exception as e:
    unreal.log_error("GTC_HDR_FAIL " + repr(e))
    unreal.log_error(traceback.format_exc())
