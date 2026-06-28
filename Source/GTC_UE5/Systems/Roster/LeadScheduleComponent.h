// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LeadSchedule.h"
#include "LeadScheduleComponent.generated.h"

/** Blueprint mirror of FLeadSchedule::FBlock — one stretch of a lead's day. */
USTRUCT(BlueprintType)
struct FGtcLeadBlock
{
    GENERATED_BODY()

    /** Hour the block begins, in [0, 24) (folded modulo 24). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Roster")
    float StartHour = 0.0f;

    /** Where the lead hangs around during this block (world position). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Roster")
    FVector Place = FVector::ZeroVector;

    /** What the lead is doing, e.g. "fishing the pier". */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Roster")
    FString Activity;
};

/**
 * ULeadScheduleComponent — a dormant lead's daily routine (UE adapter over
 * FLeadSchedule). Lives on the dormant lead's manager / pawn.
 *
 * SetRoutine() installs the 24h ring of blocks; on switch-in the adapter queries
 * PlaceAt(Hours) / ActivityAt(Hours) to drop the lead at a plausible haunt instead
 * of the frozen spot, and TimeUntilNextBlock() to schedule the next reposition.
 */
UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API ULeadScheduleComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULeadScheduleComponent();

    /** Install the routine (blocks are folded into [0,24) and sorted). */
    UFUNCTION(BlueprintCallable, Category = "GTC|Roster")
    void SetRoutine(const TArray<FGtcLeadBlock>& InBlocks);

    UFUNCTION(BlueprintPure, Category = "GTC|Roster")
    bool IsEmpty() const { return Model.IsEmpty(); }

    UFUNCTION(BlueprintPure, Category = "GTC|Roster")
    int32 GetBlockCount() const { return Model.BlockCount(); }

    /** Index of the block active at time-of-day Hours, or INDEX_NONE. */
    UFUNCTION(BlueprintPure, Category = "GTC|Roster")
    int32 BlockAt(float Hours) const { return Model.BlockAt(Hours); }

    /** Where the lead is at Hours. */
    UFUNCTION(BlueprintPure, Category = "GTC|Roster")
    FVector PlaceAt(float Hours) const { return Model.PlaceAt(Hours); }

    /** What the lead is doing at Hours. */
    UFUNCTION(BlueprintPure, Category = "GTC|Roster")
    FString ActivityAt(float Hours) const { return Model.ActivityAt(Hours); }

    /** Hours until the routine next changes block from Hours. */
    UFUNCTION(BlueprintPure, Category = "GTC|Roster")
    float TimeUntilNextBlock(float Hours) const { return static_cast<float>(Model.TimeUntilNextBlock(Hours)); }

private:
    FLeadSchedule Model;
};
