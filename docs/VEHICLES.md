# GTC Vehicle System

How the player vehicle works, and the **rules that keep it working**. The vehicle
code under `Source/GTC_UE5/Vehicles/` and its input/asset contract are owned by
**@ItsJazii** — changes that touch them must be reviewed by the owner
(see [Rules](#rules--do-not-break-the-car) and `.github/CODEOWNERS`).

## Overview

The drivable player car is a Chaos `WheeledVehiclePawn` (`BP_VehicleBase`) with a
feature-foldered C++ backbone:

| Feature | Where | What it does |
| --- | --- | --- |
| Driving | `/Game/CARINPUT/` + BP graph | Enhanced Input → Chaos throttle/brake/steer/handbrake |
| Enter / Exit | `Vehicles/Entry/` | walk up → **E** → drive → exit; adapter over the `FVehicleEntry` core |
| Spawn stability | `Vehicles/Spawn/` | stops the car launching when the streamed road loads under it; registers the car with traffic |
| Traffic yield | `GTCTrafficSubsystem::NearestLeader` | ambient cars brake/queue for the player car instead of ramming it |

## How each feature works

### Driving input
- Mapping context **`CAR_INPUT`** with actions `IA_THROTTLE` / `IA_STEER` / `IA_BREAK`
  (**Axis1D**) + `IA_HANDBRAK` (Bool) + `IA_Interact` + `IA_EXIT`.
- Keys: **W/S** throttle/brake, **A/D** steer (A carries a Negate modifier),
  **Space** handbrake, **E** enter/interact.
- ⚠ A steering action **must be Axis1D** — a Boolean re-booleanizes the Negate, so
  A and D both steer the same way.
- The pawn graph adds `CAR_INPUT` on possession and wires each action →
  `ChaosWheeledVehicleMovementComponent::Set*Input`.

### Enter / Exit — `Vehicles/Entry/`
- `FVehicleEntry` (pure core): the `OnFoot → Entering → Seated → Exiting` state
  machine — answers *which seat* and *where the timed get-in/out transition is*.
- `VehicleSeatComponent` (actor adapter): does the engine work on the transition
  edges — `Possess`/`UnPossess`, attach the driver to the seat socket, hide the
  legs in the footwell, swap the camera and the driving input context.
- The car BP implements `IGTCInteractable`: `Interact → HandleEnter`,
  `IA_EXIT → BeginExit`. The player's `InteractAction` must resolve to a real asset
  (`/Game/CARINPUT/IA_Interact`) or **E does nothing**.

### Spawn stability — `Vehicles/Spawn/`
- `VehicleSpawnSettleComponent`: on BeginPlay registers the owner via
  `UGTCTrafficSubsystem::RegisterExternalVehicle`, then each pre-physics frame
  holds the car a few cm above its spawn until the World-Partition road cell has
  streamed in underneath it — then settles it onto the wheels and disables its tick.
- Never toggles physics, never touches input. Flat level = instant settle, no change.
- Pure core `FVehicleSpawnSettle::Evaluate` + test `GTC.Vehicles.Spawn.Settle`.

### Traffic yield — `GTCTrafficSubsystem`
- `NearestLeader` projects each registered `ExternalVehicle` onto the ambient car's
  lane; if it sits ahead inside the lane corridor it is fed to the IDM as the
  leader, so the ambient car brakes/queues behind it instead of driving into it.

## Rules — do not break the car

These are **hard rules**. A change that violates one breaks the player vehicle and
will be requested-changes / closed in review.

1. **Never rename the rigged car SkeletalMesh, its Skeleton, or its PhysicsAsset**
   via an asset rename. Renaming a SkeletalMesh can silently **null its Skeleton +
   PhysicsAsset links** → the car stops simulating (no drive, no gravity, "has no
   Skeleton"). If a rename is unavoidable, verify `skeleton` + `physics_asset` are
   still set immediately after, or reimport from the source FBX with the skeleton
   specified.
2. **Car input lives in ONE place** (`/Game/CARINPUT/`). Do not move, rename,
   duplicate, or shadow it. Steering actions stay **Axis1D**.
3. **Do not break the enter/exit contract:** the car BP's `IGTCInteractable` →
   `VehicleSeatComponent` wiring, and the player's `InteractAction → IA_Interact`.
   Replacing the player character must preserve `InteractAction` + `InteractReach`.
4. **`VehicleSpawnSettleComponent` stays on the car.** Removing it brings back the
   spawn launch in the streaming city.
5. **Do not globally disable ambient traffic** (`bStreamTraffic`) in shared code to
   "fix" the flying car — the fix is the `NearestLeader` yield + `SpawnSettle`,
   already in place. A local dev toggle is fine; **never commit it**.
6. **Do not revert the `NearestLeader` ExternalVehicles yield.** Ambient traffic
   must brake for registered external vehicles.
7. **The car body mesh + materials are third-party (a CC-BY Porsche model).** Per
   `docs/ASSETS.md` they live **outside this public repo** (local / private asset
   submodule) — never commit the car mesh/materials here. The shipped game reskins
   to a non-branded body. Trademark (Porsche) is separate from the CC-BY model
   licence; attribution is kept with the asset.
8. **Changes under `Source/GTC_UE5/Vehicles/` require the vehicle owner's review**
   (`.github/CODEOWNERS`).

## Player character contract — read before changing the player

The car's enter/exit lives on the **player character**, not the car — so changing or
replacing the player can silently break "press E to get in." Anyone who adds, swaps,
or reworks the player pawn (`BP_GTCPlayerCharacter` or a replacement) **must preserve**:

1. **`UGTCInteractionComponent`** on the pawn — it ticks the interact sphere and
   gathers `IGTCInteractable` actors. No component → nothing is interactable.
2. **`InteractAction` → `/Game/CARINPUT/IA_Interact`** (a real, loadable asset). If it
   is null, the C++ skips the E-key mapping *and* the interact bind, so **E does
   nothing** — the single most common "can't get in the car" cause.
3. **`InteractReach` ≈ 400 cm.** Smaller, and the 4.5 m car never enters the selection
   sphere from the doors.

**Changing the player mesh/anim:** swap only `CharacterMesh0`'s skeletal mesh + its
Anim Class. Do **not** clear `InteractAction` / `InteractReach`, and do **not**
wholesale-replace the BP — a full BP replacement is exactly what broke enter/exit in
the #215/#216 sync (the mesh was swapped and the interact wiring went with it).

The project's intended player is the **soldier rig** (`ABP_PlayCharacter`); when it is
healthy, use it. `SKM_Manny_Simple` + `ABP_Unarmed` is the **known-good stock fallback**
that always boots a controllable, animated character (AGENTS.md: *"main must boot to a
controllable character"*).

## Credits
The car body model is "Porsche 911 GT3" by ChevroletSS (Sketchfab), CC BY 4.0 — see
the credit kept with the asset. Trademark (Porsche) is separate from the CC-BY model
licence; the shipping build reskins to a non-branded body.
