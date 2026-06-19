// Copyright Epic Games, Inc. All Rights Reserved.

#include "GtcMissionReward.h"

#include "GtcMissionDefinition.h"

FGtcRewardPlan GtcMissionReward::Plan(const FGtcRewardDefinition& R)
{
    FGtcRewardPlan P;
    P.ObjectiveReward = FMath::Max(0, R.ObjectiveReward);
    P.MissionBonus = FMath::Max(0, R.MissionBonus);
    P.HeadlineMoney = FMath::Max(0, R.Money);
    P.MissionXp = FMath::Max(0, R.Xp);
    P.Respect = FMath::Max(0, R.Respect);
    return P;
}
