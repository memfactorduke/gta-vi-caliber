"""
GT-caliber — tag every world material with a physical surface so the impact-FX system
(Source/GTC_UE5/World/Surfaces/SurfaceImpact*.{h,cpp}) plays the right burst on every hit.

WHAT IT DOES
  1. Creates four UPhysicalMaterial assets under /Game/Surfaces/Physical:
        PM_GTC_Wood  -> SurfaceType1   PM_GTC_Metal    -> SurfaceType2
        PM_GTC_Glass -> SurfaceType3   PM_GTC_Concrete -> SurfaceType4
     (Creature/SurfaceType5 is carried by the pawn actor tag, not a material.)
  2. Classifies every /Game Material + MaterialInstanceConstant by name and assigns the
     matching physical material, then saves only what it changed. Idempotent.

PREREQUISITE — READ THIS
  The editor MUST have been launched AFTER Config/DefaultEngine.ini gained its
  +PhysicalSurfaces=(Type=SurfaceTypeN,...) lines. A stale editor (one started before that
  edit) leaves unreal.PhysicsSettings.physical_surfaces EMPTY and the Python binding only
  exposes SURFACE_TYPE_DEFAULT, so surface values 1..4 cannot be constructed and this script
  raises SurfacesNotLoaded. If that happens: relaunch the editor on this project, then re-run.

HOW TO RUN (in the live editor, via MCP/Python — never spawn a second editor):
  exec(open(r"<repo>/Scripts/gtc_surfaces/apply_physical_materials.py").read())
  apply(dry_run=True)   # report classification counts, change nothing
  apply(dry_run=False)  # create phys mats + assign + save
"""

import unreal

PHYS_FOLDER = "/Game/Surfaces/Physical"

# (asset name, SurfaceTypeN index, candidate attribute names on unreal.PhysicalSurface once
# the names load). Creature/SurfaceType5 is a pawn actor tag, not a material, so it's omitted.
PHYS_SPEC = [
    ("PM_GTC_Wood", 1, ["SURFACE_TYPE1", "WOOD"]),
    ("PM_GTC_Metal", 2, ["SURFACE_TYPE2", "METAL"]),
    ("PM_GTC_Glass", 3, ["SURFACE_TYPE3", "GLASS"]),
    ("PM_GTC_Concrete", 4, ["SURFACE_TYPE4", "CONCRETE"]),
    ("PM_GTC_Asphalt", 6, ["SURFACE_TYPE6", "ASPHALT"]),
    ("PM_GTC_Brick", 7, ["SURFACE_TYPE7", "BRICK"]),
    ("PM_GTC_Ceramic", 8, ["SURFACE_TYPE8", "CERAMIC"]),
    ("PM_GTC_Rubber", 9, ["SURFACE_TYPE9", "RUBBER"]),
    ("PM_GTC_Vegetation", 10, ["SURFACE_TYPE10", "VEGETATION"]),
    ("PM_GTC_Ice", 11, ["SURFACE_TYPE11", "ICE"]),
    ("PM_GTC_Leather", 12, ["SURFACE_TYPE12", "LEATHER"]),
    ("PM_GTC_Paper", 13, ["SURFACE_TYPE13", "PAPER"]),
    ("PM_GTC_Water", 14, ["SURFACE_TYPE14", "WATER"]),
]
IDX_TO_BUCKET = {1: "Wood", 2: "Metal", 3: "Glass", 4: "Concrete", 6: "Asphalt", 7: "Brick",
                 8: "Ceramic", 9: "Rubber", 10: "Vegetation", 11: "Ice", 12: "Leather",
                 13: "Paper", 14: "Water"}

# keyword -> surface bucket. ORDER MATTERS: the most specific buckets are checked first so a
# keyword that belongs to one surface isn't stolen by the broad Concrete net at the bottom.
# Keys are deliberately conservative to avoid false positives (e.g. "sea" is omitted because it
# is a substring of "seat"; bare "tree" is omitted because of "street").
RULES = [
    ("Glass", ["glass", "window", "windshield", "windscreen", "mirror", "screen", "bottle", "lens"]),
    ("Water", ["water", "ocean", "pool", "puddle", "splash", "aqua"]),
    ("Vegetation", ["grass", "foliage", "vegetation", "hedge", "bush", "shrub", "ivy", "fern",
                    "palm", "leaf", "leaves", "branch", "frond", "canopy", "flower", "moss",
                    "vine", "plant"]),
    ("Ceramic", ["ceramic", "porcelain", "toilet", "sink", "sanitary", "pottery", "bathroom",
                 "tiles", "_tile"]),
    ("Brick", ["brick", "masonry"]),
    ("Asphalt", ["asphalt", "tarmac", "road", "tarvia"]),
    ("Rubber", ["rubber", "tyre", "tread", "hose", "_tire", "tire_"]),
    ("Wood", ["wood", "plank", "timber", "lumber", "oak", "pine", "crate", "fence_wood",
              "furniture", "plywood", "log_", "bark", "trunk"]),
    ("Leather", ["leather", "sofa", "couch", "upholst", "luggage", "seat"]),
    ("Paper", ["paper", "cardboard", "poster", "newspaper", "carton", "book", "trash"]),
    # "ice" alone is a substring of police/office/service/invoice/juice — use only safe forms.
    ("Ice", ["frozen", "frost", "icy", "iceberg", "_ice", "ice_"]),
    ("Metal", ["metal", "steel", "iron", "chrome", "alumin", "carpaint", "car_paint", "vehicle",
               "chassis", "rim", "wheel", "sign", "rail", "grate", "pipe", "beam", "car_",
               "truck", "bus_", "ladder", "pole", "tank", "veh", "tin_"]),
    ("Concrete", ["concrete", "cement", "stone", "plaster", "sidewalk", "pavement", "curb", "kerb",
                  "wall", "building", "facade", "floor", "rock", "granite", "marble", "stucco",
                  "ground", "gravel", "rubble", "block"]),
]


class SurfacesNotLoaded(RuntimeError):
    pass


def _surface_value(attr_names):
    """Resolve a PhysicalSurface enum value by trying each candidate attribute name."""
    for a in attr_names:
        v = getattr(unreal.PhysicalSurface, a, None)
        if v is not None:
            return v
    return None


def _ensure_surfaces_loaded():
    ps = unreal.get_default_object(unreal.PhysicsSettings)
    names = ps.get_editor_property("physical_surfaces")
    if len(names) == 0 or _surface_value(["SURFACE_TYPE1", "WOOD"]) is None:
        raise SurfacesNotLoaded(
            "PhysicsSettings has no named surfaces and PhysicalSurface only exposes DEFAULT. "
            "Relaunch the editor AFTER the DefaultEngine.ini PhysicalSurfaces edit, then re-run.")


def classify(name):
    n = str(name).lower()
    for surf, keys in RULES:
        for k in keys:
            if k in n:
                return surf
    return None


def _all_game_materials():
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    out = []
    for cls in ("Material", "MaterialInstanceConstant"):
        a = ar.get_assets_by_class(unreal.TopLevelAssetPath("/Script/Engine", cls), False)
        out += [x for x in a if str(x.package_name).startswith("/Game")
                and not str(x.package_name).startswith(PHYS_FOLDER)]
    return out


def _create_phys_mats():
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    made = {}
    for name, idx, attrs in PHYS_SPEC:
        path = f"{PHYS_FOLDER}/{name}"
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            pm = unreal.EditorAssetLibrary.load_asset(path)
        else:
            pm = tools.create_asset(name, PHYS_FOLDER, unreal.PhysicalMaterial,
                                    unreal.PhysicalMaterialFactoryNew())
        sv = _surface_value(attrs)
        pm.set_editor_property("surface_type", sv)
        unreal.EditorAssetLibrary.save_asset(path)
        made[IDX_TO_BUCKET[idx]] = pm
        unreal.log(f"[gtc-surfaces] phys mat {path} surface={int(pm.get_editor_property('surface_type').value)}")
    return made


def apply(dry_run=True):
    assets = _all_game_materials()
    from collections import Counter
    hits, unmatched = Counter(), 0
    plan = []  # (asset_data, bucket)
    for a in assets:
        s = classify(a.asset_name)
        if s:
            hits[s] += 1
            plan.append((a, s))
        else:
            unmatched += 1
    unreal.log(f"[gtc-surfaces] total={len(assets)} classified={sum(hits.values())} "
               f"{dict(hits)} unmatched={unmatched}")
    if dry_run:
        return {"total": len(assets), "classified": dict(hits), "unmatched": unmatched}

    _ensure_surfaces_loaded()
    mats = _create_phys_mats()
    changed = 0
    for a, bucket in plan:
        path = str(a.package_name)
        obj = unreal.EditorAssetLibrary.load_asset(path)
        if obj is None:
            continue
        current = obj.get_editor_property("phys_material")
        if current == mats[bucket]:
            continue
        obj.set_editor_property("phys_material", mats[bucket])
        unreal.EditorAssetLibrary.save_asset(path)
        changed += 1
    unreal.log(f"[gtc-surfaces] assigned phys materials to {changed} assets")
    return {"changed": changed, "classified": dict(hits), "unmatched": unmatched}


def audit_level_coverage(sample=4000):
    """The real "is every material tagged?" check: walk the CURRENT level's static-mesh actors,
    collect the materials actually on shootable geometry, and report how many resolve to a surface
    (a physical material OR a Surface.*/Creature actor tag) vs fall to the Default puff. Read-only.

    Returns {checked, tagged_physmat, tagged_actor, untagged, untagged_names(sample)}.
    """
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = eas.get_all_level_actors()
    # Which materials carry one of our physical materials.
    phys = set()
    for name, idx, _ in PHYS_SPEC:
        o = unreal.EditorAssetLibrary.load_asset(f"{PHYS_FOLDER}/{name}")
        if o is not None:
            phys.add(o)

    def actor_surface_tagged(a):
        for t in a.tags:
            ts = str(t)
            if ts == "Creature" or ts.startswith("Surface."):
                return True
        return False

    checked = tagged_pm = tagged_actor = 0
    untagged = []
    for a in actors[:sample]:
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        if not comps:
            continue
        atag = actor_surface_tagged(a)
        for c in comps:
            for i in range(c.get_num_materials()):
                m = c.get_material(i)
                if m is None:
                    continue
                checked += 1
                if atag:
                    tagged_actor += 1
                elif m.get_editor_property("phys_material") in phys:
                    tagged_pm += 1
                else:
                    if len(untagged) < 4000:
                        untagged.append(m.get_name())
    from collections import Counter
    cov = (tagged_pm + tagged_actor)
    pct = round(100.0 * cov / checked, 1) if checked else 0.0
    top_untagged = Counter(untagged).most_common(40)
    unreal.log(f"[gtc-audit] checked={checked} tagged(physmat)={tagged_pm} "
               f"tagged(actor)={tagged_actor} untagged={len(untagged)} coverage={pct}%")
    return {"checked": checked, "tagged_physmat": tagged_pm, "tagged_actor": tagged_actor,
            "untagged": len(untagged), "coverage_pct": pct, "top_untagged": top_untagged}
