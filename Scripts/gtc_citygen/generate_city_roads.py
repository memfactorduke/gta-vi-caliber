#!/usr/bin/env python3
"""City road-layer generator (offline; NO `import unreal`).

First layer of the full Caliber City, per the spec's strict order (roads before blocks).
Computes the global primary network centerlines from city_config.json (beltway ring,
ring road, 8 radial arteries, I-95 spine, causeways) and emits:

  road_network.json     centerline polylines + class/width  (consumed by the block-carving
                        step and the traffic/lane graph later)
  city_roads_plan.json  road-tile actors sampled along the centerlines (placement-plan format)

Roads may cross (junctions) so no overlap validation is needed here; the no-overlap
guarantee applies later, when buildings fill only the road-bounded free blocks.

    python3 generate_city_roads.py
"""

import json
import math
import os

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
GRID = 800
STEP = 16000   # tile sampling pitch along centerlines (keeps actor count modest)

CB = "/Game/GTCaliberAssets/Content/CityBeachStrip/Meshes/Street"
TILE = {
    "highway":  CB + "/SM_Road01_8x8",
    "arterial": CB + "/SM_Street_8m_02",
    "local":    CB + "/SM_Street_8m_01",
    "causeway": CB + "/SM_Road01_8m",
}


def snap(v):
    return int(round(v / GRID) * GRID)


def circle_pts(r, step):
    n = max(8, int(2 * math.pi * r / step))
    return [(snap(r * math.cos(2 * math.pi * i / n)), snap(r * math.sin(2 * math.pi * i / n)))
            for i in range(n + 1)]


def line_pts(p0, p1, step):
    (x0, y0), (x1, y1) = p0, p1
    d = math.hypot(x1 - x0, y1 - y0)
    n = max(1, int(round(d / step)))
    return [(snap(x0 + (x1 - x0) * i / n), snap(y0 + (y1 - y0) * i / n)) for i in range(n + 1)]


def centerlines(roads):
    nets = []
    bw = roads["beltway"]
    nets.append({"name": "Beltway", "class": "highway", "width": bw["width_cm"],
                 "pts": circle_pts(bw["radius_cm"], STEP)})
    rr = roads["ring_road"]
    nets.append({"name": "RingRoad", "class": "arterial", "width": rr["width_cm"],
                 "pts": circle_pts(rr["radius_cm"], STEP)})
    ra = roads["radial_arteries"]
    inner = ra.get("inner_radius_cm", 64000); outer = ra.get("outer_radius_cm", bw["radius_cm"])
    for k in range(int(ra["count"])):
        a = math.radians(k * 360.0 / ra["count"])
        p0 = (inner * math.cos(a), inner * math.sin(a))
        p1 = (outer * math.cos(a), outer * math.sin(a))
        nets.append({"name": "Artery_%d" % k, "class": "arterial", "width": ra["width_cm"],
                     "pts": line_pts(p0, p1, STEP)})
    sp = roads.get("i95_spine")
    if sp:
        nets.append({"name": "I95", "class": "highway", "width": sp["width_cm"],
                     "pts": line_pts((sp["x_cm"], sp["y_min_cm"]), (sp["x_cm"], sp["y_max_cm"]), STEP)})
    for i, c in enumerate(roads.get("causeways", [])):
        nets.append({"name": "Causeway_%d" % i, "class": "causeway", "width": c["width_cm"],
                     "pts": line_pts((c["from"]["x"], c["from"]["y"]), (c["to"]["x"], c["to"]["y"]), STEP)})
    return nets


def road_actors(nets, cfg):
    nsf = cfg["namespace_folder"]; pfx = cfg["label_prefix"]; z = cfg["ground_z"]
    actors = []
    for net in nets:
        tile = TILE.get(net["class"], TILE["arterial"])
        pts = net["pts"]
        for i in range(len(pts) - 1):
            (x0, y0), (x1, y1) = pts[i], pts[i + 1]
            L = math.hypot(x1 - x0, y1 - y0)
            if L < 1:
                continue
            yaw = math.degrees(math.atan2(y1 - y0, x1 - x0))
            actors.append({
                "spawn": "asset", "ref": tile, "is_li": False,
                "folder": nsf + "/Roads/" + net["name"].split("_")[0],
                "label": pfx + "Road_" + net["name"] + "_%d" % i,
                "location": {"x": (x0 + x1) // 2, "y": (y0 + y1) // 2, "z": z},
                "rotation": {"pitch": 0, "yaw": round(yaw, 2), "roll": 0},
                "scale": {"x": round(L / 800.0, 4), "y": round(net["width"] / 800.0, 4), "z": 1},
            })
    return actors


def main():
    cfg = json.load(open(os.path.join(HERE, "city_config.json")))
    out_dir = os.path.join(REPO, cfg["output_dir_rel"])
    os.makedirs(out_dir, exist_ok=True)

    nets = centerlines(cfg["roads"])
    network = {"roads": [{"name": n["name"], "class": n["class"], "width": n["width"],
                          "centerline": [[p[0], p[1]] for p in n["pts"]]} for n in nets],
               "river": cfg["roads"].get("river")}
    actors = road_actors(nets, cfg)
    plan = {"header": {"layer": "roads", "namespace_folder": cfg["namespace_folder"],
                       "label_prefix": cfg["label_prefix"], "ground_z": cfg["ground_z"],
                       "counts": {"road_actors": len(actors), "networks": len(nets)}},
            "actors": actors}

    def dump(o, name):
        p = os.path.join(out_dir, name)
        json.dump(o, open(p, "w"), indent=2, sort_keys=True)
        open(p, "a").write("\n")
        return p

    dump(network, "road_network.json")
    dump(plan, "city_roads_plan.json")
    print("road networks: %d, road tiles: %d" % (len(nets), len(actors)))
    for n in nets:
        print("  %-12s %-9s w=%-5d segments=%d" % (n["name"], n["class"], n["width"], len(n["pts"]) - 1))
    print("wrote road_network.json + city_roads_plan.json to", os.path.relpath(out_dir, REPO))


if __name__ == "__main__":
    raise SystemExit(main())
