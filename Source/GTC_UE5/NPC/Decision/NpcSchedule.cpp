// Copyright (c) 2026 GTC contributors

#include "NpcSchedule.h"

#include "Math/UnrealMathUtility.h"

FNpcScheduleBlock FNpcSchedule::Idle()
{
    return FNpcScheduleBlock(0.0, 24.0, TEXT("loiter"), TEXT("street"));
}

double FNpcSchedule::WrapHour(double Hour)
{
    // the reference fposmod(hour, 24.0): result has the sign of the divisor (always
    // non-negative here), in [0, 24).
    return FMath::Fmod(FMath::Fmod(Hour, 24.0) + 24.0, 24.0);
}

bool FNpcSchedule::BlockCovers(const FNpcScheduleBlock& Block, double Hour)
{
    const double H = WrapHour(Hour);
    const double S = Block.Start;
    const double E = Block.End;
    if (S <= E)
    {
        return H >= S && H < E;
    }
    // Wrapping block, e.g. 22 -> 6 covers [22,24) U [0,6).
    return H >= S || H < E;
}

FNpcScheduleBlock FNpcSchedule::ActivityAt(const TArray<FNpcScheduleBlock>& Blocks, double Hour)
{
    for (const FNpcScheduleBlock& Block : Blocks)
    {
        if (BlockCovers(Block, Hour))
        {
            return Block;
        }
    }
    return Idle();
}

double FNpcSchedule::HoursUntilEnd(const FNpcScheduleBlock& Block, double Hour)
{
    const double H = WrapHour(Hour);
    const double E = Block.End;
    double Delta = E - H;
    if (Delta <= 0.0)
    {
        Delta += 24.0;
    }
    return Delta;
}
