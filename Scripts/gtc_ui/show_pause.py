"""
show_pause.py — spawn WB_Pause_menu_widget into the LIVE PIE viewport to visually verify
the frosted BackgroundBlur over the real game (the designer can't show blur). Run during PIE:
  py "/Users/ziwenxu/Desktop/Code/GTA6-unreal/GT-caliber_5.8/Scripts/gtc_ui/show_pause.py"
Sentinels: GTC_SHOW_START/WORLD/CLS/DONE/FAIL
"""
import traceback
import unreal

PKG = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Pause_menu_widget"

unreal.log("GTC_SHOW_START")
try:
    ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    world = ues.get_game_world()
    unreal.log("GTC_SHOW_WORLD " + str(world is not None))
    pc = unreal.GameplayStatics.get_player_controller(world, 0)
    cls = unreal.load_object(None, PKG + ".WB_Pause_menu_widget_C")
    unreal.log("GTC_SHOW_CLS " + str(cls is not None))
    w = None
    try:
        w = unreal.WidgetBlueprintLibrary.create(world, cls, pc)
    except Exception as e1:
        unreal.log("GTC_SHOW_M1 " + repr(e1))
    if w is None:
        w = unreal.new_object(cls, world)
        unreal.log("GTC_SHOW_M2_newobject")
    w.add_to_viewport(1000)
    try:
        unreal.GameplayStatics.set_game_paused(world, True)
    except Exception as pe:
        unreal.log("GTC_SHOW_PAUSEWARN " + repr(pe))
    unreal.log("GTC_SHOW_DONE")
except Exception as e:
    unreal.log_error("GTC_SHOW_FAIL " + repr(e))
    unreal.log_error(traceback.format_exc())
