// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GTCFastTravelHub.h"
#include "GTCHelipad.generated.h"

/**
 * AGTCHelipad — a rooftop/ground helipad. A fast-travel hub of kind Helipad that can park a
 * ready-to-fly helicopter. Drop it on a rooftop, set ParkedVehicleClass to BP_Helicopter,
 * and the player can walk up, board, and lift off — the first Wings & Waves trailer beat.
 */
UCLASS()
class GTC_UE5_API AGTCHelipad : public AGTCFastTravelHub
{
    GENERATED_BODY()

public:
    AGTCHelipad();
};
