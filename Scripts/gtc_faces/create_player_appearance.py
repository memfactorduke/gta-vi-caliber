"""Create + populate DA_PlayerAppearance (a UGTCAppearanceSet) from the assets the
project already ships, so the in-game PLAYER character creator works IMMEDIATELY —
no imported character art required yet. (This is a re-creatable repro of the asset
that was authored live via the editor MCP; re-running just updates it in place.)

NOTE: only AGTCPlayerCharacter defaults its AppearanceSet to this path. The NPC
crowd (AGTCCitizen) currently leaves its AppearanceSet UNSET and falls back to
hardcoded mannequin paths — to make NPCs share this wardrobe too, point the citizen
Blueprint's AppearanceSet at this asset (not done automatically).

WHAT IT FILLS (with what exists today):
  - BodyMeshes:   SKM_Manny_Simple, SKM_Quinn_Simple   (deterministic per-seed body)
  - BodyAnimClass: ABP_Unarmed                          (locomotion)
  - SkinTones:    a 6-stop palette                      (crowd + creator skin variety)
  Heads / hair / outfits are left EMPTY on purpose — drop a modular character pack
  (skinned to the UE5 Mannequin skeleton) into those arrays later and both the
  player and every NPC gain faces/hair/clothes with zero extra code.

HOW TO RUN (in the ALREADY-RUNNING editor — do NOT launch a new one):
  1. First trigger a Live Coding compile (Ctrl+Alt+F11) so this session's C++ is
     loaded (the creator + the player/NPC wardrobe wiring).
  2. Open the editor console (the ` ~ ` key, or Output Log > Cmd) and run:
         py "<repo>/Scripts/gtc_faces/create_player_appearance.py"
     (absolute path). Re-running is safe — it updates the existing asset in place.

The asset is created at the exact path AGTCPlayerCharacter defaults its AppearanceSet
to, so the player creator needs no further wiring once it exists.
"""

import unreal

PKG = "/Game/GTCaliberAssets/Content/Characters"
NAME = "DA_PlayerAppearance"
ASSET_PATH = "%s/%s" % (PKG, NAME)

MANNY = "/Game/GTCaliberAssets/Content/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"
QUINN = "/Game/GTCaliberAssets/Content/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"
ABP = "/Game/GTCaliberAssets/Content/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"

# A neutral spread of skin tones (light -> deep). Tuned to read as distinct people.
SKIN = [
    unreal.LinearColor(0.96, 0.80, 0.69, 1.0),
    unreal.LinearColor(0.91, 0.72, 0.58, 1.0),
    unreal.LinearColor(0.80, 0.60, 0.46, 1.0),
    unreal.LinearColor(0.65, 0.46, 0.33, 1.0),
    unreal.LinearColor(0.48, 0.33, 0.22, 1.0),
    unreal.LinearColor(0.32, 0.21, 0.14, 1.0),
]


def load_or_create():
    if unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        unreal.log("DA_PlayerAppearance exists - updating in place.")
        return unreal.EditorAssetLibrary.load_asset(ASSET_PATH)

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.GTCAppearanceSet)
    asset = tools.create_asset(NAME, PKG, unreal.GTCAppearanceSet, factory)
    unreal.log("Created %s" % ASSET_PATH)
    return asset


def must_load(path):
    obj = unreal.EditorAssetLibrary.load_asset(path)
    if obj is None:
        unreal.log_warning("MISSING asset (skipped): %s" % path)
    return obj


def main():
    da = load_or_create()
    if da is None:
        unreal.log_error("Could not create/load %s - is the GTC_UE5 module loaded "
                         "(did you Live Coding compile first)?" % ASSET_PATH)
        return

    bodies = [m for m in (must_load(MANNY), must_load(QUINN)) if m is not None]
    da.set_editor_property("body_meshes", bodies)

    abp = unreal.load_object(None, ABP)
    if abp is not None:
        da.set_editor_property("body_anim_class", abp)
    else:
        unreal.log_warning("ABP_Unarmed class not found at %s" % ABP)

    da.set_editor_property("skin_tones", SKIN)

    # Heads / hair / outfits intentionally left empty for now (see module docstring).

    unreal.EditorAssetLibrary.save_asset(ASSET_PATH)
    unreal.log("DA_PlayerAppearance saved: %d bodies, %d skin tones. "
               "Player character creator now uses this wardrobe." % (len(bodies), len(SKIN)))


main()
