# GTA6-direction build plan — assets → one game

How the assets you own (TPS kit, the weapon packs, CityBeachStrip, and the GTC C++
systems already in the repo) assemble into a GTA-caliber vertical slice and then
scale. Companion to `docs/ROADMAP.md` (tasks) and `docs/TPS_KIT_INTEGRATION.md`
(the migration that kicked this off).

Guiding principle: **vertical slice before breadth.** Get ONE block of
CityBeachStrip to feel fully GTA — walk, shoot, take cover, steal a car, drive,
earn a wanted star, lose the cops, buy a gun — then replicate outward. Features
are mostly already present; the game is in the *fusion*, not in adding more.

---

## Asset ledger — what each pack does in the game

| Asset (on disk) | Provenance / location | Role in the GTA build |
|---|---|---|
| **ThirdPersonShooterKit** (`/Game/ThirdPersonKit`) | Paid Fab, gitignored | The on-foot pillar: motion-matching locomotion, cover, vault/ladder/ledge traversal, melee, grenades, turrets, destructibles, surveillance cams, EQS faction AI, **and the weapon/pickup/damage framework** that the arsenal plugs into. |
| **FPS_Weapon_Bundle** (`/Game/GTCaliberAssets/.../FPS_Weapon_Bundle`) | Submodule | Primary hero gun meshes + attachments: AR4, KA47, KA74U, KA-Val, G67, SMG11, M9 knife. |
| **MarketplaceBlockout / Modern Weapons** (`/Game/GTCaliberAssets/.../MarketplaceBlockout/Modern/Weapons`) | Submodule | Arsenal breadth + animation sets: pistols/revolvers/rifles/SMG/LMG/minigun/launchers/grenades with fire+reload ANIMs. Fills weapon classes the kit lacks (LMG, minigun, launchers). |
| **MilitaryWeap Silver/Dark** (`/Game/_WeaponsPacks/...`) | Local, gitignored | Weapon-skin variants for shop/economy tiers. |
| **CityBeachStrip** (`/Game/GTCaliberAssets/.../CityBeachStrip`) | Submodule | The open-world stage. Persistent map + `_Beach`/`_Streets`/`_Resorts` streaming sublevels, modular buildings, Sunny/Sunset lighting presets, props. |
| **GTC C++ runtime** (`Source/GTC_UE5`) | This repo | The systems the kit lacks: police/wanted, vehicles+traffic+handling, Mass NPC crowd/routines/economy, streaming, day-night+weather, save, HUD/minimap/phone, missions, front-end, radial weapon wheel. |

The split to keep straight: the **kit** gives polished *verbs* (move/shoot/cover);
**GTC** gives the *world that reacts* (cops/crowds/economy/missions); the **weapon
packs** give *arsenal breadth*. None of them is the game alone.

---

## Phase 0 — Foundation lock (in progress)

Done: kit migrated to `/Game/ThirdPersonKit`, config merged (11 collision channels,
gameplay tags, legacy input, plugins), leak closed. Now: relaunch into
`FeaturesMap_v2`, confirm the kit player walks/shoots/takes cover in-project (PIE +
screenshot via the unreal-mcp bridge). **Exit test:** kit player is controllable and
fires in your build.

## Phase 1 — Unify the player & control loop  *(the integration spine — do this first)*

Everything downstream depends on there being ONE player. Today there are two
(GTC soldier-rig pawn + kit Manny pawn).
- Adopt the kit's GameMode unit (`BP_TPS_Game_Mode` → kit pawn + `BP_TPS_PlayerController`
  + `BP_TPS_GameState`) as the player, then graft GTC's controller responsibilities
  (HUD, phone, pause, save, interaction, vehicle enter/exit) onto it. Soldier rig
  stays for NPCs.
- **One input scheme:** migrate the kit's legacy ActionMappings (merged into
  `DefaultInput.ini`) onto GTC's Enhanced Input `IA_*`/`IMC_*`, so foot-combat,
  driving and interaction share one context stack. Remove the legacy block when done.
- **Seamless foot↔car:** reconcile the kit pawn with GTC's press-E enter/exit (#216)
  and Chaos vehicles — the hand-off must possess/unpossess the unified pawn.
- **Exit test:** in CityBeachStrip you can run (kit feel), press E into a car, drive
  (GTC handling), get out, and the HUD/camera never break.

## Phase 2 — The arsenal

Build a GTA-style weapon roster on the kit's weapon framework, skinned with the packs.
- Define weapon classes mapping to real meshes: Pistol=G67, SMG=SMG11, Rifle=AR4/KA47,
  LMG/Minigun/Launcher=MarketplaceBlockout, Melee=M9 knife, plus the kit's shotgun/sniper.
- Reuse MarketplaceBlockout fire/reload ANIMs for the classes the kit lacks.
- Tie into GTC's **radial weapon wheel** (already prototyped) for selection.
- Economy hook: weapons/ammo purchasable at GTC storefronts; Silver/Dark skins as tiers.
- **Surface impact remap:** the kit's PhysicalMaterials use a different SurfaceType
  order than GTC's `SurfaceType1..14` — remap them to GTC's table so impact FX match
  (deliberately deferred during config merge; cosmetic until fixed).
- **Exit test:** weapon wheel switches between ≥6 real guns, each fires with correct
  anims, ammo, and correct GTC surface impacts.

## Phase 3 — Combat & wanted fusion

Make the kit's combat AI the *responders* in GTC's crime/wanted loop.
- Bridge kit EQS faction AI (`BP_AICharacter*`) with GTC's `AGTCPoliceDirector` /
  `UWantedSubsystem` / `APoliceOfficer`: committing crimes raises stars; the director
  dispatches kit-driven combat AI as cops.
- Cover, vault and ladder traversal usable mid-firefight in the city (kit channels
  now active).
- Melee + grenades + destructibles live in the world.
- **Exit test:** shoot an NPC → wanted star → kit-AI cops arrive, take cover, fight;
  you escape on foot/car and heat decays (GTC wanted persistence).

## Phase 4 — The living city in CityBeachStrip

Turn the stage into a populated world.
- Stream `_Beach`/`_Streets`/`_Resorts` sublevels through GTC's `FStreamingGrid`.
- Populate with GTC Mass crowd + routines/social/occupation/voice; ambient traffic on
  the GTC road network.
- Day-night + weather driving the Sunny↔Sunset lighting presets the map already ships.
- Place economy storefronts (shop/payspray/modshop press-E) and openable-door interiors
  in real buildings.
- Drop kit gameplay objects into real streets: cover points, weapon pickups/stands,
  AI encounter spawners — via the live bridge `save_current_level()` (never `open_level`).
- **Exit test:** a block of CityBeachStrip is alive — pedestrians, traffic, shops,
  time-of-day — and you can fight/drive through it.

## Phase 5 — Vehicles, traffic & traversal depth

- CitySample vehicles + GTC arcade handling/drift; traffic agents routing the road graph.
- Drive-bys: fire kit weapons from vehicles.
- **Exit test:** steal → drive → drift → drive-by → ditch, all seamless with the unified pawn.

## Phase 6 — Missions, economy & progression

- Use GTC's in-game mission editor + storefronts + save to author a 2–3 mission
  vertical slice set in CityBeachStrip (e.g. heist-style: gear up → infiltrate → fight
  → escape with wanted level).
- Persist money, loadout, wanted, position via the save subsystem.
- **Exit test:** start→finish a mission that uses every pillar, save/reload mid-mission.

## Phase 7 — Fidelity & GTA6 polish

The long tail, parallelizable once the loop is fun.
- Lighting/time-of-day on the Sunset presets; post (Smart Sharpen already in repo).
- Crowd + traffic density at city scale (Mass + tiered LOD; hero MetaHumans for key NPCs).
- Audio/radio, minimap/phone polish, front-end boot flow.
- Faces/voice for named characters.

---

## Live integration log (2026-06-25 — `/loop` session)

Ground truth verified against the live editor bridge (editor running in
CityBeachStrip). _[Note: as of 2026-06-27 the live surface is the `unreal-mcp`
MCP on :8000 — see the `caliber` skill. The VibeUE / `unrealclaude` bridges named
in this 2026-06-25 entry are legacy and no longer used.]_

**Our own GameMode set already exists** (built a prior session, in `/Game/GTCShooter/`,
gitignored because it references the paid kit):
- `BP_GTCShooterGameMode` (Engine.GameMode) → pawn `BP_GTCPlayer`, controller
  `BP_GTCShooterController`, state `BP_GTCShooterGameState`.
- `BP_GTCPlayer` is parented to the kit's `BP_PlayerCharacterBase` → inherits the
  motion-matching shooter feel.

Done this session (all PIE-verified in CityBeachStrip, no crash):
1. **CityBeachStrip now boots our gamemode.** WorldSettings GameModeOverride
   `BP_ThirdPersonGameMode` → `BP_GTCShooterGameMode`; map saved
   (`save_current_level`). PIE spawns/possesses `BP_GTCPlayer` under
   `BP_GTCShooterController`.
2. **Grafted GTC's controller onto our shooter.** Reparented
   `BP_GTCShooterController` (was a vanilla `PlayerController`) →
   C++ `AGTCPlayerController` — no build needed, the class is already compiled.
   This brings the in-game phone, Esc pause, **GTA weapon wheel** (slow-mo),
   emote wheel, character creator and Enhanced Input (`IMC_Default`) to the player.

Constraints reconfirmed: capture the editor with `unreal-mcp` `CaptureEditorImage`
(works on this Mac and includes the UMG HUD); can't build C++ / relaunch the editor on this Mac; the
`/Game/GTCShooter` set + the kit are gitignored, so the public PR carries only
legal-clean text (a C++ gamemode base, config, docs) — not the paid-derived BPs.

Done (iteration 2):
3. **Player spawns armed.** The kit's `BP_DebugWeaponGiver` (hidden) placed at the
   player start in CityBeachStrip arms `BP_GTCPlayer` via its `BP_WeaponComponent`
   (PIE-verified: pistol equipped with holo-display + flashlight attachments + weapon
   mesh). Map saved. Interim arming mechanism — to be replaced by a proper starting
   loadout / GTA weapon-stand pickups once kit BP-var names are resolved.
4. **Committable C++ gamemode base + branch.** Added `AGTCShooterGameMode`
   (Source/GTC_UE5/Core) — our extensible GTA game-mode base (starting
   loadout/cash/health, respawn + spawn-protection) with pure, unit-tested
   `GTCShooter::Normalize` (`GTC.Core.ShooterRules.Normalize`). Zero hard refs to the
   paid kit (soft, empty `StartingWeapons`). Committed with these docs to branch
   **`feat/gta6-shooter-gamemode`** (pushed). C++ unbuilt in this env (Mac, no UBT) —
   build + automation pending a build host; BP child reparent to it is a post-build step.

Done (iteration 3):
5. **Duplicate removed.** Deleted the redundant raw kit project drop
   `Content/ThirdPersonShooterKit/` (12 GB, its own `.uproject` — a full copy of the
   migrated `Content/ThirdPersonKit/` that's actually used). Verified zero references
   into it from our GTCShooter BPs, the CityBeachStrip map, or the kit gamemode before
   deleting. Contents were gitignored, so this is a pure local disk cleanup (no PR/leak
   impact); the migrated copy is intact.
6. **City is alive under our gamemode (verified).** CityBeachStrip holds **57,253
   actors** — 171 POIs/shops, 8 enterable vehicles, the GTC police director (spawns
   cops on wanted>0), with 3,742 actors within 50 m of the player spawn (player spawns
   *inside* the dense city, not the void). The gamemode swap did not break GTC's
   subsystem/placed-actor world systems.
7. **Arsenal is multi-weapon (verified).** The debug giver grants the player a
   pistol + rifle + melee knife (PIE-confirmed by reading the equipped weapon meshes:
   `SKM_PistolSciFi`, `SKM_RifleScifi_01`, `SM_Knife_01`), switchable via the kit's
   weapon inputs. The kit weapon system works end-to-end on `BP_GTCPlayer`.

**Known limit:** this Mac editor build won't enumerate kit Blueprint variables/
functions via Python (`dir(cdo)` empty, `new_variables`/`export_text` unsupported,
no UFunction listing), so configuring the FULL arsenal (all ~20 kit
`DA_WeaponDefinition`s incl. the modern non-sci-fi set + integrating MarketplaceBlockout
`PDA_WeaponDefinition`s) and swapping the giver's default sci-fi loadout for realistic
guns is a Blueprint-editor / data-asset task — not doable headless. Captured for a
build/BP-editor session.

Done (iteration 4):
8. **Wanted/police loop wired for testing.** Read the GTC C++ loop: `UWantedSubsystem`
   (GameInstanceSubsystem; `ReportCrime`/`Stars`/`TickFrame`, all non-Blueprint so not
   Python-drivable) feeds `AGTCPoliceDirector` (placed actor, present in the map), which
   each tick reads `Stars()` and streams officers/cars/heli/SWAT/K9 by heat. Found the
   real gap: **kit weapon fire does not report crimes to the wanted system** (the Phase-3
   kit-damage → `ReportWitnessedCrime` bridge is unbuilt). Added committable
   `Source/GTC_UE5/Systems/Wanted/WantedConsole.cpp` — dev-only `gtc.Wanted.Crime [0|1]`
   and `gtc.Wanted.Status` console commands that drive the existing public subsystem API,
   so the crime→cops loop can be exercised on a build (and as a GTA cheat) until the
   bridge lands.

**Verified vs build-gated (honest status):**
- *Verified live (PIE):* our gamemode + GTC controller + armed motion-matching kit
  player in CityBeachStrip; multi-weapon arsenal (pistol/rifle/knife); the city is alive
  (57k actors, police director, POIs, vehicles).
- *Committable, NOT compiled here (Mac, no UBT):* `AGTCShooterGameMode` + rules + test,
  `WantedConsole` commands. Build + automation + the BP reparent of
  `BP_GTCShooterGameMode` → `AGTCShooterGameMode` are pending a build host.
- *Needs the Blueprint editor / data work:* full arsenal breadth (all kit
  `DA_WeaponDefinition`s + MarketplaceBlockout), GTA weapon-stand pickups, and the
  kit-damage→wanted bridge.

Remaining toward "a close GTA6 game" (build/BP-editor session): compile the C++,
reparent the BP gamemode to it, configure the full realistic arsenal + MarketplaceBlockout,
build the kit-damage→wanted bridge, then a content/polish grind.

## Sequencing & risk notes

- **Order matters:** Phase 1 (one player) gates everything. Don't populate the city
  (Phase 4) before the player/control loop is unified, or you rework it.
- **Vertical slice gate:** after Phase 1–3 on a single block, stop and play it. If that
  block feels like GTA, the rest is replication; if not, fix feel before scaling.
- **Environment constraints:** licensed packs stay in the submodule / gitignored, never
  the public tree; can't build C++ or relaunch the editor on this Mac (config changes
  need a user relaunch); editor work goes through the live `unreal-mcp` bridge (:8000);
  `open_level`/live level-switch crashes — always boot/restart INTO the target map.
- **What off-the-shelf can't buy:** hand-built interiors, mission scripting depth,
  city-scale density, and the Rockstar visual bar. The plan gets the *verbs and the
  stage* to AAA-adjacent; the *content grind* is the multi-month remainder.
</content>
</invoke>
