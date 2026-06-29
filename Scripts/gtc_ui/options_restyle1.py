"""
options_restyle1.py — pass 1 of restyling the kit Options screen.

(1) Fix the washed-out backdrop: Options_background (child of Options_canvas in
    WB_Pause_menu_widget) is currently a near-white light-cyan tint -> low contrast.
    Change it to dark rounded glass with a cyan hairline outline (matches our panels).
(2) Enumerate WB_Options_widget's internal tree via WidgetTree.get_all_widgets()
    (root_widget is unreadable in 5.8) so we learn the title/scrollbox/buttons/row
    names for a targeted pass 2.
Compiles the pause widget so the backdrop change shows in PIE. Does NOT save (the
pause widget still holds temporary reveal toggles; save happens after they revert).

Run:  py "/Users/ziwenxu/Desktop/Code/GTA6-unreal/GT-caliber_5.8/Scripts/gtc_ui/options_restyle1.py"
"""
import json, traceback
import unreal

PKG = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Pause_menu_widget"
BASE = PKG + ".WB_Pause_menu_widget:WidgetTree."
OPT = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Options_widget"
OUT = "/private/tmp/claude-501/-Users-ziwenxu-Desktop-Code-GTA6-unreal-GT-caliber-5-8/d8d4bf46-eade-49bd-9282-7189b8262c91/scratchpad/options_restyle1.json"

ACCENT_DIM = (0.22, 0.86, 1.0, 0.45)
GLASS = (0.03, 0.045, 0.055, 0.94)

res = {"embedded_tree": [], "notes": []}


def lin(c):
    return unreal.LinearColor(c[0], c[1], c[2], c[3])


def info(w):
    d = {"name": w.get_name(), "class": w.get_class().get_name()}
    try:
        d["vis"] = str(w.get_editor_property("visibility"))[:32]
    except Exception:
        pass
    if isinstance(w, unreal.TextBlock):
        try:
            d["text"] = str(w.get_editor_property("text"))
        except Exception:
            pass
        try:
            c = w.get_editor_property("color_and_opacity").get_editor_property("specified_color")
            d["color"] = [round(c.r, 3), round(c.g, 3), round(c.b, 3), round(c.a, 3)]
        except Exception:
            pass
        try:
            f = w.get_editor_property("font")
            fo = f.get_editor_property("font_object")
            d["font"] = (fo.get_name() if fo else None)
            d["fsize"] = f.get_editor_property("size")
        except Exception:
            pass
    if isinstance(w, unreal.Image):
        try:
            b = w.get_editor_property("brush")
            t = b.get_editor_property("tint_color").get_editor_property("specified_color")
            d["tint"] = [round(t.r, 3), round(t.g, 3), round(t.b, 3), round(t.a, 3)]
            d["draw_as"] = str(b.get_editor_property("draw_as"))[:24]
        except Exception:
            pass
    return d


try:
    # ---- (1) backdrop -> dark glass ----
    img = unreal.load_object(None, BASE + "Options_background")
    if img is None:
        res["notes"].append("Options_background not found")
    else:
        b = img.get_editor_property("brush")
        try:
            b.set_editor_property("draw_as", unreal.SlateBrushDrawType.ROUNDED_BOX)
            out = unreal.SlateBrushOutlineSettings()
            out.set_editor_property("corner_radii", unreal.Vector4(14.0, 14.0, 14.0, 14.0))
            out.set_editor_property("color", unreal.SlateColor(lin(ACCENT_DIM)))
            out.set_editor_property("width", 1.0)
            out.set_editor_property("rounding_type", unreal.SlateBrushRoundingType.FIXED_RADIUS)
            b.set_editor_property("outline_settings", out)
        except Exception as e:
            res["notes"].append("outline_err: " + repr(e))
        b.set_editor_property("tint_color", unreal.SlateColor(lin(GLASS)))
        img.set_editor_property("brush", b)
        res["backdrop"] = "dark glass applied"

    # ---- (2) enumerate WB_Options_widget internals ----
    tree = unreal.load_object(None, OPT + ".WB_Options_widget:WidgetTree")
    if tree is None:
        res["notes"].append("WB_Options_widget WidgetTree not found")
    else:
        got = None
        for meth in ("get_all_widgets",):
            try:
                got = getattr(tree, meth)()
                res["notes"].append("enum via %s -> %d" % (meth, len(got)))
                break
            except Exception as e:
                res["notes"].append("%s failed: %s" % (meth, repr(e)))
        if got is None:
            # fallback: WidgetBlueprintLibrary on the asset
            try:
                wb = unreal.load_asset(OPT)
                got = unreal.WidgetBlueprintLibrary.get_all_widgets if False else None
            except Exception as e:
                res["notes"].append("fallback failed: " + repr(e))
        if got:
            for w in got:
                res["embedded_tree"].append(info(w))

    # compile pause widget so the backdrop shows in PIE (no save)
    bp = unreal.load_asset(PKG)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    res["_ok"] = True
except Exception as e:
    res["_fatal"] = repr(e)
    res["_trace"] = traceback.format_exc()

with open(OUT, "w") as fh:
    json.dump(res, fh, indent=2)
unreal.log("GTC_OPTIONS_RESTYLE1_DONE n=%d" % len(res.get("embedded_tree", [])))
