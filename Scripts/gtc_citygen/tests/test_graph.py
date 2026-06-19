"""Pedestrian graph: the generated sidewalk graph must bake into a connected lane graph."""
import json
import os
import sys
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
PKG = os.path.dirname(HERE)
sys.path.insert(0, PKG)

from layout import Layout         # noqa: E402
import build_graph                # noqa: E402

CFG = json.load(open(os.path.join(PKG, "slice_config.json")))


class GraphTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        lay = Layout(CFG).solve()
        sg = {"sidewalks": lay.sidewalk_nodes, "crosswalks": lay.crosswalk_nodes}
        cls.lane, cls.stats = build_graph.build(sg)

    def test_has_nodes_and_edges(self):
        self.assertGreater(self.stats["nodes"], 0)
        self.assertGreater(self.stats["linked"], 0)

    def test_dominant_connected_component(self):
        # The largest component should cover the bulk of linked nodes (one walkable network).
        frac = self.stats["largest_component"] / max(1, self.stats["linked"])
        self.assertGreaterEqual(frac, 0.8, "sidewalk network is fragmented (%.2f)" % frac)

    def test_crosswalks_bridge_kerbs(self):
        # Crosswalks must stitch sidewalks on both sides => >=2 edges each (most of them).
        total_cw = self.stats["crosswalk"]
        self.assertGreater(total_cw, 0)
        self.assertGreaterEqual(self.stats["crosswalks_with_2plus_edges"], total_cw * 0.8)


if __name__ == "__main__":
    unittest.main()
