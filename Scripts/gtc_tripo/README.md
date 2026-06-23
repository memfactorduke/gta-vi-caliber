# Tripo3D → GTC (macOS / UE 5.7)

## The bridge does not work on this machine

The downloaded **Tripo DCC Bridge for Unreal Engine** (`Tripo3d_UE_Bridge-latest`)
is **Windows-only** and cannot be installed here. Verified from the package:

- Prebuilt builds ship only `Binaries/Win64/UnrealEditor-Tripo3DUEBridge.dll`
  (a Windows DLL — a Mac editor can't load it).
- Every `Tripo3DUEBridge.uplugin` declares `"SupportedTargetPlatforms": ["Win64"]`.
- The from-source `Tripo3d_UE_Bridge/` ships only
  `ThirdParty/IXWebSocket/Lib/Win64/IXWebSocket.lib` (no Mac `.a`), and its
  `Build.cs` hard-codes Win64 paths (`zlibstatic.lib`, `ws2_32`, `crypt32`) and
  throws `BuildException` if they're missing — so it can't even compile on macOS.

Dropping it into `Plugins/` would, at best, do nothing and, at worst, break the
`GTC_UE5` C++ build (the module is `EnabledByDefault` and `bPrecompile`, so
UnrealBuildTool would try to compile it). It is therefore **not** installed.

To actually use the bridge, run this project's editor on **Windows UE 5.7** and
copy `Tripo3DUEBridge-UE5.7-Win64` into that machine's `Plugins/` folder, then
`Window → Tripo Bridge`.

## Native Mac replacement (what to do instead)

The bridge's only job is to land a Tripo model in the project. On Mac, do that
directly:

1. Generate the model in **Tripo Studio** (browser): https://www.tripo3d.ai
2. **Export / download** it — **GLB** (native, carries PBR materials; best for
   static props) or **FBX Binary** (for rigged/animated meshes). Save to
   `~/Downloads`.
3. Import it, either:
   - drag the file into the **Content Browser**, or
   - set `MODEL_FILE` in `import_tripo_model.py` and run it in the live editor:
     `py "<repo>/Scripts/gtc_tripo/import_tripo_model.py"`

`import_tripo_model.py` imports into `/Game/Tripo`, and for `.fbx` it first
disables the Interchange FBX feature flag so the **legacy** importer is used —
the proven-safe path on this UE 5.7 editor (scripted FBX-via-Interchange has
crashed it before). Keep paths/filenames ASCII with no spaces.
