# Combat / Weapons Actors — Delivery & Integration Guide

Status: **feature-complete, compile-verified green** (UnrealBuildTool, UE 5.7,
`GTC_UE5` Mac Development, `BUILD_EXIT=0`). Built by the "Combat / weapons actors"
initiative. Nothing here is committed yet — see **Landing this work** below.

## What this delivers

The roadmap item was "police that fight back." The project already had the entire
wanted → dispatch → pursuit → combat **decision** stack as unit-tested pure-core
(`AI/Combat`, `AI/Pursuit`, `AI/PoliceDispatch`, `World/Police`, `Systems/Wanted`,
`Systems/Arrest`, `Weapons/*`) but nothing in the world embodied it. This adds the
**actor layer** — thin adapters over those tested cores — plus the missing live
wiring, so the loop is playable: commit a crime → heat rises → the law responds by
land, air and ground → you fight, flee, or get cuffed; and the world is destructible.

### Police force (spawned/recalled by `AGTCPoliceDirector` by wanted level)
- **`AGTCPoliceOfficer`** — foot cop over `FPoliceCombat`/`FCombatAi` with the full
  AI loop: heat-scaled chase; **apprehend** (`FApprehend`) cuffs an unarmed low-heat
  suspect; **cover-seeking** (`FCombatCover`) to real LOS-blocked spots; **peek-from-
  cover** (lean out to fire, duck back); **pursuit memory** (`FPursuitMemory`) chases
  your last-seen position when sight breaks; **suppression** (`FSuppression`) — sustained
  fire pins it down (seek cover sooner, fire slower). Shootable (`FNpcVitals`), reports
  crimes on wound/death, drops an armor pickup on death, and stands down (despawns)
  once the heat clears.
- **`AGTCPoliceHelicopter`** — air unit over `FHelicopterPursuit`: orbits,
  searchlights, door-gunner fires when you're lit, and **fast-ropes SWAT** troopers in
  at 4+ stars. Deploys at 3+ stars.
- **`AGTCPoliceCar`** — kinematic pursuit cruiser over `FPursuitTactics`
  (intercept / ram / PIT / block), gunner + ram damage, pursuit memory. Pool of 2
  from 2+ stars.
- **`AGTCSwatVan`** — heavy ground response at 4+ stars: drives to a standoff and
  unloads a SWAT squad behind its doors; shootable.
- **`AGTCRoadblock`** + **`AGTCSpikeStrip`** — road denial: a barrier wall
  (`FRoadblock`, threadable gap) with a stinger (`FSpikeStrip` — chassis damage +
  `OnSpiked` deflation hook) laid on the approach to funnel a fleeing car into it.

### Player offense
- **`AGTCThrowable`** — frag grenade + incendiary **molotov** over `FThrowable` +
  `FExplosionModel` (arc, cook fuse, blast, Z-biased knockback). Player execs
  `GTC_ThrowGrenade` / `GTC_ThrowMolotov`. The AI-aim path added to
  `UGTCWeaponComponent` (`SetAimOverride`) lets every NPC/police gun reuse the
  player's weapon component.

### Environmental destruction
- **`AGTCExplosive`** — shootable barrel over `FExplosionModel` with chain
  reactions (`ShouldChain`, terminating guard), leaves fire.
- **`AGTCFire`** — spreading fire over `FFirePropagation` (DPS to occupants,
  generation/fuel-capped spread). Both barrels and molotovs ignite it.

### Sustain & reward
- **`AGTCPickup`** (`World/Pickups`) — walk-over health/armor pickup; applies through
  the existing public `UPlayerHealthComponent` (`Heal`/`AddArmor`), one-shot or timed
  respawn. The counterweight to police pressure; downed officers drop armor as loot.

### Non-police enemies (so the law isn't the only threat)
- **`AGTCHostile`** (`AI/Hostiles`) — an armed gang/thug, officer-lite over
  `FCombatAi::DecideAction` + the weapon component, no wanted coupling. It targets the
  **nearest** of the player or a nearby police officer, so a gang turf and a police
  response collide into a three-way firefight.
- **`AGTCGangSpawner`** — a turf that keeps a small band alive nearby while the player
  is in range.

### Live wiring (was dormant)
- Crime reporting: `AGTCCitizen` / police actors call `UWantedSubsystem::ReportCrime`
  on player wounds/kills (was never called).
- `AGTCPoliceDirector` drives `UWantedSubsystem::TickFrame` and feeds
  `UArrestSubsystem::Tick` the nearest living-officer distance.

## Activating it in the editor (the system is dormant until placed)

All of this is compiled into the module but nothing runs until actors are placed.
None of it needs Blueprints — the C++ classes work as-is (each falls back to a basic
cube mesh and `AGTCPoliceOfficer::StaticClass()` etc. when no BP subclass is set). On
a fresh editor opened directly on the playable map (the MCP level-switch crash makes
in-session switching unsafe):

1. **Police** — drop ONE `AGTCPoliceDirector` into the persistent level. It drives the
   wanted clock and spawns/recalls the whole police response by star level; no other
   police actor needs placing. Optionally set its `OfficerClass`/`HelicopterClass`/
   `CarClass`/`SwatVanClass`/`RoadblockClass`/`SpikeStripClass`/`SpawnSeed` to BP
   subclasses with real meshes.
2. **Raise heat to see them** — shoot an NPC, or call `UWantedSubsystem::ReportCrime`
   from the console / a debug key. Stars rise → officers, then cruisers (2★), heli
   (3★), SWAT van + grenades + fast-rope + roadblocks/spikes (4★).
3. **Gangs** — drop `AGTCGangSpawner` actors on the corners you want gangs to hold;
   each maintains a band (set `GangSize`, `MeleeFraction`) while the player is near,
   and they fight the player *and* nearby cops.
4. **Sustain/props** — scatter `AGTCPickup` (set `Kind` Health/Armor), `AGTCExplosive`
   barrels, and `AGTCFire` if you want pre-lit fire. Dead officers/thugs drop loot
   automatically.
5. **Player throwables** already work via the `GTC_ThrowGrenade` / `GTC_ThrowMolotov`
   console execs on the player pawn.

That is the full acceptance pass: place the director, raise heat, watch a 5★ chase
with air + ground + barricades, then drive into gang turf for a three-way fight.

## New / changed files

New (cleanly mine):
- `AI/Police/{GTCPoliceOfficer,GTCPoliceDirector,GTCPoliceHelicopter,GTCPoliceCar,GTCRoadblock,GTCSpikeStrip,GTCSwatVan}.{h,cpp}`
- `AI/Hostiles/{GTCHostile,GTCGangSpawner}.{h,cpp}`
- `AI/PoliceDispatch/PoliceSpawnPlan.{h,cpp}` + `Tests/PoliceSpawnPlanTest.cpp`
- `AI/Combat/Apprehend.h` + `Tests/ApprehendTest.cpp`; `AI/Combat/Suppression.h` + `Tests/SuppressionTest.cpp`
- `Weapons/Throwables/GTCThrowable.{h,cpp}`, `World/Hazards/{GTCExplosive,GTCFire}.{h,cpp}`, `World/Pickups/GTCPickup.{h,cpp}`
- `Scripts/gtc_police/*` (host-clang pure-core verifier: apprehend + spawn-plan + suppression)

Modified (SHARED — entangled with other loops' WIP): `GTC_UE5.Build.cs`
(`bUseUnity=false`), `Weapons/Component/GTCWeaponComponent.{h,cpp}` (AI-aim),
`NPC/Agent/GTCCitizen.{h,cpp}` (crime reporting), `Player/Pawn/GTCPlayerCharacter.{h,cpp}`
(throw execs).

## Build verification

- **Pure-core** (`FPoliceSpawnPlan`, `FApprehend`): host-clang shim compiles the
  REAL `.cpp` and re-asserts the GTC.* invariants — `Scripts/gtc_police/run_checks.sh`, PASSES.
- **Engine actors**: compiled with UnrealBuildTool (game target, non-unity so each
  file is its own TU) — every actor file builds clean, whole module links,
  `BUILD_EXIT=0`. The per-file compile caught real UE-idiom bugs static review
  missed (a missing `Engine/DamageEvents.h`; a `UFUNCTION` param named `Instigator`,
  which UHT reserves; `TSubclassOf ? x : UClass*` ternary ambiguities — use the
  if-assignment form).
- The running editor was NOT disturbed (its loaded binary lacks these new UCLASSes;
  loading them needs a restart, which the one-editor rule forbids).

## ⚠️ `bUseUnity = false` — review this

The shared tree's many copy-pasted file-scope test/helper symbols
(`Eps`/`Fwd`/`PosMod`/`Wrap`/`Hash01`/…) collide under unity builds as concurrent
loops add files — an unbounded source of `redefinition`/`-Wshadow` breakage. A full
non-unity build was verified include-clean, so unity was disabled module-wide as the
durable fix. Trade-off: slower clean builds, faster incrementals. Revert if the team
prefers a naming convention instead.

## Landing this work

The tree is shared by 5+ concurrent loops. The **new** files are cleanly mine; the
**modified shared files** also contain other loops' uncommitted edits, so a naive
`git add -A` would entangle their WIP. Suggested:
1. New files → a `feat/combat-actors` branch (they're self-contained).
2. The 5 shared-file edits → review each hunk against the other loops' work before
   staging (use `git add -p`); they're small and additive (AI-aim methods, two crime-
   report calls, two player execs, one Build.cs line).
3. `bUseUnity=false` is one line in `GTC_UE5.Build.cs` — land or revert deliberately.

## Deferred (need the editor, or touch contested files)

- Live placement of `AGTCPoliceDirector` + the throw execs into `Miami.umap`, and
  tuning the kinematic cruiser with real Chaos physics — **editor only** (MCP
  level-switch crashes this project; open the editor directly on the map).
- Spike strips (tire-deflation payoff lives in the `FVehicleHandling` layer),
  player melee / drive-by, LOS-gated witnessed-crime observers, sound→NPC alerting —
  these live in `GTCCitizen`/`GTCPlayerCharacter`/the vehicle layer that the
  human-native / radial-wheel / vehicle loops are actively editing.
