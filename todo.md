# TODO — GT-caliber_5.8

Working scratch list of deferred work. The canonical task list is `docs/ROADMAP.md`;
this is the lightweight "later on" pile from recent sessions. Boot-chain / base
rules live in `CLAUDE.md` ("The canonical base").

## Now / verify
- [ ] **Re-test car entry in PIE** — confirm "Can't cast to TPS Kit Custom Anim BP!"
      spam is gone (fixed 2026-06-28 by deleting the PrintText in the
      `CastAnimBPFailMessage` local macro on `BP_PlayerCharacterBase`). Needs a live
      drive test; couldn't verify headlessly.
- [ ] **Re-test minimap while driving** — confirm the "Accessed None …
      CallFunc_GetController_ReturnValue" error in `WB_Game_HUD_widget` →
      `GTC_UpdateMapStats` is gone (fixed 2026-06-28: minimap rotation now reads
      `GetControlRotation(GetOwningPlayer())` instead of `GetController(playerPawn)`,
      which was None whenever the pawn was unpossessed, e.g. while driving).
- [ ] **Audit other HUD/UI nodes for the same pattern** — any widget logic that does
      `playerPawn → GetController` will break while driving. Prefer
      `GetOwningPlayer()` / `GetOwningPlayerPawn()` in widgets.
- [ ] **Sweep the anim BP `MM_*` getters for None-ref reads** —
      `ABP_TPSKit_InstanceCustomPlayer` has several functions that read the cached
      `PlayerCharacterReference` and call `BPPlayerCharacterBase::GetCurrent*` on it.
      During init / the driver-mesh swap the ref is briefly None → "Accessed None …
      PlayerCharacterReference". `GTC_EnsureCharacterRef` (BeginPlay + Update) is
      correctly sticky but the AnimGraph can evaluate these getters before the ref is
      (re)acquired. Fixed `MM_WeaponChangeGunBoneOffset` 2026-06-28 (added an IsValid
      guard → returns zero vector when invalid). Root mitigation 2026-06-28: also added a
      `GTC_EnsureCharacterRef` call to `EventBlueprintInitializeAnimation` (first node),
      so the ref is re-acquired on the driver-mesh swap before the `MM_*` getters run —
      covers vehicle enter/exit for the whole class. Only guard remaining individual
      getters if specific ones still spam after testing; the bounded startup-streaming
      burst (pawn not yet possessed) is harmless.
      RESOLVED 2026-06-28: whole-log grep showed ONLY two functions ever flooded the
      ref on the player side — `MotionMatchingPelvisOffsetValue` and
      `MM_WeaponChangeGunBoneOffset` — both now guarded (0 errors after recompile). No
      full sweep needed. They were firing with PIE OFF = the AnimBP editor PREVIEW
      (owner-less mesh, no pawn). In real gameplay the ref is set normally. Always
      grep the log + check is_in_play_in_editor() before assuming a big sweep.

## Animation polish
- [ ] **Author a `WeaponEquippingCoverLeft` montage** (optional) — fixing the
      duplicate-SLOTNODE warning 2026-06-28 renamed the left-facing cover branch's
      slot from `WeaponEquippingCoverRight` to `WeaponEquippingCoverLeft` in both
      player anim BPs (`ABP_MannequinPlayer_UE5Manny`, `ABP_MannequinPlayer`). Right
      facing still plays the right equip montage; left facing is now a pass-through
      until a left-facing equip montage is authored and routed to that slot. Purely
      cosmetic for the cover system; only relevant if cover weapon-equip is used.

## Player locomotion — the real fix + foundation for new modes
- [ ] **Add a locomotion-state system on `BP_PlayerCharacterBase`** — an enum
      (`OnFoot` / `Driving` / `Swimming` / `Bike` / `Motorcycle`) tracked as the
      single source of truth for "what is the player doing".
- [ ] **Gate the ~70 `AnimBP_Set*` calls on `OnFoot`** — root cause of the cast
      spam was these running every tick while the driver rig is active. Gating them
      off-foot is the proper fix (the macro neuter just silences the symptom).
- [ ] **Possession-based mode switching, NOT new game modes** — keep the one
      canonical `BP_GTCShooterGameMode` + persistent `BP_TPS_PlayerController`;
      switch pawns/possession per mode. (Decision already made; documented here so
      we don't spin up parallel game modes later.)

## New locomotion modes (after the state system lands)
- [ ] **Bike** — pawn + enter/exit + anim set.
- [ ] **Motorcycle** — pawn + enter/exit + lean/anim set.
- [ ] **Swimming** — `CharacterMovementComponent` `MOVE_Swimming` (NOT a vehicle);
      water-volume entry/exit + swim anim set (retarget existing clips, never author).

## Quality / perf pass (the /loop goal: surpass GTA6)
- [ ] Continue the cinematic grade → materials → lighting/atmosphere → performance
      loop via Unreal MCP on the live editor (one incremental change at a time,
      before/after screenshot, check editor log for errors).
- [ ] **Bake wins into an actual Blueprint** and apply them in the **CityBeachStrip**
      map (per the loop directive — make the improvements persistent, not just
      session-only console tweaks).

## Cleanup (low priority)
- [ ] **Charref spam** — "Accessed None ... PlayerCharacterReference" anim-tick noise
      from `ABP_TPSKit_InstanceCustomPlayer`. Separate from the cast spam. Fix via
      `GTC_EnsureCharacterRef` re-cast (see memory `gtc-tpskit-animbp-charref-spam`).
- [ ] The earlier one-off severed Cast Failed branch on
      `AnimBP_SetWeaponHandRotationCorrectionEnabled` is now redundant (the macro
      neuter covers it). Harmless; leave unless it conflicts with the state-gate work.
