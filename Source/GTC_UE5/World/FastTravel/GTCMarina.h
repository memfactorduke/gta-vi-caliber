// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GTCFastTravelHub.h"
#include "GTCMarina.generated.h"

/**
 * AGTCMarina — a dock/slip hub of kind Marina that can park a ready boat or jet-ski. Set
 * ParkedVehicleClass to BP_Boat / BP_JetSki; gun it off the slip onto the bay.
 */
UCLASS()
class GTC_UE5_API AGTCMarina : public AGTCFastTravelHub
{
    GENERATED_BODY()

public:
    AGTCMarina();
};
