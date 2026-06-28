// Copyright Epic Games, Inc. All Rights Reserved.

#include "BribeOfferComponent.h"

UBribeOfferComponent::UBribeOfferComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UBribeOfferComponent::BeginPlay()
{
    Super::BeginPlay();
    ApplyTuning();
}

void UBribeOfferComponent::ApplyTuning()
{
    FBribeOffer::FParams Params;
    Params.MaxBribableStars = MaxBribableStars;
    Params.CostPerStar = static_cast<double>(CostPerStar);
    Params.GreedPerBribe = static_cast<double>(GreedPerBribe);
    Params.CooldownHours = static_cast<double>(CooldownHours);
    Params.StarsReducedPerBribe = StarsReducedPerBribe;
    Model.Configure(Params);
}

FGtcBribeResult UBribeOfferComponent::Bribe(int32 Stars, float Cash)
{
    const FBribeOffer::FResult Result = Model.Bribe(Stars, static_cast<double>(Cash));

    FGtcBribeResult Out;
    Out.bAccepted = Result.bAccepted;
    Out.Cost = static_cast<float>(Result.Cost);
    Out.StarsAfter = Result.StarsAfter;

    if (Result.bAccepted)
    {
        OnBribeAccepted.Broadcast(Out.Cost, Out.StarsAfter);
    }
    return Out;
}

void UBribeOfferComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    // Advance the cooldown in hours (the model's time unit).
    Model.Advance(static_cast<double>(DeltaTime) / 3600.0);
}
