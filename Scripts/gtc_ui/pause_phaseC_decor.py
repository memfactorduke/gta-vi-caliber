"""
pause_phaseC_decor.py — add the AAA "richness" layer to the pause menu (new passive
widgets): row numerals 01-06 in the column's left pad, a top-right FUNDS/TIME/DISTRICT
context cluster (GTA sense-of-place), and thin separator hairlines (header/title/footer).
All on Main_window_canvas (full-screen → screen coords). Idempotent (ensure-by-name).
Compiles + saves via EditorAssetLibrary (NOT save_packages). ASCII-only text (DIN lacks
glyphs like the middle dot / arrows).

Run: py "/Users/ziwenxu/Desktop/Code/GTA6-unreal/GT-caliber_5.8/Scripts/gtc_ui/pause_phaseC_decor.py"
"""
import json, traceback
import unreal

PKG = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Pause_menu_widget"
OBJ = PKG + ".WB_Pause_menu_widget:WidgetTree."
OUT = "/private/tmp/claude-501/-Users-ziwenxu-Desktop-Code-GTA6-unreal-GT-caliber-5-8/d8d4bf46-eade-49bd-9282-7189b8262c91/scratchpad/pause_phaseC.json"
FONT = "/Game/UI/Fonts/DIN_Condensed_Bold_Font.DIN_Condensed_Bold_Font"

res = {}
try:
    tree = unreal.load_object(None, OBJ.rstrip("."))
    mwc = unreal.load_object(None, OBJ + "Main_window_canvas")
    din = unreal.load_object(None, FONT)

    def lin(c):
        return unreal.LinearColor(c[0], c[1], c[2], c[3])

    def ensure(name, cls):
        for i in range(mwc.get_children_count()):
            ch = mwc.get_child_at(i)
            if ch and ch.get_name() == name:
                return ch, False
        w = unreal.new_object(cls, tree, name)
        mwc.add_child_to_canvas(w)
        return w, True

    def place(w, x, y, wd, ht, z):
        s = w.slot
        a = unreal.Anchors()
        a.set_editor_property("minimum", unreal.Vector2D(0.0, 0.0))
        a.set_editor_property("maximum", unreal.Vector2D(0.0, 0.0))
        s.set_anchors(a)
        s.set_alignment(unreal.Vector2D(0.0, 0.0))
        s.set_size(unreal.Vector2D(float(wd), float(ht)))
        s.set_position(unreal.Vector2D(float(x), float(y)))
        s.set_z_order(z)

    def text(name, txt, x, y, wd, ht, size, color, z):
        w, _new = ensure(name, unreal.TextBlock)
        fi = unreal.SlateFontInfo()
        fi.set_editor_property("font_object", din)
        fi.set_editor_property("typeface_font_name", "Bold")
        fi.set_editor_property("size", size)
        w.set_editor_property("font", fi)
        w.set_editor_property("text", txt)
        w.set_editor_property("color_and_opacity", unreal.SlateColor(lin(color)))
        place(w, x, y, wd, ht, z)
        return _new

    def line(name, x, y, wd, ht, color, z):
        w, _new = ensure(name, unreal.Image)
        br = w.get_editor_property("brush")
        br.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
        br.set_editor_property("tint_color", unreal.SlateColor(lin(color)))
        w.set_editor_property("brush", br)
        w.set_editor_property("color_and_opacity", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
        place(w, x, y, wd, ht, z)
        return _new

    ACCENT_DIM = (0.22, 0.86, 1.0, 0.45)
    TEXT_DIM = (0.55, 0.62, 0.66, 1.0)
    TEXT_BRIGHT = (0.93, 0.96, 0.98, 1.0)
    HAIR = (1.0, 1.0, 1.0, 0.10)

    # row numerals 01-06 in the left pad (button rows top y = 308..618, +18 to center)
    ys = [326, 388, 450, 512, 574, 636]
    for i, y in enumerate(ys):
        res["idx%02d" % (i + 1)] = text("GTC_Index%02d" % (i + 1), "%02d" % (i + 1), 112, y, 36, 24, 16, ACCENT_DIM, 8)

    # top-right context cluster (label over value), ASCII only
    res["distL"] = text("GTC_DistrictLabel", "DISTRICT", 690, 40, 260, 16, 11, TEXT_DIM, 9)
    res["distV"] = text("GTC_DistrictValue", "OCEAN DR / VICEPORT", 690, 56, 260, 28, 18, TEXT_BRIGHT, 9)
    res["timeL"] = text("GTC_TimeLabel", "TIME", 980, 40, 150, 16, 11, TEXT_DIM, 9)
    res["timeV"] = text("GTC_TimeValue", "13:42   DAY 03", 980, 56, 150, 28, 18, TEXT_BRIGHT, 9)
    res["fundL"] = text("GTC_FundsLabel", "FUNDS", 1140, 40, 150, 16, 11, TEXT_DIM, 9)
    res["fundV"] = text("GTC_FundsValue", "$48,250", 1140, 56, 150, 28, 22, TEXT_BRIGHT, 9)

    # hairlines
    res["hairHeader"] = line("GTC_HeaderHairline", 96, 92, 1088, 1, HAIR, 7)
    res["hairTitle"] = line("GTC_TitleHairline", 96, 292, 360, 1, HAIR, 7)
    res["hairFooter"] = line("GTC_FooterHairline", 96, 672, 1088, 1, HAIR, 7)

    bp = unreal.load_asset(PKG)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    saved = unreal.EditorAssetLibrary.save_asset(PKG, False)
    res["_save"] = bool(saved)
except Exception as e:
    res["_fatal"] = repr(e)
    res["_trace"] = traceback.format_exc()

with open(OUT, "w") as fh:
    json.dump(res, fh, indent=2)
unreal.log("GTC_PHASEC_DONE " + json.dumps(res))
