"""
options_redesign.py — Phase 1 of the WB_Options_widget redesign.
Finds the root canvas (robustly, since WidgetTree.root_widget is protected), DUMPS the child
layout (names + types + slot pos/size) for reference, then adds a dark-glass + cyan rounded
card (GTC_OptionsCard) behind the content so the settings panel matches the AAA pause menu.

Run: py "/Users/ziwenxu/Desktop/Code/GTA6-unreal/GT-caliber_5.8/Scripts/gtc_ui/options_redesign.py"
Sentinels: GTC_OPT_*
"""
import traceback
import unreal

PKG = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Options_widget"
OBJ = PKG + ".WB_Options_widget:WidgetTree"
CARD = "GTC_OptionsCard"

unreal.log("GTC_OPT_START")
try:
    tree = unreal.load_object(None, OBJ)
    if tree is None:
        raise Exception("no WidgetTree")

    # --- find the root canvas via a KNOWN child, walking up (root_widget is protected) ---
    anchor = unreal.load_object(None, OBJ + ".Apply_button")
    if anchor is None:
        raise Exception("could not load known child Apply_button")
    node = anchor
    top = node
    while node is not None:
        top = node
        try:
            node = node.get_parent()
        except Exception:
            node = None
    unreal.log("GTC_OPT_TOP %s (%s)" % (top.get_name(), type(top).__name__))

    # the topmost is expected to be the root CanvasPanel; otherwise search down for one
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
    unreal.log("GTC_OPT_CANVAS %s" % (canvas.get_name() if canvas else None))
    if canvas is None:
        raise Exception("no CanvasPanel found to host the card")

    # --- dump immediate children of the canvas for layout reference ---
    for i in range(canvas.get_children_count()):
        c = canvas.get_child_at(i)
        pos = sz = None
        try:
            s = c.get_editor_property("slot")
            pos = s.get_position()
            sz = s.get_size()
        except Exception:
            pass
        unreal.log("GTC_OPT_CHILD %s : %s pos=%s size=%s" %
                   (c.get_name(), type(c).__name__,
                    ("%.0f,%.0f" % (pos.x, pos.y)) if pos else "?",
                    ("%.0f,%.0f" % (sz.x, sz.y)) if sz else "?"))

    # --- find or create the card, place it centered + behind everything ---
    card = None
    for i in range(canvas.get_children_count()):
        c = canvas.get_child_at(i)
        if c.get_name() == CARD:
            card = c
            break
    if card is None:
        card = unreal.new_object(unreal.Image, tree, CARD)
        canvas.add_child_to_canvas(card)
        unreal.log("GTC_OPT_CARD_NEW")
    else:
        unreal.log("GTC_OPT_CARD_FOUND")

    slot = card.get_editor_property("slot")
    a = unreal.Anchors()
    a.set_editor_property("minimum", unreal.Vector2D(0.5, 0.5))
    a.set_editor_property("maximum", unreal.Vector2D(0.5, 0.5))
    slot.set_anchors(a)
    slot.set_alignment(unreal.Vector2D(0.5, 0.5))
    slot.set_size(unreal.Vector2D(660.0, 720.0))
    slot.set_position(unreal.Vector2D(0.0, 0.0))
    slot.set_z_order(-10)

    bp = unreal.load_asset(PKG)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.log("GTC_OPT_DONE")
except Exception as e:
    unreal.log_error("GTC_OPT_FAIL " + repr(e))
    unreal.log_error(traceback.format_exc())
