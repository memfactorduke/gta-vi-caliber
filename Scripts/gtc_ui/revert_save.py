"""revert_save.py — revert the temporary PIE-reveal toggles to defaults, compile, save.

Defaults: Pause_menu_canvas HIDDEN (Esc shows it), HUD Pause_menu_widget RenderOpacity 0
(Showpausemenu forces it to 1 on open). Main_window_canvas stays VISIBLE (default tab),
Options_canvas stays HIDDEN. Saves both widgets so the glassmorphism persists cleanly.
"""
import json, traceback
import unreal

PAUSE = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Pause_menu_widget"
HUD = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Game_HUD_widget"
PBASE = PAUSE + ".WB_Pause_menu_widget:WidgetTree."
HBASE = HUD + ".WB_Game_HUD_widget:WidgetTree."
OUT = "/private/tmp/claude-501/-Users-ziwenxu-Desktop-Code-GTA6-unreal-GT-caliber-5-8/d8d4bf46-eade-49bd-9282-7189b8262c91/scratchpad/revert_save.json"

res = {}
try:
    unreal.load_object(None, PBASE + "Pause_menu_canvas").set_visibility(unreal.SlateVisibility.HIDDEN)
    unreal.load_object(None, HBASE + "Pause_menu_widget").set_editor_property("render_opacity", 0.0)
    res["reverted"] = True
    for pkg in (PAUSE, HUD):
        unreal.BlueprintEditorLibrary.compile_blueprint(unreal.load_asset(pkg))
    res["saved_pause"] = bool(unreal.EditorAssetLibrary.save_asset(PAUSE, False))
    res["saved_hud"] = bool(unreal.EditorAssetLibrary.save_asset(HUD, False))
    res["_ok"] = True
except Exception as e:
    res["_fatal"] = repr(e)
    res["_trace"] = traceback.format_exc()

with open(OUT, "w") as fh:
    json.dump(res, fh, indent=2)
unreal.log("GTC_REVERT_SAVE_DONE " + json.dumps(res))
