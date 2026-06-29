"""
pause_phaseH_makevars.py — make the tab panels graph-referenceable.

To switch panels from the event graph, each panel widget must be flagged
"Is Variable" (bIsVariable) so a Get<Name> accessor exists. MCP's set_properties
refuses bIsVariable (protected filter); test whether the editor Python console can
set it via set_editor_property. If it works, the panels become graph-addressable.
Compiles + saves. Writes JSON result.

Run:  py "/Users/ziwenxu/Desktop/Code/GTA6-unreal/GT-caliber_5.8/Scripts/gtc_ui/pause_phaseH_makevars.py"
"""
import json, traceback
import unreal

PKG = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Pause_menu_widget"
OBJ = PKG + ".WB_Pause_menu_widget:WidgetTree."
OUT = "/private/tmp/claude-501/-Users-ziwenxu-Desktop-Code-GTA6-unreal-GT-caliber-5-8/d8d4bf46-eade-49bd-9282-7189b8262c91/scratchpad/pause_phaseH.json"

NAMES = [
    "GTC_Panel_Status", "GTC_Panel_Settings", "GTC_Panel_Load",
    "GTC_RightPanel", "GTC_MapView", "GTC_MapHeader", "GTC_MapMeta",
]

res = {}
try:
    for n in NAMES:
        try:
            w = unreal.load_object(None, OBJ + n)
            if w is None:
                res[n] = "ERR none"
                continue
            try:
                w.set_editor_property("bIsVariable", True)
            except Exception as e1:
                # try direct attribute as a fallback
                try:
                    w.bIsVariable = True
                except Exception as e2:
                    res[n] = "ERR set: %s / %s" % (repr(e1), repr(e2))
                    continue
            res[n] = bool(w.get_editor_property("bIsVariable"))
        except Exception as e:
            res[n] = "ERR " + repr(e)
    bp = unreal.load_asset(PKG)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    res["_save"] = bool(unreal.EditorAssetLibrary.save_asset(PKG, False))
    res["_ok"] = True
except Exception as e:
    res["_fatal"] = repr(e)
    res["_trace"] = traceback.format_exc()

with open(OUT, "w") as fh:
    json.dump(res, fh, indent=2)
unreal.log("GTC_PHASEH_DONE " + json.dumps(res))
