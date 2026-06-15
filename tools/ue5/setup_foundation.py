"""GTC foundation scaffold — create the editor-only assets the the foundation plan flags "execute first".

Run this INSIDE the UE5 editor (Output Log > Python, or `UnrealEditor-Cmd ... -run=pythonscript
-script=".../setup_foundation.py"`). It is NOT runnable headlessly from the migration sandbox, which is
why the resulting `.uasset`s are not committed — this script is the reproducible recipe.

Creates:
  * /Game/Materials/MPC_GTCGlobals  — Material Parameter Collection with the two shared globals
      `world_night_amount` (0.0) and `world_wetness` (0.0). These replace Godot's global shader params;
      sky/weather write them, and facade/road/neon/ocean materials read them (NATIVE #6/#8).
  * /Game/Input/IMC_Default         — a starter Enhanced Input Mapping Context (optional scaffold).

NOTE (per ../../UE5_NOTES.md §0): do NOT trust method/struct names from memory. Before relying on this,
confirm the signatures via `manage_skills(load='data-assets'/'enhanced-input')` or
`discover_python_class('unreal.AssetTools', ...)`. The calls below use the standard 5.7 API; if a
struct/factory name has shifted, fix it here and append the correction to UE5_NOTES.md.
"""
import unreal

ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()


def create_mpc():
    mpc = ASSET_TOOLS.create_asset(
        asset_name="MPC_GTCGlobals",
        package_path="/Game/Materials",
        asset_class=unreal.MaterialParameterCollection,
        factory=unreal.MaterialParameterCollectionFactoryNew(),
    )
    night = unreal.CollectionScalarParameter()
    night.set_editor_property("parameter_name", "world_night_amount")
    night.set_editor_property("default_value", 0.0)
    wet = unreal.CollectionScalarParameter()
    wet.set_editor_property("parameter_name", "world_wetness")
    wet.set_editor_property("default_value", 0.0)
    mpc.set_editor_property("scalar_parameters", [night, wet])
    unreal.EditorAssetLibrary.save_asset("/Game/Materials/MPC_GTCGlobals")
    unreal.log("CREATED /Game/Materials/MPC_GTCGlobals (world_night_amount, world_wetness)")


def create_starter_input():
    # Optional: the plugin + default input classes are already set in DefaultInput.ini; this just
    # seeds a starter context so designers have something to bind to. Safe to delete/replace.
    imc = ASSET_TOOLS.create_asset(
        "IMC_Default", "/Game/Input", unreal.InputMappingContext,
        unreal.InputMappingContextFactory(),
    )
    unreal.EditorAssetLibrary.save_asset("/Game/Input/IMC_Default")
    unreal.log("CREATED /Game/Input/IMC_Default (empty starter mapping context)")


if __name__ == "__main__":
    create_mpc()
    try:
        create_starter_input()
    except Exception as exc:  # IMC starter is optional; the MPC is the required scaffold.
        unreal.log_warning("starter IMC skipped: %s" % exc)
