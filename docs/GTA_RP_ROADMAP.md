# GTA RP / Online — Complete Roadmap

> The path from **what we have today** (a largely single-player open-world
> sandbox) to a **shipped, persistent, cross-platform GTA-style roleplay (RP)
> online game** running on dedicated servers.
>
> This file is the *online/RP* forward board. It complements — does not replace —
> [`ROADMAP.md`](ROADMAP.md) (single-player feature board),
> [`BUILD_ORDER.md`](BUILD_ORDER.md) (dependency order), and
> [`SYSTEMS.md`](SYSTEMS.md) (what is actually wired). Checkboxes lag the code;
> a box is checked only when the feature is on `main`, passing CI, and verified
> on a live dedicated server with ≥2 connected clients.
>
> **Read this top to bottom once.** The milestones are dependency-ordered: each
> assumes the previous one is done. Skipping ahead (e.g. building jobs before
> replication is authoritative) is the main way RP projects collapse.

---

## 0. Where we are today (the starting line)

Grounded in the current repo, not aspiration:

**Strong (single-player, built):** third-person player on the soldier rig with
8-way locomotion + aim offsets + weapons (hold/fire/reload/switch); vehicle
enter/exit + Chaos driving; police + wanted + arrest + SWAT; MassGameplay crowds
with routines/voice/social/occupation/weather/traffic; minimap radar + phone +
HUD; economy storefronts (shop/payspray/modshop) + factions + progression +
skills + robbery; mission system + in-game mission editor; seamless streaming
(`FStreamingGrid`); save subsystem; day/night + weather; boot→menu→loading
front-end; character creator. Source is ~13 feature modules; CI exports
Linux/Windows/macOS build artifacts on tag.

**The gap to "Online RP":**

- ❌ **No networking.** No replicated properties, RPCs, or Online Subsystem.
  Everything was authored single-player and assumes one local authority.
- ❌ **No dedicated-server build.** CI builds clients; no headless server target.
- ❌ **No persistence backend.** `Save` is local-file; RP needs server-side,
  per-character, durable state.
- ❌ **No identity/accounts, lobbies, or matchmaking.**
- ❌ **No proximity voice, RP chat commands, roles/jobs framework, or admin
  tooling.**
- ⚠️ **Perf debt:** CityBeachStrip is heavy (Lumen HW-RT lag on Mac); crowd
  density and Cesium streaming must be budgeted *down* for netcode.
- ⚠️ **Build constraint:** current workflow can't reliably recompile C++ on Mac
  via the live editor — **all netcode is C++**, so a working compile + server
  build path is the literal first blocker.

---

## 1. The end state (definition of "done")

A player on **Mac, Windows, or Linux** launches the game, logs into an
**account**, picks/creates a **persistent character**, and connects through a
**server browser** to a **dedicated server** hosting CityBeachStrip. On that
server they:

- see and hear (spatial **proximity voice**) dozens of other real players;
- **live a role** — civilian, or a whitelisted job (police, EMS, mechanic,
  taxi, business owner, criminal/gang) — with on/off-duty, abilities, and wages;
- own **money, inventory, vehicles (garaged), and property**, all
  **server-authoritative and persistent** across sessions;
- interact through RP affordances: `/me` `/do` text, emotes, phone apps
  (banking, jobs, comms), businesses they can run, crimes the police respond to;
- play in a world that **survives server restarts** (DB-backed) and is
  **moderated** by admins with proper tooling, on a server that resists cheating
  because the **server is authoritative over all state**.

New RP content (a new job, a new business) can be added **without an engine
recompile**, via data assets or a scripting layer.

---

## 2. Target architecture

```
   Clients (Mac / Win / Linux)              Backend services
   ┌───────────────────────┐                ┌──────────────────────┐
   │ UE5 game client       │   EOS (auth,   │ Persistence API      │
   │  - prediction/interp  │   sessions,    │  - REST/gRPC         │
   │  - proximity voice    │   matchmaking, │  - validates server  │
   └──────────┬────────────┘   voice, P2P)  └─────────┬────────────┘
              │ replication / RPC                       │
   ┌──────────▼────────────┐                ┌──────────▼────────────┐
   │ UE5 DEDICATED SERVER  │  authoritative │ Postgres (durable):   │
   │ (headless, Linux)     │◄──────────────►│  accounts, characters,│
   │  - owns ALL gameplay  │   read/write   │  money, inventory,    │
   │    state & physics    │                │  property, vehicles   │
   │  - anti-cheat by      │                │ Redis (live/session): │
   │    construction       │                │  presence, locks,     │
   └───────────────────────┘                │  rate-limit, cache    │
                                            └───────────────────────┘
```

Principles: **server-authoritative everything** (clients send *intent*, never
state); **EOS** for identity/sessions/matchmaking/voice so we don't run that
backbone ourselves; **DB behind the game server**, never touched by clients;
**data/script-driven RP content** so the world grows without recompiles.

---

## 3. Milestones

Each milestone lists **Goal**, **Work**, and **Exit criteria** (the test that
says it's truly done). Effort tags: `S`=days, `M`=weeks, `L`=1–2 months,
`XL`=multi-month.

---

### RP0 — Consolidate & stabilize solo CityBeachStrip `L`

> De-risk everything downstream. There is no point replicating systems that
> aren't even integrated or stable single-player.

**Goal:** every system we've built is wired into CityBeachStrip and the map
boots reliably to a controllable character at a playable framerate.

**Work**
- [ ] Audit: map every built system to "wired into CityBeachStrip" vs "in an
      unmerged branch / shelved" (cross-check `SYSTEMS.md`). Produce the gap list.
- [ ] Merge/integrate the shelved-but-done systems onto `main` behind the
      shipping game mode.
- [ ] Perf pass: fix Lumen HW-RT lag (viewport scalability + screen percentage,
      per existing findings), Nanite/translucent offenders, crowd density caps,
      streaming budget. Establish a frame-budget target and measure it.
- [ ] Single boot path: front-end → CityBeachStrip → controllable character,
      every time, no manual fixups.
- [ ] Green CI: format + lint + UnrealBuildTool + `GTC.*` automation tests.

**Exit:** CityBeachStrip boots from the front-end to a playable character with
all integrated systems active, holds the frame budget on the dev machine, and
`main` is green. **No networking yet.**

---

### RP1 — Build & server foundation `M`

> The blocker milestone. Nothing online is possible until C++ compiles
> reliably, a headless server exists, and EOS is in.

**Goal:** a headless **Linux dedicated server** runs CityBeachStrip and clients
on any OS can connect to it.

**Work**
- [ ] Fix the **C++ compile path** (UnrealBuildTool works for the team; the
      live shared editor stays untouched — compile out-of-editor).
- [ ] Add the **dedicated server target** (`GTC_UE5Server.Target.cs`), headless,
      no rendering.
- [ ] CI: cross-compile / Linux build agent producing a **server build** + the
      three **client builds** (Mac/Win/Linux) on every tag.
- [ ] Integrate **EOS** via the Online Subsystem (identity, sessions; voice/
      matchmaking come later) + EOS dev portal product config.
- [ ] Add `OnlineSubsystem*`, networking, and EOS modules to `Build.cs`.
- [ ] Stand up the simplest possible **GameMode/GameState/PlayerState** for a
      networked session (replacing/forking the single-player game mode).

**Exit:** a Linux dedicated server boots CityBeachStrip headless; a Mac client
and a Windows/Linux client both connect to it and spawn pawns. (They won't see
each other move correctly yet — that's RP2.)

---

### RP2 — Core replication vertical slice `L`

> Prove the hardest engine-level netcode on the *minimum* surface before
> touching 20 systems.

**Goal:** multiple players see each other move, drive, and the world feels
smooth under real latency.

**Work**
- [ ] Replicate **player movement + camera** with client prediction + server
      reconciliation + interpolation for remotes (UE `CharacterMovementComponent`
      networking, or Mover plugin path — pick one and commit).
- [ ] Replicate **one vehicle** end-to-end. ⚠️ **Chaos vehicle replication is
      the single hardest engine task here** — server-authoritative physics with
      client-side smoothing; budget accordingly.
- [ ] Replicate enter/exit handoff (attach/possession across the network).
- [ ] Relevancy/`NetCullDistance` + update-frequency tuning; a **netstat/lag
      simulation** test harness (`GTC.Net.*`).
- [ ] Decide listen-server-for-dev vs always-dedicated (recommend: always test
      against the dedicated build).

**Exit:** 2+ players on a dedicated server run around and drive together with
acceptable jitter/latency; no authority desync on movement or vehicles.

---

### RP3 — Persistence backend & identity `L`

> The heart of RP: state that *persists*. Without this it's just deathmatch.

**Goal:** accounts and per-character durable state survive logout and server
restart.

**Work**
- [ ] **Persistence API service** (REST/gRPC) — the only thing that talks to the
      DB; the game *server* calls it, clients never do.
- [ ] **Postgres** schema: accounts, characters (appearance/identity), money
      (cash + bank), inventory, position, owned vehicles, property. **Redis** for
      presence, distributed locks, rate-limits, hot cache.
- [ ] Map **EOS identity** → account; character select/create flow using the
      existing **character creator** as the identity front-end.
- [ ] Server-side **load on join / autosave / save on leave**; conflict rules
      (single active session per character; money mutations transactional).
- [ ] Migrate the local `Save` concepts to server-authoritative records.

**Exit:** a player creates a character, earns/spends money, logs out, the server
restarts, they log back in (possibly on another machine/OS), and **all state is
restored** from the backend.

---

### RP4 — Authoritative combat, police & economy `L`

> Move the existing gameplay systems server-side. Anything that grants money,
> damage, or wanted level must be server-decided or it will be cheated.

**Goal:** combat, death/respawn, the wanted system, and the money economy are
all server-authoritative and consistent for many players.

**Work**
- [ ] Replicate **health/damage/death/respawn** (server validates all hits;
      hitscan/projectile reconciliation; lag compensation).
- [ ] Make the **wanted/notoriety/arrest** systems shared & server-owned; AI
      police as networked actors (server-spawned, replicated).
- [ ] Make **economy** (cash, bank, storefront transactions, payspray, robbery,
      loot) server-authoritative and transactional against the DB.
- [ ] Replicate **weapon state** (equip, ammo, reload) authoritatively.
- [ ] Anti-cheat baseline: clients send intent only; reject impossible inputs;
      server-side rate limits on money/inventory mutations.

**Exit:** multi-player shootouts, deaths, wanted escalation, and purchases all
resolve identically for everyone, with no client able to grant itself
money/ammo/invulnerability.

---

### RP5 — Communication layer (voice, RP chat, emotes) `M`

> The feature players judge an RP server by. Spatial voice is non-negotiable.

**Goal:** players communicate as their characters.

**Work**
- [ ] **Proximity voice chat** (EOS Voice, or ODIN/Vivox): distance-attenuated,
      directional; **radio channels** for factions/jobs; mute/volume controls.
  - *Note:* existing `FNpcVoice`/`FVoiceSynth` is synthesized NPC speech — real
    player voice is a separate system layered in here.
- [ ] **RP text commands**: `/me`, `/do`, `/ooc`, local vs global chat scoping.
- [ ] Networked **emotes / animations** (reuse the radial emote wheel).
- [ ] Nameplates / speaking indicators with RP-appropriate visibility rules.

**Exit:** two players hear each other fade in/out with distance, use radio on a
shared channel, and run `/me`/emotes that everyone nearby sees.

---

### RP6 — Roles & jobs framework `L`

> The structure that turns "multiplayer sandbox" into "roleplay".

**Goal:** a reusable role/job system, proven with the first real jobs.

**Work**
- [ ] **Role framework**: definition (abilities, whitelist, duty state, wage),
      on/off-duty toggle, job HUD, paycheck loop (server-authoritative wages).
- [ ] **Whitelisting/permissions** tied to account + admin grants.
- [ ] First roles, reusing existing systems:
  - **Police** (the wanted/arrest system becomes a *playable* faction; AI cops
    become backup/fill).
  - **EMS/Paramedic** (revive/treat downed players — pairs with RP4 death).
  - **Mechanic** (repair/modify vehicles — pairs with the modshop economy).
  - **Civilian** (default, no whitelist).

**Exit:** a player goes on duty as police/EMS/mechanic, performs the job loop,
and receives server-paid wages; civilians interact with them meaningfully.

---

### RP7 — Admin, moderation & anti-cheat hardening `M`

> You cannot run a public RP server with strangers without this.

**Goal:** a server can be operated and policed safely.

**Work**
- [ ] Admin tooling: kick/ban/mute/teleport/spectate/freeze, role grants, item/
      money admin (audited).
- [ ] Server-side **logging & audit trail** (money, items, kills, admin actions)
      to the backend.
- [ ] Anti-cheat hardening pass: authority audit across all systems, sanity
      checks, rate limits, anomaly logging; report flow.
- [ ] Graceful disconnect/reconnect, AFK handling, session limits.

**Exit:** an admin can join, moderate a misbehaving stranger end-to-end, and
every state-changing action is logged and reversible.

---

### RP8 — Extensibility / content layer `L`

> So the world can grow without an engine recompile — the FiveM lesson.

**Goal:** new jobs, businesses, items, and events are **content/data/script**,
not C++.

**Work**
- [ ] Express roles, businesses, items, and shop catalogs as **DataAssets +
      GameplayTags** (data-driven), and/or
- [ ] Embed a **scripting layer** (Lua or AngelScript via UE plugin) with a
      safe, sandboxed RP API surface for server-side content.
- [ ] Content hot-load / versioning; a content authoring guide in `docs/`.

**Exit:** a designer adds a new job and a new business and deploys them to the
server **without recompiling the engine**.

---

### RP9 — RP world systems & content loop `XL`

> The actual game players show up for. Built on top of all the plumbing above.

**Goal:** a full RP day-loop exists.

**Work**
- [ ] **Inventory & items** (server-authoritative): carry/drop/give/store,
      weight/slots, item definitions (data-driven from RP8).
- [ ] **Vehicle ownership & garages** (persistent, per-character).
- [ ] **Property/housing** (purchase, enter — pairs with enterable-buildings
      work — storage, persistence).
- [ ] **Businesses** players own/operate (storefronts → player-run).
- [ ] **Gangs/factions** (the faction system goes multiplayer + territory).
- [ ] **Phone apps**: banking, job board, services, contacts (reuse phone UI).
- [ ] Crime/economy loop tuned for many players (heists, robbery, payouts).
- [ ] Ambient **NPC crowds** dialed to coexist with players (density budget).

**Exit:** a player can log in, do a job, earn money, buy a vehicle and property,
join a faction, and interact with other players' businesses — a coherent loop.

---

### RP10 — Scale, hosting & matchmaking `L`

> From "works for a handful" to "stable for a server full of players".

**Goal:** hit the target concurrent-player count per server, reliably hosted.

**Work**
- [ ] **NPC crowd replication scaling** — the hardest scaling problem: server-
      driven Mass with strict relevancy/LOD, or simplified networked crowds.
      Decide and measure.
- [ ] Server **performance & tick budgeting** under full player load; profiling.
- [ ] **Hosting**: containerized dedicated servers (cloud), restart/backup
      automation, DB backups & migrations, monitoring/alerting.
- [ ] **Server browser / matchmaking** (EOS sessions) + player counts, regions,
      ping.
- [ ] Telemetry & live dashboards (player count, perf, errors).

**Exit:** a hosted server holds the target player count at a stable server
tick/frame budget, discoverable via the server browser, with backups and
monitoring in place.

---

### RP11 — Launch & live ops `XL` (ongoing)

> Ship it, then keep it alive.

**Goal:** public launch and a sustainable live-ops cadence.

**Work**
- [ ] Onboarding/tutorial for new RP players; rules/whitelist application flow.
- [ ] Patch/deploy pipeline for client + server + content without wiping
      persistence; schema migration discipline.
- [ ] Community/mod tooling (if opening content to others).
- [ ] Optional monetization (cosmetics only — never pay-to-win) and the legal/
      ToS/privacy that persistence + accounts require.
- [ ] Live-ops loop: events, seasonal content, balance, moderation staffing.

**Exit:** public servers running, players retained, a repeatable weekly content/
patch cadence.

---

## 4. Cross-cutting tracks (run in parallel the whole way)

- **Performance budget** — every networked system has a server-tick and
  bandwidth cost; measure per milestone, don't let it accumulate.
- **Security / authority** — re-audit "is this server-authoritative?" at the end
  of RP2, RP4, RP6, RP9. The default assumption for any new feature is *cheatable
  until proven server-owned*.
- **Testing** — keep `GTC.*` automation green; add **multiplayer functional
  tests** (`GTC.Net.*`) and lag-simulated tests from RP2 onward.
- **CI/CD** — server build + 3 client builds on every tag; backend services have
  their own pipeline + DB migration checks.
- **Asset/legal provenance** — unchanged rules from `ASSETS.md`; no
  commercial-game assets/names/layouts; accounts/persistence add privacy/ToS
  obligations at RP11.

---

## 5. Dependency order (the critical path)

```
RP0 ─ consolidate/stabilize (solo)
 └─► RP1 ─ build + dedicated server + EOS        ← hard blocker
      └─► RP2 ─ core replication (move/drive)
           └─► RP3 ─ persistence + identity
                ├─► RP4 ─ authoritative combat/police/economy
                │    └─► RP6 ─ roles/jobs ──┐
                ├─► RP5 ─ voice/chat/emotes  ├─► RP8 ─ extensibility
                └────────────────────────────┘     └─► RP9 ─ RP content loop
                                   RP7 ─ admin/anti-cheat (start ASAP, harden by RP9)
                                        └─► RP10 ─ scale/hosting ─► RP11 ─ launch
```

RP5 (voice) and RP7 (admin) can progress alongside the RP4→RP6 line. Everything
downstream of RP1 is gated on the build/server foundation — **fund that first.**

---

## 6. Top risks (where RP projects die)

1. **Chaos vehicle replication** (RP2) — engine-hard; smooth networked driving
   is a milestone in itself.
2. **NPC/Mass crowd replication at scale** (RP10) — the #1 scaling killer;
   expect to cut crowd density hard on RP servers.
3. **Persistence consistency** (RP3) — money/item duping bugs from races; needs
   transactional discipline and locks from day one.
4. **Build/server pipeline complexity** (RP1) — cross-OS client + Linux server +
   backend services is a lot of DevOps; underestimating it stalls everything.
5. **Anti-cheat** (cross-cutting) — retrofitting authority is far costlier than
   building it in; treat every RP-money/item/damage feature as server-owned from
   the first line.
6. **Scope** — full GTA-RP frameworks represent years of community work; ship a
   *small* role set (RP6) and a *thin* content loop (RP9) first, then expand.
```
