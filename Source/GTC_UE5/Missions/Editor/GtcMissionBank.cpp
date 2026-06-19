// Copyright Epic Games, Inc. All Rights Reserved.

#include "GtcMissionBank.h"

#include "GtcMissionDefinition.h"

FGuid GtcMissionBank::CreateMission(TArray<FGtcMissionDefinition>& Bank, EGtcMissionContentType Type)
{
    FGtcMissionDefinition D;
    D.MissionId = FGuid::NewGuid();
    D.ContentType = Type;
    // A short, human-friendly id derived from the GUID, lowercase, e.g. "mission_a1b2c3d4".
    D.Id = FString::Printf(TEXT("mission_%s"), *D.MissionId.ToString(EGuidFormats::Digits).Left(8).ToLower());
    D.Title = TEXT("New Mission");
    if (Type == EGtcMissionContentType::Activity)
    {
        D.ActivityKind = EGtcActivityKind::SideJob;
    }

    const FGuid NewId = D.MissionId;
    Bank.Add(MoveTemp(D));
    return NewId;
}

FGtcMissionDefinition* GtcMissionBank::Find(TArray<FGtcMissionDefinition>& Bank, const FGuid& MissionId)
{
    return Bank.FindByPredicate([&MissionId](const FGtcMissionDefinition& M) { return M.MissionId == MissionId; });
}

const FGtcMissionDefinition* GtcMissionBank::Find(const TArray<FGtcMissionDefinition>& Bank, const FGuid& MissionId)
{
    return Bank.FindByPredicate([&MissionId](const FGtcMissionDefinition& M) { return M.MissionId == MissionId; });
}

bool GtcMissionBank::RemoveMission(TArray<FGtcMissionDefinition>& Bank, const FGuid& MissionId)
{
    return Bank.RemoveAll([&MissionId](const FGtcMissionDefinition& M) { return M.MissionId == MissionId; }) > 0;
}

bool GtcMissionBank::AddObjective(TArray<FGtcMissionDefinition>& Bank, const FGuid& MissionId, const FGtcObjectiveDefinition& Objective)
{
    if (FGtcMissionDefinition* M = Find(Bank, MissionId))
    {
        M->Objectives.Add(Objective);
        return true;
    }
    return false;
}

bool GtcMissionBank::RemoveObjective(TArray<FGtcMissionDefinition>& Bank, const FGuid& MissionId, const FString& ObjectiveId)
{
    if (FGtcMissionDefinition* M = Find(Bank, MissionId))
    {
        return M->Objectives.RemoveAll([&ObjectiveId](const FGtcObjectiveDefinition& O) { return O.Id == ObjectiveId; }) > 0;
    }
    return false;
}

bool GtcMissionBank::ReorderObjective(TArray<FGtcMissionDefinition>& Bank, const FGuid& MissionId, int32 FromIndex, int32 ToIndex)
{
    FGtcMissionDefinition* M = Find(Bank, MissionId);
    if (!M || !M->Objectives.IsValidIndex(FromIndex) || !M->Objectives.IsValidIndex(ToIndex))
    {
        return false;
    }
    FGtcObjectiveDefinition Moved = M->Objectives[FromIndex];
    M->Objectives.RemoveAt(FromIndex);
    // Clamp: after the removal the array is one shorter, so a ToIndex past the end appends.
    M->Objectives.Insert(MoveTemp(Moved), FMath::Clamp(ToIndex, 0, M->Objectives.Num()));
    return true;
}

bool GtcMissionBank::SetObjectiveWaypoint(TArray<FGtcMissionDefinition>& Bank, const FGuid& MissionId, const FString& ObjectiveId, const FVector& WorldLocation)
{
    FGtcMissionDefinition* M = Find(Bank, MissionId);
    if (!M)
    {
        return false;
    }
    FGtcObjectiveDefinition* O = M->Objectives.FindByPredicate(
        [&ObjectiveId](const FGtcObjectiveDefinition& Obj) { return Obj.Id == ObjectiveId; });
    if (!O)
    {
        return false;
    }
    O->Waypoint = WorldLocation;
    O->bHasWaypoint = true;
    return true;
}

bool GtcMissionBank::IsCountObjective(EGtcObjectiveKind Kind)
{
    return Kind == EGtcObjectiveKind::Collect || Kind == EGtcObjectiveKind::Eliminate;
}

bool GtcMissionBank::IsMissionAvailable(const FGtcMissionDefinition& D, const TSet<FGuid>& Completed)
{
    if (!D.bAvailableInWorld)
    {
        return false;
    }
    if (Completed.Contains(D.MissionId) && !D.bRepeatable)
    {
        return false;
    }
    for (const FGuid& Pre : D.Prerequisites)
    {
        if (!Completed.Contains(Pre))
        {
            return false;
        }
    }
    return true;
}

FGtcMissionValidation GtcMissionBank::Validate(const FGtcMissionDefinition& D)
{
    FGtcMissionValidation V;
    const auto Err = [&V](FString S) { V.Errors.Add(MoveTemp(S)); };
    const auto Warn = [&V](FString S) { V.Warnings.Add(MoveTemp(S)); };

    if (D.Title.IsEmpty())
    {
        Err(TEXT("Mission title is empty."));
    }

    if (D.ContentType == EGtcMissionContentType::Activity)
    {
        if (D.ActivityKind == EGtcActivityKind::None)
        {
            Err(TEXT("Activity mission has no ActivityKind set."));
        }
        if (D.ActivityKind == EGtcActivityKind::SideJob)
        {
            if (D.SideJob.Pickup.Equals(D.SideJob.Dropoff))
            {
                Warn(TEXT("Side job pickup and dropoff are the same point."));
            }
            if (D.SideJob.Kind < 0 || D.SideJob.Kind > 3)
            {
                Warn(TEXT("Side job kind is outside the known range [0,3] (Taxi/Delivery/Vigilante/Towing)."));
            }
            if (D.SideJob.ParTime < 0.0)
            {
                Warn(TEXT("Side job par time is negative (treated as untimed — no time bonus)."));
            }
        }
        if (D.ActivityKind == EGtcActivityKind::StreetRace)
        {
            if (D.Race.Checkpoints.Num() < 1)
            {
                Err(TEXT("Street race has no checkpoints."));
            }
            if (D.Race.Laps < 1)
            {
                Err(TEXT("Street race lap count must be >= 1."));
            }
            if (D.Race.Checkpoints.Num() > 50)
            {
                Warn(TEXT("Street race has more than 50 checkpoints."));
            }
        }
    }
    else if (D.Objectives.Num() < 1)
    {
        // A narrative mission with no objectives can never complete (it would deadlock).
        Err(TEXT("Mission has no objectives (it would never complete)."));
    }

    TSet<FString> SeenIds;
    for (int32 i = 0; i < D.Objectives.Num(); ++i)
    {
        const FGtcObjectiveDefinition& O = D.Objectives[i];
        const FString Where = FString::Printf(TEXT("Objective %d ('%s')"), i, *O.Id);

        if (O.Id.IsEmpty())
        {
            Err(FString::Printf(TEXT("Objective %d has an empty id."), i));
        }
        else if (SeenIds.Contains(O.Id))
        {
            Err(FString::Printf(TEXT("Duplicate objective id '%s'."), *O.Id));
        }
        SeenIds.Add(O.Id);

        if (O.Text.IsEmpty())
        {
            Warn(Where + TEXT(" has no display text."));
        }
        if (O.DriverKind != TEXT("reach") && O.DriverKind != TEXT("hold"))
        {
            Warn(Where + FString::Printf(TEXT(" has unknown driver kind '%s' (will behave as 'reach')."), *O.DriverKind));
        }
        if ((O.Kind == EGtcObjectiveKind::Collect || O.Kind == EGtcObjectiveKind::Eliminate) && O.TargetCount < 1)
        {
            Err(Where + TEXT(" needs TargetCount >= 1."));
        }
        if (O.bHasWaypoint && O.Waypoint.IsNearlyZero())
        {
            Warn(Where + TEXT(" has a waypoint at the world origin (unplaced?)."));
        }
        if (O.Radius > 0.0 && O.Radius < 50.0)
        {
            Warn(Where + TEXT(" has a suspiciously small radius (< 50 cm)."));
        }
    }

    for (const FGtcTriggerDefinition& T : D.Triggers)
    {
        if (!T.ObjectiveId.IsEmpty() && !SeenIds.Contains(T.ObjectiveId))
        {
            Warn(FString::Printf(TEXT("Trigger references unknown objective id '%s'."), *T.ObjectiveId));
        }
    }

    if (D.TimeLimit < 0.0)
    {
        Err(TEXT("TimeLimit cannot be negative."));
    }
    else if (D.TimeLimit > 0.0 && D.TimeLimit < 0.01)
    {
        Warn(TEXT("TimeLimit is extremely small (< 0.01s)."));
    }
    else if (D.TimeLimit > 3600.0)
    {
        Warn(TEXT("TimeLimit exceeds one hour."));
    }

    if (D.Reward.Money < 0)
    {
        Warn(TEXT("Reward money is negative (it will be a no-op at payout)."));
    }
    if (D.Reward.Xp < 0)
    {
        Warn(TEXT("Reward XP is negative (it will be a no-op at payout)."));
    }
    if (D.Reward.Unlocks.Num() > 0)
    {
        Warn(TEXT("Reward unlocks are granted + persisted (query via HasUnlock); they don't feed the level-gated progression table."));
    }

    return V;
}
