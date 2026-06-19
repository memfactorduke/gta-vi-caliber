#!/usr/bin/env python3
"""Per-district building fill (offline; NO `import unreal`).

Third layer. Lays a district's local street grid (rotated by its rotation_deg) inside its
region, subdivides into blocks -> plots -> buildings, and keeps a building ONLY if every
sampled footprint cell is FREE (not a primary road / water / outside the coastline) AND
OWNED by this district (nearest district center). That ownership gate is what prevents
neighbouring districts (whose bboxes overlap, e.g. Downtown & Wynwood) from ever placing
overlapping buildings. Intra-district plots are grid-tiled, so no-overlap holds globally.

Accumulates into city_buildings.json (idempotent per district).

    python3 generate_city_district.py "Downtown/Brickell core"
    python3 generate_city_district.py            # fill the next not-yet-done district
"""

import json
import math
import os
import random
import sys

import generate_city_blocks as gcb
from layout import floor_to, ceil_to

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
W, H, G = gcb.W, gcb.H, gcb.GRID
H_LOCAL, SB, INSET, SRI = 1200, 400, 800, 100
BUILDING_ROLES = {"resort", "midrise", "background", "highrise", "lowrise"}


def build_free_owner(cfg, net):
    forbidden = bytearray(W * H); water = bytearray(W * H)
    for r in net["roads"]:
        buf = gcb.H_ROW.get(r["class"], 2400) + r["width"] / 2 + gcb.SETBACK
        p = r["centerline"]
        for i in range(len(p) - 1):
            gcb.stamp_segment(forbidden, p[i][0], p[i][1], p[i + 1][0], p[i + 1][1], buf)
    gcb.stamp_rect(water, 320000, gcb.Y0, gcb.X1, gcb.Y1)
    gcb.stamp_rect(water, 150000, -180000, 250000, 180000)
    gcb.stamp_rect(water, 150000, -150000, 215000, -95000)
    riv = net.get("river") or {}
    gcb.stamp_segment(water, 150000, -10000, -40000, -25000, riv.get("width_cm", 8000) / 2)
    dists = [{"name": d["name"], "cx": d["center"]["x"], "cy": d["center"]["y"],
              "hx": d["half_extent"]["x"], "hy": d["half_extent"]["y"],
              "ann": d.get("annulus")} for d in cfg["districts"]]
    free = bytearray(W * H); owner = bytearray(W * H)   # owner = district index+1 (0 = none)
    R2 = gcb.LAND_RADIUS ** 2
    for ix in range(W):
        wx = gcb.X0 + ix * G + G // 2
        base = ix * H
        for iy in range(H):
            wy = gcb.Y0 + iy * G + G // 2
            r2 = wx * wx + wy * wy
            if r2 > R2:
                continue
            i = base + iy
            if forbidden[i] or water[i]:
                continue
            free[i] = 1
            best, bestd = 0, 1e30
            for di, d in enumerate(dists):
                if abs(wx - d["cx"]) <= d["hx"] and abs(wy - d["cy"]) <= d["hy"]:
                    a = d["ann"]   # annular districts (suburban ring) only own a radius band
                    if a and (r2 < a[0] * a[0] or r2 > a[1] * a[1]):
                        continue
                    dd = (wx - d["cx"]) ** 2 + (wy - d["cy"]) ** 2
                    if dd < bestd:
                        bestd, best = dd, di + 1
            owner[i] = best
    return free, owner, dists


def cell_ok(wx, wy, free, owner, oidx):
    if not (gcb.X0 <= wx < gcb.X1 and gcb.Y0 <= wy < gcb.Y1):
        return False
    i = gcb.idx(int((wx - gcb.X0) // G), int((wy - gcb.Y0) // G))
    return free[i] and owner[i] == oidx


def footprint_ok(b, c, s, cx, cy, free, owner, oidx):
    x0, y0, x1, y1 = b
    pts = [(x0, y0), (x1, y0), (x0, y1), (x1, y1), ((x0 + x1) / 2, (y0 + y1) / 2),
           ((x0 + x1) / 2, y0), ((x0 + x1) / 2, y1), (x0, (y0 + y1) / 2), (x1, (y0 + y1) / 2)]
    for lx, ly in pts:
        if not cell_ok(cx + lx * c - ly * s, cy + lx * s + ly * c, free, owner, oidx):
            return False
    return True


def tile_block(b, module, gap):
    bx0, by0, bx1, by1 = b
    ix0, iy0, ix1, iy1 = bx0 + INSET, by0 + INSET, bx1 - INSET, by1 - INSET
    if ix1 <= ix0 or iy1 <= iy0:
        return []
    out = []
    nx = max(1, (ix1 - ix0 + gap) // (module + gap))
    ny = max(1, (iy1 - iy0 + gap) // (module + gap))
    pw = floor_to(((ix1 - ix0) - gap * (nx - 1)) // nx, G)
    ph = floor_to(((iy1 - iy0) - gap * (ny - 1)) // ny, G)
    if pw < 1600 or ph < 1600:
        return [(ix0, iy0, ix1, iy1)]
    for a in range(nx):
        for d in range(ny):
            px0 = ix0 + a * (pw + gap); py0 = iy0 + d * (ph + gap)
            if px0 + pw <= ix1 and py0 + ph <= iy1:
                out.append((px0, py0, px0 + pw, py0 + ph))
    return out


def fill_district(dcfg, oidx, free, owner, cfg, rng):
    theta = math.radians(dcfg["rotation_deg"]); c, s = math.cos(theta), math.sin(theta)
    cx, cy = dcfg["center"]["x"], dcfg["center"]["y"]
    hx, hy = dcfg["half_extent"]["x"], dcfg["half_extent"]["y"]
    sp_ns = int(dcfg["road"]["spacing_ns_cm"]); sp_ew = int(dcfg["road"]["spacing_ew_cm"])
    cols = dcfg["columns"]
    # NOTE: the configs' columns frontage_cm is the lot WIDTH (up to 6400), not a setback;
    # plot sizing comes from plot_module_cm, and the build-to setback is derived from party.
    party = cols[0]["party_wall"]; module = int(dcfg["plot_module_cm"])
    frontage = 0 if party else 300
    roles = [col["role"] for col in cols if col["role"] in BUILDING_ROLES] or ["background"]
    pools = {}
    for a in dcfg["building_assets"]:
        if a["role"] in BUILDING_ROLES:
            pools.setdefault(a["role"], []).append(a)
    anybld = [a for a in dcfg["building_assets"] if a["role"] in BUILDING_ROLES] or dcfg["building_assets"]

    short = dcfg["name"].split("/")[0].replace(" ", "")
    nsf = cfg["namespace_folder"] + "/Districts/" + short
    pfx = cfg["label_prefix"] + short + "_"
    z = cfg["ground_z"]
    xs = list(range(-(hx // sp_ns) * sp_ns, hx + 1, sp_ns))
    ys = list(range(-(hy // sp_ew) * sp_ew, hy + 1, sp_ew))
    gap = 0 if party else G
    actors = []; n = 0
    for i in range(len(xs) - 1):
        for j in range(len(ys) - 1):
            b = (ceil_to(xs[i] + H_LOCAL + SB, G), ceil_to(ys[j] + H_LOCAL + SB, G),
                 floor_to(xs[i + 1] - H_LOCAL - SB, G), floor_to(ys[j + 1] - H_LOCAL - SB, G))
            if b[2] <= b[0] or b[3] <= b[1]:
                continue
            for plot in tile_block(b, module, gap):
                px0, py0, px1, py1 = plot
                bld = (px0 + SRI, py0 + (0 if party else SRI), px1 - frontage, py1 - (0 if party else SRI))
                if bld[2] <= bld[0] or bld[3] <= bld[1]:
                    continue
                if not footprint_ok(bld, c, s, cx, cy, free, owner, oidx):
                    continue
                lcx, lcy = (bld[0] + bld[2]) / 2, (bld[1] + bld[3]) / 2
                wx = cx + lcx * c - lcy * s; wy = cy + lcx * s + lcy * c
                role = roles[n % len(roles)]
                pool = pools.get(role) or anybld
                asset = pool[rng.randrange(len(pool))]
                actors.append({
                    "spawn": "level_instance" if asset["is_li"] else "asset", "ref": asset["path"],
                    "is_li": asset["is_li"], "folder": nsf, "label": pfx + "Bldg_%04d_%s" % (n, role),
                    "location": {"x": int(round(wx)), "y": int(round(wy)), "z": z},
                    "rotation": {"pitch": 0, "yaw": round(dcfg["rotation_deg"], 2), "roll": 0},
                    "scale": {"x": 1, "y": 1, "z": 1},
                })
                n += 1
    return actors, short


def main():
    cfg = json.load(open(os.path.join(HERE, "city_config.json")))
    out_dir = os.path.join(REPO, cfg["output_dir_rel"])
    net = json.load(open(os.path.join(out_dir, "road_network.json")))
    bpath = os.path.join(out_dir, "city_buildings.json")
    city = json.load(open(bpath)) if os.path.exists(bpath) else {"actors": [], "districts_done": []}

    want = sys.argv[1] if len(sys.argv) > 1 else None
    names = [d["name"] for d in cfg["districts"]]
    if want is None:
        want = next((nm for nm in names if nm not in city["districts_done"]), None)
        if want is None:
            print("all districts already filled:", city["districts_done"]); return 0
    dcfg = next(d for d in cfg["districts"] if d["name"] == want)
    oidx = names.index(want) + 1

    print("rasterizing free + ownership masks ...")
    free, owner, _ = build_free_owner(cfg, net)
    rng = random.Random(20260618 + oidx)
    actors, short = fill_district(dcfg, oidx, free, owner, cfg, rng)

    # idempotent replace of this district's actors
    city["actors"] = [a for a in city["actors"] if ("/Districts/" + short) not in a["folder"]] + actors
    if want not in city["districts_done"]:
        city["districts_done"].append(want)
    json.dump(city, open(bpath, "w"), indent=1, sort_keys=True)

    per = {}
    for a in city["actors"]:
        d = a["folder"].split("/Districts/")[-1]
        per[d] = per.get(d, 0) + 1
    print("filled %s: %d buildings" % (want, len(actors)))
    print("city_buildings.json total: %d  (%s)" % (len(city["actors"]),
          ", ".join("%s=%d" % (k, v) for k, v in sorted(per.items()))))
    print("districts done:", city["districts_done"])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
