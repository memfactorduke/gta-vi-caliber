// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure activity-based proficiency model — the genre's "get better by doing"
 * progression: driving, shooting, stamina, etc. each climb 0..MAX_SKILL as the
 * player performs the matching activity, with diminishing returns. Plain C++ value
 * type (no UObject); a subsystem (SkillsCoordinator) owns one, trains it from
 * activity and reads bonus() to scale gameplay numbers. Headless-testable (parity
 * oracle game/tests/unit/test_player_skills.gd).
 *
 * Parity note: skills are stored in first-seen insertion order via an ordered TArray
 * plus a TMap id->index, so Skills() / ToDict() / OverallMastery() iterate exactly as
 * the Godot Dictionary did. Values/rates are double. Garbage entries (missing/empty
 * id, non-positive rate, duplicate id) are dropped at construction.
 */
struct GTC_UE5_API FSkillDef
{
    FString Id;
    double Rate = 1.0;
    double Value = 0.0;

    FSkillDef() = default;
    FSkillDef(const FString& InId, double InRate, double InValue = 0.0)
        : Id(InId), Rate(InRate), Value(InValue) {}
};

class GTC_UE5_API FPlayerSkills
{
public:
    /** Ceiling every skill climbs toward. */
    static constexpr double MaxSkill = 100.0;

    /** One named proficiency tier as {Floor, Label}, ascending. */
    struct FTier
    {
        double Floor;
        FString Label;
    };

    /** Named proficiency tiers in ascending floor order. */
    static const TArray<FTier>& Tiers();

    /**
     * Construct from a list of skill definitions. An empty list uses DefaultSkills().
     * Malformed entries (empty id, non-positive rate, duplicate id) are dropped.
     */
    explicit FPlayerSkills(const TArray<FSkillDef>& Skills = TArray<FSkillDef>());

    /** The built-in skill set used when an empty list is passed. */
    static TArray<FSkillDef> DefaultSkills();

    int32 SkillCount() const { return _Order.Num(); }

    /** True if the skill id exists. */
    bool HasSkill(const FString& Id) const { return _Index.Contains(Id); }

    /** Every skill id, in first-seen order. */
    TArray<FString> Skills() const;

    /** Current proficiency of a skill in [0, MaxSkill] (0.0 if unknown). */
    double Level(const FString& Id) const;

    /**
     * Practise a skill: `Effort` is the raw activity amount. Gain = effort * rate *
     * (1 - value/MAX). Returns the actual gain applied (0.0 for unknown id or
     * non-positive effort, or when already at the cap).
     */
    double Train(const FString& Id, double Effort);

    /** The named tier a skill currently sits in (highest floor it meets). "" if unknown. */
    FString Tier(const FString& Id) const;

    /** Normalised proficiency in [0, 1]. 0.0 for an unknown skill. */
    double Bonus(const FString& Id) const { return Level(Id) / MaxSkill; }

    /** Directly set a skill's value, clamped to [0, MaxSkill]. No-op for unknown id. */
    void SetLevel(const FString& Id, double Value);

    /** Mean proficiency across all skills in [0, 1]. 0.0 with no skills. */
    double OverallMastery() const;

    /** Flatten to {id: value}, in insertion order, for the save system. */
    TArray<TPair<FString, double>> ToDict() const;

    /**
     * Restore from a {id: value} map. Unknown ids are ignored; known skills are
     * clamped into range. Skills absent from the data keep their current value.
     */
    void LoadDict(const TArray<TPair<FString, double>>& Data);

private:
    void Register(const FSkillDef& Entry);

    /** Insertion-ordered skill entries. */
    TArray<FSkillDef> _Order;
    /** id -> index into _Order. */
    TMap<FString, int32> _Index;
};
