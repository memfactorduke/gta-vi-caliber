# GT-caliber — Platform Master Plan

> **The one document that ties it all together.** How a team of **<10 people**
> turns what we have into a **GTA-style roleplay *platform*** — not a single
> finished game — where the core team builds the substrate and *creators build
> the content*.
>
> This is the top-level strategy + roadmap. The deep dives live in:
> [`GTA_RP_ROADMAP.md`](GTA_RP_ROADMAP.md) (the online/RP build order),
> [`CURRENCY_PLAN.md`](CURRENCY_PLAN.md) (the economy backbone),
> [`ASSET_PIPELINE.md`](ASSET_PIPELINE.md) / [`ASSETS.md`](ASSETS.md) (asset
> provenance), [`ARCHITECTURE.md`](ARCHITECTURE.md), [`VISION.md`](VISION.md).

---

## 1. TL;DR — the thesis

A sub-10-person team **cannot** hand-author a content-complete GTA (thousands of
assets, missions, jobs, map detail = hundreds of people, many years). So we
don't try. We build the **platform** and let creators build the content:

- **The core team builds the substrate** — engine, networking, persistence,
  currency, a **sandboxed creator framework**, tooling, hosting, and a
  marketplace.
- **We are creator #1** — we build *one* flagship RP server (CityBeachStrip RP)
  *using our own platform*. The tools that let us build it **become the creator
  SDK**. (Dogfooding is the strategy, not an afterthought.)
- **Creators build everything else** — servers, game modes, jobs, businesses,
  assets, scripts — and a creator economy gives them a reason to.

Precedent: **FiveM** (GTA RP exploded because it's a *platform*; communities
built the servers) and **Roblox** (platform + UGC + creator economy). Both are
small-core-team-enabled, large-ecosystem-built.

---

## 2. Why platform, not game (the constraint is the strategy)

| Build a *game* (<10 people) | Build a *platform* (<10 people) |
|---|---|
| Author all content yourself → impossible at scale | Build the substrate + tools; creators author content |
| One world, ships once, then content-starved | Many worlds, grows continuously without you |
| Team size caps the content | Team size caps the *tools*; ecosystem caps the content |
| You carry all the risk | Creators share the creative + content risk |

The <10-person limit isn't a problem to work around — it's the reason the
platform model is the *only* viable path. Every architectural decision below
optimizes for **"someone else builds the content."**

---

## 3. Two products

**A. The Platform Runtime** (core team owns):
dedicated-server framework, replication, persistence + identity, the
**currency/economy backbone**, the **moddable gameplay framework** (combat,
roles, vehicles, world interaction as *composable primitives*), proximity voice,
anti-cheat, multi-tenant hosting.

**B. The Creator Ecosystem** (core team owns the tools; creators use them):
a **sandboxed scripting/content SDK**, the **asset import + generation
pipeline**, content packaging/versioning/distribution, a **server browser +
marketplace/registry**, creator accounts + payouts, docs + samples, trust &
safety.

Everything in Product A is built so Product B can extend it without touching
engine C++.

---

## 4. The line: core team vs creators

**Core team builds (the 20% that enables the 80%):**
the engine module, netcode, the currency ledger, the gameplay *framework*
(combat/roles/economy/vehicle/world hooks), the scripting sandbox + API surface,
the asset pipeline, hosting, the marketplace, anti-cheat, T&S, and the reference
server.

**Creators build (on top, no recompile):**
servers/worlds, game modes (RP, racing, deathmatch…), jobs/roles, businesses,
missions, custom assets (vehicles, clothing, props, maps), scripts, economies,
rules — distributed via the marketplace.

The boundary is the **SDK/scripting API + data-driven content + asset import**.
If something can only be done by editing engine C++, it's *not* creator-reachable
— and that's the bug to fix, milestone by milestone.

---

## 5. Target architecture (multi-tenant + sandboxed extensibility)

```
        Clients (Mac / Win / Linux)
        ┌───────────────────────────┐         EOS: auth, sessions,
        │ UE5 client + proximity VC │◄──────── matchmaking, voice
        └─────────────┬─────────────┘
                      │ replication / RPC
   ┌──────────────────▼───────────────────┐    ┌──────────────────────┐
   │  PLATFORM RUNTIME (dedicated server)  │    │ Platform services     │
   │  ┌─────────────────────────────────┐  │    │  - Persistence API    │
   │  │ Gameplay FRAMEWORK (core C++)   │  │◄──►│  - Postgres + Redis   │
   │  │ combat·roles·economy·vehicles·  │  │    │  - Currency ledger    │
   │  │ world-interaction = PRIMITIVES  │  │    │  - Marketplace/registry│
   │  └───────────────┬─────────────────┘  │    │  - Creator accounts    │
   │  ┌───────────────▼─────────────────┐  │    │  - Server browser     │
   │  │ CREATOR SANDBOX (untrusted)     │  │    │  - Asset CDN          │
   │  │ scripting API (Lua/AngelScript) │  │    │  - Trust & Safety     │
   │  │ + data-driven content + assets  │  │    └──────────────────────┘
   │  └─────────────────────────────────┘  │
   │  one server instance = one creator's   │   (many instances, many
   │  world; isolated, configurable         │    creators, one platform)
   └────────────────────────────────────────┘
```

Three architectural musts the platform model adds on top of the RP roadmap:

1. **Multi-tenant** — the platform hosts *many* creator worlds, not one game.
   Server instances are isolated and per-world configurable; a server browser
   discovers them.
2. **Sandboxed extensibility** — creator code is **untrusted**. The scripting
   layer must be sandboxed so a creator script can't crash the host, read other
   players' data, or forge currency. This is the single hardest platform
   requirement (and why FiveM's security model matters).
3. **Stable creator API** — the gameplay framework exposes versioned, stable
   primitives. Creators build against the API, not engine internals, so the core
   can evolve without breaking the ecosystem.

---

## 6. Asset strategy — build the *pipeline*, not the assets

A tiny team can't author thousands of assets. Three sources, in priority:

1. **Generation pipelines (you already have these — productize them).**
   `Scripts/gtc_*` already includes `gtc_anim` (universal retarget: any humanoid
   gets the full anim library), `gtc_citygen` (procedural map), `gtc_faces`,
   `gtc_cars`, `gtc_forge` (character forge), `gtc_tripo` (AI→3D), `gtc_interiors`,
   `gtc_livingcity`. These are the force-multiplier: turn them into
   creator-facing tools so *anyone* can generate compliant content.
2. **Curated base content** — a CC0 / licensed starter set the core team ships so
   day-one servers aren't empty (legal per `ASSETS.md` provenance rules).
3. **Creator/UGC uploads** — community assets via the import pipeline + marketplace.

**Legal/provenance becomes mission-critical at platform scale.** `ASSETS.md` and
`ASSET_HANDLING.md` rules now apply to *creator* uploads too: automated
provenance checks, IP/moderation gates, no commercial-game assets. UGC + IP is a
real liability surface — design the import pipeline to *enforce* provenance, not
just request it.

---

## 7. Creator economy & the platform currency

The currency (see [`CURRENCY_PLAN.md`](CURRENCY_PLAN.md)) gains a platform
dimension:

- **In-world currency** — each server's RP economy (Cash/Bank), server-
  authoritative, dupe-proof, per `CURRENCY_PLAN.md`. Creators tune their own
  economy via the SDK.
- **Platform currency + marketplace** — a premium currency to buy creator
  content (assets, game modes, cosmetics) in a marketplace, with a **creator
  payout / DevEx** so creators *earn* — the engine of the ecosystem (the Robux
  model). This is what makes creators show up.
- **Heavy legal surface** — real money + payouts + UGC + (casino) gambling =
  consumer law, tax, money-transmission, KYC for payouts, refunds, and
  loot-box/gambling regulation. **Reviewed by counsel before any real money
  moves.** Keep premium purchases cosmetic/convenience, never pay-to-win.
- **Crypto: still no** for the core economy (per `CURRENCY_PLAN.md` §11) — a DB
  ledger gives scarcity/persistence/transfers without the regulatory burden.

---

## 8. Trust, safety & anti-cheat (platform-grade)

Running *untrusted creator code* + *strangers* + *real money* raises the bar far
above a single game:

- **Sandbox hardening** — creator scripts run with least privilege; no host FS/
  net, CPU/memory/time budgets, no cross-world data access, currency only via the
  authoritative API.
- **Anti-cheat by construction** — server authority over all state (already the
  rule); plus per-world isolation so one bad server can't poison the platform.
- **Content moderation** — automated + human review for assets, servers, names;
  reporting; takedown; provenance/IP enforcement.
- **Account & payout integrity** — identity (EOS), fraud detection on the
  marketplace, KYC for creator payouts.
- **Governance** — content policy, creator ToS, dispute resolution, audit logs
  everywhere (the currency already logs every transaction).

---

## 9. Master roadmap — three eras

Consolidates the RP milestones (`RP*`) and currency phases (`C*`) and reorders
them around the platform thesis. **Promoted early vs the game-first roadmap:**
*extensibility/SDK and the asset pipeline* — because they're the product, not a
late add-on.

### ERA I — Substrate & dogfood (build the platform, be creator #1)

- [ ] **P0 — Consolidate & stabilize CityBeachStrip solo** *(= RP0)*: integrate
      shelved systems, fix perf, reliable boot. The flagship-server foundation.
- [ ] **P1 — Build/server foundation** *(= RP1)*: C++ compile path, **Linux
      dedicated-server target**, CI for server + 3 client OSes, **EOS**. Design it
      **multi-tenant from the start** (instances, per-world config).
- [ ] **P2 — Core replication slice** *(= RP2)*: player + one vehicle
      server-authoritative; smooth under latency.
- [ ] **P3 — Persistence, identity & currency core** *(= RP3 + currency C0–C2)*:
      Postgres/Redis, accounts, **the authoritative currency ledger** + write-
      through persistence.
- [ ] **P4 — The extensibility core** *(= RP8, promoted way up)*: the **sandboxed
      scripting API + data-driven content + asset import** as a *first-class
      platform surface*. Everything after is built *through* this layer.
- [ ] **P5 — Gameplay framework as primitives** *(= RP4 + RP6, reframed)*:
      combat, health, police/wanted, roles/jobs, economy hooks, world interaction
      — built as **composable, API-exposed primitives**, not hard-coded content.
- [ ] **P6 — Proximity voice & comms** *(= RP5)*.
- [ ] **P7 — Build the flagship: CityBeachStrip RP** — a complete RP server built
      *entirely through the SDK/framework* (P4+P5). Proves the platform end-to-end
      and becomes the onboarding template. **This is the Era I exit gate.**

*Exit Era I: a real RP server you built on your own platform, multiplayer,
persistent, with a working economy — and the tools you used to build it exist.*

### ERA II — Productize the tools (turn your workflow into the SDK)

- [ ] **P8 — Creator SDK & tooling**: package the scripting API, the asset
      pipeline (`gtc_*` productized), a local creator test harness, project
      templates, and **docs/samples** (the flagship as the worked example).
- [ ] **P9 — Platform services**: **server browser/discovery**, managed
      **hosting**, content packaging/versioning/distribution (asset CDN), creator
      accounts.
- [ ] **P10 — Trust & safety v1** *(= RP7 + sandbox hardening)*: admin/mod tools,
      sandbox hardening, anti-cheat audit, provenance/IP enforcement on uploads.

*Exit Era II: a handful of external creators can stand up their own server with
their own content, hosted on the platform, without our help.*

### ERA III — Open platform & creator economy

- [ ] **P11 — Marketplace & creator economy** *(= currency C6 + platform
      currency)*: buy/sell creator content, **creator payouts/DevEx**, platform
      currency, fraud/KYC, **legal/ToS review**.
- [ ] **P12 — Scale & hosting** *(= RP10)*: NPC/crowd budgeting under netcode,
      per-world performance, autoscaling hosting, telemetry.
- [ ] **P13 — Launch & live ops** *(= RP11)*: open creator program, onboarding,
      moderation staffing, patch pipeline that never wipes persistence, seasonal
      events, community growth.

*Exit Era III: creators build and monetize worlds; the platform grows without
the core team authoring content.*

---

## 10. How <10 people actually pull this off (leverage)

The plan only works with ruthless leverage:

1. **Buy/license, don't build, the undifferentiated parts** — EOS (identity/
   sessions/voice/matchmaking), UE5 (engine), marketplace base assets, managed
   DB/hosting. Build only what's *yours*: the framework, SDK, currency, and
   marketplace.
2. **Generation over authoring** — lean on the `gtc_*` pipelines so content is
   *generated/imported*, never hand-modeled by the core team.
3. **AI-assisted development** — you already drive this repo via Claude + MCP;
   make that a first-class part of the workflow (and a creator feature — AI
   content tools lower the bar for creators too).
4. **Dogfood = free spec** — building the flagship server *is* how you discover
   what the SDK must expose. No separate "design the SDK" project.
5. **Community as workforce** — the entire point: creators are the content team
   you don't have to hire. The economy (P11) is how you pay them without payroll.
6. **Ruthless scope** — Era I ships *one* server, *one* economy, *three* roles.
   Resist breadth until the platform loop is proven.

---

## 11. MVP (the smallest thing worth shipping)

Not "a GTA." The MVP is: **one flagship CityBeachStrip RP server, multiplayer +
persistent + economy, built entirely on the platform, plus the SDK that proves a
second creator could build the next one.** That's the Era I → Era II boundary
(P7→P8). Everything else (marketplace, many creators, payouts) is earned *after*
that loop is real.

---

## 12. Risks & honest caveats

1. **Scope vs team size** — even FiveM/Roblox grew large teams over years. This
   is a multi-year arc; Era I alone is substantial. The mitigation is leverage
   (§10) and ruthless MVP scoping (§11), not heroics.
2. **Sandboxing untrusted creator code** — the hardest single technical problem;
   get it wrong and one creator script compromises the platform. Don't open to
   external creators (Era III) until P10 is solid.
3. **The chicken-and-egg of platforms** — no creators without content, no content
   without creators. Mitigation: *you* are creator #1 (P7); the flagship seeds
   the ecosystem and the economy (P11) pulls creators in.
4. **Engine-hard problems persist** — Chaos vehicle replication (P2) and Mass
   crowd scaling (P12) are still the netcode time-sinks.
5. **Legal surface** — UGC + real money + payouts + gambling = real regulatory
   exposure; counsel-gated before Era III money moves.
6. **Currency `int32` overflow** — widen to `int64` before any persistent
   economy ships (see `CURRENCY_PLAN.md` §4).

---

## 13. Document map

- **This file** — top-level platform strategy + master roadmap.
- [`GTA_RP_ROADMAP.md`](GTA_RP_ROADMAP.md) — detailed online/RP build order (RP0–RP11).
- [`CURRENCY_PLAN.md`](CURRENCY_PLAN.md) — currency/economy backbone (C0–C6).
- [`ASSET_PIPELINE.md`](ASSET_PIPELINE.md) / [`ASSETS.md`](ASSETS.md) / [`ASSET_HANDLING.md`](ASSET_HANDLING.md) — asset generation + provenance + third-party law.
- [`ARCHITECTURE.md`](ARCHITECTURE.md) / [`SYSTEMS.md`](SYSTEMS.md) — current code layering + what's wired.
- [`VISION.md`](VISION.md) — quality bar. [`ROADMAP.md`](ROADMAP.md) — single-player feature board.

> **Next concrete step:** Era I, **P0** — produce the integration-gap audit (every
> built system mapped to "wired into CityBeachStrip" vs "shelved in a branch"),
> which de-risks the entire substrate era.
```
