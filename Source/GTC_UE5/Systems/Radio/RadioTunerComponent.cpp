// Copyright Epic Games, Inc. All Rights Reserved.

#include "RadioTunerComponent.h"

URadioTunerComponent::URadioTunerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void URadioTunerComponent::ConfigureStations(const TArray<FGtcRadioStation>& InStations)
{
    TArray<FRadioTuner::FStation> Stations;
    Stations.Reserve(InStations.Num());
    for (const FGtcRadioStation& In : InStations)
    {
        FRadioTuner::FStation Station;
        Station.TrackSeconds.Reserve(In.TrackSeconds.Num());
        for (float Secs : In.TrackSeconds)
        {
            Station.TrackSeconds.Add(static_cast<double>(Secs));
        }
        Station.PhaseSeconds = static_cast<double>(In.PhaseSeconds);
        Stations.Add(MoveTemp(Station));
    }
    Model.Configure(MoveTemp(Stations));
    LastStation = Model.Station();
    LastTrackIndex = Model.NowPlaying().TrackIndex;
}

void URadioTunerComponent::Next()
{
    Model.Next();
    RefreshStation();
}

void URadioTunerComponent::Previous()
{
    Model.Previous();
    RefreshStation();
}

void URadioTunerComponent::TuneTo(int32 StationIndex)
{
    Model.TuneTo(StationIndex);
    RefreshStation();
}

void URadioTunerComponent::TuneOff()
{
    Model.TuneOff();
    RefreshStation();
}

void URadioTunerComponent::RefreshStation()
{
    const int32 Station = Model.Station();
    if (Station != LastStation)
    {
        LastStation = Station;
        OnStationChanged.Broadcast(Station);
    }
    // A dial move can also change the track on air immediately.
    const int32 TrackIndex = Model.NowPlaying().TrackIndex;
    if (TrackIndex != LastTrackIndex)
    {
        LastTrackIndex = TrackIndex;
        OnTrackChanged.Broadcast(TrackIndex);
    }
}

void URadioTunerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    Model.Advance(static_cast<double>(DeltaTime));

    // The shared clock can roll the playlist onto the next track without a dial move.
    const int32 TrackIndex = Model.NowPlaying().TrackIndex;
    if (TrackIndex != LastTrackIndex)
    {
        LastTrackIndex = TrackIndex;
        OnTrackChanged.Broadcast(TrackIndex);
    }
}
