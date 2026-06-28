---
name: caliber
description: >-
  GT-caliber (the GTC_UE5 open-world game on Unreal Engine 5.8) project how-to —
  the single skill for working in this repo's live Unreal editor. Use whenever a
  task touches the running editor or the game's content: driving the Unreal MCP
  (unreal-mcp on :8000) or editor Python; editing HUD/widgets, materials,
  Blueprints, levels, or actors; deciding whether something should be C++ vs
  Blueprint vs Anim Blueprint; figuring out "which BP / which map / which ABP is
  which"; running or debugging PIE; adding a usable weapon or fixing a weapon
  pickup ("I press E and nothing happens"); or rigging/retargeting a new
  character model so it can play the animation library. Routes to bundled topic
  files — read only the one(s) the task needs.
---

# Caliber — project skill (GT-caliber, UE 5.8)

This is the one skill for getting work done in the GT-caliber project's **live
running editor**. It is a router: the detail lives in the bundled topic files
below, so **read only the file(s) the current task needs** (progressive
disclosure — don't load them all).

## Hard rules (always apply)

- **One editor, never touch its lifecycle.** There is always exactly one Unreal
  editor running. Never close / kill / restart it and never launch a second one.
  Reuse the running one through a live connection. If none is reachable, **stop
  and ask** — do not start one. (A PreToolUse hook enforces this; see
  `CLAUDE.md`.)
- **Drive `unreal-mcp` (:8000), not vibe-ue.** The live control surface is the
  `unreal-mcp` HTTP MCP. VibeUE / `unrealclaude` are legacy and not wired up —
  ignore old docs that say otherwise (details in `editor-mcp.md`).

## Pick the topic file

| Read this | When the task is… |
| --- | --- |
| `editor-mcp.md` | Driving the editor at all — MCP toolsets, the `list_toolsets`→`describe_toolset`→`call_tool` call convention, what MCP can/can't do, PIE control, capturing the screen, and the gotchas. **Start here for any editor work.** |
| `coding-layers.md` | Deciding *where* a change belongs — C++ vs Blueprint vs UMG widget vs Anim Blueprint — and the conventions for each layer. |
| `asset-map.md` | "Which GameMode/pawn boots the game? which map? which ABP? which HUD widget?" — the inventory of the live BP / map / ABP set with paths. |
| `weapon-pickup.md` | Adding a usable weapon, wiring a weapon pickup, swapping a weapon mesh, or debugging "I press E and nothing happens / the KA74 won't pick up but the AR does." |
| `animate-model.md` | Giving any new 3D humanoid model the full animation library — auto IK Rig + Manny→model retargeter + full-body IK + bake (`Scripts/gtc_anim`). |

Most editor tasks read `editor-mcp.md` plus the topic file. Asset/layer questions
may only need `coding-layers.md` + `asset-map.md`.

## Adding a new topic

Add a `.md` file in this skill folder (e.g. `vehicles.md`), keep it
self-contained (mental model → recipe → verify, with real `/Game/...` paths),
and add a row to the table above. Keep new topics as **bundled files in this one
skill**, not as separate skills — the family is intentionally a single skill with
on-demand topic files.
