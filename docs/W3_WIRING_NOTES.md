# Wave 3 wiring & deferred-decision notes

Decisions deferred from Wave 1/2 that must be resolved when the engine-coupled
Wave-3 layer (actors, pawns, PlayerState, replication) is built. Each item names
the systems already on `main` and what W3 must do.

## Armor double-ownership (from W2 batch 2 — health + the merged player-stats)
- **State:** `UPlayerHealthComponent` (`Source/GTC_UE5/Player/Health/`) and
  `UPlayerStatsComponent` (`Source/GTC_UE5/Player/Stats/`) **each model an armor
  pool**, ported 1:1 from two separate Godot sources (`player_health_model.gd`
  and `player_stats.gd`). They are **compile-decoupled** (neither includes the
  other), but at runtime there would be **two sources of truth for armor**.
- **W3 must:** pick **one canonical armor owner** when wiring the player
  `PlayerState`/pawn, and have the other component **defer** to it (read-through
  or remove its armor field). Do not let both mutate armor independently.

## Player-health replication (from W2 batch 2)
- **State:** `UPlayerHealthComponent` shipped as a **single-player-safe
  placeholder** — `SetIsReplicatedByDefault(true)` only; no
  `GetLifetimeReplicatedProps`, no RPC guards, no `COND_*`. No hidden assumptions
  baked in (verified by review), so the change is cheap.
- **W3 must:** apply the **PlayerStats resolution** when the W3 `PlayerState`
  actor exists — **owner = `PlayerState`, `COND_OwnerOnly` replication,
  health/armor mutators server-authoritative** (server RPC around damage/heal/
  add-armor). The GAS-migration-friendly accessor+delegate interface already
  mirrors a future `UAttributeSet` 1:1 if GAS is adopted.

## Deferred runtime couplings already implemented as option-1 (own-state, defer write-through)
These W2 subsystems own their own state and take inputs as parameters; W3 wires
the real producers:
- **arrest-from-wanted** (`UArrestSubsystem`): `Tick(Stars, NearestCopDistance, Delta)`
  takes wanted stars + cop distance as params; bust effects land on owned state
  (wallet snapshot + heat-clear intent). W3 feeds it from the WantedSubsystem +
  police proximity, and applies the wallet/heat-clear write-through.
- **health-from-wanted** (`UPlayerHealthComponent`): owns health; W3 wires any
  wanted-driven regen/respawn effects.
- **Wanted W3 adapters** (`UWantedSubsystem`): WeaponController crime reporting,
  police spawner, AI Perception / LOS, disguise — all deferred to W3.
