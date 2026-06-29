"""
place_hint_bar.py — position the already-created GTC_HintBar at bottom-center of the pause
menu using the proven CanvasPanelSlot METHODS (set_anchors/set_alignment/set_position/
set_size/set_z_order). Text/font/color were set via MCP. Then compile + save.

Run: py "/Users/ziwenxu/Desktop/Code/GTA6-unreal/GT-caliber_5.8/Scripts/gtc_ui/place_hint_bar.py"
Sentinels: GTC_HPLACE_*
"""
import traceback
import unreal

PKG = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Pause_menu_widget"
OBJ = PKG + ".WB_Pause_menu_widget:WidgetTree"

unreal.log("GTC_HPLACE_START")
try:
    bar = unreal.load_object(None, OBJ + ".GTC_HintBar")
    if bar is None:
        raise Exception("GTC_HintBar not found")
    slot = bar.get_editor_property("slot")

    a = unreal.Anchors()
    a.set_editor_property("minimum", unreal.Vector2D(0.5, 1.0))
    a.set_editor_property("maximum", unreal.Vector2D(0.5, 1.0))
    slot.set_anchors(a)
    slot.set_alignment(unreal.Vector2D(0.5, 1.0))
    slot.set_size(unreal.Vector2D(1100.0, 34.0))
    slot.set_position(unreal.Vector2D(0.0, -46.0))  # 46px above the screen bottom, centered
    slot.set_z_order(50)
    unreal.log("GTC_HPLACE_SET")

    bp = unreal.load_asset(PKG)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.EditorLoadingAndSavingUtils.save_packages([PKG], True)
    unreal.log("GTC_HPLACE_DONE")
except Exception as e:
    unreal.log_error("GTC_HPLACE_FAIL " + repr(e))
    unreal.log_error(traceback.format_exc())
