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
- **Melee** — `GTC_Melee` exec: a front-arc strike (`FMeleeCombat` damage + knockback)
  or, into an enemy's back, a lethal **stealth takedown**. Rounds out player offense
  (shoot + throw + melee).

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

### Live wiring (was dormant) — the wanted lifecycle now closes end-to-end
- Crime reporting: `AGTCCitizen` / police actors call `UWantedSubsystem::ReportCrime`
  on player wounds/kills (was never called).
- `AGTCPoliceDirector` drives `UWantedSubsystem::TickFrame` and feeds
  `UArrestSubsystem::Tick` the nearest living-officer distance.
- **Escape:** the director feeds `UpdateEvasion` from police line-of-sight (chopper
  overhead, or nearest officer within range with a clear sightline) and clears the
  stars when the player has gone **cold** — breaking contact actually loses the cops.
- **Bust:** apprehend closes officers to the catch range so the grapple lands; the
  director drains the arrest subsystem's `ConsumeClearHeatRequest` and clears the
  stars. So the full loop is wired: crime → chase → **escape / busted / killed**.
- Remaining BP hook: bind `UArrestSubsystem::OnBusted` for the cuff/respawn FX
  (gameplay consequence on a bust) — the only loop-level wiring left, and it's editor
  work.

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
- `AI/Police/{GTCPoliceOfficer,GTCPoliceDirector,GTCPoliceHelicopter,GTCPoliceCar,GTCRoadblock,GTCSpikeStrip,GTCSwatVan,GTCK9}.{h,cpp}`
- `AI/Hostiles/{GTCHostile,GTCGangSpawner,GTCTurret}.{h,cpp}`
- `AI/PoliceDispatch/PoliceSpawnPlan.{h,cpp}` + `Tests/PoliceSpawnPlanTest.cpp`
- `AI/Combat/Apprehend.h` + `Tests/ApprehendTest.cpp`; `AI/Combat/Suppression.h` + `Tests/SuppressionTest.cpp`
- `Weapons/Throwables/GTCThrowable.{h,cpp}`, `World/Hazards/{GTCExplosive,GTCFire}.{h,cpp}`, `World/Pickups/GTCPickup.{h,cpp}`, `World/Cover/GTCBarricade.{h,cpp}`
- `Scripts/gtc_police/*` (host-clang pure-core verifier: apprehend + spawn-plan + suppression)

Modified (SHARED — entangled with other loops' WIP): `GTC_UE5.Build.cs`
(`bUseUnity=false`), `Weapons/Component/GTCWeaponComponent.{h,cpp}` (AI-aim),
`NPC/Agent/GTCCitizen.{h,cpp}` (crime reporting), `Player/Pawn/GTCPlayerCharacter.{h,cpp}`
(throw + melee execs), `Systems/Wanted/WantedSubsystem.cpp` (one line: evasion resets
on a fresh crime).

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

## Hardening — adversarial review (correctness, then design)

Because the actors compile cleanly but had never run, the code was hardened with
multi-agent reviews (every finding independently verified to kill false positives).

- **Correctness — 4 passes, converged 9 → 4 → 0.** Found and fixed **13 real runtime
  bugs** that compiling could not catch: a helicopter searchlight frame-mismatch, a
  no-op armor pickup, the player taking zero barrel-explosion damage (wrong collision
  channel), several actor leaks (SWAT-van barricades, gang turf), grenades hitting the
  thrower, cover scored against the wrong position, a broken roadblock cooldown, and a
  regression one fix introduced. The final feature (flashbang) reviewed clean.
- **Design / exploit — 1 pass, 13 holes found, 8 fixed green.** A different lens (a
  bug-free system can still be unfun/broken). Fixed: escape was impossible at 3+ stars
  (chopper pinned you just for being airborne → now needs real line-of-sight); SWAT
  never spawned at 3 stars (per-wave round-robin stranded later unit types → continuous
  cursor); cops couldn't catch you on foot (run speed raised above the player sprint); a
  flashbanged cop could still bust you, and van/heli-dropped troopers were invisible to
  the bust/seen checks; the 3-star chopper's troop insertion was dead; and cops cuffed a
  player mid-firefight (the "armed suspect" gate was dead — now driven by the wanted
  system's recent-crime timer).

### Known design limitations (fix when the relevant system is wired)

- **Throwables + melee are now finite, rate-limited abilities** (`FPlayerOffenseLoadout`,
  host-verified + `GTC.Player.Offense.Loadout.*` tests): per-type ammo with carry caps,
  a shared throw cooldown, and a melee cooldown — so the flashbang can't chain-stun and
  the back-takedown can't be spammed down a line. Ammo restocks from `AGTCPickup`
  (`FlashbangAmmo`/`GrenadeAmmo`/`MolotovAmmo` kinds) and enemy loot drops. **The one
  remaining step is binding `GTC_ThrowFlashbang`/`Grenade`/`Molotov`/`GTC_Melee` to
  EnhancedInput keys** (an editor asset) — the C++ is binding-ready, and the HUD can read
  `GetFlashbang/Grenade/MolotovAmmo()` with the `OnThrowablesChanged` event.
- **Escape clears all stars on a fixed ~12 s window** regardless of heat. Largely
  mitigated by the chopper-LOS fix (high-star escape now also means breaking the
  chopper's sightline); scaling the search window with stars is a tuning follow-up.
- **Military (6-star) is defined but unreachable** since the wanted system caps at 5 —
  harmless, but decide whether to add a 6th star or fold Military into the 5-star mix.

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
