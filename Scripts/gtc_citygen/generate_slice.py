#!/usr/bin/env python3
"""Ocean Drive slice generator (offline; NO `import unreal`).

    python3 generate_slice.py [--config slice_config.json] [--out <dir>]

Solves the layout, validates no-overlap, and writes three artifacts:
  placement_plan.json    ordered actors {spawn, asset_path/class, location, rotation, scale, folder, label, ...}
  sidewalk_graph.json    {"sidewalks":[{n,p}], "crosswalks":[{n,p}]}  (feeds build_lane_graph.py)
  validation_report.json {status: PASS|FAIL, counts, violations}

Deterministic: same seed => byte-identical placement_plan.json. The apply driver
refuses to touch the editor unless validation_report.status == "PASS".
"""

import argparse
import json
import os

from layout import Layout, rect_center
from validate_overlap import validate
import assets

HERE = os.path.dirname(os.path.abspath(__file__))
SCHEMA_VERSION = 1


def origin_offset(cfg):
    """Shift applied to emitted coords so the slice center sits at world origin
    (the Cesium georeference). Solver geometry stays in true spec coords."""
    if cfg.get("recenter_to_origin"):
        return (-(cfg["x_min"] + cfg["x_max"]) // 2, -(cfg["y_min"] + cfg["y_max"]) // 2)
    return (0, 0)


def _xform(loc, yaw=0, scale=(1, 1, 1)):
    return {
        "location": {"x": int(loc[0]), "y": int(loc[1]), "z": int(loc[2])},
        "rotation": {"pitch": 0, "yaw": int(yaw), "roll": 0},
        "scale": {"x": round(scale[0], 4), "y": round(scale[1], 4), "z": round(scale[2], 4)},
    }


def build_actors(lay, cfg):
    nsf = cfg["namespace_folder"]
    pfx = cfg["label_prefix"]
    z = cfg["ground_z"]
    ox, oy = origin_offset(cfg)
    actors = []

    def add(spawn, ref, loc, sub, name, yaw=0, scale=(1, 1, 1), is_li=False, props=None):
        a = {"spawn": spawn, "folder": nsf + "/" + sub, "label": pfx + name}
        a["ref"] = ref            # asset path or class path
        a["is_li"] = is_li
        a.update(_xform((loc[0] + ox, loc[1] + oy, loc[2]), yaw, scale))
        if props:
            a["props"] = props
        actors.append(a)

    cxw = (cfg["x_min"] + cfg["x_max"]) // 2
    cyw = (cfg["y_min"] + cfg["y_max"]) // 2

    # World base. With the Cesium foundation, real Miami terrain + CesiumSunSky are set
    # up by the cesium step (build_miami.py) — so we DON'T spawn a flat plane or basic
    # sky rig that would fight it. Only the 'flat' foundation gets them here.
    if cfg.get("foundation", "flat") != "cesium":
        sx = (cfg["x_max"] - cfg["x_min"]) / 100.0
        sy = (cfg["y_max"] - cfg["y_min"]) / 100.0
        add("asset", assets.GROUND_PLANE, (cxw, cyw, 0), "Ground", "Ground", scale=(sx, sy, 1))
        add("class", assets.SKY_CLASSES["sun"], (cxw, cyw, 30000), "Sky", "Sun", yaw=45)
        add("class", assets.SKY_CLASSES["sky_atmo"], (cxw, cyw, 0), "Sky", "SkyAtmosphere")
        add("class", assets.SKY_CLASSES["sky_light"], (cxw, cyw, 20000), "Sky", "SkyLight")
        add("class", assets.SKY_CLASSES["fog"], (cxw, cyw, 1000), "Sky", "HeightFog")

    # Roads: one scaled tile per segment (between intersections). Correctness rests on
    # the ROW rects + graphs, not these greybox meshes (per the plan).
    seg = 0
    ns_xs = sorted(r["x"] for r in cfg["ns_roads"])
    for rd in cfg["ns_roads"]:
        tile = assets.ROADS[rd["class"]]["tile"]
        ys = lay.cross_ys
        for k in range(len(ys) - 1):
            y0, y1 = ys[k], ys[k + 1]
            length = y1 - y0
            add("asset", tile, (rd["x"], (y0 + y1) // 2, z), "Roads", "RoadNS_%d" % seg,
                yaw=90, scale=(1, length / 800.0, 1)); seg += 1
    for cy in lay.cross_ys:
        tile = assets.ROADS[cfg["cross_class"]]["tile"]
        for k in range(len(ns_xs) - 1):
            x0, x1 = ns_xs[k], ns_xs[k + 1]
            length = x1 - x0
            add("asset", tile, ((x0 + x1) // 2, cy, z), "Roads", "RoadEW_%d" % seg,
                yaw=0, scale=(length / 800.0, 1, 1)); seg += 1

    # Crosswalk tiles at intersections (also nodes in the graph).
    for i, c in enumerate(lay.crosswalk_nodes):
        add("asset", assets.CROSSWALK, c["p"], "Crosswalks", "Crosswalk_%d" % i)

    # Buildings (LI level instances + SM silhouettes).
    for i, b in enumerate(lay.buildings):
        cx, cy = rect_center(b["rect"])
        spawn = "level_instance" if b["asset"]["is_li"] else "asset"
        add(spawn, b["asset"]["path"], (cx, cy, z), "Buildings",
            "Bldg_%03d_%s" % (i, b["role"]), yaw=b["yaw"], is_li=b["asset"]["is_li"])

    # Set-dressing in the furniture lane.
    for i, d in enumerate(lay.dressing):
        add("asset", d["asset"], d["p"], "Dressing", "%s_%03d" % (d["kind"].capitalize(), i), yaw=d["yaw"])

    # Nav bounds over the slice (extent/100 = actor scale; UE box default is 200cm).
    ex = (cfg["x_max"] - cfg["x_min"]) / 100.0
    ey = (cfg["y_max"] - cfg["y_min"]) / 100.0
    add("class", assets.NAV_BOUNDS_CLASS, (cxw, cyw, 200), "Nav", "NavBounds", scale=(ex, ey, 100))

    # Player start on Ocean Drive, facing the hero facades.
    ps = cfg["player_start"]
    add("class", assets.PLAYER_START_CLASS, (ps["x"], ps["y"], z + 100), "Spawn", "PlayerStart", yaw=ps["yaw"])

    # A few POIs so citizens walk to real Ocean-Drive spots (Kind set on apply).
    poi_x = next(r["x"] for r in cfg["ns_roads"] if r["name"] == "Collins") + 1600
    for i, kind in enumerate(("bar", "diner", "street", "park")):
        py = cfg["y_min"] + (i + 1) * (cfg["y_max"] - cfg["y_min"]) // 5
        add("class", assets.PLACE_MARKER_CLASS, (poi_x, py, z), "POIs", "POI_%s" % kind,
            props={"kind": kind})   # AGTCPlaceMarker.Kind is an FName; UE python uses snake_case

    return actors


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--config", default=os.path.join(HERE, "slice_config.json"))
    ap.add_argument("--out", default=None, help="output dir (default: repo output_dir_rel)")
    args = ap.parse_args()

    cfg = json.load(open(args.config))
    lay = Layout(cfg).solve()
    report = validate(lay)

    # Resolve output dir (default = repo-relative output_dir_rel, two levels up from Scripts/gtc_citygen).
    repo_root = os.path.dirname(os.path.dirname(HERE))
    out_dir = args.out or os.path.join(repo_root, cfg["output_dir_rel"])
    os.makedirs(out_dir, exist_ok=True)

    actors = build_actors(lay, cfg)
    ox, oy = origin_offset(cfg)

    def shift_nodes(nodes):
        return [{"p": [n["p"][0] + ox, n["p"][1] + oy, n["p"][2]], "n": n["n"]} for n in nodes]

    header = {
        "schema_version": SCHEMA_VERSION,
        "slice_id": cfg["slice_id"],
        "seed": cfg["seed"],
        "grid": cfg["grid"],
        "ground_z": cfg["ground_z"],
        "world_bounds": {"x_min": cfg["x_min"], "y_min": cfg["y_min"],
                         "x_max": cfg["x_max"], "y_max": cfg["y_max"]},
        "namespace_folder": cfg["namespace_folder"],
        "label_prefix": cfg["label_prefix"],
        "foundation": cfg.get("foundation", "flat"),
        "world_offset": [ox, oy],
        "recentered": bool(cfg.get("recenter_to_origin")),
        "counts": {
            "actors": len(actors), "buildings": len(lay.buildings), "plots": len(lay.plots),
            "blocks": len(lay.blocks), "roads": len(lay.roads),
            "sidewalk_nodes": len(lay.sidewalk_nodes), "crosswalk_nodes": len(lay.crosswalk_nodes),
            "dressing": len(lay.dressing),
        },
        "validation": report["status"],
    }
    plan = {"header": header, "actors": actors}
    sidewalk_graph = {"sidewalks": shift_nodes(lay.sidewalk_nodes), "crosswalks": shift_nodes(lay.crosswalk_nodes)}

    # Deterministic, stable JSON (sorted keys, fixed separators) for byte-identical reruns.
    def dump(obj, name):
        path = os.path.join(out_dir, name)
        with open(path, "w") as f:
            json.dump(obj, f, indent=2, sort_keys=True)
            f.write("\n")
        return path

    p1 = dump(plan, "placement_plan.json")
    p2 = dump(sidewalk_graph, "sidewalk_graph.json")
    p3 = dump(report, "validation_report.json")

    print("validation: %s  (%s)" % (report["status"],
          ", ".join("%s=%d" % (k, v) for k, v in sorted(header["counts"].items()))))
    if report["status"] != "PASS":
        print("  VIOLATIONS: %d (see %s)" % (report["violation_count"], p3))
    for p in (p1, p2, p3):
        print("  wrote", os.path.relpath(p, repo_root))
    return 0 if report["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
