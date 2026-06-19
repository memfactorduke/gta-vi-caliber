// Copyright Epic Games, Inc. All Rights Reserved.

#include "ReactionState.h"

int32 FReactionState::Rank(EReaction R)
{
    return static_cast<int32>(R);
}

FReactionState::EReaction FReactionState::Lower(EReaction R)
{
    switch (R)
    {
    case EReaction::Flee: return EReaction::Gawk;
    case EReaction::Gawk: return EReaction::Ignore;
    default:              return EReaction::Ignore;
    }
}

double FReactionState::HoldFor(EReaction R)
{
    switch (R)
    {
    case EReaction::Flee: return FleeHoldSeconds;
    case EReaction::Gawk: return GawkHoldSeconds;
    default:              return 0.0;
    }
}

FReactionState::FTick FReactionState::Update(EReaction Desired, double DeltaTime)
{
    TimeInStateSec += DeltaTime;

    FTick Out;
    Out.State = State;
    Out.bRisingEdge = false;

    const int32 Cur = Rank(State);
    const int32 Des = Rank(Desired);

    if (Des > Cur)
    {
        // Escalate immediately — this is the rising edge the adapter reacts to.
        State = Desired;
        TimeInStateSec = 0.0;
        Out.State = State;
        Out.bRisingEdge = true;
    }
    else if (Des < Cur && TimeInStateSec >= HoldFor(State))
    {
        // Cool down one level at a time, but never drop below what's still desired.
        EReaction Next = Lower(State);
        if (Rank(Next) < Des)
        {
            Next = Desired;
        }
        State = Next;
        TimeInStateSec = 0.0;
        Out.State = State;
    }

    return Out;
}
