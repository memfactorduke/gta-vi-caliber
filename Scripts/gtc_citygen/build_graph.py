#!/usr/bin/env python3
"""Build the baked pedestrian lane graph from the slice's sidewalk_graph.json.

A thin, parameterized port of Legacy_GTC/tools/build_lane_graph.py (same constants
and linking algorithm) so it can read/write the slice output dir instead of the
hardcoded CityBeachStrip paths, and be imported by tests. Output schema matches the
shipped lane_graph.json:  {"nodes":[{"p":[x,y,z],"c":0|1}], "neighbors":[[i,...]]}.

    python3 build_graph.py [--in <dir>]      # reads <dir>/sidewalk_graph.json
"""

import argparse
import json
import math
import os

SIDEWALK_LINK = 1400.0   # cm: links adjacent sidewalk tiles into runs (< road width)
CROSS_MAX = 5000.0       # cm: max crosswalk pivot -> kerb sidewalk node
CROSS_LINKS = 3          # nearby sidewalk nodes a crosswalk bridges

HERE = os.path.dirname(os.path.abspath(__file__))


def _d2(a, b):
    return math.hypot(a[0] - b[0], a[1] - b[1])


def build(sidewalk_graph):
    """sidewalk_graph = {"sidewalks":[{n,p}], "crosswalks":[{n,p}]} -> (lane_graph, stats)."""
    side = sidewalk_graph["sidewalks"]
    cross = sidewalk_graph["crosswalks"]
    nodes = [{"p": s["p"], "c": 0} for s in side]
    cross_start = len(nodes)
    nodes += [{"p": c["p"], "c": 1} for c in cross]
    n = len(nodes)
    adj = [set() for _ in range(n)]

    for i in range(cross_start):
        pi = nodes[i]["p"]
        for j in range(i + 1, cross_start):
            if _d2(pi, nodes[j]["p"]) <= SIDEWALK_LINK:
                adj[i].add(j); adj[j].add(i)

    for ci in range(cross_start, n):
        cp = nodes[ci]["p"]
        cand = sorted(range(cross_start), key=lambda k: _d2(cp, nodes[k]["p"]))
        linked = 0
        for k in cand:
            if _d2(cp, nodes[k]["p"]) > CROSS_MAX or linked >= CROSS_LINKS:
                break
            adj[ci].add(k); adj[k].add(ci); linked += 1

    # connectivity over nodes that have an edge
    seen = [False] * n
    comps = []
    for s in range(n):
        if seen[s] or not adj[s]:
            continue
        stack = [s]; seen[s] = True; c = 0
        while stack:
            u = stack.pop(); c += 1
            for v in adj[u]:
                if not seen[v]:
                    seen[v] = True; stack.append(v)
        comps.append(c)
    comps.sort(reverse=True)

    lane = {"nodes": nodes, "neighbors": [sorted(a) for a in adj]}
    stats = {
        "nodes": n, "sidewalk": cross_start, "crosswalk": n - cross_start,
        "linked": sum(1 for a in adj if a),
        "components": comps,
        "largest_component": comps[0] if comps else 0,
        "crosswalks_with_2plus_edges": sum(1 for ci in range(cross_start, n) if len(adj[ci]) >= 2),
    }
    return lane, stats


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--in", dest="indir", default=None)
    args = ap.parse_args()
    repo_root = os.path.dirname(os.path.dirname(HERE))
    cfg = json.load(open(os.path.join(HERE, "slice_config.json")))
    indir = args.indir or os.path.join(repo_root, cfg["output_dir_rel"])
    sg = json.load(open(os.path.join(indir, "sidewalk_graph.json")))
    lane, stats = build(sg)
    with open(os.path.join(indir, "lane_graph.json"), "w") as f:
        json.dump(lane, f, indent=2, sort_keys=True); f.write("\n")
    print("lane_graph: nodes=%(nodes)d sidewalk=%(sidewalk)d crosswalk=%(crosswalk)d "
          "linked=%(linked)d largest=%(largest_component)d cw>=2=%(crosswalks_with_2plus_edges)d" % stats)
    print("  components (top 8):", stats["components"][:8])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
