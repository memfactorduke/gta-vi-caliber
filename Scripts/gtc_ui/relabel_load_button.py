"""
relabel_load_button.py — relabel the kit pause-menu "Restart From Checkpoint" button to
"LOAD GAME" (its OnClicked Yes-path was rewired via MCP to LoadPlayersProgress -> RespawnPlayer,
so it now loads the saved game). Sets the button's TextBlock child text, then compiles.

Run: py "/Users/ziwenxu/Desktop/Code/GTA6-unreal/GT-caliber_5.8/Scripts/gtc_ui/relabel_load_button.py"
Sentinels: GTC_RELBL_*
"""
import traceback
import unreal

PKG = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Pause_menu_widget"
OBJ = PKG + ".WB_Pause_menu_widget:WidgetTree"
BTN = "Restart_From_Checkpoint_Button"
NEW_LABEL = "LOAD GAME"

unreal.log("GTC_RELBL_START")
try:
    btn = unreal.load_object(None, OBJ + "." + BTN)
    if btn is None:
        raise Exception("button not found: " + BTN)

    lbl = None
    n = btn.get_children_count() if hasattr(btn, "get_children_count") else 1
    for i in range(max(n, 1)):
        try:
            c = btn.get_child_at(i)
        except Exception:
            c = None
        if isinstance(c, unreal.TextBlock):
            lbl = c
            break
    if lbl is None:
        raise Exception("no TextBlock child on " + BTN)

    old = lbl.get_text()
    lbl.set_text(unreal.Text(NEW_LABEL))
    unreal.log("GTC_RELBL_SET %s old=%s -> %s" % (lbl.get_name(), str(old), NEW_LABEL))

    bp = unreal.load_asset(PKG)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.log("GTC_RELBL_DONE")
except Exception as e:
    unreal.log_error("GTC_RELBL_FAIL " + repr(e))
    unreal.log_error(traceback.format_exc())
