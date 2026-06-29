// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GTCBoatComponent.h"
#include "GTCJetSkiComponent.generated.h"

/**
 * UGTCJetSkiComponent — a personal watercraft: the boat strategy tuned light and snappy
 * (more accel, less drag, a tighter rudder, a small low-draft hull). BP_JetSki is the pawn
 * + this component. Mechanically a boat, so it rides the same ocean and the Coast Guard
 * chases it the same way.
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UGTCJetSkiComponent : public UGTCBoatComponent
{
    GENERATED_BODY()

public:
    UGTCJetSkiComponent();
};
