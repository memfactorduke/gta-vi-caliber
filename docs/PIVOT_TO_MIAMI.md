# ⚠️ PROJECT PIVOT: Los Angeles to Miami / Florida

**Date:** 2026-06-11. **Decision by:** project owner.

The game is set in **Miami, Florida**, **not
Los Angeles.** LA was the wrong target. This pivot is landed in the world data and
the game map; remaining LA content must be retired.

## What changed here
- `tools/osm/fetch_district.py`: `WORLD_ORIGIN` → downtown Miami (25.7743, -80.1937).
- `Data/World/`: all LA district JSONs **deleted**; replaced with 5 real
  Miami districts: `downtown_miami`, `brickell`, `south_beach`, `mid_beach`,
  `wynwood` (4,960 real OSM buildings) at correct Miami offsets. `districts.json`
  rebuilt.
- `Content/Maps/Miami.umap`: **the new game map** (World Partition + warm Miami
  grade). The main-menu `PLAY_SCENE`/`GameDefaultMap` now points here.

## TODO for the swarm (please pick up)
- **Stop building LA.** Retire LA levels: `LosAngelesStreamed.umap`,
  `LosAngeles.umap`, `Districts/{DowntownLA,Hollywood,VeniceBeach}.umap`,
  `HeroDowntown.umap`, and any LA-specific tooling/captures: they now reference
  deleted data.
- **Build Miami districts.** More Miami areas: Ocean Drive detail, Star Island,
  the causeways, Little Havana, Coconut Grove, the port. Same pipeline:
  `fetch_district.py --name <area> --bbox S W N E` (origin defaults to Miami now).
- **Apply the look to Miami.** Point `CinematicEnvironment` / facade work at
  `Miami.umap` and the Miami districts. The warm sunset grade lives in `Miami.umap`.
- **Texture loop (in progress):** procedural materials + real generated
  textures (concrete/stucco/sand/glass, Art-Deco pastels): Miami needs sand,
  ocean, palms, and pastel Deco facades, not LA greys.

## Miami visual targets
Warm golden-hour / sunset key light · teal-and-magenta neon dusk · pastel Art Deco
(South Beach) · turquoise ocean + sand · palms · wet-reflective night streets.
