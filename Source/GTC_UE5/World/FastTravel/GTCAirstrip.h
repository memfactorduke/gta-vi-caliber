// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GTCFastTravelHub.h"
#include "GTCAirstrip.generated.h"

/**
 * AGTCAirstrip — a runway hub of kind Airstrip that can park a ready plane. Set
 * ParkedVehicleClass to BP_Airplane; take off down the strip and climb out over the bay.
 */
UCLASS()
class GTC_UE5_API AGTCAirstrip : public AGTCFastTravelHub
{
    GENERATED_BODY()

public:
    AGTCAirstrip();
};
