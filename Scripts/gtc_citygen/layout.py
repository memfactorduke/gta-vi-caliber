"""Pure-Python layout solver for the Ocean Drive slice. NO `import unreal`.

Realizes the spec's organize-first pipeline on the 800 cm grid, but for the slice
everything is axis-aligned (N-S roads along Y, cross streets along X), so every
ROW / block / plot / building is an axis-aligned rectangle and overlap math is exact
integer rectangle math:

    roads -> ROW rects -> blocks (road inner edges, dilated by setback)
          -> plots (perimeter inset, party-wall or gapped) -> buildings (plot inset)
          -> sidewalk + crosswalk nodes -> furniture-lane dressing

The output is structured geometry (rectangles + points); `generate_slice.py` turns it
into a placement plan and the pedestrian graph. The no-overlap guarantee holds *by
construction* (building subset plot subset block subset land-minus-ROW-minus-water);
`validate_overlap.py` proves it independently as a safety net.

A Rect is a 4-tuple of ints (x0, y0, x1, y1) with x0 < x1 and y0 < y1, in cm.
"""

import random


# --- grid helpers -----------------------------------------------------------
def floor_to(v, g):
    return (v // g) * g          # // floors toward -inf, correct for negatives


def ceil_to(v, g):
    return -((-v) // g) * g


def rect_valid(r):
    return r is not None and r[0] < r[2] and r[1] < r[3]


def rect_center(r):
    return ((r[0] + r[2]) // 2, (r[1] + r[3]) // 2)


def interiors_overlap(a, b):
    """True iff the rectangles share positive area (touching edges => False)."""
    ix0 = max(a[0], b[0]); iy0 = max(a[1], b[1])
    ix1 = min(a[2], b[2]); iy1 = min(a[3], b[3])
    return ix1 > ix0 and iy1 > iy0


# --- main solver ------------------------------------------------------------
class Layout:
    def __init__(self, cfg):
        self.cfg = cfg
        self.g = cfg["grid"]
        self.rng = random.Random(cfg["seed"])
        self.roads = []      # {name, axis:'NS'|'EW', klass, h_row, row: Rect}
        self.water = []      # Rect (ocean / bay just outside the buildable land)
        self.blocks = []     # {col, row, rect, role, party_wall, frontage, is_end}
        self.plots = []      # {rect, role, party_wall, frontage, is_end, block}
        self.buildings = []  # {rect, role, asset, yaw}
        self.sidewalk_nodes = []   # {p:[x,y,z], n:mesh}
        self.crosswalk_nodes = []  # {p:[x,y,z], n:mesh}
        self.dressing = []   # {kind, asset, is_li, p:[x,y,z], yaw}

    # -- roads ---------------------------------------------------------------
    def build_roads(self):
        cfg = self.cfg
        rc = cfg["road_classes"]
        ns = {r["name"]: r for r in cfg["ns_roads"]}
        # N-S roads span the whole slice in Y.
        xs = [r["x"] for r in cfg["ns_roads"]]
        x_lo_road = min(xs) - rc[ns[min(ns, key=lambda k: ns[k]["x"])]["class"]]["h_row"]
        x_hi_road = max(xs) + rc[ns[max(ns, key=lambda k: ns[k]["x"])]["class"]]["h_row"]
        for r in cfg["ns_roads"]:
            h = rc[r["class"]]["h_row"]
            self.roads.append({
                "name": r["name"], "axis": "NS", "klass": r["class"], "h_row": h,
                "row": (r["x"] - h, cfg["y_min"], r["x"] + h, cfg["y_max"]),
            })
        # E-W cross streets every cross_spacing, spanning the road network in X.
        h = rc[cfg["cross_class"]]["h_row"]
        self.cross_ys = self._cross_ys()
        for cy in self.cross_ys:
            self.roads.append({
                "name": "Cross_%d" % cy, "axis": "EW", "klass": cfg["cross_class"], "h_row": h,
                "row": (x_lo_road, cy - h, x_hi_road, cy + h),
            })
        # Water just outside the island (for the validator to prove plots avoid it).
        self.water.append((cfg["ocean_edge_x"], cfg["y_min"], cfg["ocean_edge_x"] + 10000, cfg["y_max"]))
        self.water.append((cfg["bay_edge_x"] - 10000, cfg["y_min"], cfg["bay_edge_x"], cfg["y_max"]))

    def _cross_ys(self):
        cfg = self.cfg
        ys, y = [], cfg["y_min"]
        while y < cfg["y_max"]:
            ys.append(y)
            y += cfg["cross_spacing"]
        if ys[-1] != cfg["y_max"]:
            ys.append(cfg["y_max"])
        return ys

    # -- blocks --------------------------------------------------------------
    def build_blocks(self):
        cfg = self.cfg
        g = self.g
        rc = cfg["road_classes"]
        ns = {r["name"]: r for r in cfg["ns_roads"]}
        h_cross = rc[cfg["cross_class"]]["h_row"]
        sb = cfg["setback"]
        for ci, col in enumerate(cfg["columns"]):
            left = ns[col["between"][0]]; right = ns[col["between"][1]]
            hl = rc[left["class"]]["h_row"]; hr = rc[right["class"]]["h_row"]
            bx0 = ceil_to(left["x"] + hl + sb, g)
            bx1 = floor_to(right["x"] - hr - sb, g)
            n_rows = len(self.cross_ys) - 1
            for ri in range(n_rows):
                cy0, cy1 = self.cross_ys[ri], self.cross_ys[ri + 1]
                by0 = ceil_to(cy0 + h_cross + sb, g)
                by1 = floor_to(cy1 - h_cross - sb, g)
                rect = (bx0, by0, bx1, by1)
                if not rect_valid(rect):
                    continue
                is_end = col["role"] == "hero_row" and (ri < cfg["hero_end_blocks"] or ri >= n_rows - cfg["hero_end_blocks"])
                self.blocks.append({
                    "col": ci, "row": ri, "rect": rect, "role": col["role"],
                    "party_wall": col["party_wall"], "frontage": col["frontage"], "is_end": is_end,
                })

    # -- plots ---------------------------------------------------------------
    def _plot_params(self, b):
        """(target_plot_len, party_wall, building_role) per block.

        Hero-row STRIP ENDS become one big resort plot; the hero-row MIDDLE becomes a
        continuous party-wall (gap 0) Art Deco row; other columns are gapped plots.
        """
        role = b["role"]
        if role == "hero_row" and b["is_end"]:
            return (10 ** 9, False, "resort")    # single big plot for the anchor resort hotel
        if role == "hero_row":
            return (3200, True, "midrise")      # narrow party-wall Deco row
        if role == "midrise":
            return (4800, False, "midrise")
        return (6400, False, "background")

    def build_plots(self):
        cfg = self.cfg
        g = self.g
        inset = cfg["plot_inset"]
        for b in self.blocks:
            bx0, by0, bx1, by1 = b["rect"]
            ix0, iy0 = bx0 + inset, by0 + inset
            ix1, iy1 = bx1 - inset, by1 - inset
            if ix1 <= ix0 or iy1 <= iy0:
                continue
            inner_len = iy1 - iy0
            target, party, brole = self._plot_params(b)
            n = max(1, inner_len // target)
            gap = 0 if party else g                     # party wall => shared edge, gap 0
            plot_len = floor_to((inner_len - gap * (n - 1)) // n, g)
            if plot_len < 2400:
                continue
            for k in range(n):
                py0 = iy0 + k * (plot_len + gap)
                py1 = py0 + plot_len
                if py1 > iy1:
                    break
                self.plots.append({
                    "rect": (ix0, py0, ix1, py1), "role": b["role"], "party_wall": party,
                    "brole": brole, "frontage": b["frontage"], "is_end": b["is_end"],
                    "block": (b["col"], b["row"]), "block_rect": b["rect"],
                })

    # -- buildings -----------------------------------------------------------
    def build_buildings(self):
        cfg = self.cfg
        sri = cfg["side_rear_inset"]
        for p in self.plots:
            px0, py0, px1, py1 = p["rect"]
            # Frontage faces east (+X): set the building's east edge back from the plot's east edge.
            bx1 = px1 - p["frontage"]
            bx0 = px0 + sri
            side = 0 if p["party_wall"] else sri        # party wall => fill plot in Y
            by0 = py0 + side
            by1 = py1 - side
            rect = (bx0, by0, bx1, by1)
            if not rect_valid(rect):
                continue
            from assets import pick_building
            asset = pick_building(self.rng, p["brole"])
            self.buildings.append({"rect": rect, "role": p["brole"], "asset": asset, "yaw": 0,
                                   "plot_rect": p["rect"], "party_wall": p["party_wall"]})

    # -- pedestrian graph: sidewalk runs + crosswalks ------------------------
    def build_graph(self):
        cfg = self.cfg
        from assets import ROADS, CROSSWALK
        step = 1200                       # < SIDEWALK_LINK (1400) so build_lane_graph links runs
        z = cfg["ground_z"]
        # Sidewalk nodes along the inner edge of each road's ROW, both sides.
        for rd in self.roads:
            mesh = ROADS[rd["klass"]]["sidewalk"]
            name = mesh.rsplit("/", 1)[-1] if mesh else "Sidewalk"
            x0, y0, x1, y1 = rd["row"]
            if rd["axis"] == "NS":
                for edge_x in (x0, x1):
                    y = y0
                    while y <= y1:
                        self.sidewalk_nodes.append({"p": [edge_x, y, z], "n": name})
                        y += step
            else:
                for edge_y in (y0, y1):
                    x = x0
                    while x <= x1:
                        self.sidewalk_nodes.append({"p": [x, edge_y, z], "n": name})
                        x += step
        # Crosswalk node at each N-S x cross-street intersection.
        ns_roads = [r for r in self.roads if r["axis"] == "NS"]
        cw = CROSSWALK.rsplit("/", 1)[-1]
        for rd in ns_roads:
            cx = (rd["row"][0] + rd["row"][2]) // 2
            for cy in self.cross_ys:
                self.crosswalk_nodes.append({"p": [cx, cy, z], "n": cw})

    # -- set-dressing in the furniture lane ----------------------------------
    def build_dressing(self):
        cfg = self.cfg
        from assets import DRESSING, pick_palm
        z = cfg["ground_z"]
        ocean = next(r for r in self.roads if r["name"] == "OceanDrive")
        h = ocean["h_row"]
        cx = (ocean["row"][0] + ocean["row"][2]) // 2
        # Furniture band = midline of the setback gap just WEST of Ocean Drive's ROW.
        band_x = cx - h - cfg["setback"] // 2
        # Skip the intersection mouths so props never sit in a cross-street ROW.
        h_cross = cfg["road_classes"][cfg["cross_class"]]["h_row"]
        keepout = h_cross + cfg["setback"]
        y = cfg["y_min"] + 4000
        i = 0
        while y <= cfg["y_max"] - 4000:
            if all(abs(y - cy) > keepout for cy in self.cross_ys):
                if i % 2 == 0:
                    asset = pick_palm(self.rng, lit=True)
                    self.dressing.append({"kind": "palm", "asset": asset, "is_li": False, "p": [band_x, y, z], "yaw": 0})
                else:
                    self.dressing.append({"kind": "lamp", "asset": DRESSING["lamp"], "is_li": False, "p": [band_x, y, z], "yaw": 0})
            y += 6000
            i += 1

    def solve(self):
        self.build_roads()
        self.build_blocks()
        self.build_plots()
        self.build_buildings()
        self.build_graph()
        self.build_dressing()
        return self
