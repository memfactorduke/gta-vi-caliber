# NPC Living City — the citizen-realism layer

This document maps the NPC realism systems built to make the crowd feel like a
city of people who talk, work, and have their own lives — rather than extras
walking randomly. It is a reference + verification checklist; the deeper design
notes live alongside each header.

Every system follows the repo's pure-core + adapter pattern: scene-free,
deterministic logic in a plain (non-UObject) struct under `NPC/`, covered by an
in-module `GTC.*` automation test, with `AGTCCitizen` (and a couple of
subsystems) as the thin actor adapter that wires it into the running world. The
adapters hand decisions to the animation/audio layer through
`BlueprintImplementableEvent` seams (the same convention as `OnContactReaction`),
so the gameplay choice is testable in C++ and the presentation stays data.

## The voice (every NPC can talk)

A citizen's voice has two halves. `FNpcVoice` (`NPC/Voice/NpcVoice.*`,
`GTC.NPC.Voice`) derives a deterministic vocal signature — pitch, jitter, rate,
breathiness, roughness, formant, mute — from the persona token + spawn seed, so
two "influencer"s sound related but distinct and the crowd spreads across
registers. `FVoiceSynth` (`NPC/Voice/VoiceSynth.*`, `GTC.NPC.Voice.Synth`) turns
a line into stylised PCM (one voiced blip per syllable, paced to the speaker).
`UGTCVoiceComponent` (`NPC/Voice/GTCVoiceComponent.*`) is the runtime adapter: a
procedural sound wave on a spatial audio component that plays the synthesised
utterance. `AGTCCitizen::SpeakLine` drives it, and `OnVoiceAssigned` /
`OnSpeak` expose the profile + lip-sync timing to a MetaHuman/Audio2Face layer.

## Doing their own job

`FNpcOccupation` (`NPC/Decision/NpcOccupation.*`, `GTC.NPC.Decision.NpcOccupation`)
maps each of the 12 archetypes to a trade and a bank of work micro-actions; at
`work` the citizen performs job-specific actions (barista pulls shots, crossing
guard waves traffic) via the idle-action seam. `FWorkBark`
(`NPC/Dialogue/WorkBark.*`, `GTC.NPC.Dialogue.WorkBark`) gives those actions
spoken callouts, and `FNpcHail` (`NPC/Dialogue/NpcHail.*`,
`GTC.NPC.Dialogue.NpcHail`) is what a working citizen calls to the player walking
past (the vendor sells, the life coach pitches). The mime stays silent throughout.

## A social life

`FNpcAcquaintance` (`NPC/Decision/NpcAcquaintance.*`,
`GTC.NPC.Decision.NpcAcquaintance`) is each citizen's bounded, decaying memory of
faces; it persists across despawn/respawn in `FNpcCitizenRecord`. Friends greet
each other warmly in conversation and wave on the street
(`AGTCCitizen::Recognizes` + `UGTCCrowdSubsystem::FindNearestAcquaintance` +
`OnWaveTo`). Conversations have clean turn-taking (`FConversationFloor`,
`GTC.NPC.Conversation.Floor`) and a shared, coherent topic (`FNpcTopic`,
`GTC.NPC.Dialogue.NpcTopic`) so a reply actually answers the opener.

## Reacting to the world

`FNpcWeatherReaction` (`GTC.NPC.Decision.NpcWeatherReaction`) decides shelter /
hurry / discomfort from weather severity + hour + bravery; the weather controller
broadcasts severity to the crowd (`UGTCCrowdSubsystem::SetWeatherSeverity` ->
`AGTCCitizen::ReactToWeather` -> `OnWeatherResponse`). `FNpcCrowding`
(`GTC.NPC.Decision.NpcCrowding`) reads place busyness and makes citizens ring /
queue a packed spot instead of stacking on one point. Pedestrians already halt
and dodge traffic; `FTrafficBark` (`GTC.NPC.Dialogue.TrafficBark`) adds the
flinch + yelp on a near-miss (`OnTrafficStartle`).

## Their own rhythm

`FNpcScheduleJitter` (`GTC.NPC.Decision.NpcScheduleJitter`) shifts each citizen's
routine clock ~±45 min so a shared schedule template doesn't move the city in
lockstep. `FNpcDetour` (`GTC.NPC.Decision.NpcDetour`) drives occasional curious
glances while en route (`OnCuriousGlance`) so people look around rather than
beeline.

## Verification

The pure-core models are deterministic and headless-tested. To validate in this
project's toolchain (see `docs`/`AGENTS.md` for the build path):

1. Build with UnrealBuildTool — this is the load-bearing check that has NOT been
   run for the wiring above; the pure-core logic was validated out-of-tree only.
2. Run the automation tests by prefix: `GTC.NPC.Voice`, `GTC.NPC.Voice.Synth`,
   `GTC.NPC.Decision.NpcOccupation`, `GTC.NPC.Dialogue.WorkBark`,
   `GTC.NPC.Dialogue.NpcHail`, `GTC.NPC.Decision.NpcAcquaintance`,
   `GTC.NPC.Conversation.Floor`, `GTC.NPC.Dialogue.NpcTopic`,
   `GTC.NPC.Decision.NpcWeatherReaction`, `GTC.NPC.Decision.NpcCrowding`,
   `GTC.NPC.Dialogue.TrafficBark`, `GTC.NPC.Decision.NpcScheduleJitter`,
   `GTC.NPC.Decision.NpcDetour` (or all at once with the `GTC.NPC` prefix).
3. Bind the animation/audio seams (`OnVoiceAssigned`, `OnSpeak`, `OnWeatherResponse`,
   `OnTrafficStartle`, `OnWaveTo`, `OnCuriousGlance`, `OnPlaceBusyness`,
   `OnIdleAction`) on the citizen Blueprint to make the decisions visible/audible.

The animation/audio binding (3) and a real build (1) are the outstanding work;
the decision logic itself is complete and tested.
