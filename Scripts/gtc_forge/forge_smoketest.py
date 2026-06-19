# GT-caliber Character Forge — in-editor smoke test.
#
# Runs the Forge's auto-wiring analysis (GTC.Character.Rig + .Inspect) over the
# skeletal meshes in the project so you can confirm, across your real assets,
# that "drop a skeleton in -> backend connects every action" behaves. Results go
# to the Output Log (Window > Output Log; filter on "LogGtcCharacterForge").
#
# Run it from the editor (no MCP needed):
#   - Output Log is open, then in the Python console (Window > Python Console, or
#     Tools > Execute Python Script) run:
#         exec(open(r"<repo>/Scripts/gtc_forge/forge_smoketest.py").read())
#   - or point Tools > Execute Python Script at this file.
#
# Optional: set CONTENT_ROOT / MAX_MESHES below to scope the scan.

import unreal

CONTENT_ROOT = "/Game"   # narrow to e.g. "/Game/GTCaliberAssets" to scan less
MAX_MESHES = 12          # cap so the log stays readable


def _editor_world():
    # 5.x-safe way to get a world for execute_console_command.
    try:
        ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
        w = ues.get_editor_world()
        if w:
            return w
    except Exception as e:
        unreal.log_warning("Forge smoketest: UnrealEditorSubsystem world failed: {}".format(e))
    try:
        return unreal.EditorLevelLibrary.get_editor_world()  # deprecated fallback
    except Exception:
        return None


def _skeletal_meshes(root, limit):
    found = []
    try:
        paths = unreal.EditorAssetLibrary.list_assets(root, recursive=True, include_folder=False)
    except Exception as e:
        unreal.log_error("Forge smoketest: list_assets failed: {}".format(e))
        return found
    for p in paths:
        try:
            data = unreal.EditorAssetLibrary.find_asset_data(p)
            # Cheap class check before loading the asset.
            if data.asset_class_path.asset_name != "SkeletalMesh":
                continue
        except Exception:
            # Older API: fall back to loading and isinstance.
            obj = unreal.EditorAssetLibrary.load_asset(p)
            if not isinstance(obj, unreal.SkeletalMesh):
                continue
        found.append(p)
        if len(found) >= limit:
            break
    return found


def run():
    world = _editor_world()
    if world is None:
        unreal.log_error("Forge smoketest: could not resolve an editor world.")
        return

    meshes = _skeletal_meshes(CONTENT_ROOT, MAX_MESHES)
    unreal.log("Forge smoketest: {} skeletal mesh(es) under {}".format(len(meshes), CONTENT_ROOT))
    if not meshes:
        unreal.log_warning("No skeletal meshes found — widen CONTENT_ROOT.")
        return

    for path in meshes:
        # `path` is an object path like /Game/.../SK_X.SK_X already.
        unreal.log("==== Forge: {} ====".format(path))
        for cmd in ("GTC.Character.Rig {}".format(path),
                    "GTC.Character.Inspect {}".format(path)):
            unreal.SystemLibrary.execute_console_command(world, cmd)


run()
