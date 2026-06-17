// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"

/**
 * Pure model for assembling a heist crew (driver, hacker, gunman, ...). Plain C++
 * value type (no UObject): a mission controller owns one, feeds it members, then
 * queries odds and payouts. Headless-testable (parity oracle test_heist_crew.gd).
 *
 * Each member takes a cut of the take; the player keeps the remainder. A more skilled
 * crew raises the heist's success chance against a base difficulty.
 *
 * RNG parity note: Attempt() rolls through FRandomStream — deterministic and
 * seed-reproducible WITHIN UE5, NOT byte-identical to Godot. The oracle only pins
 * determinism (same seed -> same result) and certain/hopeless boundaries (chance
 * 1.0 / 0.0), never an exact seed->value sequence, so FRandomStream satisfies it.
 * Mapping: randf() -> GetFraction().
 *
 * Members are stored insertion-ordered (Roles() order is observable).
 */
class GTC_UE5_API HeistCrew
{
public:
    /** Crew skill swing and floor penalty: success = base - PENALTY + skill*SWING. */
    static constexpr double SkillSwing = 0.6;
    static constexpr double SkillFloorPenalty = 0.25;

    struct FMember
    {
        FString Role;
        double Skill = 0.0;
        double Cut = 0.0;
    };

    explicit HeistCrew(int32 MaxMemberCount = 3);

    /** Add a member. Fails on full crew, duplicate role, or total cut > 1.0. */
    bool AddMember(const FString& Role, double Skill, double Cut);

    /** Drop a member by role. Returns true if one was removed. */
    bool RemoveMember(const FString& Role);

    int32 MemberCount() const;

    /** Member roles, in insertion order. */
    TArray<FString> Roles() const;

    /** Sum of every member's cut. */
    double TotalCut() const;

    /** The player's share of the take (1 - total cut, floored at 0). */
    double PlayerShare() const;

    /** Average member skill (0 when empty). */
    double CrewSkill() const;

    /** Odds of pulling the heist off, clamped [0,1]. */
    double SuccessChance(double BaseDifficulty) const;

    /** Roll the heist against SuccessChance using Rng. */
    bool Attempt(double BaseDifficulty, FRandomStream& Rng) const;

    /** No-rng overload mirroring Godot's `attempt(..., null)`: never rolls a success. */
    bool AttemptNoRng(double BaseDifficulty) const;

    /** The player's whole-currency take on a heist outcome (0 on failure / <= 0 take). */
    int32 Payout(int32 Take, bool bSuccess) const;

    /** Planning value: expected player take = chance * take * share. */
    double ExpectedPayout(int32 Take, double BaseDifficulty) const;

    /** Determinism helper: a fresh stream seeded with Seed. */
    static FRandomStream MakeRng(int32 Seed);

private:
    int32 MaxMembers = 0;
    TArray<FMember> Members;

    bool HasRole(const FString& Role) const;
};
