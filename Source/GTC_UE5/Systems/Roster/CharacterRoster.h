// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure dual-protagonist roster — UE 5.7 port of the reference `character_roster.gd` (class
 * CharacterRoster, a RefCounted). The modern open-world "switch between playable leads"
 * mechanic: each character keeps INDEPENDENT persistent state (wallet, wanted level,
 * world position); switching parks the current lead where they are and resumes the
 * other exactly where you left them, after a short cooldown.
 *
 * Plain C++ value type (no UObject), headless-testable against the reference behavior
 * game/tests/unit/test_character_roster.gd (14 assertions). No nodes, no scene access.
 *
 * Each character is {Id, Name, Money}; Wanted/Position start neutral and accrue in play.
 * Malformed entries (missing/empty id) are dropped at construction; a duplicate id keeps
 * the first registration.
 *
 * ----------------------------------------------------------------------------------
 * DEFERRED-OWNERSHIP (option-1 own-state — REQUIRED to document, flag for lead sign-off):
 *
 * This roster OWNS per-character SNAPSHOT BLOBS — money, position, and wanted stars — as
 * PLAIN VALUES. It does NOT include, reference, or depend on any PlayerStats / Wanted /
 * armor UE type. The snapshot is self-contained, exactly as the the reference oracle exercises it
 * over plain {id, name, money} dicts plus neutral position/wanted.
 *
 * At Wave 3, a live character switch will SNAPSHOT/RESTORE the canonical runtime state:
 *   - money AND armor from the SOLE armor owner `UPlayerStatsComponent`
 *     (armor ownership resolved 2026-06-16, commit f0c0c36 / docs/W3_WIRING_NOTES.md:
 *     UPlayerStatsComponent is the sole armor owner; UPlayerHealthComponent owns health
 *     only). The W3 switch reads/writes money+armor through UPlayerStatsComponent.
 *   - the wanted LEVEL from `UWantedSubsystem`.
 * That runtime wiring is DEFERRED to Wave 3 and is intentionally NOT modelled here; this
 * roster's `Money`/`Position`/`Wanted` fields are the neutral, decoupled placeholders the
 * W3 adapter snapshots into/out of. Flagged for lead sign-off.
 * ----------------------------------------------------------------------------------
 */
struct GTC_UE5_API FRosterCharacter
{
    FString Id;
    FString Name;
    int32 Money = 0;

    FRosterCharacter() = default;
    FRosterCharacter(const FString& InId, const FString& InName, int32 InMoney)
        : Id(InId)
        , Name(InName)
        , Money(InMoney)
    {
    }
};

class GTC_UE5_API FCharacterRoster
{
public:
    /** Minimum seconds between switches (anti-spam, matches the genre's switch wheel). */
    static constexpr double SwitchCooldown = 3.0;
    /** Wanted stars are clamped to this ceiling. */
    static constexpr int32 MaxStars = 5;

    /** Construct from a roster; an empty list uses DefaultCharacters(). First valid id is active. */
    explicit FCharacterRoster(const TArray<FRosterCharacter>& Characters = TArray<FRosterCharacter>());

    /** The built-in two leads (the project's Mara plus an original second protagonist). */
    static TArray<FRosterCharacter> DefaultCharacters();

    int32 CharacterCount() const;
    bool HasCharacter(const FString& Id) const;

    /** Ids in insertion order. */
    TArray<FString> Ids() const;

    /** The active character's id ("" if the roster is empty). */
    const FString& Active() const;
    FString ActiveName() const;

    /** Display name of a character, or "" if unknown. */
    FString NameOf(const FString& Id) const;

    /**
     * Whether a switch to `Id` is allowed at time `Now`: a known, non-active character,
     * and the cooldown since the last switch has elapsed.
     */
    bool CanSwitch(const FString& Id, double Now) const;

    /**
     * Switch the active lead. Returns false if the switch isn't allowed (unknown id,
     * already active, or still cooling down). `Now` defaults to far-future so a caller
     * that doesn't track time can always switch.
     */
    bool SwitchTo(const FString& Id, double Now = TNumericLimits<double>::Max());

    /** A character's wallet (0 if unknown). */
    int32 MoneyOf(const FString& Id) const;

    /** Add to (negative spends from) a character's wallet, floored at 0. No-op for unknown id. */
    void AddMoney(const FString& Id, int32 Amount);

    void SetMoney(const FString& Id, int32 Amount);

    /** A character's wanted stars (0 if unknown). */
    int32 WantedOf(const FString& Id) const;

    void SetWanted(const FString& Id, int32 Stars);

    /** A character's last world position (FVector::ZeroVector if unknown). */
    FVector PositionOf(const FString& Id) const;

    void SetPosition(const FString& Id, const FVector& Pos);

    /** Per-character snapshot blob: the state a Wave-3 switch parks/resumes (money/wanted/position). */
    struct FSnapshot
    {
        FString Name;
        int32 Money = 0;
        int32 Wanted = 0;
        FVector Position = FVector::ZeroVector;
    };

    /** Flatten to {Active, Characters} for the save system. Insertion-ordered. */
    void Serialize(FString& OutActive, TArray<TPair<FString, FSnapshot>>& OutCharacters) const;

    /**
     * Restore from Serialize() output. Unknown/malformed entries are ignored; known
     * characters are updated in place; Active is only adopted if it names a known character.
     */
    void Deserialize(const FString& InActive, const TArray<TPair<FString, FSnapshot>>& InCharacters);

private:
    /** Live per-character state. */
    struct FState
    {
        FString Name;
        int32 Money = 0;
        int32 Wanted = 0;
        FVector Position = FVector::ZeroVector;
    };

    /** Insertion-ordered ids (the reference Dictionary key order is observable: ids()/roster order). */
    TArray<FString> Order;
    /** Id -> live state. */
    TMap<FString, FState> Characters;
    FString ActiveId;
    /** Last switch timestamp; -inf so the first timed switch is never cooldown-blocked. */
    double LastSwitchAt = -TNumericLimits<double>::Max();

    void Register(const FRosterCharacter& Entry);
    FState* Find(const FString& Id);
    const FState* Find(const FString& Id) const;
};
