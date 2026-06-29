// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "../PoliceDispatch/AirSeaEscalation.h" // EPlayerMedium

class APawn;

/**
 * Classify what the player is currently travelling in (Land / Air / Sea) from their pawn,
 * by which Wings & Waves locomotion component it carries. Feeds FAirSeaEscalation so the
 * police chopper scrambles for a flyer and the Coast Guard only launches against a boat.
 */
namespace GTCVehicleMedium
{
    GTC_UE5_API EPlayerMedium MediumOf(const APawn* Pawn);
}
