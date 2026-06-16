# Wave 3 wiring & deferred-decision notes

Decisions deferred from Wave 1/2 that must be resolved when the engine-coupled
Wave-3 layer (actors, pawns, PlayerState, replication) is built. Each item names
the systems already on `main` and what W3 must do.

## Armor ownership — RESOLVED (decision 2026-06-16; supersedes the open double-ownership item)

**Decision: `UPlayerStatsComponent` is the SOLE armor owner. `UPlayerHealthComponent` owns health only.**

**Why two pools existed.** Both `FPlayerHealthModel` (`Player/Health/`, ported from
`player_health_model.gd`) and `FPlayerStats` (`Player/Stats/`, ported from `player_stats.gd`) model an
**identical** armor pool — `Armor` in `[0, MaxArmor=100]`, add = `Clamp(Armor + max(amt,0), 0, MaxArmor)`,
soak-first, **no regen/decay**. The UE port faithfully reproduced **both** because the Godot upstream is
itself split-brained (evidence below). Authoritative source = `player_stats.gd`: both the Godot and UE
docs state it owns "body armor, the wallet, and the active objective ... health lives in [the health
component]," and it is the HUD-/economy-wired pool (`OnArmorChanged` drives the bar; shops call its
`AddArmor`). `FPlayerHealthModel`'s armor is the vestigial duplicate.

**Upstream evidence — the split-brain / double-soak in `player_health.gd::take_damage`:**
```gdscript
func take_damage(amount, _point, _normal):
    if _dead:
        return
    var to_health := amount
    if amount > 0.0:
        var stats := get_tree().get_first_node_in_group("player_stats")
        if stats != null and stats.has_method("soak_damage"):
            to_health = float(stats.soak_damage(amount))   # (1) STATS armor soaks first
    if _model.apply(to_health):                            # (2) _model.apply soaks MODEL armor AGAIN, then health
        _die()
    else:
        changed.emit(_model.fraction())
```
A single hit soaks **stats armor → model armor → health** (double-soak). Compounding it: pickups fill the
*model* pool (`player_health.add_armor → _model.add_armor`), shops fill the *stats* pool, the HUD
`armor_changed` signal is on *stats* but `armor_fraction()` reads the *model*, and the health save uses the
*model* armor. The `take_damage` comment admits stats armor "had ZERO callers — a paid vest gave no
protection," i.e. a half-finished migration toward PlayerStats that never removed the model pool.

**W3 resolution / wiring (do at the W3 PlayerState/pawn wiring site — NOT now):**
- `UPlayerStatsComponent` is the canonical armor owner; armor is **server-authoritative, `COND_OwnerOnly`,
  replicated alongside `Money`** (one pool, one replicated value — see the replication item below).
- **Neutralize the health model's armor WITHOUT touching the pure model or its tests:** construct
  `FPlayerHealthModel` with **`ArmorMax = 0`** → its `AddArmor` clamps to 0 and `Apply` soaks 0, so it
  becomes health-only. All 18 `FPlayerHealthModel` parity tests (incl. the 5 armor cases) stay green
  untouched — do **not** strip the field.
- **Damage pipeline (exact steps):**
  1. damage source → `UPlayerStatsComponent::SoakDamage(amount)` → returns `overflow` (drains stats armor,
     fires `OnArmorChanged`).
  2. `overflow` → `UPlayerHealthComponent` → `FPlayerHealthModel::Apply(overflow)` (health-only, since
     `ArmorMax = 0`).
- **Pickups / shop** → `UPlayerStatsComponent::AddArmor`.
- **HUD armor bar** → `UPlayerStatsComponent::GetArmorFraction` / `OnArmorChanged`.
- **Save:** armor from `UPlayerStatsComponent::Serialize` (`{money, armor}`); the health save records
  **health only** (drop the model's armor field from the health snapshot).

**Consequence — retires the upstream double-soak.** Because only the stats pool soaks and the health
model's armor is neutralized (`ArmorMax = 0`), a single hit now soaks armor **exactly once** (stats) and
the remainder hits health. This fixes the Godot stats-then-model-then-health double-soak as a free side
effect, and collapses pickup / shop / HUD / save onto the single stats pool.

## Player-health replication (from W2 batch 2)
- **State:** `UPlayerHealthComponent` shipped as a **single-player-safe
  placeholder** — `SetIsReplicatedByDefault(true)` only; no
  `GetLifetimeReplicatedProps`, no RPC guards, no `COND_*`. No hidden assumptions
  baked in (verified by review), so the change is cheap.
- **W3 must:** apply the **PlayerStats resolution** when the W3 `PlayerState`
  actor exists — **owner = `PlayerState`, `COND_OwnerOnly` replication**. Per the
  Armor-ownership resolution above, **armor replicates on `UPlayerStatsComponent`**
  (server-authoritative, `COND_OwnerOnly`, alongside `Money`); `UPlayerHealthComponent`
  is **health-only**, so its server-authoritative mutators are damage/heal/revive
  (no armor). Server RPCs guard those mutators. The GAS-migration-friendly
  accessor+delegate interface already mirrors a future `UAttributeSet` 1:1 if GAS
  is adopted.

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
