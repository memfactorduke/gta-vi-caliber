// Copyright Epic Games, Inc. All Rights Reserved.

#include "MusicDirectorSubsystem.h"

void UMusicDirectorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ApplyTuning();
}

void UMusicDirectorSubsystem::ApplyTuning()
{
    FMusicDirector::FParams Params;
    Params.AttackPerSec = static_cast<double>(AttackPerSec);
    Params.ReleasePerSec = static_cast<double>(ReleasePerSec);
    Params.LayerCount = LayerCount;
    Params.Hysteresis = static_cast<double>(Hysteresis);
    Model.Configure(Params);
    LastLayer = Model.Layer();
}

void UMusicDirectorSubsystem::UpdateMusic(float Heat, float DeltaSeconds)
{
    Model.Update(static_cast<double>(Heat), static_cast<double>(DeltaSeconds));
    const int32 Layer = Model.Layer();
    if (Layer != LastLayer)
    {
        LastLayer = Layer;
        OnMusicLayerChanged.Broadcast(Layer);
    }
}
