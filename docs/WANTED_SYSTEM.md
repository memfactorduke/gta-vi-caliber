# Wanted System — design & build plan

> Goal: a **gritty, realistic, immersive** police-response system that beats GTA's
> abstract stars. The player should read the world (sirens, search zones, radio
> chatter, line of sight) instead of a meter, and escape by *acting like a
> fugitive* — break contact, change appearance, lay low — not by draining a bar.
>
> Status: design. Chosen direction = **gritty realism (sim)**. This doc is the
> spec for a phased build. It is grounded in the C++ that already exists (see
> "What already exists").

---

## 1. Why this beats GTA

GTA's wanted system fakes the hard parts: cops are effectively omniscient, the
"meter" is an abstract star count, and you escape by outrunning a radius. Three
things make a police response *feel real*, and GTA does none of them well:

1. **Perception is earned.** A crime only matters if someone *sees* it, and a
   witness needs time to call it in. Killing or fleeing the only witness should
   actually work.
2. **The hunt is spatial and informational.** Dispatch routes units to a
   *last-known position*; sirens come from real directions; when you break line
   of sight, police *search an area* that shrinks over time (you leave it — you
   don't drain it). This is the RDR2 "search zone" feel, done deeper.
3. **They're hunting a description, not a GUID.** Change clothes, swap vehicles,
   get indoors → the description goes stale → you cool down and can slip a
   pursuit. This is the single biggest differentiator from GTA.

Feedback is **diegetic**: a search zone on the minimap, spatial police radio and
sirens, a helicopter searchlight, a red edge-vignette only while actively
spotted. The abstract star readout becomes minimal/optional.

---

## 2. What already exists (reuse, don't rebuild)

The C++ wanted core was ported from a sim-grade Godot design and is unit-tested,
but the live game currently **bypasses most of it** (a pedestrian hit calls the
unconditional `ReportCrime`, omnisciently). The realistic behavior is mostly a
*wiring* job.

| Piece | File | What it already does | Used live? |
| --- | --- | --- | --- |
| Heat + star quantisation | `Systems/Wanted/WantedSystem.{h,cpp}` (`FWantedSystem`) | Heat accumulate/decay, thresholds `[1,3,6,10,16]`, `Stars()` | yes |
| Subsystem orchestrator | `Systems/Wanted/WantedSubsystem.{h,cpp}` (`UWantedSubsystem`) | owns models, `ReportCrime`/`ReportWitnessedCrime`/`TickFrame`/`UpdateEvasion`, `OnStarsChanged`, save hooks | partial |
| **Witness perception** | `Systems/Wanted/CrimeWitness.{h,cpp}` (`FCrimeWitness`, `FCrimeObserver`) | LOS + FOV cones (peds vs police), witness-count→heat saturating curve, crowd-aware **report delay**, **silenceable** in-progress report | **NO — dormant** |
| **Go-cold evasion** | `Systems/Wanted/WantedEvasion.{h,cpp}` (`FWantedEvasion`) | `Visible → Searching → Cold` state machine, search duration scales with heat, re-sight refill, `NotifyCrime` | **NO — dormant** |
| Crime taxonomy | `World/Police/WantedLevel.{h,cpp}` | crime types + heat (trespass 0.5, theft 1.5, reckless 1.0, assault 3.0, shooting 5.0) | partial |
| Police actors / escalation | `AI/Police/GTCPoliceDirector`, `GTCPoliceOfficer`, `GTCPoliceCar`, `GTCSwatVan`, `GTCPoliceHelicopter` | spawn plan, read `Stars()`, drive `TickFrame`, report crimes they witness | yes |
| Emergency dispatch gate | `AI/Emergency/EmergencyServices.{h,cpp}` | `ShouldDispatch(incident, bPlayerCaused, WantedStars)` | partial |
| Crime triggers | `NPC/Agent/GTCCitizen.cpp` (1135/1179), police actors | player wounds/kills → `ReportCrime` | yes (unconditional) |
| Arrest / bust | `ArrestSubsystem`, `WBP_Busted`, `WBP_ArrestMeter` | grapple/arrest, BUSTED screen, fee | yes |
| HUD stars | `Content/UI/WBP_WantedStars` + wired into `WB_Game_HUD_widget` | text ★/☆ driven by `OnStarsChanged` | yes (just fixed) |
| Minimap | `M_Minimap` RT (SceneCapture) | live radar | yes |
| Disguise enablers | character creator (outfits), enterable buildings, vehicles | exist as systems | n/a |

Key gap: `UWantedSubsystem::Stars()` / `ReportCrime` / `ReportWitnessedCrime` are
**not `UFUNCTION(BlueprintCallable)`** and there is no per-frame observer/LOS
feed. The witness + evasion models are pure and tested but nothing supplies them
`FCrimeObserver`s or `bSeenByPolice`. Bridging that is the core of Phase 2.

---

## 3. The experience (target scenario)

> You pickpocket someone in a quiet alley — nobody in line of sight. **No heat.**
> You do it on Ocean Drive at dusk — two pedestrians inside the FOV cone see it.
> A **report timer** starts (a few seconds; faster with a bigger, closer crowd).
> If you sprint off and break their sight before it lands, no dispatch. If it
> lands, **radio chatter** fires and a **search zone** appears on the minimap
> around your last-known position. Sirens approach *from the direction units are
> coming*. A cop rounds the corner and spots you → state snaps to **Pursuit**, a
> red edge-vignette pulses, the helicopter searchlight sweeps. You duck into a
> store (enterable building) and change your jacket → you're no longer matching
> the **description**; the search timer drains faster and the zone stops tracking
> you. You wait it out in cover → **Cold**. Heat clears.

This is the loop: **commit → be perceived → dispatch → pursuit ⇄ search →
escape (break contact + change description) or get caught (arrest)**.

---

## 4. State model

Replace "0–5 stars" *as the mental model* with an explicit awareness state
(stars become a secondary severity readout). Backed by `FWantedEvasion` +
`FWantedSystem`:

```
CLEAN ──crime witnessed──▶ REPORTED ──unit arrives & sees you──▶ PURSUIT
  ▲                            │                                   │
  │                            │ break LOS                         │ break LOS
  │                            ▼                                   ▼
  └──── COLD ◀──timer──── SEARCHING ◀───────re-spotted────────────┘
                              │
                         caught → ARREST (BUSTED)
```

- **Clean** — no heat (`Stars()==0`, evasion `Cold/Visible` idle).
- **Suspected** (optional, light) — a witness is mid-report (`FCrimeWitness`
  in progress, not yet landed). Silence/flee to cancel.
- **Reported** — report landed, heat applied, dispatch begins; units en route.
- **Pursuit** — a cop has LOS (`FWantedEvasion::Visible`, `bSeenByPolice`).
- **Searching** — LOS broken; `FWantedEvasion::Searching`, `SearchProgress()`
  ramps 0→1; search zone shrinks. Re-sight → back to Pursuit (refill).
- **Cold** — search elapsed unseen (`FWantedEvasion::Cold`) → clear heat.
- **Arrest** — caught while wanted → existing `ArrestSubsystem` / BUSTED.

Severity (current heat → stars `[1,3,6,10,16]`) controls *how hard* the response
is (number/type of units, search duration via `EvasionBaseSeconds +
EvasionPerStarSeconds * Stars`, helicopter/SWAT gating through
`EmergencyServices::ShouldDispatch`).

---

## 5. Mechanics

### 5.1 Perception (who knows, and when)
Route player crimes through `UWantedSubsystem::ReportWitnessedCrime(bKilled,
crimePos, observers)` instead of unconditional `ReportCrime`. A per-frame (or
per-crime) **observer gatherer** builds `TArray<FCrimeObserver>` from nearby
pedestrians + police (position, facing/forward, `bIsPolice`) within a broad
radius; `FCrimeWitness::CollectWitnesses` applies the real cones. Cop witness →
**instant** radio. Civilian witness → **delayed** call via the in-progress
`FCrimeWitness` report (`ReportDelayFor` already scales by crowd size/proximity).
Unseen crime → **no heat**. Silencing/killing/fleeing all witnesses before the
timer lands → no report (`Silence()`).

### 5.2 Dispatch (spatial + delayed)
On a landed report, record **last-known position** (the crime/sighting point).
Units route there, not to the live player. Sirens are spatialized and approach
from the unit's bearing. Dispatch *callouts* reference direction ("heading north
on Ocean Drive"). Helicopter/SWAT gated by severity via `EmergencyServices`.

### 5.3 Escalation by crime & context (not just count)
Use the `WantedLevel` crime taxonomy so *what* you did and *where* matters:
a silent theft (1.5) ≠ a public shooting (5.0); more witnesses → more heat
(saturating). Context multipliers (later): near a police station, daytime,
dense crowd → faster/larger response.

### 5.4 Search & go-cold
Feed `UpdateEvasion(bSeenByPolice, dt)` every frame from real police LOS. Break
LOS → `Searching`; `SearchProgress()` drives the shrinking minimap zone and the
star "flash". Elapsed unseen → `Cold` → `UWantedSubsystem::Clear()` (or decay to
0). Re-sight refills. A fresh crime during search → `NotifyCrime()` re-hots you.

### 5.5 Description / identity (the differentiator)
Track an **Identified** factor on the player: matched by current *outfit* +
*vehicle* against the description on file at report time. Reduce it by:
changing clothes (character creator), swapping/ditching the vehicle, or being
**indoors / out of sight** (enterable buildings). Low Identified →
faster search drain, smaller zone, easier to lose. Re-committing or being
clearly re-spotted refreshes the description.

### 5.6 De-escalation paths (player agency)
Multiple believable ways down, not just "wait": break contact + stay unseen
(search → cold); change description; pay it off at a Pay'n'Spray-style shop
(`GTCPaySprayShop` already touches `UWantedSubsystem`); or get arrested (BUSTED,
fee). No magic decay while a cop is staring at you (already enforced by the
crime-active hold + `Visible` state).

---

## 6. Feedback layer (diegetic, gritty)

Felt, not displayed. Priority order = biggest immersion per effort:

1. **Search zone on minimap** — a circle/cone at last-known position that shrinks
   with `SearchProgress()`; you must physically leave it. (Reuse `M_Minimap` RT.)
2. **Spatial audio** — police radio chatter on report, sirens that pan/Doppler
   from each unit's real direction, dispatch callouts with direction/severity.
3. **Spotted vignette** — subtle red screen-edge pulse *only* in `Pursuit`
   (post-process), fading in `Searching`.
4. **Helicopter searchlight** — a moving cone during airborne search.
5. **Minimal stars** — keep `WBP_WantedStars` as a small severity readout (maybe
   only while wanted), de-emphasized; the *state* is conveyed by the world.

---

## 7. Architecture & layer mapping

Per repo rules (C++-first, events up / calls down, levels self-contained):

- **C++ (logic, `Source/GTC_UE5/`, needs build, `GTC.*` tests):** all heat /
  witness / evasion / identity math stays in the pure models + `UWantedSubsystem`.
  Add: `UFUNCTION(BlueprintCallable)` surface (`GetStars`, `IsWanted`,
  `GetAwarenessState`, `GetSearchProgress`, `GetLastKnownLocation`,
  `GetIdentifiedFactor`, `NotifyPlayerCrime`); the observer gatherer + police-LOS
  feed (an `AGTCPoliceDirector` responsibility — it already drives `TickFrame`);
  the identity/description model; new delegates (`OnAwarenessStateChanged`,
  `OnSearchZoneUpdated`). Keep pure logic in non-UObject structs with tests
  (pattern: existing `FCrimeWitness`/`FWantedEvasion` + their `Tests/`).
- **Blueprint (glue/tunables):** `GTCPoliceDirector` / police actors feed LOS &
  observers; `WB_Game_HUD_widget` hosts widgets; audio cue triggers; PaySpray.
- **UMG (`Content/UI/`):** awareness-state widget, search-zone overlay on
  minimap, spotted vignette material, de-emphasized stars.
- **Audio:** radio/siren/dispatch cues (assets per `docs/ASSETS.md` provenance).

Note: anything touching C++ logic needs a UnrealBuildTool build; on this Mac
builds have been constrained, so Phase 1 (feel) is deliberately doable **live in
the editor** (BP/UMG/audio/materials) against the *existing* heat signal.

---

## 8. Phased plan

Each phase ships independently and leaves `main` bootable.

### Phase 1 — Feel layer (live-editable; no C++ build) ⟵ start here after doc
Make the *existing* heat feel alive, purely in BP/UMG/audio/PP.
- Minimap **search-zone** overlay driven off current stars/`OnStarsChanged`
  (approximate last-known = player pos until Phase 2 supplies the real one).
- Police **radio chatter + sirens** (spatial) on wanted gain; **spotted
  vignette** PP while wanted; helicopter **searchlight** cone.
- Keep stars minimal.
- **Done when:** committing a crime produces audible dispatch + a minimap search
  zone + vignette, and it all clears when heat decays. PIE-verified.

### Phase 2 — Switch on the simulation (needs C++ build)
- BlueprintCallable surface + delegates on `UWantedSubsystem`.
- Observer gatherer + police-LOS feed in `GTCPoliceDirector`; route player crimes
  through `ReportWitnessedCrime`; wire `UpdateEvasion` from real LOS.
- Replace approximate feedback with the real **last-known position**, **report
  delay**, and **search-zone shrink** (`SearchProgress`).
- Awareness-state HUD (Suspected→…→Cold).
- **Done when:** unseen crimes don't alert; witnessed crimes call in after a
  delay you can interrupt by silencing/fleeing; breaking LOS starts a real search
  that goes cold. New `GTC.*` tests for the gatherer/identity stay green.

### Phase 3 — Description / disguise (needs C++ build + content)
- Identity model (outfit+vehicle ⇒ Identified factor); hook character creator,
  vehicle swap, indoors.
- Identified factor modulates search drain + zone size.
- **Done when:** changing clothes/vehicle/going indoors measurably accelerates
  losing the cops; re-committing refreshes the description.

### Phase 4 — Depth & polish
- Context multipliers (location/time/crowd), richer dispatch callouts,
  perimeter/setup AI behaviors, notoriety persistence, PaySpray/bribe balancing.

---

## 9. Tuning parameters (all already on `UWantedSubsystem` unless noted)

`WoundHeat 0.7`, `KillHeat 2.5`, `DecayRate 0.35/s`, `HeatCap 20`,
thresholds `[1,3,6,10,16]`; `PedSightRange 24`, `PedFovDegrees 70`,
`PoliceSightRange 40`, `PoliceFovDegrees 100`; `ReportDelaySec 2.5`,
`CrimeActiveWindow 0.6`; `EvasionBaseSeconds 6`, `EvasionPerStarSeconds 6`.
New (Phase 3): `IdentifiedDecayPerSecondHidden`, `OutfitChangeIdentityDrop`,
`VehicleChangeIdentityDrop`. Tune for gritty pace (longer searches, meaningful
report delays, real cost to being seen).

---

## 10. Open questions / risks
- Observer gathering cost: cap radius + count, throttle (not every frame) to stay
  cheap with Mass crowds.
- Mac C++ build availability gates Phases 2–4 (see `docs/` build notes); Phase 1
  is intentionally build-free.
- Last-known position fidelity vs. fairness (how stale before it stops tracking).
- Keep `main` bootable each phase; one concern per PR (per `AGENTS.md`).
- Audio/material assets need provenance rows (`docs/ASSETS.md`).

---

## 11. Definition of done (per `AGENTS.md`)
Every phase: format + lint + build (UBT) + `GTC.*` automation tests pass, and
`main` boots to a controllable character. Where a build can't run in-environment,
say so explicitly rather than claiming the checks passed.
</content>
