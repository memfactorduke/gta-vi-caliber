#!/usr/bin/env python3
"""Combine all Caliber City layers into one applyable plan (offline; NO `import unreal`).

Merges: road tiles (city_roads_plan.json) + per-district buildings (city_buildings.json)
+ the Ocean Drive slice generated at TRUE spec coords (recenter off) + a city-wide
NavMeshBoundsVolume and PlayerStart. Writes city_plan.json (placement-plan format) for the
in-editor applier.

    python3 generate_city.py
"""

import json
import os

import generate_slice
from layout import Layout
import assets

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))


def main():
    cfg = json.load(open(os.path.join(HERE, "city_config.json")))
    out_dir = os.path.join(REPO, cfg["output_dir_rel"])
    z = cfg["ground_z"]; nsf = cfg["namespace_folder"]; pfx = cfg["label_prefix"]

    actors = []
    # 1) roads
    roads = json.load(open(os.path.join(out_dir, "city_roads_plan.json")))["actors"]
    actors += roads
    # 2) district buildings
    blds = json.load(open(os.path.join(out_dir, "city_buildings.json")))["actors"]
    actors += blds
    # 3) Ocean Drive slice at spec coords (recenter OFF in city mode)
    slice_cfg = json.load(open(os.path.join(HERE, "slice_config.json")))
    slice_cfg["recenter_to_origin"] = False
    od = generate_slice.build_actors(Layout(slice_cfg).solve(), slice_cfg)
    actors += od

    # 4) city-wide nav bounds + player start
    actors.append({"spawn": "class", "ref": assets.NAV_BOUNDS_CLASS, "is_li": False,
                   "folder": nsf + "/Nav", "label": pfx + "NavBounds",
                   "location": {"x": 0, "y": 0, "z": 200},
                   "rotation": {"pitch": 0, "yaw": 0, "roll": 0},
                   "scale": {"x": 6400, "y": 6400, "z": 100}})
    actors.append({"spawn": "class", "ref": assets.PLAYER_START_CLASS, "is_li": False,
                   "folder": nsf + "/Spawn", "label": pfx + "PlayerStart",
                   "location": {"x": 104000, "y": 24000, "z": z + 200},
                   "rotation": {"pitch": 0, "yaw": 90, "roll": 0},
                   "scale": {"x": 1, "y": 1, "z": 1}})

    by_folder = {}
    li = 0
    for a in actors:
        top = a["folder"].split("/")[2] if a["folder"].count("/") >= 2 else a["folder"]
        by_folder[top] = by_folder.get(top, 0) + 1
        if a.get("is_li"):
            li += 1
    plan = {"header": {"layer": "city", "foundation": "cesium",
                       "namespace_folder": nsf, "label_prefix": pfx, "ground_z": z,
                       "counts": {"actors": len(actors), "roads": len(roads),
                                  "district_buildings": len(blds), "ocean_drive": len(od),
                                  "level_instances": li}},
            "actors": actors}
    p = os.path.join(out_dir, "city_plan.json")
    json.dump(plan, open(p, "w"), indent=1, sort_keys=True)
    print("CITY PLAN: %d actors (%d LI level-instances)" % (len(actors), li))
    print("  by group:", ", ".join("%s=%d" % (k, v) for k, v in sorted(by_folder.items())))
    print("  roads=%d district_buildings=%d ocean_drive=%d" % (len(roads), len(blds), len(od)))
    print("wrote", os.path.relpath(p, REPO))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
