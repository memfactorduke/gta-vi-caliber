// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RadioTuner.h"
#include "RadioTunerComponent.generated.h"

/**
 * FGtcRadioStation — Blueprint-facing mirror of FRadioTuner::FStation so a station
 * lineup can be authored / fed in from BP. Converted to the pure-core station on
 * ConfigureStations.
 */
USTRUCT(BlueprintType)
struct FGtcRadioStation
{
    GENERATED_BODY()

    /** Per-track lengths in seconds, in broadcast order; the playlist loops forever. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Radio")
    TArray<float> TrackSeconds;

    /** Seconds of head-start on the shared clock (desyncs stations). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Radio")
    float PhaseSeconds = 0.0f;
};

/** Dial moved -> (Station index, INDEX_NONE == Radio Off). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGtcRadioStationChanged, int32, Station);
/** Track on air changed -> (TrackIndex, INDEX_NONE == nothing playing). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGtcRadioTrackChanged, int32, TrackIndex);

/**
 * URadioTunerComponent — the in-vehicle radio dial (UE adapter over FRadioTuner).
 *
 * Owns an FRadioTuner, advances its shared broadcast clock every frame, and
 * re-exposes the dial (Next/Previous/TuneTo/Off) plus the NowPlaying snapshot for
 * the audio adapter. Picking the matching USoundBase and seeking the audio
 * component to the track offset is left to a Blueprint/audio layer that listens to
 * OnTrackChanged — this component is the deterministic dial only.
 *
 * Lives on the drivable vehicle BP. Add it as a component, call ConfigureStations
 * once (e.g. on BeginPlay) with the city's lineup, and drive the dial from input.
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API URadioTunerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URadioTunerComponent();

    UPROPERTY(BlueprintAssignable, Category = "GTC|Radio")
    FGtcRadioStationChanged OnStationChanged;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Radio")
    FGtcRadioTrackChanged OnTrackChanged;

    /** Install the station lineup; resets the dial to Off and the clock to 0. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Radio")
    void ConfigureStations(const TArray<FGtcRadioStation>& InStations);

    UFUNCTION(BlueprintCallable, Category = "GTC|Radio")
    void Next();

    UFUNCTION(BlueprintCallable, Category = "GTC|Radio")
    void Previous();

    UFUNCTION(BlueprintCallable, Category = "GTC|Radio")
    void TuneTo(int32 StationIndex);

    UFUNCTION(BlueprintCallable, Category = "GTC|Radio")
    void TuneOff();

    UFUNCTION(BlueprintPure, Category = "GTC|Radio")
    bool IsOn() const { return Model.IsOn(); }

    UFUNCTION(BlueprintPure, Category = "GTC|Radio")
    int32 GetStation() const { return Model.Station(); }

    UFUNCTION(BlueprintPure, Category = "GTC|Radio")
    int32 GetNumStations() const { return Model.NumStations(); }

    UFUNCTION(BlueprintPure, Category = "GTC|Radio")
    float GetClock() const { return static_cast<float>(Model.Clock()); }

    /** Track index on air for the tuned station now, or INDEX_NONE. */
    UFUNCTION(BlueprintPure, Category = "GTC|Radio")
    int32 GetCurrentTrackIndex() const { return Model.NowPlaying().TrackIndex; }

    /** Seconds into the current track. */
    UFUNCTION(BlueprintPure, Category = "GTC|Radio")
    float GetCurrentTrackOffset() const { return static_cast<float>(Model.NowPlaying().TrackOffset); }

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    FRadioTuner Model;

    /** Last broadcast values, so we only fire delegates on an actual change. */
    int32 LastStation = FRadioTuner::OffStation;
    int32 LastTrackIndex = INDEX_NONE;

    /** Re-read the dial and broadcast OnStationChanged if it moved. */
    void RefreshStation();
};
