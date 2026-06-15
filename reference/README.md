# reference/ — local-only study material

Drop art-direction reference footage here (e.g. open-world game trailers you
are studying for lighting, density, and mood). Files in this folder are
**gitignored** — only this README is committed.

**Never commit copyrighted footage, screenshots, or extracted assets to the
repository.** Reference material is for local, private study only. Anything
that ships in the game must be original or CC0/CC-BY licensed — see
[docs/ASSETS.md](../docs/ASSETS.md).

## Canonical study footage

These are the official trailers the project's art direction is calibrated
against. Watch them on YouTube, or grab a personal local copy (1080p is
plenty) for frame-by-frame study:

| Reference | Official link | Study for |
| --- | --- | --- |
| AAA open-world reference — Trailer 1 | <https://www.youtube.com/watch?v=QdBZY2fkU-0> | Golden-hour lighting, beach/street crowd density, color grade |
| AAA open-world reference — Trailer 2 | <https://www.youtube.com/watch?v=VQRLujxTm3c> | Neon-wet nights, interiors, character close-ups, motion feel |

To download a local copy for study (stays on your machine — gitignored):

```sh
yt-dlp -f "bv*[height<=1080]+ba" "https://www.youtube.com/watch?v=QdBZY2fkU-0" -o "reference/%(title)s.%(ext)s"
yt-dlp -f "bv*[height<=1080]+ba" "https://www.youtube.com/watch?v=VQRLujxTm3c" -o "reference/%(title)s.%(ext)s"
```

When referencing a specific moment in an issue or PR discussion, cite it as a
timestamp (e.g. "Trailer 2 @ 1:14, the wet asphalt reflections") rather than
attaching frames — keeps the repo clean of copyrighted imagery.
