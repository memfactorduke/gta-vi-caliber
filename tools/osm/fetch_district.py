#!/usr/bin/env python3
"""Fetch a real-world district from OpenStreetMap and normalize it for the game.

The game must build offline (CI has no network), so this tool does all the
network + GIS work *once* and writes a compact JSON resource that the engine
reads deterministically. Heights and road widths are resolved here from
OSM tags into plain numbers; projection to local meters happens in-engine
(the UE5 GeoProjection C++ helper — Wave 1 port of the old geo_projection.gd)
so it stays unit-testable.

Usage:
    tools/osm/fetch_district.py --name downtown_la \\
        --bbox 34.0470 -118.2570 34.0540 -118.2480

Output: Data/World/<name>.json   (OSM data © OpenStreetMap contributors, ODbL)
Re-pointed for UE5: source JSON lives under Data/World/ at the project root and is consumed by the
UE5 World Partition / PCG district-build step (was game/assets/world/ + res:// in the Godot project).
"""
from __future__ import annotations

import argparse
import json
import sys
import urllib.parse
import urllib.request
from pathlib import Path

# Shared world anchor for the whole map: downtown Miami, Florida (Miami).
# Every district projects against this so they line up at correct real-world
# offsets, ready for a streamer to page them in/out of one continuous world.
WORLD_ORIGIN = (25.7743, -80.1937)

OVERPASS_URL = "https://overpass-api.de/api/interpreter"
USER_AGENT = "open-world6-OSS-worldgen/0.1 (https://github.com/; open-source game world importer)"

# Storeys → metres. OSM `building:levels` is the most reliable height signal.
METRES_PER_LEVEL = 3.2
# Fallback heights (metres) by building type when no level/height tag exists.
DEFAULT_HEIGHT = {
    "skyscraper": 120.0,
    "apartments": 18.0,
    "residential": 9.0,
    "house": 6.0,
    "commercial": 12.0,
    "retail": 8.0,
    "office": 24.0,
    "industrial": 10.0,
    "parking": 12.0,
    "hotel": 40.0,
    "church": 16.0,
    "yes": 9.0,
}
# Road full width (metres) by highway class, and a default lane count.
ROAD_SPEC = {
    "motorway": (16.0, 4),
    "trunk": (14.0, 4),
    "primary": (12.0, 3),
    "secondary": (10.0, 2),
    "tertiary": (9.0, 2),
    "residential": (7.0, 2),
    "service": (4.5, 1),
    "living_street": (6.0, 1),
    "pedestrian": (5.0, 1),
    "footway": (2.0, 1),
    "path": (1.5, 1),
    "steps": (2.0, 1),
    "cycleway": (2.5, 1),
}
ROAD_CLASSES = set(ROAD_SPEC)


def _parse_metres(value: str) -> float | None:
    """Parse an OSM height string like '47', '47 m', '47.5' → metres."""
    try:
        return float(str(value).split()[0].replace(",", "."))
    except (ValueError, IndexError):
        return None


def building_height(tags: dict) -> float:
    if "height" in tags:
        h = _parse_metres(tags["height"])
        if h:
            return h
    if "building:levels" in tags:
        lv = _parse_metres(tags["building:levels"])
        if lv:
            return max(3.0, lv * METRES_PER_LEVEL)
    kind = tags.get("building", "yes")
    return DEFAULT_HEIGHT.get(kind, DEFAULT_HEIGHT["yes"])


def road_spec(tags: dict) -> tuple[float, int]:
    width, lanes = ROAD_SPEC.get(tags.get("highway", ""), (6.0, 2))
    if "width" in tags:
        w = _parse_metres(tags["width"])
        if w:
            width = w
    if "lanes" in tags:
        try:
            lanes = max(1, int(float(tags["lanes"])))
        except ValueError:
            pass
    return width, lanes


def build_query(s: float, w: float, n: float, e: float) -> str:
    bbox = f"{s},{w},{n},{e}"
    return (
        "[out:json][timeout:120];"
        "("
        f'way["building"]({bbox});'
        f'way["highway"]({bbox});'
        ");"
        "out geom;"
    )


def fetch(query: str) -> dict:
    data = urllib.parse.urlencode({"data": query}).encode()
    req = urllib.request.Request(OVERPASS_URL, data=data, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=180) as resp:
        return json.load(resp)


def normalize(raw: dict, world_origin: dict | None = None) -> dict:
    elements = raw.get("elements", [])
    buildings: list[dict] = []
    roads: list[dict] = []
    lats: list[float] = []
    lons: list[float] = []

    for el in elements:
        geom = el.get("geometry")
        tags = el.get("tags", {})
        if not geom:
            continue
        ring = [[round(p["lat"], 7), round(p["lon"], 7)] for p in geom]
        for p in geom:
            lats.append(p["lat"])
            lons.append(p["lon"])

        if "building" in tags:
            buildings.append(
                {
                    "id": el["id"],
                    "name": tags.get("name", ""),
                    "kind": tags.get("building", "yes"),
                    "height_m": round(building_height(tags), 2),
                    "footprint": ring,
                }
            )
        elif tags.get("highway") in ROAD_CLASSES:
            width, lanes = road_spec(tags)
            roads.append(
                {
                    "id": el["id"],
                    "kind": tags["highway"],
                    "name": tags.get("name", ""),
                    "width_m": round(width, 2),
                    "lanes": lanes,
                    "path": ring,
                }
            )

    if not lats:
        raise SystemExit("error: OSM returned no geometry for this bbox")

    # Centroid of this district's geometry, kept for reference / map UI.
    centroid = {
        "lat": round(sum(lats) / len(lats), 7),
        "lon": round(sum(lons) / len(lons), 7),
    }
    # The engine projects every district against ONE shared world origin so all
    # districts sit at correct real-world offsets in a single seamless LA map
    # (a streamer can then page them in/out). Defaults to the downtown anchor.
    origin = world_origin if world_origin else centroid
    return {
        "attribution": "© OpenStreetMap contributors, ODbL (https://www.openstreetmap.org/copyright)",
        "origin": origin,
        "centroid": centroid,
        "bounds": {
            "min_lat": round(min(lats), 7),
            "min_lon": round(min(lons), 7),
            "max_lat": round(max(lats), 7),
            "max_lon": round(max(lons), 7),
        },
        "buildings": buildings,
        "roads": roads,
    }


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--name", required=True, help="district id, e.g. downtown_la")
    ap.add_argument(
        "--bbox",
        nargs=4,
        type=float,
        required=True,
        metavar=("S", "W", "N", "E"),
        help="south west north east, decimal degrees",
    )
    ap.add_argument("--out-dir", default="Data/World")
    ap.add_argument(
        "--world-origin",
        nargs=2,
        type=float,
        default=list(WORLD_ORIGIN),
        metavar=("LAT", "LON"),
        help="shared projection origin for the whole LA map (default: downtown anchor)",
    )
    args = ap.parse_args()

    s, w, n, e = args.bbox
    print(f"querying Overpass for {args.name} bbox=({s},{w},{n},{e}) ...", file=sys.stderr)
    raw = fetch(build_query(s, w, n, e))
    origin = {"lat": args.world_origin[0], "lon": args.world_origin[1]}
    district = normalize(raw, world_origin=origin)
    district["name"] = args.name

    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    out_path = out_dir / f"{args.name}.json"
    out_path.write_text(json.dumps(district, separators=(",", ":")))

    nb, nr = len(district["buildings"]), len(district["roads"])
    print(f"wrote {out_path}  ({nb} buildings, {nr} roads)", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
