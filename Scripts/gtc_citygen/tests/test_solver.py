"""Solver invariants for the Ocean Drive slice. Run:  python3 -m unittest discover tests"""
import json
import os
import sys
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
PKG = os.path.dirname(HERE)
sys.path.insert(0, PKG)

from layout import Layout, interiors_overlap   # noqa: E402
import generate_slice                           # noqa: E402

CFG = json.load(open(os.path.join(PKG, "slice_config.json")))
G = CFG["grid"]


def contains(outer, inner):
    return (outer[0] <= inner[0] and outer[1] <= inner[1]
            and inner[2] <= outer[2] and inner[3] <= outer[3])


class SolverTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lay = Layout(CFG).solve()

    def test_nonempty(self):
        self.assertGreater(len(self.lay.blocks), 0)
        self.assertGreater(len(self.lay.plots), 0)
        self.assertGreater(len(self.lay.buildings), 0)

    def test_blocks_and_plots_grid_snapped(self):
        # Blocks come from ceil_to/floor_to(G); plots are grid insets of blocks.
        for b in self.lay.blocks:
            for v in b["rect"]:
                self.assertEqual(v % G, 0, "block coord %d not on grid" % v)
        for p in self.lay.plots:
            for v in p["rect"]:
                self.assertEqual(v % G, 0, "plot coord %d not on grid" % v)

    def test_spacing_invariant(self):
        # Cross-street spacing must satisfy D >= 2*H_row + 2*setback + min_plot_depth.
        rc = CFG["road_classes"][CFG["cross_class"]]["h_row"]
        sb = CFG["setback"]
        min_plot_depth = 2400
        self.assertGreaterEqual(CFG["cross_spacing"], 2 * rc + 2 * sb + min_plot_depth)

    def test_containment_chain(self):
        # building subset plot subset block (transitively keeps buildings out of ROWs/water).
        for b in self.lay.buildings:
            self.assertTrue(contains(b["plot_rect"], b["rect"]), "building escapes its plot")
        for p in self.lay.plots:
            self.assertTrue(contains(p["block_rect"], p["rect"]), "plot escapes its block")

    def test_side_of_street(self):
        # Ocean Drive buildings only on the WEST side of the spine (beach is east, no buildings).
        spine = next(r["x"] for r in CFG["ns_roads"] if r["name"] == "OceanDrive")
        for b in self.lay.buildings:
            self.assertLess(b["rect"][2], spine, "building east of Ocean Drive spine")

    def test_furniture_lane_clear(self):
        # Dressing sits in the setback band: not inside any plot, not inside any ROW.
        plots = [p["rect"] for p in self.lay.plots]
        rows = [r["row"] for r in self.lay.roads]
        for d in self.lay.dressing:
            px, py = d["p"][0], d["p"][1]
            pt = (px, py, px + 1, py + 1)   # 1cm probe rect
            self.assertFalse(any(interiors_overlap(pt, r) for r in plots), "dressing inside a plot")
            self.assertFalse(any(interiors_overlap(pt, r) for r in rows), "dressing inside a ROW")

    def test_hero_resorts_at_strip_ends(self):
        resorts = [b for b in self.lay.buildings if b["role"] == "resort"]
        # one hero block at each Y end of the single hero_row column => 2 resort buildings.
        self.assertEqual(len(resorts), 2 * CFG["hero_end_blocks"])
        ys = sorted((b["rect"][1] + b["rect"][3]) // 2 for b in resorts)
        span = CFG["cross_spacing"] * CFG["hero_end_blocks"]
        # resorts must anchor the END block rows (within the first/last row band).
        self.assertLess(ys[0], CFG["y_min"] + span, "south resort not in the southern end row")
        self.assertGreater(ys[-1], CFG["y_max"] - span, "north resort not in the northern end row")

    def test_party_wall_row_exists(self):
        # The hero-row middle should produce touching (gap-0) party-wall buildings.
        pw = [b["rect"] for b in self.lay.buildings if b.get("party_wall")]
        self.assertGreater(len(pw), 0, "no party-wall buildings generated")
        touching = 0
        for i in range(len(pw)):
            for j in range(i + 1, len(pw)):
                a, b = pw[i], pw[j]
                shares_y_edge = (a[3] == b[1] or b[3] == a[1])
                x_overlap = min(a[2], b[2]) > max(a[0], b[0])
                if shares_y_edge and x_overlap:
                    touching += 1
        self.assertGreater(touching, 0, "party-wall buildings do not share walls")

    def test_determinism_byte_identical(self):
        a = json.dumps(generate_slice.build_actors(Layout(CFG).solve(), CFG), sort_keys=True)
        b = json.dumps(generate_slice.build_actors(Layout(CFG).solve(), CFG), sort_keys=True)
        self.assertEqual(a, b, "generator is not deterministic for a fixed seed")


if __name__ == "__main__":
    unittest.main()
