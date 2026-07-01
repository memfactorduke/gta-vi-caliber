# Adapter wiring guide

`PURE_CORE_INTEGRATION.md` tracked the deferred **adapters** — the thin Unreal glue
that turns the pure-core gameplay structs into playable features — as a TODO backlog.
A batch of those adapters is now **built** (PRs #263–#268). This file is the other
side of that doc: not *what to write*, but *how to wire what's now written* into the
`CityBeachStrip` playable slice.

Every adapter here follows one shape, so wiring them is the same three moves each time:

1. **Add the component** to the right actor (or, for traffic weather, just place the
   weather actor). Each is a plain `UActorComponent` — no new subsystem, no
   `ChaosVehicles` module dependency.
2. **Feed it** each frame / on an event (`SetX(...)` / `StartX(...)`), and **apply its
   published outputs** — the components compute and expose values but never poke the
   physics or the HUD themselves ("expose the seam, let the Blueprint apply it").
3. **Bind its events** (`On...`) to the mission/reward/HUD wiring.

They are inert until started (`StartX` / being driven), so dropping one on an actor
can't change how the map plays until you wire it.

> **Gate (same as every row in `PURE_CORE_INTEGRATION.md`).** Each PR's pure logic is
> verified out-of-tree (a `GTC.*` automation test + the `Scripts/gtc_livingcity/oracle`
> shim runner) and went through an adversarial review, but the **UE-API compile (UHT/UBT)**,
> the in-editor `GTC.*` automation, and the felt PIE loop still need CI / the editor — the
> shim can't see reflection codegen or physics. Treat that as the entry gate.

---

## At a glance

| Feature (PR) | Component | Lives on | Start / feed | Key outputs → apply | Events |
|---|---|---|---|---|---|
| Rain-cautious traffic (#263) | `UGTCTrafficSubsystem` (+ `AGTCWeatherController::GetWetness/GetVisibility`) | (world subsystem, auto) | place `AGTCWeatherController` | — (folds caution into ambient IDM) | — |
| Player-car wet grip (#264) | `UGTCVehicleHandlingComponent` | the car (`BP_VehicleBase`) | `SetWetness`, `SetHandbrake`, `TyreGripFactor` | `GripFactor` → tyre friction; `GrippedVelocity` (full arcade) | drift scorer: `BankDrift/WipeDrift` |
| Gearbox + input (#268) | `UGTCVehicleDriveComponent` | the car | `SetDriveInput(steer,throttle,brake,handbrake)` | `SteerOut/ThrottleOut/BrakeOut/HandbrakeOut` → Chaos movement; `Gear/EngineRpm` → HUD | — |
| Delivery runs (#265) | `UGTCDeliveryRunComponent` | the courier | `StartRunTo(dropoff, timeLimit)`, `NotifyCargoImpact` | `Progress/TimeLeftSeconds/CargoCondition` → HUD | `OnDeliveryCompleted(payout)` / `OnDeliveryFailed(reason)` |
| Street races (#266) | `UGTCStreetRaceComponent` | the player car | set `Checkpoints`, `RivalPaceSeconds`; `StartRace()` | `PlayerPlacement/CurrentLap/CountdownRemaining/NextCheckpointLocation` → HUD | `OnRaceGreen` / `OnCheckpoint(idx)` / `OnRaceFinished(place, reward)` |
| Tail missions (#267) | `UGTCTailObjectiveComponent` | the player | set `Target`; `StartTail()` | `Suspicion/TrackProgress/DistanceToTarget/bInTargetView` → HUD | `OnTailSucceeded` / `OnTailFailed(reason)` |

---

## Rain-cautious ambient traffic — #263

The only one with no component to add: `UGTCTrafficSubsystem` already looks up the
level's `AGTCWeatherController` (lazily, cached) and folds `FTrafficWeather`'s
speed/headway caution into every ambient car's IDM from a stored base, so a downpour
slows the whole city and stretches the gaps.

**Wire:** place an `AGTCWeatherController` in `CityBeachStrip` (if one isn't already
there). No traffic changes needed — the subsystem finds it. With no controller, traffic
drives clear-weather (caution factors = 1). The PR also added
`AGTCWeatherController::GetWetness()` / `GetVisibility()`, the seam the player-car grip
below reads from the *same* weather source.

## Player-car wet-road grip — #264

`UGTCVehicleHandlingComponent` on the car. Each frame:
`SetWetness(WeatherController->GetWetness())`, `SetHandbrake(<handbrake held>)`, and set
`TyreGripFactor` (1 = fresh; feed `FVehicleCondition::GripFactor` when that's wired).

**Apply:** multiply the chassis' lateral tyre friction by the published `GripFactor`
(wet road → less grip → the tail steps out; past the aquaplane speed the car floats).
For a full arcade feel, adopt `GrippedVelocity` as the chassis linear velocity instead.
`AquaplaneAmount`/`bAquaplaning` drive the loss-of-control FX + a "slow down, it's
flooding" tell. The GTA-style drift scorer (`GetPendingDriftPayout`, `BankDrift` on a
clean exit, `WipeDrift` on a spin-out) is the stunt/photo/clout hook.

## Gearbox + shaped input — #268

`UGTCVehicleDriveComponent` on the car (pairs with the grip core — one grips the road,
this decides what the player is asking the chassis to do). Each frame, from the car BP's
Enhanced Input: `SetDriveInput(steer, throttle, brake, handbrake)`.

**Apply:** feed the published `SteerOut` / `ThrottleOut` / `BrakeOut` / `HandbrakeOut`
to the `UChaosWheeledVehicleMovementComponent`, and show `Gear` (Parked/Reverse/Neutral/
Drive) + `ForwardGearNumber` + `EngineRpm` on the HUD. Optionally feed the real driven-
wheel RPM via `SetWheelRpm` each frame (else it's estimated from speed + `WheelRadiusCm`).
It only computes while the car is a possessed pawn.

## Delivery runs — #265

`UGTCDeliveryRunComponent` on the courier (the player car, or set `Courier`).
`StartRunTo(DropoffLocation, TimeLimit)`; bind the car's `OnHit` to
`NotifyCargoImpact(Impulse)` so crashes erode the cargo.

**Apply:** `Progress` / `TimeLeftSeconds` / `CargoCondition` / `DistanceToDropoff` →
HUD. Bind `OnDeliveryCompleted(PayoutFactor)` → `UMissionController::Complete` + pay the
reward (`PayoutFactor` is a 0..1 quality multiplier for the SideJob/MissionReward cash),
and `OnDeliveryFailed(reason)` → `Fail`. `ArrivalRadius` is how close counts as delivered.

## Street races — #266

`UGTCStreetRaceComponent` on the player car. Author the checkpoint ring as `Checkpoints`
(world-space `FVector`s — use the god-mode `PlaceHere`/marker flow), one solo finish time
per rival in `RivalPaceSeconds`, then `StartRace()`.

**Apply:** `RacePhase` / `PlayerPlacement` / `FieldSize` / `CurrentLap` / `TotalLaps` /
`CountdownRemaining` → race HUD; `NextCheckpointLocation` → the on-screen waypoint. Bind
`OnRaceGreen` (drop the lights / enable input), `OnCheckpoint(index)` (gate SFX), and
`OnRaceFinished(placement, reward)` → results screen + payout. Rivals are paced (not real
pawns), so no rival AI needed to stand a race up.

## Tail missions — #267

`UGTCTailObjectiveComponent` on the player. Set `Target` to the mark actor, `StartTail()`.

**Apply:** `Suspicion` (0..1) → a "you're being followed" HUD tell; `TrackProgress` → the
objective bar; `DistanceToTarget` / `bInTargetView` for feedback. Bind `OnTailSucceeded`
and `OnTailFailed(FailedLost | FailedSpotted)` → the mission wiring. Optional refinement:
AND `bInTargetView` with a real line-of-sight trace before it counts against you.

---

## How the six compose (the slice)

- **One weather source, whole-world reaction.** A single `AGTCWeatherController` drives
  both the ambient traffic caution (#263, automatic) and the player-car grip (#264, feed
  `GetWetness`). Rain is felt everywhere at once.
- **A complete car.** Grip (#264) + drive (#268) are the two halves of the same chassis:
  `SetDriveInput` → gearbox → actuators into Chaos, while `GripFactor` scales the tyres'
  hold. A **street race in the rain** (#266) leans on both — wet grip makes the racing
  line matter.
- **Activities that run themselves.** Delivery / race / tail each stand up from an
  authored point (or target) + a `StartX` call, and report out through events — hang them
  off `UMissionController` and the reward economy without new subsystems.

## Shared conventions (so wiring one teaches you the rest)

- **Pure resolver + thin shell.** The per-frame math lives in a headless-tested pure
  struct (`F...Resolver`); the `UActorComponent` only gathers inputs and publishes
  outputs. Retunes and edge-cases are proven without the editor.
- **Config snapshotted at start.** Race checkpoints/paces and tail tuning are copied on
  `StartX`, so editing the properties mid-run can't desync the active run.
- **Chassis-agnostic.** The vehicle components read the car as a bare `AActor` and expose
  scalars for the BP to apply to Chaos — no `ChaosVehicles` module dependency in `GTC_UE5`.
- **Reflected enum mirrors** (`EGTCGear`, `EGTCRacePhase`, `EGTCTailState`, …) are pinned
  1:1 to their pure-core enums with `static_assert`s, so a core reorder breaks the build
  rather than silently mis-reporting state.
