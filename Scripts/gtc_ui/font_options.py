"""
font_options.py — set EVERY TextBlock in WB_Options_widget to the ammo HUD font
(DIN_Condensed_Bold_Font) for a consistent type style. Walks the whole tree via a known
child (Apply_button) -> top, since WidgetTree.root_widget is protected.

Run: py "/Users/ziwenxu/Desktop/Code/GTA6-unreal/GT-caliber_5.8/Scripts/gtc_ui/font_options.py"
Sentinels: GTC_FONT_*
"""
import traceback
import unreal

PKG = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Options_widget"
OBJ = PKG + ".WB_Options_widget:WidgetTree"
FONT = "/Game/UI/Fonts/DIN_Condensed_Bold_Font"

unreal.log("GTC_FONT_START")
try:
    font_asset = unreal.load_asset(FONT)
    if font_asset is None:
        raise Exception("font not found: " + FONT)

    anchor = unreal.load_object(None, OBJ + ".Apply_button")
    if anchor is None:
        raise Exception("Apply_button not found")
    node = anchor
    top = node
    while node is not None:
        top = node
        try:
            node = node.get_parent()
        except Exception:
            node = None
    unreal.log("GTC_FONT_TOP %s" % top.get_name())

    count = [0]

    def apply_font(w):
        if w is None:
            return
        if isinstance(w, unreal.TextBlock):
            try:
                f = w.get_editor_property("font")
                f.set_editor_property("font_object", font_asset)
                w.set_editor_property("font", f)
                count[0] += 1
                unreal.log("GTC_FONT_SET %s" % w.get_name())
            except Exception as fe:
                unreal.log("GTC_FONT_WARN %s %s" % (w.get_name(), repr(fe)))
        try:
            n = w.get_children_count()
        except Exception:
            n = 0
        for i in range(n):
            apply_font(w.get_child_at(i))

    apply_font(top)
    unreal.log("GTC_FONT_COUNT %d" % count[0])

    bp = unreal.load_asset(PKG)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.log("GTC_FONT_DONE")
except Exception as e:
    unreal.log_error("GTC_FONT_FAIL " + repr(e))
    unreal.log_error(traceback.format_exc())
