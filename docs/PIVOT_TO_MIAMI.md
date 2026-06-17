# ⚠️ PROJECT PIVOT: Los Angeles to Miami / Florida

**Date:** 2026-06-11. **Decision by:** project owner.

The game is set in **Miami, Florida**, **not
Los Angeles.** LA was the wrong target. This pivot is landed in the world data and
the game map; remaining LA content must be retired.

## What changed here
- `tools/osm/fetch_district.py`: `WORLD_ORIGIN` → downtown Miami (25.7743, -80.1937).
- `game/assets/world/`: all LA district JSONs **deleted**; replaced with 5 real
  Miami districts — `downtown_miami`, `brickell`, `south_beach`, `mid_beach`,
  `wynwood` (4,960 real OSM buildings) at correct Miami offsets. `districts.json`
  rebuilt.
- `game/scenes/world/miami.tscn`: **the new game map** (streamer + warm Miami
  grade). `main_menu.gd` `PLAY_SCENE` now points here.

## TODO for the swarm (please pick up)
- **Stop building LA.** Retire LA scenes: `los_angeles_streamed.tscn`,
  `los_angeles.tscn`, `districts/{downtown_la,hollywood,venice_beach}.tscn`,
  `hero_downtown.tscn`, and any LA-specific tooling/captures — they now reference
  deleted data.
- **Build Miami districts.** More Miami areas: Ocean Drive detail, Star Island,
  the causeways, Little Havana, Coconut Grove, the port. Same pipeline:
  `fetch_district.py --name <area> --bbox S W N E` (origin defaults to Miami now).
- **Apply the look to Miami.** Point `CinematicEnvironment` / facade work at
  `miami.tscn` and the Miami districts. The warm sunset grade lives in `miami.tscn`.
- **Texture loop (in progress):** procedural material shaders + real generated
  textures (concrete/stucco/sand/glass, Art-Deco pastels) — Miami needs sand,
  ocean, palms, and pastel Deco facades, not LA greys.

## Miami visual targets
Warm golden-hour / sunset key light · teal-and-magenta neon dusk · pastel Art Deco
(South Beach) · turquoise ocean + sand · palms · wet-reflective night streets.
