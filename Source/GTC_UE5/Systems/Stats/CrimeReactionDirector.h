// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NewsBulletin.h"
#include "../Economy/DistrictEconomy.h"
#include "CrimeReactionDirector.generated.h"

/**
 * Self-wiring coordinator that makes the city REACT to the player's heat — the UE 5.7
 * port of Godot's CrimeReactionDirector Node. It owns an FNewsBulletin and an
 * FDistrictEconomy (the merged Economy model, reused via #include — NOT re-ported) and
 * drives both from the player's wanted level: a rising wanted level files a
 * severity-scaled news headline AND heats up the active district (dragging its
 * real-estate desirability down), and that heat bleeds off over time.
 *
 * Lifecycle: a UGameInstanceSubsystem. It owns one FNewsBulletin + one FDistrictEconomy
 * for its whole lifetime.
 *
 * DEFERRED-OWNERSHIP (option-1 own-state): CrimeReactionDirector OWNS the news + district
 * models and takes the wanted level as INPUT via explicit driver methods (OnStarsChanged /
 * Tick). It reuses the merged FDistrictEconomy and FWantedTracker/wanted star data as
 * INPUT; it never reaches into the live WantedSubsystem. The Godot scene-graph wiring —
 * resolving the "wanted" group, binding the stars_changed signal, and the per-frame
 * _process heat decay — is Wave 3 engine wiring. The radio/news ACTOR wiring (the radio
 * NEWS slot draining News, the news-bulletin display) is ALSO DEFERRED to Wave 3; here
 * News is exposed publicly so that future actor wiring can drain it. The reaction math
 * (heat-per-star, rampage threshold, decay) is preserved as the drivers, so behavior
 * stays headless-testable.
 */
UCLASS()
class GTC_UE5_API UCrimeReactionDirector : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** Crime heat added to the active district per extra wanted star gained (Godot @export). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CrimeReaction")
    double HeatPerStar = 0.15;

    /** Crime heat bled off the whole city per second (Godot @export). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CrimeReaction")
    double HeatDecayPerSec = 0.02;

    /** District that takes the heat (a real scene updates this from player location). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CrimeReaction")
    FString ActiveDistrict = TEXT("downtown");

    /** At/above this many stars a crime reads as a full-blown rampage in the news. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CrimeReaction")
    int32 RampageStars = 4;

    // USubsystem lifecycle.
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // --- Event drivers (ported from the Godot Node) ------------------------

    /**
     * The wanted star count changed to Stars. A rise heats the active district by
     * HeatPerStar per gained star and files a news headline (kind "rampage" at/above
     * RampageStars, else "crime"; severity = Stars). Mirrors _on_stars_changed.
     */
    void OnStarsChanged(int32 Stars);

    /**
     * Advance heat decay by DeltaSeconds: bleeds HeatDecayPerSec * delta off every
     * district. Mirrors the Godot _process body. No-op for non-positive delta/rate.
     */
    void Tick(double DeltaSeconds);

    // --- Owned-model access ------------------------------------------------

    /** Mutable access to the owned news model (a radio NEWS slot drains it — W3 wiring). */
    FNewsBulletin& GetNews() { EnsureModels(); return *_News; }
    const FNewsBulletin& GetNews() const { EnsureModels(); return *_News; }

    /** Access to the owned district economy (a property UI reads desirability). */
    DistrictEconomy& GetDistricts() { EnsureModels(); return *_Districts; }
    const DistrictEconomy& GetDistricts() const { EnsureModels(); return *_Districts; }

private:
    /** Lazily allocate the owned models if not present. */
    void EnsureModels() const;

    mutable TUniquePtr<FNewsBulletin> _News;
    mutable TUniquePtr<DistrictEconomy> _Districts;

    int32 _LastStars = 0;
};
