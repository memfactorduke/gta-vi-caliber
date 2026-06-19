"""Overlap validator: must pass clean layouts and BITE on injected overlaps."""
import json
import os
import sys
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
PKG = os.path.dirname(HERE)
sys.path.insert(0, PKG)

from layout import Layout                      # noqa: E402
from validate_overlap import validate, validate_rects   # noqa: E402

CFG = json.load(open(os.path.join(PKG, "slice_config.json")))


class OverlapTest(unittest.TestCase):
    def test_generated_layout_passes(self):
        self.assertEqual(validate(Layout(CFG).solve())["status"], "PASS")

    def test_rejects_overlapping_plots(self):
        r = validate_rects([
            ("plot", "a", (0, 0, 1000, 1000)),
            ("plot", "b", (500, 500, 1500, 1500)),
        ])
        self.assertEqual(r["status"], "FAIL")
        self.assertEqual(r["violation_count"], 1)

    def test_rejects_plot_in_road_row(self):
        r = validate_rects([
            ("row", "OceanDrive", (0, 0, 2000, 2000)),
            ("plot", "p", (1000, 1000, 3000, 3000)),
        ])
        self.assertEqual(r["status"], "FAIL")

    def test_rejects_building_in_water(self):
        r = validate_rects([
            ("water", "ocean", (0, 0, 2000, 2000)),
            ("building", "b", (1500, 1500, 2500, 2500)),
        ])
        self.assertEqual(r["status"], "FAIL")

    def test_party_wall_touching_is_legal(self):
        # plots sharing an edge (gap 0) have zero shared area => allowed.
        r = validate_rects([
            ("plot", "a", (0, 0, 1000, 1000)),
            ("plot", "b", (0, 1000, 1000, 2000)),
        ])
        self.assertEqual(r["status"], "PASS")

    def test_roads_may_cross(self):
        # ROW-ROW overlap is allowed (intersections).
        r = validate_rects([
            ("row", "ns", (0, 0, 2000, 5000)),
            ("row", "ew", (0, 0, 5000, 2000)),
        ])
        self.assertEqual(r["status"], "PASS")

    def test_building_within_plot_is_legal(self):
        # containment is not an overlap violation.
        r = validate_rects([
            ("plot", "p", (0, 0, 1000, 1000)),
            ("building", "b", (100, 100, 900, 900)),
        ])
        self.assertEqual(r["status"], "PASS")


if __name__ == "__main__":
    unittest.main()
