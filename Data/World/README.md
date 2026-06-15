# Data/World — OSM-derived district source data

INVESTIGATE #7 is **decided: keep the software-generated world from real Miami OSM data** (authored
levels rejected). This folder is the re-pointed output location for the OSM pipeline (was
`game/assets/world/` in the Godot project):

- `tools/osm/fetch_district.py --name <district> --bbox S W N E` writes `<district>.json` here.
- `tools/osm/build_manifest.py` regenerates `districts.json` (offsets from the shared Miami world
  origin) from every district JSON present.

Each district JSON is engine-agnostic: `buildings[]` (`id, name, kind, height_m, footprint:[[lat,lon]]`)
and `roads[]` (`id, kind, name, width_m, lanes, path:[[lat,lon]]`). Projection to local metres happens
in-engine (the Wave 1 `GeoProjection` C++ helper).

**Migration note:** porting the existing 5 Miami district JSONs and standing up the UE5 World-Partition
/ PCG build that consumes them is the INVESTIGATE #7 follow-on (Wave 3 "District build/commit
pipeline"), not part of the FOUNDATION pass — so the large data files are intentionally not copied here
yet. The pipeline above is ready to (re)generate them into this folder.
