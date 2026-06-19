# GTC face / appearance pipeline

How "an actual human face for me and every NPC" is wired, and exactly what you
drop in (in-editor) to light it up. The C++ seams already exist and compile; the
remaining work is authoring/importing the head + body assets and pointing the
slots at them. Nothing here requires touching engine code.

## The two seams (already in code)

Both the player and every NPC carry a **separate head/face skeletal mesh** that
rides the body as a *leader-pose follower* — it animates off the body's skeleton,
so one body animation drives the face's head/neck motion. This is the standard
MetaHuman body+face rig shape.

- **NPCs** — `AGTCCitizen` owns a `HeadMesh` component (created in the ctor,
  attached to the body mesh). Each persistent citizen has a deterministic
  `HeadVariant` in `[0, FNpcPopulation::NumHeadVariants)` (= 8), derived from its
  seed, so the same person always wears the same face. `ApplyIdentity` loads the
  head for that variant from `HeadMeshPaths[]` and calls `SetLeaderPoseComponent`.
  Files: `Source/GTC_UE5/NPC/Agent/GTCCitizen.{h,cpp}`,
  `Source/GTC_UE5/NPC/Population/NpcPopulation.{h,cpp}`.

- **Player** — `AGTCPlayerCharacter` owns a `HeadMesh` component plus three
  soft-referenced slots: `BodyMesh`, `FaceMesh`, `BodyAnimClass`. `ApplyAppearance`
  (called from `BeginPlay`) loads them, guarded. Repoint `FaceMesh` at a MetaHuman
  head and that is *you*.
  File: `Source/GTC_UE5/Player/Pawn/GTCPlayerCharacter.{h,cpp}`.

Every load is soft + guarded: if the asset isn't there yet, the character simply
keeps its body mesh's built-in head and nothing crashes (headless build stays
green). So you can author assets incrementally.

## Asset paths the code expects

Put assets here (or repoint the slots in a Blueprint child / the C++ defaults):

```
NPC faces:    /Game/GTCaliberAssets/Content/Characters/Heads/SK_Head_00 .. SK_Head_07
Player body:  /Game/GTCaliberAssets/Content/Characters/Player/SK_PlayerBody
Player face:  /Game/GTCaliberAssets/Content/Characters/Player/SK_PlayerFace
Player anim:  /Game/GTCaliberAssets/Content/Characters/Player/ABP_Player
```

Each `SK_Head_0N` (and the player face) must be **skinned to the same skeleton as
the body mesh** so the leader-pose follow works. For MetaHuman, that means using
the MetaHuman face mesh against the MetaHuman body skeleton.

## Option A — MetaHuman (hero / player / close-up NPCs)

1. Enable the MetaHuman plugin (UE 5.6+ ships it integrated; confirm for 5.8).
2. Create or import a MetaHuman; for *your* face, use MetaHuman Animator's
   mesh-from-photos / capture path, or sculpt in MetaHuman Creator.
3. In the imported MetaHuman, the body is one skeletal mesh and the face is a
   second skeletal mesh that follows it — exactly our body + `HeadMesh` split.
4. Point the player's `BodyMesh` / `FaceMesh` / `BodyAnimClass` at the MetaHuman
   body, face, and a body locomotion ABP. For a handful of named NPCs, drop the
   MetaHuman face at one of the `SK_Head_0N` slots.

MetaHuman is expensive per character — reserve it for the player and a small set
of story NPCs the camera gets close to. Do **not** put it on the whole crowd.

## Option B — modular heads (the crowd)

For the streamed crowd, author ~8 lightweight modular heads skinned to the body
skeleton and save them as `SK_Head_00..07`. The seeded `HeadVariant` spreads them
across the population for free, and combined with the existing per-archetype tint
the crowd stops reading as clones. Add skin-tone / hair material variation on top
later (next pipeline step — see TODO).

## Crowd tiering (so faces don't wreck the frame budget)

The persistent-population layer already separates "who exists" (cheap roster
records) from "who has a body" (streamed actors near the player). Layer face cost
on top of that:

- Crowd distance: no active face rig — the baked head mesh is enough.
- Conversational NPCs / contacts: rigged head at a reduced LOD.
- Player + named story NPCs: full MetaHuman face.

The promotion seam is `AGTCCitizen::ApplyIdentity` (swap to a richer head when a
citizen becomes a contact / the player gets close).

## Lip-sync hook (wired)

`AGTCCitizen::OnSpeak(Line, EstimatedSeconds)` is a `BlueprintImplementableEvent`
fired whenever a citizen voices ANY line — ambient bark, contact retort, pass-by
greeting, idle mutter, or chatter — funnelled through `SpeakLine`. Bind it in the
citizen's Anim Blueprint (or a MetaHuman/Audio2Face audio component on `HeadMesh`)
to drive jaw/viseme motion for the estimated duration. The C++ already picks and
logs the line; an unbound event is harmless. Point a real audio clip's length at
the driver to override `EstimatedSeconds`.

## Face LOD promotion (wired)

`UGTCAppearanceSet.HeroHeadMeshes` is the high-detail (MetaHuman) head pool, same
`HeadVariant` indexing as the crowd `HeadMeshes`. `AGTCCitizen::SetFaceHighDetail`
swaps a citizen between the two and re-applies the leader-pose follow; the crowd
director (`UGTCCrowdSubsystem::ApplyTickLod`) promotes citizens within
`FacePromoteRadius` and demotes them past it. So author cheap heads in `HeadMeshes`
for the crowd and MetaHuman heads in `HeroHeadMeshes`; only the faces near the
player pay the rigged cost. Leave `HeroHeadMeshes` empty to stay single-tier.

## TODO (later loop iterations)

- Hair/outfit modular variation per citizen (beyond skin tone + outfit tint).
- Give the player the same `UGTCAppearanceSet` + `OnSpeak` treatment.
- Off-screen position sim so an embodied citizen spawns at its current activity's
  place rather than the streaming ring.
