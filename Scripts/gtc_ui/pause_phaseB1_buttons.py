"""
pause_phaseB1_buttons.py — refine the 6 pause buttons into an AAA left-aligned
"command column": left-align each label (justify Left, HAlign Fill, 64px left pad,
vertical center), set per-role label color, and tone the button plates to subtle
glass (normal/hovered/pressed tint). Operates on existing buttons by name +
get_content() for labels (no widget creation). Idempotent. Compiles + saves.

Run: py "/Users/ziwenxu/Desktop/Code/GTA6-unreal/GT-caliber_5.8/Scripts/gtc_ui/pause_phaseB1_buttons.py"
"""
import json, traceback
import unreal

PKG = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Pause_menu_widget"
OBJ = PKG + ".WB_Pause_menu_widget:WidgetTree."
OUT = "/private/tmp/claude-501/-Users-ziwenxu-Desktop-Code-GTA6-unreal-GT-caliber-5-8/d8d4bf46-eade-49bd-9282-7189b8262c91/scratchpad/pause_phaseB1.json"

def C(r, g, b, a):
    return unreal.SlateColor(unreal.LinearColor(r, g, b, a))

# role -> (label_color, normal_tint, hovered_tint, pressed_tint)
ROLES = {
    "resume": ((0.95, 0.98, 1.0, 1.0), (0.22, 0.86, 1.0, 0.14), (0.22, 0.86, 1.0, 0.20), (0.22, 0.86, 1.0, 0.26)),
    "std":    ((0.86, 0.91, 0.95, 1.0), (0.0, 0.0, 0.0, 0.0),     (0.22, 0.86, 1.0, 0.12), (0.22, 0.86, 1.0, 0.22)),
    "quit":   ((0.86, 0.42, 0.42, 1.0), (0.0, 0.0, 0.0, 0.0),     (1.0, 0.27, 0.30, 0.14), (1.0, 0.27, 0.30, 0.22)),
}
BUTTONS = [
    ("Resume_button", "resume"),
    ("Restart_From_Checkpoint_Button", "std"),
    ("Restart_Level_button", "std"),
    ("Options_button", "std"),
    ("Main_menu_button", "std"),
    ("Quit_button", "quit"),
]

res = {}
try:
    for name, role in BUTTONS:
        try:
            btn = unreal.load_object(None, OBJ + name)
            if btn is None:
                res[name] = "ERR no button"
                continue
            lblcol, ncol, hcol, pcol = ROLES[role]
            # --- button plate glass: retint normal/hovered/pressed brushes ---
            style = btn.get_editor_property("widget_style")
            for state, col in (("normal", ncol), ("hovered", hcol), ("pressed", pcol)):
                br = style.get_editor_property(state)
                br.set_editor_property("tint_color", C(*col))
                style.set_editor_property(state, br)
            btn.set_editor_property("widget_style", style)
            # --- label: left-align + color ---
            lbl = btn.get_content()
            note = ""
            if lbl is not None and isinstance(lbl, unreal.TextBlock):
                lbl.set_editor_property("justification", unreal.TextJustify.LEFT)
                lbl.set_editor_property("color_and_opacity", C(*lblcol))
                bslot = lbl.slot
                if bslot is not None:
                    bslot.set_editor_property("horizontal_alignment", unreal.HorizontalAlignment.H_ALIGN_FILL)
                    bslot.set_editor_property("vertical_alignment", unreal.VerticalAlignment.V_ALIGN_CENTER)
                    m = unreal.Margin()
                    m.set_editor_property("left", 64.0)
                    m.set_editor_property("top", 0.0)
                    m.set_editor_property("right", 0.0)
                    m.set_editor_property("bottom", 0.0)
                    bslot.set_editor_property("padding", m)
                note = "label=%s" % lbl.get_name()
            else:
                note = "label_type=%s" % (type(lbl).__name__ if lbl else "None")
            res[name] = "OK " + note
        except Exception as e:
            res[name] = "ERR " + repr(e)
    bp = unreal.load_asset(PKG)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.EditorLoadingAndSavingUtils.save_packages([PKG], True)
    res["_save"] = "done"
except Exception as e:
    res["_fatal"] = repr(e)
    res["_trace"] = traceback.format_exc()

with open(OUT, "w") as fh:
    json.dump(res, fh, indent=2)
unreal.log("GTC_PHASEB1_DONE " + json.dumps(res))
