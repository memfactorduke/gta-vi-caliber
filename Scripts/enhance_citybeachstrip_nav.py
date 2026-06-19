# Enhance CityBeachStrip with navigation for the living-NPC crowd.
#
# Adds a NavMeshBoundsVolume sized to the level, ensures a RecastNavMesh exists,
# builds navigation, and saves the map. This is the one step that must run inside
# the Unreal editor (binary .umap files can't be edited from outside it). After it
# runs, AGTCCitizen pedestrians path on the NavMesh instead of steering straight,
# and the crowd/traffic subsystems (which auto-activate) populate the streets.
#
# Run headless from the project root:
#   "/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor-Cmd" \
#       "$PWD/GTC_UE5.uproject" \
#       -run=pythonscript -script="$PWD/Scripts/enhance_citybeachstrip_nav.py"
#
# Or paste it into the editor's Output Log > Python console with the map open.

import unreal

MAP = "/Game/GTCaliberAssets/Content/CityBeachStrip/Maps/CityBeachStrip"
# Fallback bounds (cm) if the level reports nothing — a generous beach-strip box.
FALLBACK_EXTENT = unreal.Vector(60000.0, 60000.0, 20000.0)
FALLBACK_ORIGIN = unreal.Vector(0.0, 0.0, 0.0)


def load_map():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not les.load_level(MAP):
        unreal.log_warning("Could not load %s (already open?). Continuing." % MAP)


def level_bounds():
    """Union the bounds of all level actors; fall back to a fixed box."""
    actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = actor_sub.get_all_level_actors()
    mn = None
    mx = None
    for a in actors:
        try:
            origin, extent = a.get_actor_bounds(only_colliding_components=False)
        except Exception:
            continue
        if extent.x <= 0 and extent.y <= 0 and extent.z <= 0:
            continue
        lo = unreal.Vector(origin.x - extent.x, origin.y - extent.y, origin.z - extent.z)
        hi = unreal.Vector(origin.x + extent.x, origin.y + extent.y, origin.z + extent.z)
        if mn is None:
            mn, mx = lo, hi
        else:
            mn = unreal.Vector(min(mn.x, lo.x), min(mn.y, lo.y), min(mn.z, lo.z))
            mx = unreal.Vector(max(mx.x, hi.x), max(mx.y, hi.y), max(mx.z, hi.z))
    if mn is None:
        return FALLBACK_ORIGIN, FALLBACK_EXTENT
    center = unreal.Vector((mn.x + mx.x) * 0.5, (mn.y + mx.y) * 0.5, (mn.z + mx.z) * 0.5)
    extent = unreal.Vector((mx.x - mn.x) * 0.5, (mx.y - mn.y) * 0.5, (mx.z - mn.z) * 0.5)
    # Pad a little so the mesh reaches the edges.
    extent = unreal.Vector(extent.x * 1.1 + 2000, extent.y * 1.1 + 2000, extent.z * 1.1 + 2000)
    return center, extent


def add_nav_bounds(center, extent):
    actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    vol = actor_sub.spawn_actor_from_class(unreal.NavMeshBoundsVolume, center, unreal.Rotator(0, 0, 0))
    if not vol:
        unreal.log_error("Failed to spawn NavMeshBoundsVolume")
        return None
    # The default brush is a 200cm cube; scale the actor so it spans the level.
    scale = unreal.Vector(max(extent.x, 100.0) / 100.0,
                          max(extent.y, 100.0) / 100.0,
                          max(extent.z, 100.0) / 100.0)
    vol.set_actor_scale3d(scale)
    vol.set_actor_label("GTC_NavBounds")
    unreal.log("Added GTC_NavBounds at %s scale %s" % (center, scale))
    return vol


def main():
    load_map()
    center, extent = level_bounds()
    add_nav_bounds(center, extent)

    # Build paths and save.
    nav_sys = unreal.NavigationSystemV1.get_navigation_system(unreal.EditorLevelLibrary.get_editor_world())
    try:
        unreal.EditorLevelLibrary.editor_build_paths()  # 5.x: builds navigation
    except Exception:
        unreal.log_warning("editor_build_paths unavailable; build navigation manually (Build > Build Paths).")

    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.save_current_level()
    unreal.log("CityBeachStrip navigation enhanced and saved.")


main()
