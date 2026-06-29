"""
anim_probe.py — determine whether a UWidgetAnimation (fade/scale entrance) can be authored
in Python on the kit pause menu, or whether the animations array is protected like WidgetTree.
Probe only (no compile/save). Run: py "<this path>"
Sentinels: GTC_ANIM_*
"""
import traceback
import unreal

PKG = "/Game/ThirdPersonKit/Blueprints/UserInterface/WB_Pause_menu_widget"

unreal.log("GTC_ANIM_START")
try:
    wbp = unreal.load_asset(PKG)
    unreal.log("GTC_ANIM_WBP ok=%s type=%s" % (wbp is not None, type(wbp).__name__))

    # 1) can we read the WBP animations array directly?
    try:
        anims = wbp.get_editor_property("animations")
        unreal.log("GTC_ANIM_ARRAY_READ ok len=%s" % len(anims))
    except Exception as e:
        unreal.log("GTC_ANIM_ARRAY_READ_FAIL " + repr(e))

    # 2) can we construct a WidgetAnimation object?
    anim = unreal.new_object(unreal.WidgetAnimation, wbp, "GTC_PauseIn")
    unreal.log("GTC_ANIM_CREATED " + anim.get_name())

    # 3) does it expose a movie scene (needed for tracks)?
    try:
        ms = anim.get_movie_scene()
        unreal.log("GTC_ANIM_MS get_movie_scene -> %s" % (ms is not None))
    except Exception as e:
        unreal.log("GTC_ANIM_MS_FAIL " + repr(e))
    try:
        ms2 = anim.get_editor_property("movie_scene")
        unreal.log("GTC_ANIM_MS2 movie_scene_prop -> %s" % (ms2 is not None))
    except Exception as e:
        unreal.log("GTC_ANIM_MS2_FAIL " + repr(e))

    # 4) are the MovieSceneSequenceExtensions helpers usable on it?
    try:
        tracks = unreal.MovieSceneSequenceExtensions.get_master_tracks(anim)
        unreal.log("GTC_ANIM_SEQEXT get_master_tracks -> %s" % str(tracks is not None))
    except Exception as e:
        unreal.log("GTC_ANIM_SEQEXT_FAIL " + repr(e))

    # 5) bindings array on the animation?
    try:
        b = anim.get_editor_property("animation_bindings")
        unreal.log("GTC_ANIM_BIND ok len=%s" % len(b))
    except Exception as e:
        unreal.log("GTC_ANIM_BIND_FAIL " + repr(e))

    unreal.log("GTC_ANIM_PROBE_DONE")
except Exception as e:
    unreal.log_error("GTC_ANIM_FAIL " + repr(e))
    unreal.log_error(traceback.format_exc())
