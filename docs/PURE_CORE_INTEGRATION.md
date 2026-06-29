# Pure-core integration plan

A batch of **pure-core gameplay logic** landed as PRs #230–#250 — scene-free,
deterministic, `UObject`-free structs, each unit-tested headless (`GTC.*`). They
deliberately ship *without* their adapters: the interesting decision logic is
proven in isolation, and the thin Unreal glue that wires each into actors,
components, and the world is the **deferred adapter** work tracked here.

This file is the integration backlog — what each core produces, what it plugs
into, and the adapter still to write. It exists because the cores are the easy,
verifiable half; turning them into playable features is the half that needs the
editor, assets, and a human in the loop.

> **Provenance + build note.** Each core links the PR that introduced it. All were
> validated headless against a shim of the UE primitives they use (the repo gate
> `tools/check.sh` couldn't run in the authoring environment — the editor build
> needs the `CesiumForUnreal` plugin, which wasn't installed there). The **UE-API
> compile** and the `GTC.*` automation runs still need CI / a machine with the
> plugin before merge — treat that as the entry gate for every row below.

---

## How to read a row

`Core` — the struct. `Path` — where it lives. `Produces` — the value(s) it hands
back. `Plugs into` — the existing system/actor the adapter feeds it from / to.
`Adapter TODO` — the concrete glue to write.

---

## Dual-protagonist layer

The roster already switches leads; these flesh out *where the other lead is* and
*how the pair feel*.

| Core | Path | Produces | Plugs into | Adapter TODO |
|---|---|---|---|---|
| `FLeadSchedule` (#232) | `Systems/Roster/LeadSchedule` | dormant lead's place + activity at a time of day | `FCharacterRoster`, world clock | on `SwitchTo`, query `PlaceAt(now)` and spawn/reposition the dormant lead there instead of the frozen spot; play the activity anim |
| `FPartnerBond` (#242) | `Systems/Roster/PartnerBond` | bond 0..1 + tier | co-op events, perk layer | feed warm/cool from shared wins / betrayals; gate co-op perks on `Tier()` |
| `FDownedState` (#245) | `Player/Health/DownedState` | Up/Downed/Dead + bleed timer | `FPlayerHealthModel`, `FPartnerBond` | call `GoDown()` when health hits 0; a partner reaching you calls `Revive()` (+restore HP, `FPartnerBond::Warm`) |

## Weather-reactive layer

One weather read drives the whole world's grip and caution.

| Core | Path | Produces | Plugs into | Adapter TODO |
|---|---|---|---|---|
| `FRoadGrip` (#237) | `Vehicles/Traction/RoadGrip` | grip factor 0..1 (wet + aquaplane) | `FVehicleHandling` GripFactor seam, `WeatherController` | source wetness/speed; multiply road-grip × tyre/wear grip → `LateralRetention`/`ApplyGrip` |
| `FTrafficWeather` (#240) | `AI/Traffic/TrafficWeather` | speed × + headway × | `FTrafficAgent` IDM (`DesiredSpeed`/`TimeHeadway`) | fold the factors into each ambient agent's IDM params each tick |
| `FTideModel` (#234) | `World/Environment/TideModel` | still-water level + rising/next-high | Ocean stack (`OceanSurface`/`OceanShore`) | offset the ocean datum / shoreline blend by `LevelAt(worldHours)`; float docked boats |

## Audio layer

| Core | Path | Produces | Plugs into | Adapter TODO |
|---|---|---|---|---|
| `FRadioTuner` (#230) | `Systems/Radio/RadioTuner` | tuned station + track + offset (live clock) | in-vehicle audio | resolve `(station,track)`→`USoundBase`, seek to offset, crossfade on tune; read dial from Enhanced Input |
| `FMusicDirector` (#241) | `Systems/Music/MusicDirector` | score intensity + stem layer | wanted/combat/chase → heat | cook a 0..1 heat each tick; crossfade stems on `Layer()` change |

## Economy layer

Closes the heist → dirty cash → clean money → recover loop.

| Core | Path | Produces | Plugs into | Adapter TODO |
|---|---|---|---|---|
| `FMoneyLaundering` (#244) | `Systems/Economy/MoneyLaundering` | clean payout over time | `HeistCrew`/contraband, `PlayerStats` | deposit hot cash; bank `Advance()` clean payout to `PlayerStats` |
| `FVehicleInsurance` (#243) | `Vehicles/Insurance/VehicleInsurance` | claim grant + deductible | `Vehicles/Condition`, `PlayerStats`, garage | on destroyed→`ReportDestroyed`; claim charges deductible + respawns at garage |
| `FCloutLedger` (#235) | `Systems/Clout/CloutLedger` | follower count + tier | brand-deal/gig layer | feed `Post(appeal)` from acts; gate gigs on `Tier()` |

## Crime / police layer

All three are **producers for** the existing Wanted/Police systems — they never
mutate police state, the adapter reads a flag/result and applies it.

| Core | Path | Produces | Plugs into | Adapter TODO |
|---|---|---|---|---|
| `FBribeOffer` (#247) | `Systems/Wanted/BribeOffer` | bribe cost + new star level | `FWantedSubsystem`, `PlayerStats` | on bribe prompt, charge cost + set lowered stars |
| `FHostageStandoff` (#249) | `Systems/Hostage/HostageStandoff` | `IsShieldActive()` + struggle | police fire control, bystander pawn | grab a bystander; suppress police fire while shield active |
| `FCivilUnrest` (#250) | `Systems/Unrest/CivilUnrest` | district unrest + stage | district, police presence | `Provoke` from crimes/explosions; `Suppress` from police; spawn looters/brawls while `IsRioting()` |

## Living-world layer

| Core | Path | Produces | Plugs into | Adapter TODO |
|---|---|---|---|---|
| `FTrafficSignal` (#231) | `AI/Traffic/TrafficSignal` | per-approach Red/Yellow/Green | junction lanes, `FIntersection` | map lanes→groups; red/yellow → braked stop-line gap via `FIntersection::StopLineGap`; drive light emissive |
| `FWildlifeBehavior` (#233) | `NPC/Wildlife/WildlifeBehavior` | per-animal Idle/Alert/Flee/Charge | animal pawns | measure player distance; drive flee/charge move + anim |
| `FBirdFlock` (#239) | `NPC/Wildlife/BirdFlock` | flock phase + altitude 0..1 | instanced/Niagara bird cloud | feed startles from weapon/explosion; lerp cloud height/spread from `Altitude()` |

## Activities & minigames

Self-contained loops; the adapter supplies inputs and consumes the outcome.

| Core | Path | Produces | Plugs into | Adapter TODO |
|---|---|---|---|---|
| `FPhotoScore` (#238) | `Systems/Photo/PhotoScore` | shot Appeal 0..1 | phone camera, `FCloutLedger` | measure subject/fill/tilt/light; `clout.Post(Appeal(shot))` |
| `FCollectionTracker` (#236) | `Systems/Collection/CollectionTracker` | per-category + overall completion, reward tier | collectible actors, reward layer | `Collect(category,index)` on pickup; grant tier rewards |
| `FFishingFight` (#246) | `Systems/Fishing/FishingFight` | Fighting/Landed/Lost | fishing rod input | cast/bite → `Configure`; feed reel input; award the catch on `Landed` |
| `FLockpick` (#248) | `Systems/Lockpick/Lockpick` | Picking/Unlocked/Broken | doors/safes/cars | map stick→angle+tension; rumble from `AlignmentAt`; open on `Unlocked`, consume pick on `Broken` |

---

## Suggested integration order

1. **Weather-reactive** (`FRoadGrip`, `FTrafficWeather`) — the seams already exist
   (`FVehicleHandling` GripFactor, `FTrafficAgent` IDM); lowest-risk, immediately felt.
2. **Audio** (`FRadioTuner`, `FMusicDirector`) — no gameplay coupling, high polish payoff.
3. **Dual-protagonist** (`FLeadSchedule` → `FPartnerBond` → `FDownedState`) — the
   emotional core; do as a set since they cross-reference.
4. **Crime/police producers** (`FBribeOffer`, `FHostageStandoff`, `FCivilUnrest`) —
   each just reads into the existing Wanted/Police flow.
5. **Activities** — independent, ship whenever the content (assets) is ready.

Every adapter is small by design. The cores carry the logic and the tests; the
adapter is "read input → call core → apply output" against systems that already
exist.
