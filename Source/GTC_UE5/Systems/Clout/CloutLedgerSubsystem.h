// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CloutLedger.h"
#include "CloutLedgerSubsystem.generated.h"

/** Blueprint mirror of FCloutLedger::ETier. */
UENUM(BlueprintType)
enum class EGtcCloutTier : uint8
{
    Unknown,
    Nano,
    Micro,
    Macro,
    Celebrity
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGtcOnFollowersChanged, float, Followers);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGtcOnCloutTierChanged, EGtcCloutTier, Tier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGtcOnPosted, float, NetChange);

/**
 * UCloutLedgerSubsystem — the player's social following (UE adapter over FCloutLedger).
 *
 * A GameInstanceSubsystem (player-global, outlives sublevels). Post(Appeal) publishes
 * content and returns the net follower change; Advance(Hours) cools the audience.
 * Pairs with the photo scorer: Post(UPhotoScoreLibrary::PhotoAppeal(shot)). Gigs /
 * brand deals gate on GetTier().
 */
UCLASS()
class GTC_UE5_API UCloutLedgerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Clout")
    float BaseReach = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Clout")
    float AudienceFactor = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Clout")
    float ViralAppeal = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Clout")
    float ViralMultiplier = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Clout")
    float DecayPerHour = 0.01f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Clout")
    float NanoAt = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Clout")
    float MicroAt = 25000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Clout")
    float MacroAt = 250000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Clout")
    float CelebrityAt = 1000000.0f;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Clout")
    FGtcOnFollowersChanged OnFollowersChanged;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Clout")
    FGtcOnCloutTierChanged OnCloutTierChanged;

    UPROPERTY(BlueprintAssignable, Category = "GTC|Clout")
    FGtcOnPosted OnPosted;

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "GTC|Clout")
    void ApplyTuning();

    /** Publish a post of quality Appeal (0..1); returns the net follower change. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Clout")
    float Post(float Appeal);

    /** Advance Hours: the audience cools. */
    UFUNCTION(BlueprintCallable, Category = "GTC|Clout")
    void Advance(float Hours);

    UFUNCTION(BlueprintPure, Category = "GTC|Clout")
    float GetFollowers() const { return static_cast<float>(Model.Followers()); }

    UFUNCTION(BlueprintPure, Category = "GTC|Clout")
    EGtcCloutTier GetTier() const;

    UFUNCTION(BlueprintPure, Category = "GTC|Clout")
    float GetHoursSinceLastPost() const { return static_cast<float>(Model.HoursSinceLastPost()); }

    /** Reach a post would have right now before appeal/fatigue. */
    UFUNCTION(BlueprintPure, Category = "GTC|Clout")
    float GetCurrentReach() const { return static_cast<float>(Model.CurrentReach()); }

private:
    FCloutLedger Model;
    EGtcCloutTier LastTier = EGtcCloutTier::Unknown;

    void BroadcastFollowers();
};
