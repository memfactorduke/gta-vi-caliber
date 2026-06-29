"""
options_enum.py — map the kit Options screen so we can restyle it.

Walks the Options_canvas subtree inside WB_Pause_menu_widget, records each
widget's name/class/visibility/opacity (+ text for TextBlocks, brush tint for
Images). When it hits the embedded WBOptionsWidget UserWidget instance, it
records the generated-class asset path AND tries to walk that asset's own
WidgetTree (root_widget) so we can see the panel/title/buttons defined there.
Read-only. Writes JSON.

Run:  py "/Users/ziwenxu/Desktop/Code/GTA6-unreal/GT-caliber_5.8/Scripts/gtc_ui/options_enum.py"
"""
import json, traceback
import unreal

PKG = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Pause_menu_widget"
BASE = PKG + ".WB_Pause_menu_widget:WidgetTree."
OUT = "/private/tmp/claude-501/-Users-ziwenxu-Desktop-Code-GTA6-unreal-GT-caliber-5-8/d8d4bf46-eade-49bd-9282-7189b8262c91/scratchpad/options_enum.json"

res = {"options_canvas": [], "embedded": {}, "notes": []}


def info(w):
    d = {"name": w.get_name(), "class": w.get_class().get_name()}
    try:
        d["vis"] = str(w.get_editor_property("visibility"))
    except Exception:
        pass
    try:
        d["opacity"] = w.get_editor_property("render_opacity")
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
    if isinstance(w, unreal.Image):
        try:
            b = w.get_editor_property("brush")
            t = b.get_editor_property("tint_color").get_editor_property("specified_color")
            d["tint"] = [round(t.r, 3), round(t.g, 3), round(t.b, 3), round(t.a, 3)]
            d["draw_as"] = str(b.get_editor_property("draw_as"))
        except Exception:
            pass
    return d


def walk(w, depth, acc):
    d = info(w)
    d["depth"] = depth
    acc.append(d)
    if isinstance(w, unreal.UserWidget):
        d["is_userwidget"] = True
        try:
            d["asset_class"] = w.get_class().get_path_name()
        except Exception:
            pass
    if hasattr(w, "get_children_count"):
        try:
            for i in range(w.get_children_count()):
                ch = w.get_child_at(i)
                if ch:
                    walk(ch, depth + 1, acc)
        except Exception as e:
            d["children_err"] = repr(e)


try:
    oc = unreal.load_object(None, BASE + "Options_canvas")
    if oc is None:
        res["notes"].append("Options_canvas not found")
    else:
        walk(oc, 0, res["options_canvas"])

    # Find the embedded WBOptionsWidget asset class path from the walk
    embedded_path = None
    for d in res["options_canvas"]:
        if d.get("is_userwidget") and "option" in d["name"].lower():
            embedded_path = d.get("asset_class")
            res["notes"].append("embedded options widget: %s -> %s" % (d["name"], embedded_path))

    # Try to walk the WBOptionsWidget asset's own tree (panel/title/buttons defined there)
    # asset_class like /Game/.../WBOptionsWidget.WBOptionsWidget_C -> asset = strip _C
    if embedded_path:
        try:
            asset_pkg = embedded_path.split(".")[0]
            res["embedded"]["asset_pkg"] = asset_pkg
            tree = unreal.load_object(None, asset_pkg + "." + asset_pkg.split("/")[-1] + ":WidgetTree")
            if tree:
                try:
                    root = tree.get_editor_property("root_widget")
                except Exception as e:
                    root = None
                    res["embedded"]["root_err"] = repr(e)
                if root:
                    acc = []
                    walk(root, 0, acc)
                    res["embedded"]["tree"] = acc
                else:
                    res["embedded"]["note"] = "root_widget unreadable; will need name-based access"
        except Exception as e:
            res["embedded"]["err"] = repr(e)

    res["_ok"] = True
except Exception as e:
    res["_fatal"] = repr(e)
    res["_trace"] = traceback.format_exc()

with open(OUT, "w") as fh:
    json.dump(res, fh, indent=2)
unreal.log("GTC_OPTIONS_ENUM_DONE keys=%s" % list(res.keys()))
