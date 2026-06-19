# apply_in_editor.py — spawn the generated Ocean Drive slice into the OPEN level.
#
# Runs INSIDE the Unreal editor (it uses the `unreal` module + reads placement_plan.json
# from disk). This is the reliable path that doesn't depend on the MCP client.
#
#   1. Open the NEW World Partition map you want to build into (NOT CityBeachStrip).
#   2. (Cesium base) run Content/GTCaliberAssets/Content/Python/build_miami.py once.
#   3. Output Log > switch "Cmd" to "Python", then run:
#         py "<repo>/Scripts/gtc_citygen/apply_in_editor.py"
#      or paste this whole file into the Python console.
#
# SAFE: idempotent. It first deletes ONLY actors in our namespace (folder
# GTC_CityGen/OceanDrive/* or label prefix GTC_CG_OceanDrive_), then respawns from the
# plan. Your hand-placed / C++ / Cesium actors are never touched. Leaves everything
# UNSAVED for review (SAVE=False). Re-run any time after regenerating the plan.

import json
import os
import unreal

# ---- knobs ----
PLAN_FILE = "city_plan.json"   # which plan to apply (city_plan.json = full city; placement_plan.json = Ocean Drive slice)
PURGE = True          # delete our previous namespace actors first (idempotent re-apply)
SAVE = False          # leave unsaved for review; set True once you're happy
LIMIT = 0             # 0 = all actors; set e.g. 25 for a quick pilot
ALLOW_CITYBEACHSTRIP = False   # safety: refuse to spawn into the existing main map
# ---------------

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)


def plan_path():
    # /Game/GTCaliberAssets/Content/CityGen == <project>/Content/GTCaliberAssets/Content/CityGen
    base = unreal.Paths.project_content_dir()
    return os.path.join(base, "GTCaliberAssets", "Content", "CityGen", PLAN_FILE)


def current_world_name():
    try:
        return ues.get_editor_world().get_path_name()
    except Exception:
        return "<unknown>"


def vec(d):
    return unreal.Vector(float(d["x"]), float(d["y"]), float(d["z"]))


def rot(d):
    return unreal.Rotator(pitch=float(d["pitch"]), yaw=float(d["yaw"]), roll=float(d["roll"]))


def resolve_class(ref):
    try:
        c = unreal.load_class(None, ref)
        if c:
            return c
    except Exception:
        pass
    return getattr(unreal, ref.split(".")[-1], None)


def spawn_one(a):
    loc, rt = vec(a["location"]), rot(a["rotation"])
    spawn = a["spawn"]
    if spawn == "class":
        cls = resolve_class(a["ref"])
        if cls is None:
            raise RuntimeError("unresolved class %s" % a["ref"])
        actor = eas.spawn_actor_from_class(cls, loc, rt)
    elif spawn == "level_instance":
        world_asset = unreal.EditorAssetLibrary.load_asset(a["ref"])
        if world_asset is None:
            raise RuntimeError("could not load level asset %s" % a["ref"])
        actor = eas.spawn_actor_from_class(unreal.LevelInstance, loc, rt)
        # set_world_asset is the documented loader method (returns bool); fall back to the
        # world_asset property if a given 5.x build lacks it.
        ok = False
        try:
            ok = bool(actor.set_world_asset(world_asset))
        except Exception:
            try:
                actor.set_editor_property("world_asset", world_asset)
                ok = True
            except Exception as e:
                unreal.log_warning("LI world_asset set failed for %s: %s" % (a["ref"], e))
        if not ok:
            unreal.log_warning("LI %s spawned but world_asset may be unset" % a["ref"])
    else:  # "asset" -> static mesh actor
        mesh = unreal.EditorAssetLibrary.load_asset(a["ref"])
        if mesh is None:
            raise RuntimeError("could not load mesh %s" % a["ref"])
        actor = eas.spawn_actor_from_class(unreal.StaticMeshActor, loc, rt)
        actor.static_mesh_component.set_static_mesh(mesh)

    s = a.get("scale", {"x": 1, "y": 1, "z": 1})
    actor.set_actor_scale3d(unreal.Vector(float(s["x"]), float(s["y"]), float(s["z"])))
    actor.set_actor_label(a["label"])
    try:
        actor.set_folder_path(a["folder"])
    except Exception:
        pass
    for k, v in (a.get("props") or {}).items():
        try:
            actor.set_editor_property(k, v)
        except Exception as e:
            unreal.log_warning("prop %s on %s failed: %s" % (k, a["label"], e))
    return actor


def purge(namespace, prefix):
    n = 0
    for act in list(eas.get_all_level_actors()):
        try:
            folder = str(act.get_folder_path())
            label = act.get_actor_label()
        except Exception:
            continue
        if folder.startswith(namespace) or label.startswith(prefix):
            eas.destroy_actor(act)
            n += 1
    return n


def main():
    world = current_world_name()
    unreal.log("=== GTC CityGen apply -> %s ===" % world)
    if (not ALLOW_CITYBEACHSTRIP) and "CityBeachStrip" in world:
        unreal.log_error("Refusing to spawn into CityBeachStrip. Open the new WP map first "
                         "(or set ALLOW_CITYBEACHSTRIP=True).")
        return

    path = plan_path()
    if not os.path.exists(path):
        unreal.log_error("placement_plan.json not found at %s — run generate_slice.py first." % path)
        return
    plan = json.load(open(path))
    hdr = plan["header"]
    # Purge by the shared roots so a re-apply clears every gtc_citygen actor (city + slice),
    # regardless of which sub-namespace (CaliberCity / OceanDrive) or label prefix it used.
    ns = "GTC_CityGen"
    pfx = "GTC_CG"
    unreal.log("plan: %d actors, foundation=%s, world_offset=%s, validation=%s"
               % (len(plan["actors"]), hdr.get("foundation"), hdr.get("world_offset"), hdr.get("validation")))
    if hdr.get("validation") != "PASS":
        unreal.log_error("plan validation != PASS; aborting.")
        return

    purged = purge(ns, pfx) if PURGE else 0
    unreal.log("purged %d prior namespace actors" % purged)

    actors = plan["actors"]
    if LIMIT:
        actors = actors[:LIMIT]
    spawned, errors = 0, []
    with unreal.ScopedSlowTask(len(actors), "Spawning Ocean Drive slice") as task:
        task.make_dialog(True)
        for a in actors:
            if task.should_cancel():
                break
            task.enter_progress_frame(1, a["label"])
            try:
                spawn_one(a)
                spawned += 1
            except Exception as e:
                errors.append((a["label"], a["ref"], str(e)))

    unreal.log("=== spawned %d / %d (errors: %d) ===" % (spawned, len(actors), len(errors)))
    for lbl, ref, err in errors[:25]:
        unreal.log_warning("  FAIL %s (%s): %s" % (lbl, ref, err))
    if SAVE:
        les.save_current_level()
        unreal.log("saved current level")
    else:
        unreal.log("left UNSAVED for review (set SAVE=True to persist)")


main()
