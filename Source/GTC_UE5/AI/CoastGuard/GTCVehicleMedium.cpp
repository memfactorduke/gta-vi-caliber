// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCVehicleMedium.h"

#include "GameFramework/Pawn.h"
#include "../../Vehicles/Aircraft/GTCHelicopterComponent.h"
#include "../../Vehicles/Aircraft/GTCAirplaneComponent.h"
#include "../../Vehicles/Watercraft/GTCBoatComponent.h"

namespace GTCVehicleMedium
{
    EPlayerMedium MediumOf(const APawn* Pawn)
    {
        if (Pawn == nullptr)
        {
            return EPlayerMedium::Land;
        }
        if (Pawn->FindComponentByClass<UGTCHelicopterComponent>() != nullptr ||
            Pawn->FindComponentByClass<UGTCAirplaneComponent>() != nullptr)
        {
            return EPlayerMedium::Air;
        }
        // A jet-ski component derives from the boat component, so this catches both.
        if (Pawn->FindComponentByClass<UGTCBoatComponent>() != nullptr)
        {
            return EPlayerMedium::Sea;
        }
        return EPlayerMedium::Land;
    }
}
