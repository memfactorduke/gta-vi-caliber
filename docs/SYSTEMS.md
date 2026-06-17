# Gameplay Systems Reference

A catalogue of the pure, unit-tested simulation systems that make up the open-world-style
gameplay layer, with their purpose, key API, and **how to wire each into a live
level**. Every system here is a plain C++ class you can instantiate (stateful)
or call statically (pure helpers); all are covered by UE5 automation tests under
the owning module's `Tests/` folder (`GTC.*` prefixes).

Most systems follow one of two repo patterns:

- **Pure model + thin component/actor**: the math lives in a plain (non-UObject)
  model; an `Actor`/`UActorComponent` reads it and drives the engine (see
  `WantedTracker` wrapping `WantedSystem`).
- **Self-wiring coordinator**: a small actor/component that finds collaborators by
  tag/subsystem in `BeginPlay`/`Tick` and needs no per-level plumbing (see
  `MissionReward`, `ProgressionTracker`, `StatsCoordinator`,
  `WantedEvasionController`, `PaySprayShop`). This is the lowest-friction way to
  wire a model into a level.

Tags the live level already publishes: `player`, `player_health`,
`player_stats`, `wanted`, `mission`, `police`, `stats`, `progression`, `world`,
`spawn_points`.

---

## Wanted / law enforcement

| System | Purpose | Key API | Wiring |
|---|---|---|---|
| `WantedSystem` | heat -> stars core | `add_crime`, `stars`, `is_wanted`, `tick` | wrapped by `WantedTracker` (tag `wanted`) |
| `CrimeWitness` | a crime only raises heat if seen | `can_witness`, `count_witnesses`, `heat_for_crime`, report timer | gate `WantedTracker._on_crime` on `count_witnesses(crime_pos, peds)` before adding heat |
| `WantedEvasion` | "go cold" search timer | `update(seen, dt)`, `is_cold`, `search_progress` | wired live via `WantedEvasionController` (raycasts cop LOS) |
| `PaySpray` | respray/hideout instant clear | `cost_for`, `is_seen_entering`, instance respray timer | wired live via `PaySprayShop` trigger volume (tag `pay_spray`) |
| `Disguise` | change your look to lose the heat | `set_appearance`, `log_sighting`, `recognition`, `evasion_speedup`, `changed_slots` | **live on the player** via `DisguiseTracker` (tag `player_disguise`): `WardrobeShop` pushes worn looks into it, and `WantedEvasionController` scales unseen search delta by `evasion_speedup()` once police have a description. CI-guarded by the `GTC.*` WardrobeDisguise + DisguiseEvasion automation tests. TODO: call `log_sighting()` from witness/cop perception |
| `Wardrobe` | buy/own/wear clothing that feeds `Disguise` | `buy`, `wear`, `worn_looks`, `worn_look`, `items_in_slot` | **wired live** via `WardrobeShop` (Actor, tag `interactables`): press interact at the storefront to buy + wear an outfit against `PlayerStats`; worn looks now update the live `DisguiseTracker`. CI-guarded by the `GTC.*` WardrobeShop + WardrobeDisguise automation tests |
| `PoliceEscalation` | response tier per star | `response_units`, `has_swat/helicopter/military`, `aggression`, `weapon_tier` | feed `PoliceSpawner`: pick the blueprint + count per `response_units(stars)` |
| `PursuitTactics` | chase tactics | `intercept_point`, `should_ram`, `pit_side`, `choose_tactic` | drive `Police`/traffic-cop movement when chasing |
| `GangTerritory` | turf control | `add_influence`, `take_over`, `controlled_fraction` | **surfaced to the player** via `TurfClaim` (Actor, tag `interactables`): a walk-up claim point where you spend cash to buy influence in a district and take it over at full influence. CI-guarded by the `GTC.*` TurfClaim automation test. TODO: a district-wide influence tracker + visible turf-war state |
| `FactionStanding` | per-faction reputation (-100 hostile..+100 allied) | `adjust`, `tier_of`, `will_attack`, `will_assist`, `to_dict` | `TurfClaim` calls `adjust()` on the rival crew when you muscle into their turf; NPC AI still reads `will_attack`/`will_assist`; faction ids align with `GangTerritory` |

## Combat

| System | Purpose | Key API | Wiring |
|---|---|---|---|
| `WeaponBallistics` | falloff / hit-zones / spread / recoil bloom | `effective_damage`, `spread_direction`, `Bloom` | apply in `WeaponController` when resolving a shot |
| `ExplosionModel` | radial damage + knockback + chain | `damage_at`, `knockback`, `should_chain`, `apply_to_many` | on grenade/vehicle blast, query nearby hittables |
| `MeleeCombat` | combos / block / counter / stamina | `strike`, `block_reduction`, `counter_damage` | drive `MeleeController` strikes + a stamina HUD |
| `CombatCover` | cover-point eval | `provides_cover`, `best_cover`, `peek_position`, `is_exposed` | give `CombatAi`/`Police` cover-seeking behaviour |
| `StealthDetection` | awareness meter | `update(can_see, vis, dt)`, `is_alerted`, `detection_speed` | per-NPC perception atop `CrimeWitness` FOV |
| `SoundPropagation` | the world's "ear": who hears a gunshot/alarm/engine, masked by distance + ambient | `base_loudness`, `is_alarming`, `perceived_intensity`, `is_audible`, `audible_radius`, `reaction_for`, `emit`, `loudest_heard`, `ambient_for` | the auditory complement to `StealthDetection` (visual). On a noisy event call `emit(src, kind, npc_listeners, ambient_for(base, is_night, rain01))`; for each `heard` listener seed `StealthDetection`, and on `ALARMED` promote via `CrimeWitness` / raise `WantedSystem` / spike `CrowdPanic` / dispatch `EmergencyServices`. `loudest_heard` picks what an idle NPC turns toward. Pure — no world access |
| `FirePropagation` | spreading fire + fuel burnout | `ignite_intensity`, `update_fires`, `damage_per_second` | molotov/vehicle fire that spreads across flammables |
| `WeaponLoadout` | attachments (optic/muzzle/mag/grip) tune a weapon | `equip`, `mult_for`, `apply_mult`, `mag_size`, `is_suppressed` | a weapon-mod UI; apply `apply_mult(base, "spread"/"range"/"recoil")` around the `WeaponBallistics` calls, `mag_size` to ammo, `is_suppressed` to witness noise |

## Vehicles

| System | Purpose | Key API | Wiring |
|---|---|---|---|
| `VehicleHandling` | arcade grip/drift | `apply_friction`, `slip_angle`, `drift_factor`, `DriftScorer` | layer onto `Car`/`Bike` velocity each physics frame |
| `VehicleHealth` | damage -> fire -> wreck | `apply_damage`, `tick`, `state`, `just_exploded` | on car damage; trigger `ExplosionModel` on wreck |
| `VehicleCondition` | persistent per-vehicle fuel + engine/tire wear (the gas/mechanic loop) | `drive`, `apply_crash`, `refuel`, `service`, `condition`, `fuel_fraction`, `top_speed_factor`, `grip_factor` | a `Car` actor owns one per active vehicle: each frame `drive(id, dist, intensity)` burns fuel + wears; `VehicleHealth` impacts feed `apply_crash`. HUD reads `fuel_fraction`/`condition`; `top_speed_factor` caps max-speed and `grip_factor` multiplies into `VehicleHandling`. A gas-station calls `refuel`, a mechanic `service`. **Produces the `condition` float `ChopShop.value()` consumes** (the seam nothing fed before). Persists via `to_dict`/`load_dict`. Pure |
| `VehicleModShop` | tiered upgrades -> stat multipliers | `upgrade`, `top_speed_multiplier`, `grip_multiplier` | a mod-garage trigger; multipliers feed `VehicleHandling` |
| `Carjacking` | yank a driver out | `can_reach`, `door_side`, struggle timer, `heat_for_jack` | player enter-vehicle path + `WantedTracker.report_crime` |
| `GarageStorage` | store/retrieve/impound vehicles | `store`, `retrieve`, `impound`, `recover_from_impound` | **wired live** via `GarageStorageTrigger` (Actor, tag `interactables`): drive a car into the `StoreZone` trigger volume to park it (hidden + reparented out of the world, remembered by `store()`); press interact at the garage to `retrieve()` the most-recently parked car back to the door (same actor, optional fee to `PlayerStats`). CI-guarded by the `GTC.*` GarageStorage automation test. TODO: surface `impound`/`recover_from_impound` for cars abandoned while wanted + a HUD garage list |
| `ChopShop` | fence a vehicle: class × condition × demand × heat | `value`, `deliver`, `set_requests`, `rotate_requests`, `total_earned` | **wired live** via `ChopShopTrigger` (Actor, tag `chop_shop`): drive a car into its `FenceZone` trigger volume and it's stripped — priced by class (inferred from the car's blueprint) × condition (`Car.health/max_health`) × demand, paid to `PlayerStats`, then the car is removed. CI-guarded by the `GTC.*` ChopShop automation test. TODO: rotate most-wanted orders for the demand bonus + a heat discount off `WantedTracker` |
| `Parachute` | freefall/deploy/land | `deploy`, `update_fall_speed`, `landing_impact` | player skydive state above a height threshold |

## Driving the world alive

| System | Purpose | Key API | Wiring |
|---|---|---|---|
| `PedestrianTraffic` | peds dodge cars / cross safely | `nearest_threat`, `dodge_velocity`, `safe_to_cross` | blend `dodge_velocity` into `Pedestrian` steering near traffic |
| `CrowdPanic` | gunfire panic ripples through a crowd | `initial_fear`, `update_crowd`, `flee_direction` | **wired live** via `CrowdPanicDirector` (Actor): hooks `WeaponController.crime_committed` (tag `weapon_controller`) and on a shooting calls `Pedestrian.scare()` on every ped within `scare_radius` (fear falloff from `initial_fear`). CI-guarded by the `GTC.*` CrowdPanic automation test. TODO: panic contagion via `propagated_fear`, + spike on explosions |
| `TrafficSignal` | junction light cycle + right-of-way | `tick`, `light_for`, `should_stop`, `yields_to` | place at intersections; gate `TrafficCar` at the stop line |
| `EmergencyServices` | ambulance/fire dispatch | `service_for`, `nearest_responder`, `should_dispatch`, `eta`, response timer | **wired live** via `ResponderDispatcher` (Actor): tag-polls `weapon_controller`, and on a player kill rolls out an ambulance that drives in, treats the scene, and clears. CI-guarded by the `GTC.*` ResponderDispatcher automation test. TODO: WRECK/FIRE incidents off `VehicleHealth.just_exploded`, + a visible on-scene treat dwell |
| `WeatherEffects` | rain/fog gameplay impact | `grip_multiplier`, `visibility_range`, `ai_sight_multiplier`, `ai_sight_range` | `WeatherController` exposes normalized wetness/fog, and `Police` now shortens spotting range in heavy rain/fog. TODO: feed wetness into vehicle grip/traffic speed |
| `AmbientEvents` | weighted freeroam encounters (mugging/race/heist) | `trigger_next`, `eligible_ids`, `can_fire`, `trigger` | **wired live** via `AmbientEventDirector` (timer rolls by stars+district) + `AmbientEncounterSpawner` (handles `street_race` by activating the live `RaceController` and setting a `PlayerStats` objective). CI-guarded by the `GTC.*` AmbientEvent + AmbientStreetRace automation tests. TODO: spawn handlers for the remaining default ids (`mugging`, `getaway_driver`, …) |

## Missions / activities

| System | Purpose | Key API | Wiring |
|---|---|---|---|
| `MissionChain` | campaign sequencing | `current`, `complete_current`, `is_campaign_complete` | wired live via `MissionCampaign` (5-mission arc in Miami.umap) |
| `MissionObjectiveTypes` | reach/collect/eliminate/escort/survive/defend | `*_satisfied`, `Counter` | use in `MissionController` for varied objectives |
| `SideJob` | taxi/delivery/vigilante contracts | `fare`, `payout`, active-job lifecycle | wired live via `SideJobBoard` (pickup/dropoff trigger volumes) |
| `StreetRace` | checkpoint laps + placement | `reached`, `progress`, `placement`, `reward` | **wired live** via `RaceController` (Actor, tag `race`): ordered child spawn-point actor checkpoints, pass within `checkpoint_radius` in order to finish a lap, finishing pays `reward`. CI-guarded by the `GTC.*` Race automation test. TODO: rival drivers + a longer road circuit + a HUD progress/checkpoint readout |
| `HeistCrew` | crew skill/cut -> odds + take | `add_member`, `success_chance`, `attempt`, `payout` | **surfaced to the player** via `HeistPlanningBoard` (Actor, tag `interactables`): each press recruits the next crew member (charged to `PlayerStats`); once staffed, a press pulls the job — a win banks the player's crew-adjusted share, a loss pays nothing, then the crew resets. CI-guarded by the `GTC.*` HeistPlanningBoard automation test. TODO: pick-your-crew UI + a real heist mission finale |
| `StuntScore` | freeform trick-combo scorer (air/near-miss) | `add_trick`, `multiplier`, `pending_score`, `bank`, `wipe` | a stunt controller calls `add_trick` on detected tricks, `bank` on a clean land / `wipe` on crash; apply `cash_for`/`respect_for` to wallet + `PlayerProgression` |
| `HitContract` | assassination board that moves the stock market | `available`, `accept`, `market_effect_of`, `complete`, `total_earned` | **wired live** via `HitContractBoard` (Actor, tag `interactables`): accept the next market-moving contract, press again to complete the hit — pays the `reward` to `PlayerStats` and best-effort feeds the `market_effect` to the live `MarketEventCoordinator` (`apply_hit_effect`) so the kill shocks `StockMarket` (the invest-then-hit loop, pairs with `BrokerageTerminal`). CI-guarded by the `GTC.*` HitContractBoard automation test. TODO: a go-to-target objective + an active-hit HUD |

## Economy / progression

| System | Purpose | Key API | Wiring |
|---|---|---|---|
| `ShopModel` | priced catalogue + purchase | `purchase`, `can_afford`, `sell_value` | a shop trigger + `PlayerStats.spend_money` |
| `PropertyOwnership` | buy properties + passive income | `buy`, `accrue`, `collect`, `daily_income` | property triggers + a daily income tick |
| `BusinessVenture` | own + OPERATE production businesses (supply→product→sell) — the active layer atop PropertyOwnership's passive income | `acquire`, `buy_supplies`, `accrue`, `hire`, `upgrade`, `production_rate`, `sale_price`, `sell`, `total_product` | **wired live** via `BusinessVentureHub` (Actor, tag `interactables`): first interact `acquire()`s against `PlayerStats.money` + stocks supplies, the actor `accrue()`s product in `Tick` between visits, and a later interact `sell()`s the stockpile and banks `proceeds`. CI-guarded by the `GTC.*` BusinessVentureHub automation test. TODO: a "manage" UI for `hire`/`upgrade`, and feed real `DistrictEconomy.desirability` as `demand` + a `WantedSystem` 0..1 `heat` into the cash-out |
| `DistrictEconomy` | living real-estate: turf/crime move desirability | `desirability`, `property_value`, `income_multiplier`, `set_control`, `add_heat`, `invest` | feed `GangTerritory.influence_in` into `set_control` and crimes into `add_heat`; scale `PropertyOwnership` price/income by `desirability` |
| `ContrabandMarket` | buy-low/sell-high arbitrage | `price_in`, `buy`, `sell`, `best_market`, `bust_risk` | **wired live** via `ContrabandDealer` (Actor, tag `contraband_dealer`): a DealZone trigger volume buys, a FenceZone trigger volume in a dearer district sells. `bust_risk` is now **surfaced on police proximity** — `resolve_fence_sale()` voids the payout and raises wanted heat (tag `wanted`) when cops are near the fence at sell time, scaling with deal size and how many/how close. CI-guarded by the `GTC.*` ContrabandMarket + ContrabandBust automation tests. TODO: a price-board UI |
| `CasinoGames` | roulette/slots/blackjack | `roulette_spin`, `roulette_payout`, `slot_payout`, `blackjack_settle`, `hand_value`, `dealer_should_hit`, bankroll | **slots + roulette + blackjack wired live**: `SlotMachine`, `RouletteTable`, and `BlackjackTable` (all Actors, tag `interactables`) — press interact to bet + play a round, paying/charging `PlayerStats`. CI-guarded by the `GTC.*` SlotMachine + RouletteTable + BlackjackTable automation tests. TODO: a reel/wheel/card-result HUD |
| `PlayerProgression` | respect/XP + unlocks | `add_xp`, `level`, `unlocks_at`, `is_unlocked` | wired live via `ProgressionTracker` (missions grant XP) |
| `PlayerSkills` | activity-based proficiency (drive/shoot/stamina...) | `train`, `level`, `tier`, `bonus`, `overall_mastery`, `to_dict` | **wired live** via `SkillsCoordinator` (code-spawned under `Player`, tag `player_skills`): walking distance trains stamina, driven-vehicle distance trains driving, and confirmed weapon hits train shooting. `SaveManager` persists it as `player_skills`. CI-guarded by the in-module `Tests/` automation tests (`GTC.*` SkillsCoordinator + SaveManagerPlayerSkills) |
| `StatTracker` | lifetime stats + 100% | `add`, `is_achieved`, `completion_percent`, serialize | wired live via `StatsCoordinator`; persisted by `SaveManager` as `lifetime_stats` |
| `StockMarket` | event-driven equities + tracked portfolio | `apply_rivalry_shock`, `apply_sector_event`, `price`, `buy`, `sell`, `unrealized_gain` | **surfaced to the player** via `BrokerageTerminal` (Actor, tag `interactables`): a walk-up terminal that trades the world's ONE LIVE `StockMarket` — the one owned by `MarketEventCoordinator` that `HitContractBoard` shocks and wanted spikes rally (it no longer drifts a private copy). First press buys a lot, a later press sells for realized P&L against `PlayerStats`, so a completed assassination or a wanted spike moves what the brokerage sells for — the invest → take-the-hit → cash-out loop is closed end to end. CI-guarded by the `GTC.*` BrokerageTerminal automation test. TODO: a multi-company trade UI |

## Support

| System | Purpose | Key API | Wiring |
|---|---|---|---|
| `GpsNavigation` | route progress/ETA/next-turn | `distance_remaining`, `progress`, `next_turn`, `has_arrived` | **wired live** via `GpsRouteCoordinator` (code-spawned under `Minimap`, tag `gps_route`): active objective waypoints feed `Minimap.set_gps_route()` with a NavGrid-backed route when a live traffic/crowd/city director has one, then fall back to the minimap's direct waypoint line when no route is available. CI-guarded by the in-module `Tests/` automation test (`GTC.*` GpsRouteCoordinator) |
| `RadioScheduler` | song/DJ/ad/news programming | `next_segment`, `pick_song`, `advance` | bridged by `RadioNewsDirector`: advances the station program and broadcasts a delegate carrying a line for songs/DJ/ads/IDs/NEWS. `RadioReadout` on the gameplay HUD subscribes to those lines and to `VehicleRadio.now_playing_changed`, so station/news text is visible to the player. CI-guarded by the `GTC.*` RadioNews automation test |
| `NewsBulletin` | player deeds -> reactive radio/TV headlines | `report`, `next_bulletin`, `has_pending`, `recent` | `CrimeReactionDirector` reports crimes; `RadioNewsDirector` finds that queue and drains `next_bulletin()` when `RadioScheduler` yields a NEWS slot. CI-guarded by the `GTC.*` RadioNews automation test |
| `ContactServices` | call a contact for a favour (lower-wanted/mechanic/backup) | `request`, `can_use`, `cooldown_remaining`, `is_ready` | wired into the live `Phone`: connected service contacts charge `PlayerStats`, broadcast a `service_requested` delegate, the heat service clears `WantedTracker`, and the mechanic repairs the current driven vehicle. CI-guarded by the `GTC.*` PhoneContactServices + PhoneMechanic automation tests. TODO: map weapons/transport/combat delegates to actual spawns |
| `MusicDirector` | dynamic score intensity (calm→tension→combat→chase) | `update`, `current_tier`, `current_stem`, `is_intense` | an audio actor calls `update({stars, in_combat, in_chase}, dt)` each frame and crossfades to `current_stem()`; escalates instantly, de-escalates on a hold |
| `SwimStamina` | oxygen/stamina/drowning | `update`, `is_drowning`, `swim_speed`, `drown_damage` | the meter layer above the swim motion actor |
| `LootTable` | weighted seeded drops | `roll`, `roll_many`, `drop_chance_satisfied` | **wired live** via `LootDropDirector` (Actor, tag `loot_drop`): hooks `WeaponController.crime_committed` and on a kill spawns a `Pickup` (medkit/armor) at the kill site; `LootCrate` props use the same combat `take_damage()` contract and call `drop_from_crate()` when smashed. CI-guarded by the `GTC.*` LootDrop automation test + the in-module `Tests/` automation test (`GTC.*` LootCrate). TODO: cash/ammo pickup kinds |
| `CharacterRoster` | dual-protagonist switching + per-lead state | `switch_to`, `can_switch`, `active`, `money_of`, `position_of`, `to_dict` | load the active lead's wallet/wanted/position into PlayerStats + world on `switch_to`; write it back before switching away |

---

## Already wired & CI-guarded in `Miami.umap`

The playable map runs the full loop, each gated by a runtime check in
`tools/check.sh`: player health/stats/wanted/mission/bark + crowd/traffic/police
directors (`GTC.*` MiamiWiring), crime -> wanted -> police dispatch
(`GTC.*` MiamiLoop), the 5-mission campaign paying money/respect/stats
(`GTC.*` MiamiMission), pay-n-spray wanted-clear (`GTC.*` MiamiPayspray), and
the busted/arrest fail-loop (`GTC.*` MiamiArrest). `WantedEvasionController`,
`MissionReward`, `ProgressionTracker`, `StatsCoordinator`, `MissionCampaign`,
`PaySprayShop`, and `SideJobBoard` are the self-wiring coordinators already in the
level — copy their shape to wire the rest.

`MarketEventCoordinator` is **wired live in `Miami.umap`** (self-wiring actor, cf.
PaySprayShop): it owns a `StockMarket`, subscribes to the `wanted` tag's
`stars_changed` delegate to rally defense stocks on a crime spree, and applies
`HitContract` effects via `apply_hit_effect`. Actor logic is CI-gated headless by
the `GTC.*` MarketEvent automation test (a mock world); the live in-level connection is
asserted by the `GTC.*` MiamiWiring automation test. Remaining to surface to the player: a
brokerage/ticker UI reading its public `market`.

`CrimeReactionDirector` is its sibling on the same `wanted` hook and is **wired
live in `Miami.umap`** too: it owns a `NewsBulletin` + `DistrictEconomy` and, on a
wanted spike, files a severity-scaled headline and heats the active district
(which cools over time via `Tick`). The two directors split the signal cleanly
— market vs news+real-estate. Actor logic CI-gated by the `GTC.*` CrimeReaction automation test;
live connection asserted by the `GTC.*` MiamiWiring automation test. `RadioNewsDirector`
now joins tag `radio_news`, finds this director through tag `crime_reaction`,
and drains `news.next_bulletin()` into a scheduled NEWS slot. `RadioReadout` in
the `WBP_GameHUD` UMG widget subscribes to those program lines and vehicle-radio now-playing
updates, so the station/news line has a player-facing HUD surface. World-free
bridge + HUD readout are CI-gated by the `GTC.*` RadioNews automation test.

`CharacterSwitcher` owns a `CharacterRoster` and syncs each lead's wallet through
the live `player_stats` actor on `request_switch()` (write the current wallet back,
load the incoming lead's), so per-character money persists across switches. CI-gated
headless by the `GTC.*` CharacterSwitch automation test.

`AmbientEventDirector` is **wired live in `Miami.umap`**: on a timer it builds
`{stars (from the wanted tag), district}`, rolls `trigger_next`, and broadcasts an
`encounter_triggered(id, kind)` delegate. Actor logic CI-gated headless by
the `GTC.*` AmbientEvent automation test; live in-level connection asserted by
the `GTC.*` MiamiWiring automation test. `AmbientEncounterSpawner` listens for those delegates
and, for `street_race`, calls `RaceController.start_challenge()` plus a HUD
objective on `PlayerStats` — CI-guarded by the `GTC.*` AmbientStreetRace automation test.
Remaining to wire: spawn/act handlers for the other default encounter ids.
