"""Import a Tripo Studio model into the GTC project (macOS / UE 5.7).

WHY THIS EXISTS
  The official "Tripo DCC Bridge for Unreal Engine" is Windows-only: every build
  in the download ships a Win64 .dll, the .uplugin declares
  SupportedTargetPlatforms = ["Win64"], and its Build.cs hard-links Win64 libs
  (IXWebSocket.lib, zlibstatic.lib, ws2_32/crypt32). It cannot load or even
  compile on this Mac/UE5.7 editor. This script is the native replacement for the
  bridge's "Send to Unreal" step.

MAC WORKFLOW (replaces the bridge)
  1. Generate your model in Tripo Studio (https://www.tripo3d.ai) in the browser.
  2. Export / download it. Recommended formats, in order:
        - GLB  (Tripo native; carries PBR materials + textures; static props)
        - FBX Binary (use this for rigged/animated meshes)
     Save it to ~/Downloads (or anywhere) and set MODEL_FILE below.
  3. Either drag the file into the Content Browser, OR run this in the
     already-running editor:
        py "<repo>/Scripts/gtc_tripo/import_tripo_model.py"

CRASH NOTE (this project, UE 5.7): scripted FBX import through Interchange has
crashed this editor before. For .fbx we therefore disable the Interchange FBX
feature flag first so the legacy FBX importer is used (the proven-safe path that
imported SK_Wooden / the soldier rifle). .glb uses the glTF importer, which is
fine as-is.
"""

import os
import unreal

# ---- CONFIG ----------------------------------------------------------------
MODEL_FILE = os.path.expanduser("~/Downloads/tripo_model.glb")  # your Tripo export
DEST = "/Game/Tripo"            # Content Browser folder to import into
AS_SKELETAL = False             # False = static prop (typical Tripo); True = rigged mesh
UNIFORM_SCALE = 1.0             # bump toward ~100 if the mesh imports tiny (m vs cm)
# ----------------------------------------------------------------------------


def _use_legacy_fbx_importer():
    """Route .fbx through the legacy importer (avoids the UE5.7 Interchange crash)."""
    try:
        unreal.SystemLibrary.execute_console_command(
            None, "Interchange.FeatureFlags.Import.FBX 0")
        unreal.log("[tripo] Interchange FBX flag disabled -> legacy FBX importer.")
    except Exception as e:  # noqa: BLE001 - best-effort guard, import still attempted
        unreal.log_warning("[tripo] could not toggle Interchange FBX flag: %s" % e)


def _fbx_options():
    opts = unreal.FbxImportUI()
    opts.import_mesh = True
    opts.import_as_skeletal = AS_SKELETAL
    opts.import_materials = True
    opts.import_textures = True
    opts.import_animations = AS_SKELETAL
    opts.create_physics_asset = AS_SKELETAL
    opts.mesh_type_to_import = (
        unreal.FBXImportType.FBXIT_SKELETAL_MESH if AS_SKELETAL
        else unreal.FBXImportType.FBXIT_STATIC_MESH)
    data = (opts.skeletal_mesh_import_data if AS_SKELETAL
            else opts.static_mesh_import_data)
    data.set_editor_property("import_uniform_scale", float(UNIFORM_SCALE))
    data.set_editor_property("convert_scene", True)
    if not AS_SKELETAL:
        data.set_editor_property("combine_meshes", True)
        data.set_editor_property("generate_lightmap_u_vs", True)
    return opts


def main():
    if not os.path.exists(MODEL_FILE):
        unreal.log_error(
            "[tripo] MODEL_FILE not found: %s -- export from Tripo Studio and set "
            "MODEL_FILE in this script's CONFIG." % MODEL_FILE)
        return

    ext = os.path.splitext(MODEL_FILE)[1].lower()
    task = unreal.AssetImportTask()
    task.filename = MODEL_FILE
    task.destination_path = DEST
    task.automated = True
    task.replace_existing = True
    task.save = True

    if ext == ".fbx":
        _use_legacy_fbx_importer()
        task.options = _fbx_options()
    elif ext in (".glb", ".gltf"):
        # glTF importer handles materials/textures with its own defaults.
        unreal.log("[tripo] importing via glTF importer (%s)." % ext)
    else:
        unreal.log_warning(
            "[tripo] unusual extension '%s'; attempting a default import." % ext)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    made = list(task.get_objects_or_redirectors())
    if made:
        unreal.log("[tripo] imported into %s : %s"
                   % (DEST, [o.get_name() for o in made]))
    else:
        unreal.log_warning(
            "[tripo] nothing imported -- check the Output Log; ensure the path "
            "and filename use plain ASCII with no spaces/special characters.")


main()
