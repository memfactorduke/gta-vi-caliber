// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure mission state: one objective with a progress count and an optional timer.
 *
 * the reference parity: game/scripts/missions/mission.gd (RefCounted). No scene access —
 * a MissionDirector owns one and feeds it kills and time, so the objective/fail
 * logic is unit-tested headless. Deliberately small (single counted objective);
 * multi-step missions chain several of these via MissionChain.
 *
 * Plain C++ value type (no UObject): the objectives -> StateTree wiring and any
 * mission Subsystem are deferred to W2/W3 and are NOT part of this parity port.
 */
class GTC_UE5_API Mission
{
public:
    enum class EStatus : uint8
    {
        Active,
        Complete,
        Failed
    };

    FString Title;
    FString Objective;
    int32 Required = 1;
    int32 Progress = 0;
    EStatus Status = EStatus::Active;
    /** 0 = untimed; otherwise the mission fails when the clock runs out. */
    double TimeLimit = 0.0;
    double TimeLeft = 0.0;

    /** required is floored at 1, limit floored at 0 (untimed). */
    Mission(const FString& MissionTitle, const FString& ObjectiveText, int32 Target, double Limit = 0.0);

    bool IsActive() const;

    /** Count progress toward the objective; completes (and locks) at Required. */
    void Record(int32 Amount = 1);

    /** Advance the timer; a timed mission fails when it hits zero. */
    void Tick(double Delta);

    double Fraction() const;

    void Reset();
};
