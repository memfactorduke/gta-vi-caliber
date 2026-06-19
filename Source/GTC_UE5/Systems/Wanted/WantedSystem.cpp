// Copyright Epic Games, Inc. All Rights Reserved.

#include "WantedSystem.h"

const TArray<double>& FWantedSystem::StarThresholds()
{
    // Godot: const STAR_THRESHOLDS: Array[float] = [1.0, 3.0, 6.0, 10.0, 16.0]
    static const TArray<double> Thresholds = {1.0, 3.0, 6.0, 10.0, 16.0};
    return Thresholds;
}
