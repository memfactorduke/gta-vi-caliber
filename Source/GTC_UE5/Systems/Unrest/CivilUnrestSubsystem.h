// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CivilUnrest.h"
#include "CivilUnrestSubsystem.generated.h"

/** Blueprint mirror of FCivilUnrest::EStage. */
UENUM(BlueprintType)
enum class EGtcUnrestStage : uint8
{
    Calm,
    Tense,
    Rioting
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGtcOnDistrictStageChanged, const FString&, District, EGtcUnrestStage, Stage);

/**
 * UCivilUnrestSubsystem — per-district civil unrest (UE adapter over FCivilUnrest).
 *
 * A WorldSubsystem keying one FCivilUnrest per district id. Provoke() a district on a
 * crime/explosion, Suppress() with police presence, AdvanceAll() each tick to run the
 * bistable dynamic. Districts that tip past the flashpoint riot until suppressed; fires
 * OnDistrictStageChanged when any district changes stage.
 */
UCLASS()
class GTC_UE5_API UCivilUnrestSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Unrest")
    float Flashpoint = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Unrest")
    float TenseAt = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Unrest")
    float DecayPerSec = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Unrest")
    float GrowthPerSec = 0.04f;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Unrest")
    FGtcOnDistrictStageChanged OnDistrictStageChanged;

    /** Add (or reset) a district to track. Safe to call repeatedly. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Unrest")
    void RegisterDistrict(const FString& District);

    /** A provocation in District (adds unrest). Auto-registers an unknown district. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Unrest")
    void Provoke(const FString& District, float Amount);

    /** Police presence in District (removes unrest). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Unrest")
    void Suppress(const FString& District, float Amount);

    /** Advance every district one tick over DeltaSeconds. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Unrest")
    void AdvanceAll(float DeltaSeconds);

    UFUNCTION(BlueprintPure, Category = "GTC|Unrest")
    float GetUnrest(const FString& District) const;

    UFUNCTION(BlueprintPure, Category = "GTC|Unrest")
    EGtcUnrestStage GetStage(const FString& District) const;

    UFUNCTION(BlueprintPure, Category = "GTC|Unrest")
    bool IsRioting(const FString& District) const;

private:
    FCivilUnrest::FParams MakeParams() const;
    static EGtcUnrestStage MapStage(FCivilUnrest::EStage Stage);

    /** One tracked district: its model plus the last stage broadcast (for change detection). */
    struct FDistrict
    {
        FCivilUnrest Model;
        EGtcUnrestStage LastStage = EGtcUnrestStage::Calm;
    };

    TMap<FString, FDistrict> Districts;

    FDistrict& FindOrAddDistrict(const FString& District);
    void DetectStageChange(const FString& District, FDistrict& D);
};
