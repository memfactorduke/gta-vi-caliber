"""
pause_phaseD_map.py — PHASE D slice 1: build the RIGHT-SIDE content container for the
tabbed pause, populated with the DEFAULT tab = MAP panel. Fills the empty right side.

Uses PROPORTIONAL ANCHORS (min!=max, offsets 0) so the panel occupies the right
region of the screen at ANY resolution/aspect (last tick's lesson: absolute coords
land unpredictably in PIE). Idempotent (ensure-by-name). Tries to use a live minimap
material for the map view; falls back to a styled dark map field if none is found.
Compiles + saves via EditorAssetLibrary.

Run:  py "/Users/ziwenxu/Desktop/Code/GTA6-unreal/GT-caliber_5.8/Scripts/gtc_ui/pause_phaseD_map.py"
"""
import json, traceback
import unreal

PKG = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Pause_menu_widget"
OBJ = PKG + ".WB_Pause_menu_widget:WidgetTree."
OUT = "/private/tmp/claude-501/-Users-ziwenxu-Desktop-Code-GTA6-unreal-GT-caliber-5-8/d8d4bf46-eade-49bd-9282-7189b8262c91/scratchpad/pause_phaseD.json"
FONT = "/Game/UI/Fonts/DIN_Condensed_Bold_Font.DIN_Condensed_Bold_Font"

ACCENT = (0.22, 0.86, 1.0, 1.0)
ACCENT_DIM = (0.22, 0.86, 1.0, 0.45)
TEXT_DIM = (0.55, 0.62, 0.66, 1.0)
TEXT_BRIGHT = (0.93, 0.96, 0.98, 1.0)

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

    def anchor(w, ax, ay, bx, by, z):
        """Proportional stretch: fill the rect [ax,ay]-[bx,by] of the parent."""
        s = w.slot
        a = unreal.Anchors()
        a.set_editor_property("minimum", unreal.Vector2D(ax, ay))
        a.set_editor_property("maximum", unreal.Vector2D(bx, by))
        s.set_anchors(a)
        try:
            s.set_offsets(unreal.Margin(0.0, 0.0, 0.0, 0.0))
        except Exception:
            # fallback: set_offsets may not exist; zero via individual setters
            s.set_editor_property("offsets", unreal.Margin(0.0, 0.0, 0.0, 0.0))
        s.set_alignment(unreal.Vector2D(0.0, 0.0))
        s.set_z_order(z)

    # ---- find a live minimap material (optional) ----
    mat = None
    matpath = None
    cands = []
    for d in ["/Game/UI", "/Game/ThirdPersonKit", "/Game/Blueprints", "/Game/Core", "/Game/GTCShooter"]:
        try:
            for a in unreal.EditorAssetLibrary.list_assets(d, True, False):
                ln = a.lower()
                if "minimap" in ln or "radar" in ln:
                    cands.append(a)
        except Exception:
            pass
    res["minimap_candidates"] = cands[:25]
    # prefer a Material (M_/MI_/MM_) over a render target/texture
    pick = None
    for a in cands:
        leaf = a.split("/")[-1]
        if leaf.startswith(("M_", "MI_", "MM_")):
            pick = a
            break
    if pick is None:
        for a in cands:
            leaf = a.split("/")[-1]
            if "rt_" in leaf.lower() or leaf.startswith("T_"):
                pick = a
                break
    if pick is None and cands:
        pick = cands[0]
    if pick:
        try:
            mat = unreal.load_asset(pick)
            matpath = pick
        except Exception as e:
            res["mat_load_err"] = repr(e)
    res["map_material"] = matpath

    # ---- 1) right-panel frame (rounded dark glass + faint cyan outline) ----
    frame, _ = ensure("GTC_RightPanel", unreal.Image)
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
    fb.set_editor_property("tint_color", unreal.SlateColor(lin((0.03, 0.045, 0.055, 0.50))))
    frame.set_editor_property("brush", fb)
    frame.set_editor_property("color_and_opacity", lin((1.0, 1.0, 1.0, 1.0)))
    anchor(frame, 0.500, 0.160, 0.955, 0.860, 2)

    # ---- 2) the map view (live minimap material if found, else dark field) ----
    mapv, _ = ensure("GTC_MapView", unreal.Image)
    if mat is not None:
        try:
            mapv.set_brush_from_material(mat)
            res["map_view"] = "material"
        except Exception as e:
            res["map_view_err"] = repr(e)
            mb = mapv.get_editor_property("brush")
            mb.set_editor_property("tint_color", unreal.SlateColor(lin((0.06, 0.08, 0.10, 1.0))))
            mapv.set_editor_property("brush", mb)
            res["map_view"] = "fallback_dark"
    else:
        mb = mapv.get_editor_property("brush")
        mb.set_editor_property("draw_as", unreal.SlateBrushDrawType.IMAGE)
        mb.set_editor_property("tint_color", unreal.SlateColor(lin((0.06, 0.08, 0.10, 1.0))))
        mapv.set_editor_property("brush", mb)
        mapv.set_editor_property("color_and_opacity", lin((1.0, 1.0, 1.0, 1.0)))
        res["map_view"] = "fallback_dark"
    anchor(mapv, 0.512, 0.180, 0.943, 0.844, 3)

    # ---- 3) panel header "MAP" (above the frame) ----
    def text(name, txt, ax, ay, bx, by, size, color, z, justify=None):
        w, _ = ensure(name, unreal.TextBlock)
        fi = unreal.SlateFontInfo()
        fi.set_editor_property("font_object", din)
        fi.set_editor_property("typeface_font_name", "Bold")
        fi.set_editor_property("size", size)
        w.set_editor_property("font", fi)
        w.set_editor_property("text", txt)
        w.set_editor_property("color_and_opacity", unreal.SlateColor(lin(color)))
        if justify is not None:
            w.set_editor_property("justification", justify)
        anchor(w, ax, ay, bx, by, z)
        return w

    text("GTC_MapHeader", "MAP", 0.500, 0.112, 0.70, 0.152, 22, ACCENT, 9, unreal.TextJustify.LEFT)
    text("GTC_MapMeta", "OCEAN DR / VICEPORT", 0.640, 0.118, 0.945, 0.150, 13, TEXT_DIM, 9, unreal.TextJustify.RIGHT)

    # ---- save ----
    bp = unreal.load_asset(PKG)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    res["_save"] = bool(unreal.EditorAssetLibrary.save_asset(PKG, False))
    res["_ok"] = True
except Exception as e:
    res["_fatal"] = repr(e)
    res["_trace"] = traceback.format_exc()

with open(OUT, "w") as fh:
    json.dump(res, fh, indent=2)
unreal.log("GTC_PHASED_DONE " + json.dumps(res))
