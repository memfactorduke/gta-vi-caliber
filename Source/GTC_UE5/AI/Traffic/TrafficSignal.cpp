// Copyright Epic Games, Inc. All Rights Reserved.

#include "TrafficSignal.h"

void FTrafficSignal::Configure(TArray<FPhase> InPhases, double InAllRedSeconds, double InOffsetSeconds)
{
    Phases = MoveTemp(InPhases);
    AllRedSeconds = FMath::Max(0.0, InAllRedSeconds);
    OffsetSeconds = InOffsetSeconds;
    ClockSeconds = 0.0;

    // One full cycle is every phase's green + yellow, plus one all-red clearance per phase.
    TotalCycle = 0.0;
    for (const FPhase& Phase : Phases)
    {
        const double Green = FMath::Max(0.0, Phase.GreenSeconds);
        const double Yellow = FMath::Max(0.0, Phase.YellowSeconds);
        TotalCycle += Green + Yellow + AllRedSeconds;
    }
}

void FTrafficSignal::Advance(double Dt)
{
    // The clock never runs backwards: a paused/rewound frame must not flip the lights.
    if (Dt > 0.0)
    {
        ClockSeconds += Dt;
    }
}

FTrafficSignal::FResolved FTrafficSignal::Resolve() const
{
    FResolved Out;
    if (TotalCycle <= 0.0)
    {
        return Out; // no cycle -> dark junction, everything Red, bValid stays false
    }

    // Where on the looping cycle we sit. FMath::Fmod keeps the dividend's sign, so a
    // negative offset can land negative — fold it positive.
    double Position = FMath::Fmod(ClockSeconds + OffsetSeconds, TotalCycle);
    if (Position < 0.0)
    {
        Position += TotalCycle;
    }

    // Walk the phases; each occupies [green | yellow | all-red] back to back.
    double Accumulated = 0.0;
    for (int32 Index = 0; Index < Phases.Num(); ++Index)
    {
        const double Green = FMath::Max(0.0, Phases[Index].GreenSeconds);
        const double Yellow = FMath::Max(0.0, Phases[Index].YellowSeconds);
        const double Block = Green + Yellow + AllRedSeconds;

        if (Position < Accumulated + Block)
        {
            const double Into = Position - Accumulated;
            Out.Phase = Index;
            Out.bValid = true;
            if (Into < Green)
            {
                Out.Segment = ESegment::Green;
                Out.Remaining = Green - Into;
            }
            else if (Into < Green + Yellow)
            {
                Out.Segment = ESegment::Yellow;
                Out.Remaining = (Green + Yellow) - Into;
            }
            else
            {
                Out.Segment = ESegment::AllRed;
                Out.Remaining = Block - Into;
            }
            return Out;
        }
        Accumulated += Block;
    }

    // Floating-point drift could leave Position == TotalCycle exactly; treat it as the
    // very start of the first phase's green.
    Out.Phase = 0;
    Out.bValid = true;
    Out.Segment = ESegment::Green;
    Out.Remaining = FMath::Max(0.0, Phases.Num() > 0 ? FMath::Max(0.0, Phases[0].GreenSeconds) : 0.0);
    return Out;
}

FTrafficSignal::ESignal FTrafficSignal::StateFor(int32 Group) const
{
    const FResolved R = Resolve();
    if (!R.bValid || R.Segment == ESegment::AllRed)
    {
        return ESignal::Red;
    }

    // Only the active phase's GoGroups show a non-red light.
    if (!Phases[R.Phase].GoGroups.Contains(Group))
    {
        return ESignal::Red;
    }
    return R.Segment == ESegment::Green ? ESignal::Green : ESignal::Yellow;
}

int32 FTrafficSignal::ActivePhase() const
{
    return Resolve().Phase;
}

bool FTrafficSignal::IsAllRed() const
{
    const FResolved R = Resolve();
    return R.bValid && R.Segment == ESegment::AllRed;
}

double FTrafficSignal::TimeUntilChange() const
{
    return Resolve().Remaining;
}
