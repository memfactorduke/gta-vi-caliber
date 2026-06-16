// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcMemory.h"

#include "Math/UnrealMathUtility.h"

double FNpcMemory::Decay(double Memory, double Dt, double Rate)
{
    return FMath::Max(Memory - Rate * Dt, 0.0);
}

FString FNpcMemory::Category(double Memory)
{
    if (Memory >= Alarmed)
    {
        return TEXT("alarmed");
    }
    if (Memory >= Uneasy)
    {
        return TEXT("uneasy");
    }
    return TEXT("none");
}

bool FNpcMemory::Recognizes(double Memory, double PlayerDistance, double RangeM)
{
    return Memory >= Uneasy && PlayerDistance <= RangeM;
}
