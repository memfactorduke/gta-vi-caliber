"""
pause_phaseF_settings.py — PHASE D slice 3: the SETTINGS tab content panel.

Builds GTC_Panel_Settings (CanvasPanel container, same right region, HIDDEN by default)
with a rounded frame, "SETTINGS" header, and a two-column option list. Values are
formatted as "< VALUE >" selectors to read as adjustable settings (real binding/
interaction needs graph/C++ later). Same VerticalBox auto-stack pattern as STATUS.
Idempotent. Compiles + saves.

Run:  py "/Users/ziwenxu/Desktop/Code/GTA6-unreal/GT-caliber_5.8/Scripts/gtc_ui/pause_phaseF_settings.py"
"""
import json, traceback
import unreal

PKG = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Pause_menu_widget"
OBJ = PKG + ".WB_Pause_menu_widget:WidgetTree."
OUT = "/private/tmp/claude-501/-Users-ziwenxu-Desktop-Code-GTA6-unreal-GT-caliber-5-8/d8d4bf46-eade-49bd-9282-7189b8262c91/scratchpad/pause_phaseF.json"
FONT = "/Game/UI/Fonts/DIN_Condensed_Bold_Font.DIN_Condensed_Bold_Font"

ACCENT = (0.22, 0.86, 1.0, 1.0)
ACCENT_DIM = (0.22, 0.86, 1.0, 0.45)
TEXT_DIM = (0.55, 0.62, 0.66, 1.0)
TEXT_BRIGHT = (0.93, 0.96, 0.98, 1.0)

ROWS = [
    ("QUALITY", "< HIGH >"),
    ("RESOLUTION", "1920 x 1080"),
    ("V-SYNC", "< ON >"),
    ("FRAME LIMIT", "< 60 >"),
    ("MASTER VOLUME", "80%"),
    ("MUSIC", "70%"),
    ("LOOK SENSITIVITY", "6.0"),
    ("INVERT Y", "< OFF >"),
    ("FIELD OF VIEW", "90"),
]

res = {}
try:
    tree = unreal.load_object(None, OBJ.rstrip("."))
    mwc = unreal.load_object(None, OBJ + "Main_window_canvas")
    din = unreal.load_object(None, FONT)

    def lin(c):
        return unreal.LinearColor(c[0], c[1], c[2], c[3])

    def ensure(parent, name, cls):
        for i in range(parent.get_children_count()):
            ch = parent.get_child_at(i)
            if ch and ch.get_name() == name:
                return ch
        w = unreal.new_object(cls, tree, name)
        parent.add_child(w)
        return w

    def canvas_anchor(w, ax, ay, bx, by, z):
        s = w.slot
        a = unreal.Anchors()
        a.set_editor_property("minimum", unreal.Vector2D(ax, ay))
        a.set_editor_property("maximum", unreal.Vector2D(bx, by))
        s.set_anchors(a)
        try:
            s.set_offsets(unreal.Margin(0.0, 0.0, 0.0, 0.0))
        except Exception:
            s.set_editor_property("offsets", unreal.Margin(0.0, 0.0, 0.0, 0.0))
        s.set_alignment(unreal.Vector2D(0.0, 0.0))
        s.set_z_order(z)

    def font(size):
        fi = unreal.SlateFontInfo()
        fi.set_editor_property("font_object", din)
        fi.set_editor_property("typeface_font_name", "Bold")
        fi.set_editor_property("size", size)
        return fi

    panel = ensure(mwc, "GTC_Panel_Settings", unreal.CanvasPanel)
    canvas_anchor(panel, 0.500, 0.160, 0.955, 0.860, 4)
    panel.set_visibility(unreal.SlateVisibility.HIDDEN)

    frame = ensure(panel, "GTC_SettingsFrame", unreal.Image)
    fb = frame.get_editor_property("brush")
    try:
        fb.set_editor_property("draw_as", unreal.SlateBrushDrawType.ROUNDED_BOX)
        out = unreal.SlateBrushOutlineSettings()
        out.set_editor_property("corner_radii", unreal.Vector4(10.0, 10.0, 10.0, 10.0))
        out.set_editor_property("color", unreal.SlateColor(lin(ACCENT_DIM)))
        out.set_editor_property("width", 1.0)
        out.set_editor_property("rounding_type", unreal.SlateBrushRoundingType.FIXED_RADIUS)
        fb.set_editor_property("outline_settings", out)
    except Exception as e:
        res["frame_brush_err"] = repr(e)
    fb.set_editor_property("tint_color", unreal.SlateColor(lin((0.03, 0.045, 0.055, 0.55))))
    frame.set_editor_property("brush", fb)
    canvas_anchor(frame, 0.0, 0.0, 1.0, 1.0, 0)

    hdr = ensure(panel, "GTC_SettingsHeader", unreal.TextBlock)
    hdr.set_editor_property("font", font(22))
    hdr.set_editor_property("text", "SETTINGS")
    hdr.set_editor_property("color_and_opacity", unreal.SlateColor(lin(ACCENT)))
    hdr.set_editor_property("justification", unreal.TextJustify.LEFT)
    canvas_anchor(hdr, 0.05, 0.045, 0.6, 0.13, 2)

    labels = ensure(panel, "GTC_SettingsLabels", unreal.VerticalBox)
    canvas_anchor(labels, 0.06, 0.165, 0.55, 0.95, 2)
    values = ensure(panel, "GTC_SettingsValues", unreal.VerticalBox)
    canvas_anchor(values, 0.50, 0.165, 0.94, 0.95, 2)

    def add_row(vbox, name, txt, color, justify):
        t = ensure(vbox, name, unreal.TextBlock)
        t.set_editor_property("font", font(16))
        t.set_editor_property("text", txt)
        t.set_editor_property("color_and_opacity", unreal.SlateColor(lin(color)))
        t.set_editor_property("justification", justify)
        sl = t.slot
        try:
            sl.set_padding(unreal.Margin(0.0, 7.0, 0.0, 7.0))
            sl.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
        except Exception as e:
            res.setdefault("rowslot_err", repr(e))
        return t

    for i, (lbl, val) in enumerate(ROWS):
        add_row(labels, "GTC_SetL_%02d" % i, lbl, TEXT_DIM, unreal.TextJustify.LEFT)
        add_row(values, "GTC_SetV_%02d" % i, val, TEXT_BRIGHT, unreal.TextJustify.RIGHT)

    # highlight the first row as the "selected" setting (cyan)
    try:
        labels.get_child_at(0).set_editor_property("color_and_opacity", unreal.SlateColor(lin(ACCENT)))
        values.get_child_at(0).set_editor_property("color_and_opacity", unreal.SlateColor(lin(ACCENT)))
    except Exception:
        pass

    res["rows"] = len(ROWS)
    bp = unreal.load_asset(PKG)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    res["_save"] = bool(unreal.EditorAssetLibrary.save_asset(PKG, False))
    res["_ok"] = True
except Exception as e:
    res["_fatal"] = repr(e)
    res["_trace"] = traceback.format_exc()

with open(OUT, "w") as fh:
    json.dump(res, fh, indent=2)
unreal.log("GTC_PHASEF_DONE " + json.dumps(res))
