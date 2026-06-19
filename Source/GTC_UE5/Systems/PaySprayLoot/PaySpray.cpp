// Copyright (c) 2026 GTC contributors

#include "PaySpray.h"

bool PaySpray::CanEnter(const FVector& Player, const FVector& Shop, float Radius)
{
    const float Reach = FMath::Max(Radius, 0.0f);
    return FVector::Dist(Player, Shop) <= Reach;
}

int32 PaySpray::CostFor(int32 Stars, int32 Base, int32 PerStar)
{
    const int32 Heat = FMath::Clamp(Stars, 0, MaxStars);
    if (Heat <= 0)
    {
        return 0;
    }
    return FMath::Max(Base, 0) + FMath::Max(PerStar, 0) * Heat;
}

bool PaySpray::IsSeenEntering(const FVector& Shop, const TArray<FVector>& Police, float Sight)
{
    const float SightReach = FMath::Max(Sight, 0.0f);
    for (const FVector& Cop : Police)
    {
        if (FVector::Dist(Cop, Shop) <= SightReach)
        {
            return true;
        }
    }
    return false;
}

PaySpray::PaySpray(float Duration)
{
    // Guard against zero/negative so Progress() never divides by zero and the
    // respray always takes a finite, completable amount of time.
    ResprayDuration = FMath::Max(Duration, 0.001f);
}

void PaySpray::Begin()
{
    bRunning = true;
    Time = 0.0f;
}

void PaySpray::Tick(float Delta)
{
    if (!bRunning)
    {
        return;
    }
    if (Time >= ResprayDuration)
    {
        return;
    }
    Time = FMath::Min(Time + FMath::Max(Delta, 0.0f), ResprayDuration);
}

float PaySpray::Progress() const
{
    if (!bRunning)
    {
        return 0.0f;
    }
    return FMath::Clamp(Time / ResprayDuration, 0.0f, 1.0f);
}

bool PaySpray::IsComplete() const
{
    return bRunning && Time >= ResprayDuration;
}

void PaySpray::Cancel()
{
    bRunning = false;
    Time = 0.0f;
}

void PaySpray::Reset()
{
    bRunning = false;
    Time = 0.0f;
}

PaySpray::FResolveResult PaySpray::Resolve(int32 Stars, int32 Balance, bool bSeen, int32 Base, int32 PerStar) const
{
    if (Stars <= 0)
    {
        return Deny(Balance, TEXT("not wanted: nothing to respray"));
    }
    if (bSeen)
    {
        return Deny(Balance, TEXT("seen entering: police traced you"));
    }
    const int32 Cost = CostFor(Stars, Base, PerStar);
    if (Balance < Cost)
    {
        return Deny(Balance, FString::Printf(TEXT("insufficient funds: need %d, have %d"), Cost, Balance));
    }

    FResolveResult Result;
    Result.bAllowed = true;
    Result.Cost = Cost;
    Result.NewBalance = Balance - Cost;
    Result.Reason = FString();
    return Result;
}

PaySpray::FResolveResult PaySpray::Deny(int32 Balance, const FString& Reason)
{
    FResolveResult Result;
    Result.bAllowed = false;
    Result.Cost = 0;
    Result.NewBalance = Balance;
    Result.Reason = Reason;
    return Result;
}
