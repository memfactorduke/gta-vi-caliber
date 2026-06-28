// Copyright Epic Games, Inc. All Rights Reserved.

#include "TideModelSubsystem.h"

void UTideModelSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ApplyTuning();
}

void UTideModelSubsystem::ApplyTuning()
{
    FTideModel::FParams Params;
    Params.MeanLevel = static_cast<double>(MeanLevel);
    Params.MeanAmplitude = static_cast<double>(MeanAmplitude);
    Params.SpringNeapRange = static_cast<double>(SpringNeapRange);
    Params.PeriodHours = static_cast<double>(PeriodHours);
    Params.SpringNeapHours = static_cast<double>(SpringNeapHours);
    Params.PhaseHours = static_cast<double>(PhaseHours);
    Params.SpringNeapPhaseHours = static_cast<double>(SpringNeapPhaseHours);
    Model.Configure(Params);
}
