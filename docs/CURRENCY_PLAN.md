# Our Own Currency — Complete Design & Implementation Plan

> How GT-caliber gets a single, **server-authoritative, persistent,
> dupe-proof** in-game currency that every economy system spends and earns
> against — plus the optional premium (real-money) layer, and an honest note on
> the crypto path.
>
> Companion to [`GTA_RP_ROADMAP.md`](GTA_RP_ROADMAP.md). The currency is the
> backbone of milestones **RP3 (persistence)** and **RP4 (authoritative
> economy)** there; this doc is the deep-dive for that backbone.

---

## 1. What "our own currency" means here

A branded in-game money with one authoritative source of truth, used by every
earning and spending system in the game. Concretely:

- A **primary currency** with two wallets per character — **Cash** (on-hand;
  can be dropped/robbed/lost) and **Bank** (secure; transfers, ATM, wages).
  *(This is the GTA-RP standard; it's what makes robbery and banking meaningful.)*
- An optional **premium currency** (real-money purchasable, like GTA's Shark
  Cards) — a separate wallet that is **never convertible back** to real money.
- Everything **server-owned and DB-persisted**; clients only ever *display* a
  replicated balance and *request* transactions.

Working names (rename freely — kept as constants in one place): primary =
**GTC$ "Caliber Dollar"** (symbol `$`), premium = **GTC Coins** (symbol `◈`).

---

## 2. Current state (what we build on)

The economy already exists as **pure, headless-testable C++ value models** under
`Source/GTC_UE5/Systems/Economy/` (+ `PaySprayLoot`, `Missions/Coordinators/
MissionReward`, `Missions/StuntScore`). The pattern is uniform:

```cpp
// e.g. ShopModel / PropertyOwnership / BusinessVenture / StockMarket ...
FResult Op(... , int32 Balance);   // takes a balance in
// returns { bSuccess, Cost/Reward, NewBalance, Reason }  — caller applies it
```

So each system already speaks "balance in → new balance out" and **does not own
the money**. What's missing is the single thing that *does* own it:

- ❌ No central **wallet / ledger / account** type (grep confirms none).
- ❌ No **transaction log / audit trail** (no dupe protection, no history).
- ❌ No **persistence** (balances are transient locals).
- ❌ No **server authority / replication** (everything assumes single local
      authority).
- ⚠️ Balances are **`int32`** everywhere (max ~2.14 billion). RP economies
      inflate; this *will* overflow. Decision needed (see §4).

The plan: introduce the authoritative ledger, then route every existing model's
result through it. The models barely change — they keep computing, the ledger
becomes the only thing that *commits*.

---

## 3. Design principles (non-negotiable)

1. **Server-authoritative.** Clients send *intent* ("buy item X"); the server
   validates, mutates the ledger, replicates the new balance. A client never
   sets its own balance. This is the entire anti-cheat foundation.
2. **Integers only, never floats.** Money is whole minor units. Percentages
   (sell-back 50%, taxes) compute in integer math with explicit rounding rules.
3. **Double-entry / conservation.** Money is *created* only by defined **sources
   (faucets)** and *destroyed* only by defined **sinks**. A transfer never
   changes the total. Every mutation references a reason code. This makes dupes
   detectable: total supply must equal Σ(faucets) − Σ(sinks).
4. **Every mutation is a logged transaction** with an **idempotency key** (retry
   the same key → no double-apply). No silent balance writes anywhere.
5. **One source of truth.** The DB is canonical; the live ledger is a
   write-through cache of it. On conflict, DB wins; reconcile on load.
6. **One place to rename/configure** currency name, symbol, denomination, and
   tuning — a `CurrencyConfig` data asset / constants header.

---

## 4. Key decisions (resolve before coding)

- **Denomination/precision** — Recommend **whole dollars** (no cents) for an RP
  economy; simpler UI and math. If cents are wanted later, store minor units
  (×100). *Pick one now; changing later is a migration.*
- **Integer width** — Ledger + DB use **`int64`**; the pure models currently use
  `int32`. Options: (a) widen the models to `int64` (cleanest, touches many
  files + their parity tests), or (b) keep models `int32` but **cap per-op
  amounts** and hold the *account total* in `int64` at the ledger. **Recommend
  (a)** for the wallet-facing path, (b) acceptable short-term. Flag the parity
  oracles that must be updated.
- **Wallets** — Cash + Bank for primary; one wallet for premium. Death/robbery
  affects **Cash** only.
- **Premium currency** — build the architecture as a separate wallet from day
  one (even if you don't sell it yet), so it's never entangled with earned money.
- **Crypto/blockchain** — see §11. **Recommended: no** for the core economy.

---

## 5. Architecture

```
 Pure model (unchanged-ish)        Authoritative runtime            Persistence
 ┌──────────────────────┐         ┌───────────────────────┐       ┌──────────────┐
 │ ShopModel / Property  │ result  │ UGTCCurrencySubsystem │ write │ Persistence  │
 │ / BusinessVenture /   │────────►│  (server, GameState)  │──────►│ API → DB     │
 │ Heist / Casino / ...  │ {cost,  │  - FCurrencyLedger    │through│ accounts,    │
 │  (compute only)       │  reward}│  - per-character acct │       │ transactions │
 └──────────────────────┘         │  - txn log + idemp.   │◄──────│ (canonical)  │
                                   │  - replicate balance  │ load  └──────────────┘
                                   └──────────┬────────────┘
                                              │ replicated (read-only) balance
                                   ┌──────────▼────────────┐
                                   │ Client UI: HUD money,  │
                                   │ phone bank app, ATM    │
                                   └───────────────────────┘
```

Three layers, mirroring the existing repo idiom (pure model + UObject wrapper +
`GTC.*` test):

1. **`FCurrencyLedger`** — *pure, non-UObject, headless-testable.* Owns the
   account math: wallets, `CanAfford`, `Debit`, `Credit`, `Transfer`, applying a
   model result, conservation invariant. **No engine, no network, no DB.** Full
   `GTC.Currency.*` automation test (this is where correctness lives).
2. **`UGTCCurrencySubsystem`** (or component on `PlayerState`/`GameState`) —
   *server-authoritative runtime.* Holds the ledger, exposes **Server RPCs**
   (`Server_RequestPurchase`, `Server_RequestTransfer`…), validates intent, calls
   the ledger, writes through to persistence, **replicates** the resulting
   balances to the owning client. Emits transaction events for HUD/phone/audit.
3. **Persistence binding** — write-through to the RP3 persistence API
   (Postgres `accounts`/`characters`/`transactions`; Redis for locks + hot
   balance). Idempotency keys dedupe retries; a distributed lock serializes
   concurrent mutations on one character.

---

## 6. Data model

**Transaction record** (the unit of all money movement):

```
Transaction {
  id            uuid            // server-generated
  idempotencyKey string        // client/op-supplied; unique → apply-once
  characterId   uuid
  wallet        enum(Cash,Bank,Premium)
  delta         int64          // + credit / − debit
  balanceAfter  int64
  reason        enum           // WAGE, MISSION_REWARD, HEIST_CUT, LOOT,
                               // SHOP_BUY, SHOP_SELL, PROPERTY_BUY, REPAIR,
                               // TRANSFER_IN, TRANSFER_OUT, TAX, FEE, ADMIN,
                               // PREMIUM_PURCHASE, ROBBERY_LOSS/GAIN ...
  refId         string?        // shop item / property / mission / counterparty
  serverTime    timestamptz
}
```

**Account** = (characterId → {cashBalance, bankBalance, premiumBalance}),
derivable from the transaction log (the log is the truth; balances are a
materialized cache). Keeping the full log gives free history, audit, dispute
resolution, and dupe forensics.

---

## 7. Integrity & anti-dupe (the part that matters most)

- **Server authority** — every faucet/sink runs server-side only.
- **Idempotency** — duplicate `idempotencyKey` (network retry, double-click) is a
  no-op returning the original result.
- **Atomicity + locking** — debit/credit and the transfer pair are atomic;
  per-character lock (Redis) serializes concurrent ops so two purchases can't
  both read the same stale balance.
- **Conservation audit** — a periodic job verifies Σ balances == Σ faucets −
  Σ sinks; any drift = a dupe/bug alarm.
- **Rate limits & sanity bounds** — per-reason caps (no $10M "wage"), velocity
  limits, negative-balance prohibition, max-balance guard (int64 headroom).
- **Full audit log** — every mutation is queryable; admin actions tagged.
- **No client-trusted amounts** — for a shop buy, the *server* looks up the
  price from the catalogue; it never trusts a client-sent price/cost.

---

## 8. Economic design — sources & sinks (avoid inflation)

A currency is only as good as its faucet/sink balance. Map the existing systems:

**Faucets (money in):** job wages (RP6 roles), `MissionReward`, `HeistCrew`
cuts, `HitContract` rewards, `PaySpray`/`LootTable`, `BusinessVenture` income,
`StockMarket`/`CasinoGames` winnings, `StuntScore` payouts, `PropertyOwnership`
daily income.

**Sinks (money out):** `ShopModel` purchases, `PropertyOwnership` buys, vehicle
repairs/mods (modshop), `BusinessVenture` supplies/upgrades, casino losses, fees
& **taxes**, fines (wanted/arrest), insurance. RP economies need strong sinks or
they inflate — taxes/fees/repair costs are the dials. `DistrictEconomy` can
modulate prices regionally.

Action: define a **faucet/sink balance sheet** in `CurrencyConfig` and tune it;
the conservation audit (§7) tells you when it drifts.

---

## 9. UI surfaces (reuse what exists)

- **HUD money display** — `UI/Hud/GtcHudFormat` + `GTCHudWidget` already format
  money; bind to the replicated balance, animate deltas (GTA-style + / −).
- **Phone bank app** — `UI/Phone/SGTCPhone` already references money; build the
  banking app: balance, transfer to player, transaction history.
- **ATM / bank actor** — deposit/withdraw between Cash↔Bank (world interaction).
- **Shop/store UIs** — already spend via `ShopModel`; reroute through the
  subsystem's `Server_RequestPurchase`.
- **Premium store** — separate screen for buying premium currency (§10).

---

## 10. Premium currency layer (optional, real money)

Build the *wallet* now, enable *selling* later. Like GTA Shark Cards:

- Separate **Premium** wallet; **earned money and premium never inter-convert**
  in a way that cashes out (premium → in-game spend only; never premium → real
  money).
- Purchased via **platform store / EOS / Steam / mobile IAP**; the **store
  validates the receipt server-side**, then the server credits Premium. Never
  trust a client "I paid" claim.
- **Cosmetic / convenience only — never pay-to-win** (keeps it fair and reduces
  regulatory/gambling exposure, especially with the casino present).
- **Legal at RP11:** real-money purchases + accounts → ToS, refunds, regional
  consumer law, tax, and **loot-box/gambling regulation** if casino + premium
  mix. Get this reviewed before selling anything.

---

## 11. Crypto / blockchain (honest appendix)

If "our own currency" meant a real token: **strongly not recommended for the
core game economy.** It adds securities/money-transmitter regulation, KYC/AML,
wallet custody/key-loss liability, chain latency/fees fighting a real-time game
loop, and serious anti-fraud burden — for benefits a server-authoritative DB
ledger already provides (persistence, scarcity, transfers, audit). If ever
pursued, it's a *separate product* layered outside the game economy, with
dedicated legal counsel — not a substitute for the ledger in this plan.

---

## 12. Implementation phases

Effort: `S`=days, `M`=weeks. Maps onto RP-roadmap RP3/RP4.

### C0 — Ledger core (pure) `S`
- [ ] `FCurrencyLedger` (Cash/Bank/Premium wallets; CanAfford/Debit/Credit/
      Transfer/ApplyModelResult; conservation invariant; int64).
- [ ] `CurrencyConfig` (name/symbol/denomination/faucet-sink tuning constants).
- [ ] `GTC.Currency.*` automation tests (afford, overdraw rejected, transfer
      conserves total, idempotent apply, overflow guard).
*Exit: ledger math proven headless, zero engine deps.*

### C1 — Authoritative runtime (single-player first) `S`
- [ ] `UGTCCurrencySubsystem` holding the ledger; transaction log in memory.
- [ ] Route existing economy models' results through it (ShopModel,
      PropertyOwnership, BusinessVenture, MissionReward, PaySpray, Heist, Casino,
      StockMarket…) — one funnel, no scattered balance writes.
- [ ] Bind HUD + phone to the balance; delta animations.
*Exit: solo CityBeachStrip — earn/spend across all systems flows through one
ledger with full local history.*

### C2 — Persistence `M`  *(needs RP3 backend)*
- [ ] DB schema: `accounts`, `transactions`; Redis locks/cache.
- [ ] Write-through + load-on-join + reconcile; idempotency keys.
*Exit: balances survive logout + server restart; replayed ops don't double-apply.*

### C3 — Replication & authority `M`  *(needs RP1/RP2)*
- [ ] Move the subsystem server-side; Server RPCs for purchase/transfer/etc.;
      replicate read-only balances to the owning client.
- [ ] Server looks up all prices/rewards (never trusts client amounts).
*Exit: two players transact on a dedicated server; client cannot alter its own
balance; conservation audit clean.*

### C4 — Integrity hardening `M`  *(with RP7)*
- [ ] Rate limits, per-reason caps, anomaly logging, conservation audit job,
      admin money tools (audited), dispute/history queries.
*Exit: survives an adversarial dupe attempt; every cent is traceable.*

### C5 — Economic tuning `M` (ongoing)
- [ ] Faucet/sink balance sheet; taxes/fees/repair dials; `DistrictEconomy`
      modulation; monitor inflation via the audit + telemetry.

### C6 — Premium currency `M` (optional, with RP11)
- [ ] Premium wallet sell flow; server-side receipt validation; cosmetic store;
      legal/ToS review.

---

## 13. Testing

- **Unit (pure):** `GTC.Currency.*` — afford/overdraw, transfer conservation,
  idempotency, rounding rules, overflow/underflow guards.
- **Integration:** model→subsystem funnel applies results correctly; persistence
  round-trip; load reconciliation.
- **Multiplayer (`GTC.Net.*`):** two clients, server-authority enforced, no
  client-side balance forge, concurrent-purchase race serialized.
- **Property/fuzz:** random op sequences must preserve the conservation
  invariant (Σ balances == Σ faucets − Σ sinks).

---

## 14. Risks

1. **int32 → int64 migration** of the pure models + their parity oracles —
   plan the file sweep; don't let int32 balances ship into a persistent economy.
2. **Dupe bugs** from races/retries — mitigated by idempotency + locks + the
   conservation audit; treat any audit drift as P0.
3. **Inflation** from weak sinks — needs active tuning + telemetry, not a
   one-time setting.
4. **Premium/gambling legal surface** (casino + real money) — review before any
   real-money feature ships.
5. **Authority retrofit** — every economy entry point must funnel through the
   server subsystem; a single bypass = an exploit. Audit at C3 and C4.
```
