# create_caliber_city_map.py — make the NEW World Partition map for the full Caliber City.
#
# This is the ONE step the MCP toolset can't do (no "new level" tool; the MCP
# programmatic sandbox has no `unreal` module). Run it ONCE in the already-running
# editor's Python console (do NOT launch a second editor):
#
#     Output Log > switch the "Cmd" dropdown to "Python", then:
#         py "<repo>/Scripts/gtc_citygen/create_caliber_city_map.py"
#
# It creates an Open World (World Partition) level at the canonical path and saves
# it, then leaves it open. After this, the /loop can apply boundaries -> highways ->
# districts -> buildings entirely over MCP (no more manual steps). Re-running just
# re-opens the existing map.
#
# Cesium base (optional, recommended): after this, run
#   Content/GTCaliberAssets/Content/Python/build_miami.py
# to add the CesiumGeoreference (South Beach) + sun/sky.

import unreal

MAP_PATH = "/Game/GTCaliberAssets/Content/CityGen/Maps/CaliberCity"
OPEN_WORLD_TEMPLATE = "/Engine/Maps/Templates/OpenWorld"  # World Partition template

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)


def current_world():
    try:
        return ues.get_editor_world().get_path_name()
    except Exception:
        return "<unknown>"


def main():
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        unreal.log("CaliberCity already exists -> opening it.")
        les.load_level(MAP_PATH)
        unreal.log("current level: %s" % current_world())
        return

    # World Partition map from the Open World template (matches File > New Level > Open World).
    ok = False
    try:
        ok = bool(les.new_level_from_template(MAP_PATH, OPEN_WORLD_TEMPLATE))
    except Exception as e:
        unreal.log_warning("new_level_from_template failed (%s); falling back to new_level "
                           "(NON World-Partition)." % e)
        try:
            ok = bool(les.new_level(MAP_PATH))
        except Exception as e2:
            unreal.log_error("new_level also failed: %s" % e2)
            return

    if not ok:
        unreal.log_error("Could not create %s" % MAP_PATH)
        return

    les.save_current_level()
    unreal.log("=== Created + saved %s ===" % MAP_PATH)
    unreal.log("current level: %s" % current_world())
    unreal.log("Next: the city /loop will apply boundaries + highways over MCP. "
               "(Optional Cesium base: run build_miami.py.)")


main()
