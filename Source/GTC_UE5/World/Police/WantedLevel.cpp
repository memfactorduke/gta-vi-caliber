// Copyright (c) 2026 GTC contributors

#include "WantedLevel.h"

// Out-of-line storage for the static threshold table, ported VERBATIM from the
// Godot STAR_THRESHOLDS constant (game/scripts/world/wanted_level.gd).
const double FWantedLevel::StarThresholds[6] = {0.0, 1.0, 3.0, 6.0, 10.0, 15.0};

const TMap<FString, double>& FWantedLevel::CrimeHeat()
{
    // Function-local static: heat added per crime kind, ported VERBATIM from the
    // Godot CRIME_HEAT dictionary.
    static const TMap<FString, double> Table = {
        {TEXT("trespass"), 0.5},
        {TEXT("theft"), 1.5},
        {TEXT("reckless_driving"), 1.0},
        {TEXT("assault"), 3.0},
        {TEXT("shooting"), 5.0},
    };
    return Table;
}
