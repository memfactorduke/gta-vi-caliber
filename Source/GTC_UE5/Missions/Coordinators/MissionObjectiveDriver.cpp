// Copyright (c) 2026 GTC contributors

#include "MissionObjectiveDriver.h"

FMissionObjectiveDriver::FVerdict FMissionObjectiveDriver::Evaluate(
    const FObjectiveDef& Def,
    const FVector& PlayerPos,
    const FVector& Target,
    double Held,
    double Delta)
{
    // Godot: var radius := float(def.get("radius", 6.0)); the def already carries
    // the 6.0 default in FObjectiveDef, so Def.Radius is the resolved radius.
    const bool bInside = MissionObjectiveTypes::ReachSatisfied(PlayerPos, Target, Def.Radius);

    if (Def.Kind == TEXT("hold"))
    {
        // hold: accumulate while inside, reset to 0 the moment the player leaves.
        const double Now = bInside ? (Held + Delta) : 0.0;
        FVerdict Verdict;
        Verdict.bSatisfied = MissionObjectiveTypes::SurviveSatisfied(Now, Def.Duration);
        Verdict.Held = Now;
        return Verdict;
    }

    // reach (and any unknown kind, which degrades to reach): no hold clock kept.
    FVerdict Verdict;
    Verdict.bSatisfied = bInside;
    Verdict.Held = 0.0;
    return Verdict;
}

void FMissionObjectiveDriver::Bind()
{
    Held = 0.0;
    ArmedId.Reset();
}

bool FMissionObjectiveDriver::TickObjective(
    const FString& ObjectiveId,
    const FVector& PlayerPos,
    const FVector& Target,
    double Delta)
{
    // Godot: if oid.is_empty() or not defs.get(oid) is Dictionary: return.
    const FObjectiveDef* Def = Defs.Find(ObjectiveId);
    if (ObjectiveId.IsEmpty() || Def == nullptr)
    {
        return false;
    }

    // Re-arm the hold clock when the active objective changes (Godot _armed_id swap).
    if (ObjectiveId != ArmedId)
    {
        ArmedId = ObjectiveId;
        Held = 0.0;
    }

    const FVerdict Verdict = Evaluate(*Def, PlayerPos, Target, Held, Delta);
    Held = Verdict.Held;
    return Verdict.bSatisfied;
}
