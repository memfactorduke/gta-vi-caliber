#!/usr/bin/env python3
"""City BLOCKMASK + block carving (offline; NO `import unreal`).

Second layer of the buildout. Rasterizes all road ROWs (centerline buffered by H_row +
half-width + setback) plus water (ocean, Biscayne Bay, river, port basin) onto the 800 cm
city grid, then flood-fills the connected FREE-LAND regions into blocks. Each block is
assigned to the district whose bbox contains its centroid (nearest center on ties). This is
what guarantees global no-overlap: districts later fill ONLY these road-separated free blocks.

    python3 generate_city_blocks.py   ->  city_blocks.json (consumed by the district-fill step)
"""

import json
import math
import os
from collections import deque

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
GRID = 800
SETBACK = 400
LAND_RADIUS = 360000          # spec mean coastline radius (organic edge refined later)
MIN_BLOCK_CELLS = 9           # drop slivers (< 3x3 cells)

H_ROW = {"highway": 5000, "arterial": 2400, "collector": 1800, "local": 1200, "causeway": 3600}

# city raster bounds (cm)
X0, Y0, X1, Y1 = -280000, -310000, 330000, 310000
W = (X1 - X0) // GRID
H = (Y1 - Y0) // GRID


def cxi(x):
    return int((x - X0) // GRID)


def cyi(y):
    return int((y - Y0) // GRID)


def idx(ix, iy):
    return ix * H + iy


def seg_dist(px, py, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    L2 = dx * dx + dy * dy
    if L2 == 0:
        return math.hypot(px - ax, py - ay)
    t = max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / L2))
    return math.hypot(px - (ax + t * dx), py - (ay + t * dy))


def stamp_segment(forbidden, ax, ay, bx, by, buf):
    lo_x = cxi(min(ax, bx) - buf); hi_x = cxi(max(ax, bx) + buf)
    lo_y = cyi(min(ay, by) - buf); hi_y = cyi(max(ay, by) + buf)
    for ix in range(max(0, lo_x), min(W, hi_x + 1)):
        wx = X0 + ix * GRID + GRID // 2
        for iy in range(max(0, lo_y), min(H, hi_y + 1)):
            wy = Y0 + iy * GRID + GRID // 2
            if seg_dist(wx, wy, ax, ay, bx, by) <= buf:
                forbidden[idx(ix, iy)] = 1


def stamp_rect(mask, x0, y0, x1, y1):
    for ix in range(max(0, cxi(x0)), min(W, cxi(x1) + 1)):
        for iy in range(max(0, cyi(y0)), min(H, cyi(y1) + 1)):
            mask[idx(ix, iy)] = 1


def main():
    cfg = json.load(open(os.path.join(HERE, "city_config.json")))
    net = json.load(open(os.path.join(REPO, cfg["output_dir_rel"], "road_network.json")))

    forbidden = bytearray(W * H)   # road ROW + setback OR water  -> no building
    water = bytearray(W * H)

    # Roads -> forbidden (ROW = H_row + half carriageway + setback).
    for r in net["roads"]:
        buf = H_ROW.get(r["class"], 2400) + r["width"] / 2 + SETBACK
        pts = r["centerline"]
        for i in range(len(pts) - 1):
            stamp_segment(forbidden, pts[i][0], pts[i][1], pts[i + 1][0], pts[i + 1][1], buf)

    # Water: ocean (east of island), Biscayne Bay, river, port basin.
    stamp_rect(water, 320000, Y0, X1, Y1)                 # Atlantic
    stamp_rect(water, 150000, -180000, 250000, 180000)    # Biscayne Bay
    stamp_rect(water, 150000, -150000, 215000, -95000)    # dredged port basin
    riv = net.get("river") or {}
    rbuf = (riv.get("width_cm", 8000)) / 2
    for seg in [((150000, -10000), (-40000, -25000))]:    # Miami-river-analog (straight approx)
        stamp_segment(water, seg[0][0], seg[0][1], seg[1][0], seg[1][1], rbuf)

    # Free land = inside coastline radius, not water, not forbidden.
    free = bytearray(W * H)
    for ix in range(W):
        wx = X0 + ix * GRID + GRID // 2
        for iy in range(H):
            wy = Y0 + iy * GRID + GRID // 2
            if wx * wx + wy * wy > LAND_RADIUS * LAND_RADIUS:
                continue
            i = idx(ix, iy)
            if not forbidden[i] and not water[i]:
                free[i] = 1

    # Districts for assignment.
    dists = [{"name": d["name"], "cx": d["center"]["x"], "cy": d["center"]["y"],
              "hx": d["half_extent"]["x"], "hy": d["half_extent"]["y"]} for d in cfg["districts"]]

    def assign(wx, wy):
        best, bestd = None, 1e30
        for d in dists:
            if abs(wx - d["cx"]) <= d["hx"] and abs(wy - d["cy"]) <= d["hy"]:
                dd = (wx - d["cx"]) ** 2 + (wy - d["cy"]) ** 2
                if dd < bestd:
                    best, bestd = d["name"], dd
        return best

    # Flood-fill free cells into blocks (4-connectivity).
    seen = bytearray(W * H)
    blocks = []
    for s in range(W * H):
        if not free[s] or seen[s]:
            continue
        q = deque([s]); seen[s] = 1
        cells = []
        while q:
            c = q.popleft(); cells.append(c)
            ix, iy = divmod(c, H)
            for nx, ny in ((ix - 1, iy), (ix + 1, iy), (ix, iy - 1), (ix, iy + 1)):
                if 0 <= nx < W and 0 <= ny < H:
                    ni = idx(nx, ny)
                    if free[ni] and not seen[ni]:
                        seen[ni] = 1; q.append(ni)
        if len(cells) < MIN_BLOCK_CELLS:
            continue
        ixs = [c // H for c in cells]; iys = [c % H for c in cells]
        bx0 = X0 + min(ixs) * GRID; bx1 = X0 + (max(ixs) + 1) * GRID
        by0 = Y0 + min(iys) * GRID; by1 = Y0 + (max(iys) + 1) * GRID
        ctx = sum(ixs) / len(ixs) * GRID + X0; cty = sum(iys) / len(iys) * GRID + Y0
        blocks.append({"bbox": [bx0, by0, bx1, by1], "cell_count": len(cells),
                       "centroid": [int(ctx), int(cty)], "district": assign(ctx, cty)})

    out = {"grid": GRID, "bounds": [X0, Y0, X1, Y1], "raster_dims": [W, H],
           "block_count": len(blocks), "blocks": blocks}
    op = os.path.join(REPO, cfg["output_dir_rel"], "city_blocks.json")
    json.dump(out, open(op, "w"), indent=1, sort_keys=True)

    per = {}
    for b in blocks:
        per[b["district"]] = per.get(b["district"], 0) + 1
    print("raster %dx%d cells=%d" % (W, H, W * H))
    print("carved %d blocks (>= %d cells)" % (len(blocks), MIN_BLOCK_CELLS))
    for k in sorted(per, key=lambda x: -per[x]):
        print("  %-26s %d blocks" % (k, per[k]))
    print("wrote", os.path.relpath(op, REPO))


if __name__ == "__main__":
    raise SystemExit(main())
