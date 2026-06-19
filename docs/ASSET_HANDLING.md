# Asset handling — standing rules

How third-party art assets enter and are used in this repo. **Config/docs policy — read before
referencing, importing, or onboarding to any asset set.** These rules are binding on every contributor
and on any AI agent operating in this repo.

## Where assets live

All third-party art assets live **only** in the private submodule:

| | |
|---|---|
| **Submodule repo** | `duolahypercho/GT-Caliber-Asset` (private) |
| **Mount path** | `Content/GTCaliberAssets/` (a tracked mode-160000 **gitlink** — a pointer, never blobs) |
| **UE reference path** | `/Game/GTCaliberAssets/Content/...` (the asset repo carries its own `Content/`, hence the double-`Content` nesting — intentional, lowest-friction; see the migration notes) |
| **Transport** | Git LFS (`*.uasset`, `*.umap`); ~15 GB on disk when smudged |

The raw asset files are **never committed to the project repo.** The parent repo tracks only the
gitlink pointer. This is enforced by two layers:

1. **Submodule boundary** — while the submodule is initialized, git refuses parent-repo operations
   inside `Content/GTCaliberAssets/` (`fatal: ... is in submodule`), so you cannot `git add` an asset
   into the parent.
2. **`.gitignore` defense-in-depth** — `/Content/GTCaliberAssets/**` blocks stray *loose* copies from
   being staged into the parent if the submodule is ever deinitialized/absent (when the path is just a
   plain directory). The rule matches contents only, so the gitlink itself stays tracked.

The repo-isolation guard (run before every push) remains the final backstop.

## Reference resolution: CoreRedirects (required for /Game-rooted packs)

Marketplace packs reference their own assets by absolute `/Game/<Pack>/...` paths (the layout you get by
dropping a pack into the project Content root). Because this project mounts the asset submodule one level
deep at `/Game/GTCaliberAssets/Content/...`, those internal references do not resolve as-is. The fix is a
package-path redirect per pack root, in `Config/DefaultEngine.ini`:

```ini
[CoreRedirects]
+PackageRedirects=(OldName="/Game/CityBeachStrip/",NewName="/Game/GTCaliberAssets/Content/CityBeachStrip/",MatchSubstring=true)
+PackageRedirects=(OldName="/Game/Characters/",NewName="/Game/GTCaliberAssets/Content/Characters/",MatchSubstring=true)
```

Verified 2026-06-17: with these two rules the CityBeachStrip map resolves all 1,109 dependencies (0
missing, was 1,104 missing) and loads ~55k actors with geometry, lighting, and level-instances intact;
the Mannequin resolves its materials and skeleton. CoreRedirects load at engine startup, so editing them
requires an editor restart.

**Recurring cost (accepted):** every future `/Game/`-rooted marketplace pack added to the submodule
needs ONE more redirect line mapping `/Game/<PackRoot>/` to `/Game/GTCaliberAssets/Content/<PackRoot>/`.
That is the trade for not relocating the submodule to the project Content root.

## Provenance — multi-source (log per asset set, auditable per actor entry)

The asset repo is **not single-source.** Each set keeps its own source + license. Mirror the
`path / source / license` discipline of the old Godot `docs/ASSETS.md`, and update this table whenever a
new asset set is introduced or first referenced by an actor entry.

| Path (under `Content/GTCaliberAssets/Content/`) | Source | License | Notes |
|---|---|---|---|
| `CityBeachStrip/` (+ its `__ExternalActors__/`) | **Fab — PropHaus** | **Standard License** | the ~15 GB paid Beach City pack; the bulk of the assets |
| `Characters/` (Mannequins + Pistol/Rifle/Death/HitReact anim sets) | **Epic Games** | **UE EULA** ("Epic Content") | **current placeholder** character set; finals (MetaHuman/Fab) TBD |
| `ThirdPerson/` | **Epic Games** | **UE EULA** | Third Person template content |
| `LevelPrototyping/` | **Epic Games** | **UE EULA** | gray-box template prototyping assets |
| `Input/` | **Epic Games** | **UE EULA** | template Enhanced Input assets |
| `CesiumSettings/` | **Cesium for Unreal** | per Cesium plugin terms | geospatial config |
| `Characters/Mannequins/Anims/Emotes/` (`MM_Wave`, `MM_MiddleFinger`, `MM_Piss`) | **project-authored** (hand-keyed on the Manny skeleton in Blender, 2026-06-19) | **CC0** | original player emotes (wave / middle-finger / piss); full-body one-shots played on the ABP `DefaultSlot` |
| `Characters/Mannequins/Anims/Locomotion/` (`MM_Crouch_Idle`, `MM_Crouch_Walk`, `MM_Swim_Idle`, `MM_Swim_Fwd`) | **Quaternius — Universal Animation Library**, retargeted to `SK_Mannequin` | **CC0 1.0** | crouch + swim, from `opengameart.org/content/universal-animation-library`; Quaternius rig → Manny via rotation-delta retarget (2026-06-19) |

License obligations differ per row — honor each set's own terms at ship (Fab Standard License for
PropHaus, UE EULA for Epic content, Cesium terms for Cesium). The "no AI ingestion" rule below applies
**uniformly regardless of source.**

## Hard line — never feed asset contents into any AI model

Asset **file contents** (mesh, texture, material, animation, `.uasset`/`.umap` binaries) must **never**
be used as input to any AI model. Specifically prohibited:

- reading, opening, `cat`-ing, parsing, or decoding an asset file's bytes;
- describing, summarizing, or transcribing what an asset depicts from its contents;
- transforming an asset, generating variations of it, or using it as a style/training reference;
- training, fine-tuning, or conditioning any model on asset files.

**Allowed** (this is normal game development): referencing an asset by its **path** in a spawn call, a
`DataTable` row, a Blueprint, a material reference, or config. Shipping the asset in the game used the
normal way (loaded by code/Blueprints, rendered, spawned) is fine. The line is: **referencing a path is
fine; ingesting the contents is not.**

### Agent rules (binding on AI agents in this repo)
- The agent **references asset paths only.** It does **not** download, clone, copy, open, or read asset
  file contents. Jake/contributors bring assets onto the machine via the submodule; the agent only
  writes path references (spawns, DataTable rows, Blueprint wiring, config).
- The agent may read/modify **config and docs** (this file, `.gitignore`, `.gitmodules`, provenance
  rows) and may inspect repo **metadata** (file names, sizes, gitlink status) — never blob contents.

## Onboarding — getting the assets on a new machine

Each developer needs:

1. **Read access** to the private `duolahypercho/GT-Caliber-Asset` repo (without it, the submodule fetch
   403s).
2. **git-lfs installed** (`git lfs install`, once per machine).
3. **Clone with submodules:** `git clone --recurse-submodules <repo-url>` — or after a plain clone,
   `git submodule update --init --recursive`.
4. **Pull LFS objects:** `cd Content/GTCaliberAssets && git lfs pull` if any pointer stubs remain.

⚠️ The LFS pull is **~15 GB** and draws on **duolahypercho's** GitHub LFS bandwidth/storage quota.

## Public-repo note (for the README / open-source prep)

Once this repo is published, external cloners receive the **gitlink** but **cannot fetch the assets**
without access to the private `GT-Caliber-Asset` repo — submodule init will fail for them. This is the
**intended protection** (paid/licensed assets never become public). Consequence: the public repo is
**not runnable as-is** without asset-repo access. State this plainly in the README so external users
aren't confused — the code is open; the art is licensed and gated.
