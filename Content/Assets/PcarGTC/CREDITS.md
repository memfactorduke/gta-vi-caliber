# Credit & License — Porsche 911 GT3 (drivable car model)

Attribution and license record for the 3D model used as the GTC drivable vehicle
(`carGTC1` skeletal mesh / `BP_VehicleBase`). Kept here, next to the asset, so the
attribution is never lost. **When the car ships, this credit must appear in the
in-game credits screen** (that is the CC-BY requirement) — fold it into the repo's
`CREDITS.md` at that point.

## Model
- **Title:** Porsche 911 GT3
- **Author:** ChevroletSS (Sketchfab)
- **Source:** Sketchfab — `https://sketchfab.com/...`  ← paste the exact model URL
  (use the **"Copy Credits"** button on the model's download page)
- **Format downloaded:** `.blend` (original format)
- **Used in GTC as:** `Content/Assets/PcarGTC/carGTC1` (skeletal mesh) + `BP_VehicleBase`.
  Source `.blend` lives at `caliber/Cars/porsche-911-gt3/source/`. The rig (skeleton,
  4-wheel Chaos setup), physics asset, and material tuning are original project work.

## License
- **License:** Creative Commons Attribution 4.0 International (CC BY 4.0)
- **License text:** https://creativecommons.org/licenses/by/4.0/
- **Commercial use:** Allowed.
- **Requirement:** The author must be credited wherever the work (or a derivative of
  it) is distributed.

## Required attribution line (put this in the shipped game's credits)
> "Porsche 911 GT3" (`https://sketchfab.com/...`) by **ChevroletSS**, licensed under
> **CC BY 4.0** (https://creativecommons.org/licenses/by/4.0/). Modified for GTC
> (rigged as a Chaos vehicle; physics, wheels, and materials are project work).

*(Replace the two `https://sketchfab.com/...` placeholders with the exact URLs from
the page's "Copy Credits" button.)*

## ⚠️ License is NOT the same as trademark
CC BY 4.0 covers the **uploader's copyright in the 3D model**. It does **not** grant
any rights to **Porsche's trademark / trade dress** — the "Porsche" name, the 911
body design, and badges belong to Porsche AG, and no Sketchfab uploader can license
those. Porsche enforces this, especially against commercial games.

- **Local development / testing:** fine — this credit covers you.
- **A shipped / sold build:** plan to **reskin to a non-branded car body**. The whole
  rig — skeleton, wheels, physics, enter/exit, input, camera — carries straight over
  to a new body; only the visible shell changes. (This matches the project's own
  standing decision in `docs/ASSETS.md` / project notes.)
