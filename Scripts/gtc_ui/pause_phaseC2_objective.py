"""
pause_phaseC2_objective.py — final spec element: a GTA-style "CURRENT OBJECTIVE"
block in the lower-right void (right-justified to the margin, above the wordmark).
Three passive TextBlocks on Main_window_canvas. Idempotent. Compiles + saves.
Run: py "/Users/ziwenxu/Desktop/Code/GTA6-unreal/GT-caliber_5.8/Scripts/gtc_ui/pause_phaseC2_objective.py"
"""
import json, traceback
import unreal

PKG = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Pause_menu_widget"
OBJ = PKG + ".WB_Pause_menu_widget:WidgetTree."
OUT = "/private/tmp/claude-501/-Users-ziwenxu-Desktop-Code-GTA6-unreal-GT-caliber-5-8/d8d4bf46-eade-49bd-9282-7189b8262c91/scratchpad/pause_phaseC2.json"
FONT = "/Game/UI/Fonts/DIN_Condensed_Bold_Font.DIN_Condensed_Bold_Font"

res = {}
try:
    tree = unreal.load_object(None, OBJ.rstrip("."))
    mwc = unreal.load_object(None, OBJ + "Main_window_canvas")
    din = unreal.load_object(None, FONT)

    def ensure(name):
        for i in range(mwc.get_children_count()):
            ch = mwc.get_child_at(i)
            if ch and ch.get_name() == name:
                return ch
        w = unreal.new_object(unreal.TextBlock, tree, name)
        mwc.add_child_to_canvas(w)
        return w

    def text(name, txt, x, y, wd, ht, size, color, z):
        w = ensure(name)
        fi = unreal.SlateFontInfo()
        fi.set_editor_property("font_object", din)
        fi.set_editor_property("typeface_font_name", "Bold")
        fi.set_editor_property("size", size)
        w.set_editor_property("font", fi)
        w.set_editor_property("text", txt)
        w.set_editor_property("color_and_opacity", unreal.SlateColor(unreal.LinearColor(*color)))
        w.set_editor_property("justification", unreal.TextJustify.RIGHT)
        s = w.slot
        a = unreal.Anchors()
        a.set_editor_property("minimum", unreal.Vector2D(0.0, 0.0))
        a.set_editor_property("maximum", unreal.Vector2D(0.0, 0.0))
        s.set_anchors(a)
        s.set_alignment(unreal.Vector2D(0.0, 0.0))
        s.set_size(unreal.Vector2D(float(wd), float(ht)))
        s.set_position(unreal.Vector2D(float(x), float(y)))
        s.set_z_order(z)

    # right edge aligns at x = 760 + 424 = 1184 (the right margin)
    text("GTC_ObjEyebrow", "CURRENT OBJECTIVE", 760, 556, 424, 16, 11, (0.22, 0.86, 1.0, 0.45), 9)
    text("GTC_ObjLine", "Meet the Fixer at the Marina", 760, 574, 424, 28, 20, (0.93, 0.96, 0.98, 1.0), 9)
    text("GTC_ObjStats", "REWARD $25,000      1.2 MI", 760, 606, 424, 20, 14, (0.55, 0.62, 0.66, 1.0), 9)
    res["objective"] = "placed"

    bp = unreal.load_asset(PKG)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    res["_save"] = bool(unreal.EditorAssetLibrary.save_asset(PKG, False))
except Exception as e:
    res["_fatal"] = repr(e)
    res["_trace"] = traceback.format_exc()

with open(OUT, "w") as fh:
    json.dump(res, fh, indent=2)
unreal.log("GTC_PHASEC2_DONE " + json.dumps(res))
