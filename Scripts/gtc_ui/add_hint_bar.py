"""
add_hint_bar.py — add a GTA-style bottom hint bar (passive TextBlock) to the live UMG pause
menu WB_Pause_menu_widget. Passive widgets are allowed via the protected-WidgetTree bypass
(unlike clickable buttons -> GUID wall). Anchored bottom-stretch, centered, DIN ammo font,
cyan-dim. Idempotent (reuses GTC_HintBar if present).

Run: py "/Users/ziwenxu/Desktop/Code/GTA6-unreal/GT-caliber_5.8/Scripts/gtc_ui/add_hint_bar.py"
Sentinels: GTC_HINT_*
"""
import traceback
import unreal

PKG = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Pause_menu_widget"
OBJ = PKG + ".WB_Pause_menu_widget:WidgetTree"
NAME = "GTC_HintBar"
FONT = "/Game/UI/Fonts/DIN_Condensed_Bold_Font"

unreal.log("GTC_HINT_START")
try:
    tree = unreal.load_object(None, OBJ)
    if tree is None:
        raise Exception("no WidgetTree")

    # root canvas via a known child (root_widget is protected)
    anchor = unreal.load_object(None, OBJ + ".Resume_button")
    if anchor is None:
        raise Exception("Resume_button not found")
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
        raise Exception("no CanvasPanel host")
    unreal.log("GTC_HINT_CANVAS %s" % canvas.get_name())

    # find or create the hint TextBlock
    bar = None
    for i in range(canvas.get_children_count()):
        c = canvas.get_child_at(i)
        if c.get_name() == NAME:
            bar = c
            break
    if bar is None:
        bar = unreal.new_object(unreal.TextBlock, tree, NAME)
        canvas.add_child_to_canvas(bar)
        unreal.log("GTC_HINT_NEW")
    else:
        unreal.log("GTC_HINT_FOUND")

    bar.set_editor_property("text", unreal.Text.from_string(
        "ESC   RESUME          ENTER   SELECT          ↑ ↓   NAVIGATE"))
    bar.set_editor_property("justification", unreal.TextJustify.CENTER)

    # font: DIN ammo font, size 18, cyan-dim
    font_asset = unreal.load_asset(FONT)
    f = bar.get_editor_property("font")
    if font_asset:
        f.set_editor_property("font_object", font_asset)
    f.set_editor_property("size", 18)
    bar.set_editor_property("font", f)
    bar.set_editor_property("color_and_opacity", unreal.SlateColor(
        unreal.LinearColor(0.55, 0.78, 0.86, 0.85)))

    # bottom-stretch, centered, ~44px above the screen bottom
    slot = bar.get_editor_property("slot")
    a = unreal.Anchors()
    a.set_editor_property("minimum", unreal.Vector2D(0.0, 1.0))
    a.set_editor_property("maximum", unreal.Vector2D(1.0, 1.0))
    slot.set_anchors(a)
    slot.set_alignment(unreal.Vector2D(0.5, 1.0))
    slot.set_offsets(unreal.Margin(0.0, 0.0, 0.0, 44.0))  # L,T,R,B; B pushes it up from bottom
    slot.set_z_order(50)

    bp = unreal.load_asset(PKG)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.log("GTC_HINT_DONE")
except Exception as e:
    unreal.log_error("GTC_HINT_FAIL " + repr(e))
    unreal.log_error(traceback.format_exc())
