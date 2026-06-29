"""
font_pauseui.py — unify type across the pause experience: set EVERY TextBlock in the pause
menu and the confirmation dialog to the ammo HUD font (DIN_Condensed_Bold_Font). Preserves
each text's size/spacing/color (only swaps font_object). Walks each tree via a known child
(root_widget is protected).

Run: py "/Users/ziwenxu/Desktop/Code/GTA6-unreal/GT-caliber_5.8/Scripts/gtc_ui/font_pauseui.py"
Sentinels: GTC_PFONT_*
"""
import traceback
import unreal

FONT = "/Game/UI/Fonts/DIN_Condensed_Bold_Font"
TARGETS = [
    ("/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Pause_menu_widget",
     "WB_Pause_menu_widget", "Resume_button"),
    ("/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Confirmation_Widget",
     "WB_Confirmation_Widget", "Yes_button"),
]

unreal.log("GTC_PFONT_START")
try:
    font_asset = unreal.load_asset(FONT)
    if font_asset is None:
        raise Exception("font not found: " + FONT)

    for pkg, cls, anchor_name in TARGETS:
        obj = pkg + "." + cls + ":WidgetTree"
        anchor = unreal.load_object(None, obj + "." + anchor_name)
        if anchor is None:
            unreal.log("GTC_PFONT_SKIP %s (no %s)" % (cls, anchor_name))
            continue
        node = anchor
        top = node
        while node is not None:
            top = node
            try:
                node = node.get_parent()
            except Exception:
                node = None

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
                except Exception as fe:
                    unreal.log("GTC_PFONT_WARN %s/%s %s" % (cls, w.get_name(), repr(fe)))
            try:
                n = w.get_children_count()
            except Exception:
                n = 0
            for i in range(n):
                apply_font(w.get_child_at(i))

        apply_font(top)
        bp = unreal.load_asset(pkg)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        unreal.log("GTC_PFONT_OK %s count=%d" % (cls, count[0]))

    unreal.log("GTC_PFONT_DONE")
except Exception as e:
    unreal.log_error("GTC_PFONT_FAIL " + repr(e))
    unreal.log_error(traceback.format_exc())
