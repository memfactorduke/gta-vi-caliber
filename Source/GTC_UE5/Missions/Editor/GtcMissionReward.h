// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FGtcRewardDefinition;

/**
 * The payout intent compiled from an authored reward — what the runtime should actually
 * grant. Pure value type so the field mapping + clamping is unit-tested headlessly; the
 * subsystem applies it to the live UMissionReward / UProgressionTracker / player wallet.
 *
 * Channels (matching the existing reward systems):
 *  - ObjectiveReward : per-objective cash drip, fed to UMissionReward::ObjectiveReward.
 *  - MissionBonus    : completion cash bonus, fed to UMissionReward::MissionBonus.
 *  - HeadlineMoney   : an extra lump added straight to the wallet on completion (the
 *                      authored "this mission pays $X").
 *  - MissionXp       : respect/XP granted on completion via UProgressionTracker.
 *  - Respect         : extra GTA-style respect (currently folded into the XP track).
 */
struct GTC_UE5_API FGtcRewardPlan
{
    int32 ObjectiveReward = 0;
    int32 MissionBonus = 0;
    int32 HeadlineMoney = 0;
    int32 MissionXp = 0;
    int32 Respect = 0;
};

namespace GtcMissionReward
{
    /**
     * Map an authored reward into a clamped payout plan. Negative amounts become 0 — they
     * are silent no-ops in the underlying systems, so we normalize them here for clarity.
     */
    GTC_UE5_API FGtcRewardPlan Plan(const FGtcRewardDefinition& Reward);
}
