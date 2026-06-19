"""Verified asset catalog + selection rules for the Ocean Drive slice.

Every path here was confirmed on disk under
Content/GTCaliberAssets/Content/CityBeachStrip/ (see the M1 verification pass).
Centralizing them is the single place to fix the spec's asset-name drift
(e.g. fairy-light palms exist ONLY as 01/02/03/06; the awning hero is
SM_Awning_01_3m). Pure data + tiny helpers, no Unreal import.

`is_li=True` means the asset is a .umap *Level Instance* (LI_Resort_*, LI_Building_*,
LI_MetalTableSet_*): the apply step must spawn it as a LevelInstance actor, not via
add_to_scene_from_asset on a static mesh. `footprint` is the nominal reserved
footprint [width_x, depth_y] in cm used by the solver to size plots; the building
rectangle is always derived as a subset of its plot, so the reservation is what the
no-overlap guarantee is proven against.
"""

GAME_ROOT = "/Game/GTCaliberAssets/Content/CityBeachStrip"


def _p(rel):
    return GAME_ROOT + "/" + rel


# --- Buildings, by role (FULL palette — every building asset on disk) --------
# Footprints are nominal hints (multiples of 800); the solver reserves the
# plot-derived rect, so exact footprint accuracy is not required for no-overlap.
_LI_BUILDING_IDS = (1, 2, 3, 4, 5, 9, 10, 11, 14, 15, 16, 17, 18, 19, 20,
                    24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 35, 37)

BUILDINGS = {
    "resort": [   # signature hotels — anchor the strip ends
        {"path": _p("Maps/Buildings/LI_Resort_0%d" % i), "is_li": True, "footprint": [6400, 6400]}
        for i in (1, 2, 3)
    ],
    "midrise": [  # full LI_Building variety — the party-wall Deco row + mid-rise columns
        {"path": _p("Maps/Buildings/LI_Building_%02d" % i), "is_li": True, "footprint": [4000, 4800]}
        for i in _LI_BUILDING_IDS
    ],
    "background": [  # lightweight static-mesh silhouettes for back rows / far fill
        {"path": _p("Meshes/BackgroundBuildings/SM_CityBeach_BackgroundBuilding_0%d" % i),
         "is_li": False, "footprint": [3200, 4000]}
        for i in (1, 2, 3, 4, 5, 6, 7)
    ],
    "highrise": [  # skyscrapers — reserved for the Downtown/Brickell district (not the beach slice)
        {"path": _p("Meshes/BackgroundBuildings/SM_Skyscraper_0%d" % i), "is_li": False, "footprint": [4800, 4800]}
        for i in (1, 2, 3, 4)
    ],
}

# --- Roads & sidewalks, by class -------------------------------------------
ROADS = {
    "arterial":  {"tile": _p("Meshes/Street/SM_Street_8m_02"),   "sidewalk": _p("Meshes/Street/SM_Street_Sidewalk_8x4m")},
    "collector": {"tile": _p("Meshes/Street/SM_Street_8m_02_b"), "sidewalk": _p("Meshes/Street/SM_Street_Sidewalk_8x2_5m_02")},
    "local":     {"tile": _p("Meshes/Street/SM_Street_8m_01"),   "sidewalk": _p("Meshes/Street/SM_Street_Sidewalk_8x2m")},
    "alley":     {"tile": _p("Meshes/Street/SM_BackStreet_01_8m"), "sidewalk": None},
}
CROSSWALK = _p("Meshes/Street/SM_CrossRoad01")
STREET_END = _p("Meshes/Street/SM_Street_End")

# --- Set-dressing (furniture-lane props) -----------------------------------
DRESSING = {
    "palm_lit":   [_p("Meshes/Foliage/Trees/PalmTrees/PalmTree_Lit/SM_PalmTree_0%d_FairyLights" % i) for i in (1, 2, 3, 6)],
    "palm":       [_p("Meshes/Foliage/Trees/PalmTrees/SM_PalmTree_0%d" % i) for i in (1, 2, 3, 4)],
    "lamp":       _p("Meshes/LampPost/SM_LampPostBeach_01"),
    "umbrella":   _p("Meshes/Umbrella/SM_Umbrella"),
    "table_set":  {"path": _p("Meshes/OutdoorFurniture/LI_MetalTableSet_Umbrella"), "is_li": True},
    "awning":     _p("Meshes/Architecture/SM_Awning_01_3m"),
    "sign_neon":  _p("Meshes/Signs/Neon/SM_OpenNeonSign"),
    "sign_hotel": _p("Meshes/Signs/SM_SunsetHotel_Sign"),
}

# --- Lighting / sky rig (engine classes, spawned by class) ------------------
SKY_CLASSES = {
    "sun":       "/Script/Engine.DirectionalLight",
    "sky_atmo":  "/Script/Engine.SkyAtmosphere",
    "sky_light": "/Script/Engine.SkyLight",
    "fog":       "/Script/Engine.ExponentialHeightFog",
}
PLAYER_START_CLASS = "/Script/Engine.PlayerStart"
NAV_BOUNDS_CLASS = "/Script/NavigationSystem.NavMeshBoundsVolume"
GROUND_PLANE = "/Engine/BasicShapes/Plane"
# POI markers for the GTC place registry (kinds: home/diner/bar/park/office/gym/street/restroom)
PLACE_MARKER_CLASS = "/Script/GTC_UE5.GTCPlaceMarker"


def pick_building(rng, role):
    """Deterministic pick from a role pool (rng is a seeded random.Random)."""
    pool = BUILDINGS[role]
    return pool[rng.randrange(len(pool))]


def pick_palm(rng, lit=True):
    pool = DRESSING["palm_lit"] if lit else DRESSING["palm"]
    return pool[rng.randrange(len(pool))]
