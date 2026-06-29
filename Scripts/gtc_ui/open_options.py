"""open_options.py — open the WB_Options_widget UMG editor so we can read its Hierarchy."""
import unreal
aes = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
aes.open_editor_for_assets([unreal.load_asset("/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Options_widget")])
unreal.log("GTC_OPEN_OPTIONS_DONE")
