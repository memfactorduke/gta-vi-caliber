"""
pause_enum.py — enumerate Main_window_canvas + Pause_menu_canvas children
(name, class, visibility, slot pos/size, text/label). Canvas-iteration based
(RootWidget traversal is unreadable in 5.8). Writes JSON to scratchpad.
Run: py "/Users/ziwenxu/Desktop/Code/GTA6-unreal/GT-caliber_5.8/Scripts/gtc_ui/pause_enum.py"
"""
import json, traceback
import unreal

OBJ = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Pause_menu_widget.WB_Pause_menu_widget:WidgetTree."
OUT = "/private/tmp/claude-501/-Users-ziwenxu-Desktop-Code-GTA6-unreal-GT-caliber-5-8/d8d4bf46-eade-49bd-9282-7189b8262c91/scratchpad/pause_enum.json"

def safe(fn, d=None):
    try: return fn()
    except Exception: return d

def describe(w):
    cls = safe(lambda: w.get_class().get_name())
    rec = {"name": safe(lambda: w.get_name()), "class": cls,
           "vis": safe(lambda: str(w.get_editor_property("Visibility")))}
    s = safe(lambda: w.slot)
    if s is not None:
        rec["pos"] = safe(lambda: [round(s.get_position().x,1), round(s.get_position().y,1)])
        rec["size"] = safe(lambda: [round(s.get_size().x,1), round(s.get_size().y,1)])
        rec["z"] = safe(lambda: s.get_z_order())
    if cls and "TextBlock" in cls:
        rec["text"] = safe(lambda: str(w.get_text()))
    if cls and "Button" in cls:
        lbl = safe(lambda: w.get_content())
        rec["label"] = safe(lambda: str(lbl.get_text()) if lbl else None)
        rec["label_name"] = safe(lambda: lbl.get_name() if lbl else None)
    return rec

out = {}
try:
    for cname in ["Main_window_canvas", "Pause_menu_canvas"]:
        c = unreal.load_object(None, OBJ + cname)
        kids = []
        n = safe(lambda: c.get_children_count(), 0)
        for i in range(n):
            ch = safe(lambda: c.get_child_at(i))
            if ch is not None:
                kids.append(describe(ch))
        out[cname] = kids
except Exception as e:
    out["_err"] = repr(e); out["_trace"] = traceback.format_exc()

with open(OUT, "w") as fh:
    json.dump(out, fh, indent=2)
unreal.log("GTC_ENUM_DONE")
