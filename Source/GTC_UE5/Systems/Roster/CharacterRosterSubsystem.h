// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CharacterRoster.h"
#include "CharacterRosterSubsystem.generated.h"

/**
 * UCharacterRosterSubsystem — UE 5.7 port of Godot's self-wiring `character_switcher.gd`
 * (class CharacterSwitcher, a Node). Owns one FCharacterRoster and coordinates
 * dual-protagonist switching: park the outgoing lead, flip the active character, resume
 * the incoming lead.
 *
 * Why a GameInstanceSubsystem: the Godot CharacterSwitcher is a persistent self-wiring
 * Node that holds the roster for the whole session and survives streamed sublevels; the
 * dual-protagonist roster is a player-global, save-persisted thing with that lifetime. A
 * GameInstanceSubsystem matches. (If a future design needs per-world reset semantics this
 * can move to a WorldSubsystem — the one lifecycle choice flagged here.)
 *
 * OWNERSHIP MODEL: the subsystem OWNS its FCharacterRoster and exposes it only via a const
 * getter plus driver methods (RequestSwitch / money / position / wanted accessors). It owns
 * its OWN state and does NOT depend on any other Wave-2 sibling type (PlayerStats, Wanted,
 * Health, armor) — this branch is fully decoupled.
 *
 * ----------------------------------------------------------------------------------
 * DEFERRED-OWNERSHIP (Wave-3 adapter — documented, NOT implemented or depended on here;
 * flag for lead sign-off):
 *
 * The Godot CharacterSwitcher.request_switch wires the live PlayerStats wallet through each
 * switch (write the active lead's money back from PlayerStats, flip, load the incoming
 * lead's money into PlayerStats — group "player_stats"). Position/wanted sync follow the
 * same shape and were left to the scene controller in Godot.
 *
 * Here that runtime PlayerStats/Wanted wiring is DEFERRED to Wave 3. At W3 the switch will
 * SNAPSHOT/RESTORE the canonical runtime state against:
 *   - `UPlayerStatsComponent` for money AND armor — UPlayerStatsComponent is the SOLE armor
 *     owner (resolved 2026-06-16, commit f0c0c36 / docs/W3_WIRING_NOTES.md; the health
 *     component owns health only). W3 reads/writes money+armor through it.
 *   - `UWantedSubsystem` for the wanted level.
 * Until then, RequestSwitch drives only the owned roster's neutral snapshot blobs (money,
 * position, wanted) — no live component is touched. OnCharacterSwitched mirrors the Godot
 * `character_switched(id)` signal.
 * ----------------------------------------------------------------------------------
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterSwitched, const FString&, Id);

UCLASS()
class GTC_UE5_API UCharacterRosterSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** Fires when the active lead changes (Godot character_switched). */
    UPROPERTY(BlueprintAssignable, Category = "Roster")
    FOnCharacterSwitched OnCharacterSwitched;

    // UGameInstanceSubsystem lifecycle.
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Const access to the owned roster (ownership stays with the subsystem). */
    const FCharacterRoster& GetRoster() const { return Roster; }

    // --- Queries (delegate to the owned roster) ----------------------------

    int32 CharacterCount() const { return Roster.CharacterCount(); }
    const FString& ActiveId() const { return Roster.Active(); }
    FString ActiveName() const { return Roster.ActiveName(); }
    int32 MoneyOf(const FString& Id) const { return Roster.MoneyOf(Id); }

    // --- Drivers (ported from the CharacterSwitcher Node) ------------------

    /**
     * Switch the active lead. Returns false if the owned roster refuses (unknown id,
     * already active, on cooldown). Broadcasts OnCharacterSwitched on success. Mirrors
     * Godot request_switch MINUS the live PlayerStats wallet sync, which is the Wave-3
     * adapter (see header). `Now` defaults to far-future so an untimed caller can switch.
     */
    bool RequestSwitch(const FString& Id, double Now = TNumericLimits<double>::Max());

    /** Credit/spend a character's wallet (floored at 0). Drives the owned roster snapshot. */
    void AddMoney(const FString& Id, int32 Amount) { Roster.AddMoney(Id, Amount); }

private:
    /** The owned roster. Constructed with DefaultCharacters() on Initialize. */
    FCharacterRoster Roster;
};
