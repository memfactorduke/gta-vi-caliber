#!/usr/bin/env python3
"""Rebuild the district manifest (Data/World/districts.json) from every
district JSON present, computing each district's offset from the shared world
origin. Run after fetching new districts so World Partition / the district build
picks them up — adding coverage becomes pure data, no code changes.

Usage:  tools/osm/build_manifest.py
"""
from __future__ import annotations

import json
import math
import sys
from pathlib import Path

from fetch_district import WORLD_ORIGIN


def main() -> int:
    world = Path("Data/World")
    o_lat, o_lon = WORLD_ORIGIN
    cos0 = math.cos(math.radians(o_lat))

    districts = []
    for f in sorted(world.glob("*.json")):
        if f.name == "districts.json":
            continue
        d = json.loads(f.read_text())
        if "centroid" not in d:
            continue
        c = d["centroid"]
        north = (c["lat"] - o_lat) * 111320
        east = (c["lon"] - o_lon) * 111320 * cos0
        districts.append(
            {
                "name": d.get("name", f.stem),
                "data": f"Data/World/{f.name}",
                "centroid": c,
                "world_offset": {"x": round(east, 1), "z": round(-north, 1)},
                "buildings": len(d.get("buildings", [])),
                "roads": len(d.get("roads", [])),
            }
        )

    manifest = {
        "world_origin": {"lat": o_lat, "lon": o_lon},
        "attribution": "© OpenStreetMap contributors, ODbL",
        "districts": districts,
    }
    out = world / "districts.json"
    out.write_text(json.dumps(manifest, indent=2))
    print(f"wrote {out}: {len(districts)} districts", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
